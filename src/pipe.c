/*
 * Computer Architecture - Professor Onur Mutlu
 *
 * MIPS pipeline timing simulator
 *
 * Chris Fallin, 2012
 */

#include "pipe.h"
#include "shell.h"
#include "mips.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
//#define DEBUG

/* debug */
void print_op(Pipe_Op *op)
{
    if (op)
        printf("OP (PC=%08x inst=%08x) src1=R%d (%08x) src2=R%d (%08x) dst=R%d valid %d (%08x) br=%d taken=%d dest=%08x mem=%d addr=%08x\n",
                op->pc, op->instruction, op->reg_src1, op->reg_src1_value, op->reg_src2, op->reg_src2_value, op->reg_dst, op->reg_dst_value_ready,
                op->reg_dst_value, op->is_branch, op->branch_taken, op->branch_dest, op->is_mem, op->mem_addr);
    else
        printf("(null)\n");
}

/* global pipeline state */
Pipe_State pipe;
cache_I cache;
cache_D data_cache;
mshr mshr_array[16];
int L2_stall;
int stall_L2_miss;
cache_L2 L2_cache;
int L2_hits_in_a_cycle;
request request_array[16];
int open_row_in_bank[8];
int cmd_bus_tracker[500];
int data_bus_tracker[500];
bank banks[8];
request schedulable_request[16];
int get_row_buffer_status(uint32_t address){
    //0:hit, 1:miss, 2:conflict
    int row=address>>16;
    int bank=(address>>5)&0X7;
    if(open_row_in_bank[bank]==row){
        return 0;
    }
    else if(open_row_in_bank[bank]==-1){
        return 1;
    }
    else{
        return 2;
    }
}
void init_row(){
    for(int i=0;i<8;i++){
        open_row_in_bank[i]= -1;
    }
}
void init_request_array(){
    for(int i=0;i<16;i++){
        request_array[i].is_free=1;
    }
}
void init_schedulable_array(){
    for(int i=0;i<16;i++){
        schedulable_request[i].is_free=1;
    }
}
void l2_cache_replacement(int index){
        cache_block *selected_cache;
        u_int32_t tag=(mshr_array[index].addr>>14)&0x3FFFF;
        uint32_t set=(mshr_array[index].addr>>5)&0X1FF;
        int num_ways=16;
        int lru=1;
        selected_cache=L2_cache.cache_set_I_L2[set].cache_block;
                for(int ways=0;ways<num_ways;ways++){
                    if(!selected_cache[ways].valid){
                        selected_cache[ways].valid=1;
                        selected_cache[ways].tag=tag;
                        selected_cache[ways].lru_counter=0;
                        selected_cache[ways].dirty=0;
                            for(int i=0;i<num_ways;i++){
                                if(!(i==ways)&&selected_cache[i].valid){
                                    selected_cache[i].lru_counter++;
                                }         
                }
                        if(mshr_array[index].is_mem==1&&mshr_array[index].mem_write){
                            selected_cache[ways].dirty=1;
                            }
                        lru=0;
                        break;
                            }
                        }
                if(lru){
                        int max=0;
                        int maxindex=0;
                        for(int ways=0;ways<num_ways;ways++){
                            if(selected_cache[ways].lru_counter>max){
                                max=selected_cache[ways].lru_counter;
                                maxindex=ways;
                            }
                            }
                            selected_cache[maxindex].lru_counter=0;
                            selected_cache[maxindex].tag=tag;
                            selected_cache[maxindex].dirty=0;
                            if(mshr_array[index].is_mem&&mshr_array[index].mem_write){
                                selected_cache[maxindex].dirty=1;
                            }
                        for(int i=0;i<num_ways;i++){
                                if(!(i==maxindex)&&selected_cache[i].valid){
                                    selected_cache[i].lru_counter++;
                                }
                                
                }
                    } 


}
void set_offsets(){
    for(int i=0;i<16;i++){
    if(request_array[i].is_free==0){
        request_array[i].status=get_row_buffer_status(request_array[i].addr);
        if(request_array[i].status==0){
            request_array[i].offset_array[0].duration=4;
            request_array[i].offset_array[0].start=0;
            request_array[i].offset_array[1].duration=100;
            request_array[i].offset_array[1].start=0;
            request_array[i].offset_array[2].duration=50;
            request_array[i].offset_array[2].start=100;  
            request_array[i].num_actions=3;            
        }
        else if(request_array[i].status==1){
            request_array[i].offset_array[0].duration=4;
            request_array[i].offset_array[0].start=0;

            request_array[i].offset_array[1].duration=100;
            request_array[i].offset_array[1].start=0;

            request_array[i].offset_array[2].duration=4;
             request_array[i].offset_array[2].start=100;  

            request_array[i].offset_array[3].duration=100;
            request_array[i].offset_array[3].start=100;
            
            request_array[i].offset_array[4].start=200;
            request_array[i].offset_array[4].duration=50;
            request_array[i].num_actions=5;  
        }
        else if(request_array[i].status==2){
            request_array[i].offset_array[0].duration=4;
            request_array[i].offset_array[0].start=0;

            request_array[i].offset_array[1].duration=100;
            request_array[i].offset_array[1].start=0;

            request_array[i].offset_array[2].duration=4;
            request_array[i].offset_array[2].start=100;  

            request_array[i].offset_array[3].duration=100;
            request_array[i].offset_array[3].start=100;

            request_array[i].offset_array[4].start=200;
            request_array[i].offset_array[4].duration=4;

            request_array[i].offset_array[5].start=200;
            request_array[i].offset_array[5].duration=100;  
            
            request_array[i].offset_array[6].start=300;
            request_array[i].offset_array[6].duration=50;

            request_array[i].num_actions=7;  
    }
}
}
}
void schedulable(){
    set_offsets();
    for(int i=0;i<16;i++){
        if(request_array[i].is_free==0){
            int not_continue=0;
            int bank_index=(request_array[i].addr>>5)&0X7;
            int row_index=request_array[i].addr>>16;
            for(int j=0;j<request_array[i].num_actions;j++){
                if(not_continue==1){
                    break;
                }
                if(j==request_array[i].num_actions-1){
                    for(int k=0;k<request_array[i].offset_array[j].duration;k++){
                        if(data_bus_tracker[(stat_cycles+request_array[i].offset_array[j].start+k)%500]==0){
                            continue;
                        }
                        else{
                            not_continue=1;
                            break;
                        }
                    }
                }
                else if(j%2==0){
                    for(int k=0;k<request_array[i].offset_array[j].duration;k++){
                        if(cmd_bus_tracker[(stat_cycles+request_array[i].offset_array[j].start+k)%500]==0){
                            continue;
                        }
                        else{
                            not_continue=1;
                            break;
                        }
                    }
                }
                else{
                    for(int k=0;k<request_array[i].offset_array[j].duration;k++){
                        if(banks[bank_index].bank_tracker[(stat_cycles+request_array[i].offset_array[j].start+k)%500]==0){
                            continue;
                        }
                        else{
                            not_continue=1;
                            break;
                        }
                    }
                }
                if(!not_continue){
                if(j==request_array[i].num_actions-1){
                    for(int t=0;t<16;t++){
                        if(schedulable_request[t].is_free==1){
                            schedulable_request[t]=request_array[i];
                            schedulable_request[t].is_free=0;
                            schedulable_request[t].bank_index=bank_index;
                            schedulable_request[t].row_index=row_index;
                            schedulable_request[t].original_index=i;
                            break;
                        }
                    }
                }
            }
            }
        }
    }
}
void controller(){
    schedulable();
    request best_candidate;
    int count=0;
    int index;
    for(int i=0;i<16;i++){
        if(schedulable_request[i].is_free==0){
            count++;
        }
    }
    for(int i=0;i<16;i++){
        if(schedulable_request[i].is_free==0){
            best_candidate=schedulable_request[i];
            index=i;
            break;
        }
    }
    if(count==0){
        for(int i=0;i<8;i++){
            banks[i].bank_tracker[stat_cycles%500]=0;
        }
        cmd_bus_tracker[stat_cycles%500]=0;
        data_bus_tracker[stat_cycles%500]=0;


        return;
    }
    else if(count>1){
    for(int i=0;i<16;i++){
        if(schedulable_request[i].is_free==0){
        if((open_row_in_bank[schedulable_request[i].bank_index]==schedulable_request[i].row_index)&&!(open_row_in_bank[best_candidate.bank_index]==best_candidate.row_index)){
            best_candidate=schedulable_request[i];
            index=i;
        }
        else if(((open_row_in_bank[schedulable_request[i].bank_index]==schedulable_request[i].row_index))==(open_row_in_bank[best_candidate.bank_index]==best_candidate.row_index)){
            if(schedulable_request[i].arrival_cycle<best_candidate.arrival_cycle){
                best_candidate=schedulable_request[i];
                index=i;
            }
            else if(schedulable_request[i].arrival_cycle==best_candidate.arrival_cycle){
                if(schedulable_request[i].is_mem_req){
                    best_candidate=schedulable_request[i];
                    index=i;
                }
            }
        }
    }
    }
    }
    if(best_candidate.num_actions==3){
        mshr_array[best_candidate.mshr_index].memory_done_cycle=stat_cycles+150;
    }
   else if(best_candidate.num_actions==5){
        mshr_array[best_candidate.mshr_index].memory_done_cycle=stat_cycles+250;
   }
   else if(best_candidate.num_actions==7){
        mshr_array[best_candidate.mshr_index].memory_done_cycle=stat_cycles+350;
   }
   mshr_array[best_candidate.mshr_index].scheduled=1;
   for(int i=0;i<16;i++){
    schedulable_request[i].is_free=1;
   }
   request_array[best_candidate.original_index].is_free=1;
   //open row
    open_row_in_bank[best_candidate.bank_index]=best_candidate.row_index;

    //occupy the cycles based on selected request
    for(int i=0;i<best_candidate.num_actions;i++){
        if(i==best_candidate.num_actions-1){
            for(int j=0;j<50;j++){
                data_bus_tracker[(stat_cycles+best_candidate.offset_array[i].start+j)%500]=1;
            }
        } 
        else if(i%2==0){
            for(int k=0;k<best_candidate.offset_array[i].duration;k++){
                cmd_bus_tracker[(stat_cycles+best_candidate.offset_array[i].start+k)%500]=1;
            }
        }
        else{
            for(int k=0;k<best_candidate.offset_array[i].duration;k++){
                banks[best_candidate.bank_index].bank_tracker[(stat_cycles+best_candidate.offset_array[i].start+k)%500]=1;
            }
        }
    }
    for(int i=0;i<8;i++){
        banks[i].bank_tracker[stat_cycles%500]=0;
    }
    cmd_bus_tracker[stat_cycles%500]=0;
    data_bus_tracker[stat_cycles%500]=0;

    }






void pipe_init()
{

    init_row();
    init_request_array();
    init_schedulable_array();
    memset(&pipe, 0, sizeof(Pipe_State));
    pipe.PC = 0x00400000;
    for(int i=0;i<16;i++){
    mshr_array[i].state=0;
}

}
void send_request(u_int32_t address,int arrival_cycle,int is_mem,int mshr_index){
    for(int i=0;i<16;i++){
        if(request_array[i].is_free==1){
            request_array[i].addr=address;
            request_array[i].arrival_cycle=arrival_cycle;
            request_array[i].is_mem_req=is_mem;
            request_array[i].mshr_index=mshr_index;
            request_array[i].is_free=0;
            break;
        }
    }
}
void pipe_cycle()
{
#ifdef DEBUG
    printf("\n\n----\n\nPIPELINE:\n");
    printf("DCODE: "); print_op(pipe.decode_op);
    printf("EXEC : "); print_op(pipe.execute_op);
    printf("MEM  : "); print_op(pipe.mem_op);
    printf("WB   : "); print_op(pipe.wb_op);
    printf("\n");
#endif
    int request_index;
    int request_state=0;
    L2_hits_in_a_cycle=0;
    int address;
    int arrival_cycle;
    int is_mem;
    int mshr_index;

    //memory state machine for handling communication between L2 and DRAM
    for(int i=0;i<16;i++){
        if(mshr_array[i].valid==1){
            request_index=i;
            request_state=mshr_array[i].state;
            address=mshr_array[i].addr;
            arrival_cycle=mshr_array[i].arrival_cycle;
            is_mem=mshr_array[i].is_mem;
        
        switch(request_state){
            //free,stall
            case 0:
                if(stat_cycles>=mshr_array[i].send_to_mc_cycle){
                    send_request(address,arrival_cycle,is_mem,request_index);
                    mshr_array[i].state=1;
                }
                break;
            case 1:
            if(mshr_array[i].scheduled){
                if(stat_cycles>=mshr_array[i].memory_done_cycle){
                    mshr_array[i].state=2;
                    mshr_array[i].fill_ready_cycle=stat_cycles+5;
                }
            }
            else{
                mshr_array[i].state=1;
            }
                break;
            case 2:
                if(stat_cycles>=mshr_array[i].fill_ready_cycle){
                    l2_cache_replacement(i);
                    mshr_array[i].valid=0;
                    mshr_array[i].done=1;
                    mshr_array[i].state=0;
                    mshr_array[i].fetch_done=1;
               } 
               break;              
        }
        }
    }

    pipe_stage_wb();
    if (!RUN_BIT)
    return;
    pipe_stage_mem();
    pipe_stage_execute();
    pipe_stage_decode();
    pipe_stage_fetch();
    controller();

    /* handle branch recoveries */
    if (pipe.branch_recover) {
#ifdef DEBUG
        printf("branch recovery: new dest %08x flush %d stages\n", pipe.branch_dest, pipe.branch_flush);
#endif
        //fetch cancellation asks for the same adress,L2 miss path is already handled but this check handles where it is a hit and asks for the same block
        int temp=pipe.PC;
        pipe.PC = pipe.branch_dest;       
        if(!(((temp&0XFFFFFFE0)<=pipe.PC)&&(pipe.PC<((temp&0XFFFFFFE0)+32)))){
            pipe.IF_cache_state=0;  
        }
        

        if (pipe.branch_flush >= 2) {
            if (pipe.decode_op) free(pipe.decode_op);
            pipe.decode_op = NULL;
        }

        if (pipe.branch_flush >= 3) {
            if (pipe.execute_op) free(pipe.execute_op);
            pipe.execute_op = NULL;
        }

        if (pipe.branch_flush >= 4) {
            if (pipe.mem_op) free(pipe.mem_op);
            pipe.mem_op = NULL;
        }

        if (pipe.branch_flush >= 5) {
            if (pipe.wb_op) free(pipe.wb_op);
            pipe.wb_op = NULL;
        }

        pipe.branch_recover = 0;
        pipe.branch_dest = 0;
        pipe.branch_flush = 0;

        stat_squash++;
    }
}

void pipe_recover(int flush, uint32_t dest)
{
    /* if there is already a recovery scheduled, it must have come from a later
     * stage (which executes older instructions), hence that recovery overrides
     * our recovery. Simply return in this case. */
    if (pipe.branch_recover) return;

    /* schedule the recovery. This will be done once all pipeline stages simulate the current cycle. */
    pipe.branch_recover = 1;
    pipe.branch_flush = flush;
    pipe.branch_dest = dest;
}
//allocate mshr
int L2_allocate(uint32_t address,int option,Pipe_Op* op){
    int index=-1;
    for(int i=0;i<16;i++){
        if(mshr_array[i].valid==0){
            mshr_array[i].valid=1;
            if(option==0){
                mshr_array[i].fetch_done=0;
            }
            else{
                mshr_array[i].done=0;
                mshr_array[i].is_mem=1;
                if(op->mem_write){
                    mshr_array[i].mem_write=1;
                }
                else{
                    mshr_array[i].mem_write=0;
                }
            }
            mshr_array[i].addr=address;
            mshr_array[i].is_mem=option;
            mshr_array[i].arrival_cycle=stat_cycles;
            mshr_array[i].scheduled=0;
            index=i;
            break;
        }
    }
    if(option){
        op->mshr_index=index;
    }
    else{
        pipe.IF_mshr_index=index;
    }
    return index;
}


int L2_logic(u_int32_t address,int option,Pipe_Op* op){
        u_int32_t tag=(address>>14)&0x3FFFF;
        uint32_t set=(address>>5)&0X1FF;
        int hit=0;
        int valid=0;
        int indexblock;
        int on_its_way=0;
    //miss  based on address
        for(int ways=0;ways<16;ways++){
                hit=(tag==L2_cache.cache_set_I_L2[set].cache_block[ways].tag);
                valid=L2_cache.cache_set_I_L2[set].cache_block[ways].valid;
                if(hit&&valid){
                    indexblock=ways;
                    break;
                }
            }
    //actual logic
    if(!(hit&&valid)){
        //handles the case where a fetch cancelletion also asks for the same block from the L2 cache,fixing the another 100 cycle delay caused by it
            for(int i=0;i<16;i++){
                if(mshr_array[i].valid){
                    if(((address&0XFFFFFFE0)<=mshr_array[i].addr)&&(mshr_array[i].addr<(address&0XFFFFFFE0)+32)){
                        pipe.IF_mshr_index=i;
                        on_its_way=1;
                        return i;
                    }  
                }
            }
            if(!(on_its_way)){
                int return_index=L2_allocate(address,option,op);
                mshr_array[return_index].send_to_mc_cycle=stat_cycles+5;
                return return_index;
            }
        }
    else{
            L2_cache.cache_set_I_L2[set].cache_block[indexblock].tag=tag;  
                    int prevcounter=L2_cache.cache_set_I_L2[set].cache_block[indexblock].lru_counter;
                    L2_cache.cache_set_I_L2[set].cache_block[indexblock].lru_counter=0;
                    for(int i=0;i<16;i++){
                        if(L2_cache.cache_set_I_L2[set].cache_block[i].lru_counter<prevcounter&&!(i==indexblock)&&(L2_cache.cache_set_I_L2[set].cache_block[i].valid)){
                            L2_cache.cache_set_I_L2[set].cache_block[i].lru_counter++;
                        }
                    }
                    if(option==1&&op->mem_write){
                        L2_cache.cache_set_I_L2[set].cache_block[indexblock].dirty=1;
                    }
            
        }
    
    return -1;

}




int cache_replacement(uint32_t address,int option,Pipe_Op *op){
    uint32_t tag;
    uint32_t set;
    int num_ways;
    cache_block *selected_cache;
    uint32_t *selected_stall;
    int hit=0;
    int valid=0;
    int lru=1;
    int *state;
    int index;
    if(option==0){
        tag=(address>>11)&0x1FFFFF;
        set=(address>>5)&0X3F;
        num_ways=4;
        selected_cache=cache.cache_set[set].cache_block;
        selected_stall=&pipe.IF_l2_hit_ready_cycle;
        state=&pipe.IF_cache_state;
        index=pipe.IF_mshr_index;
    }
    else{
         uint32_t block_number =
             ((uint32_t)address) / DATA_CACHE_BLOCK_SIZE_BYTES;
         set=block_number % DATA_CACHE_NUM_SETS;
         tag=block_number / DATA_CACHE_NUM_SETS;
         num_ways=DATA_CACHE_ASSOCIATIVITY;
         selected_cache=data_cache.cache_set[set].cache_block;
         selected_stall=&op->l2_hit_ready_cycle;
         state=&op->cache_state;
         index=op->mshr_index;
    }
    int indexblock=0;
        if(*state==0){
            for(int ways=0;ways<num_ways;ways++){
                hit=(tag==selected_cache[ways].tag);
                valid=selected_cache[ways].valid;
                if(hit&&valid){
                    indexblock=ways;
                    break;
                }
            }
            if(hit&&valid){
                    int prevcounter=selected_cache[indexblock].lru_counter;
                    selected_cache[indexblock].lru_counter=0;
                    for(int i=0;i<num_ways;i++){
                        if(selected_cache[i].lru_counter<prevcounter&&!(i==indexblock)&&(selected_cache[i].valid)){
                            selected_cache[i].lru_counter++;
                        }
                    }
                    if(option==1&&op->mem_write){
                        selected_cache[indexblock].dirty=1;
                    }
                    return 1;
                }
            else{
                    //allocate MSHR
                    int mshr_index=L2_logic(address,option,op);
                    if(mshr_index>=0){
                        *state=1;
                    }
                    else{
                        *selected_stall =stat_cycles+15;
                        *state=3;
                    }
                    
                    
                }
                return 0;
        }
    else if(*state==1){
        if(option==0){
            if(mshr_array[index].fetch_done==1){
                *state=2;
            }
            else{
                return 0;
            }  
        }
        else{
            if(mshr_array[index].done==1){
                *state=2;
            }
            else{
                return 0;
            }  
        }
   
    }
    else if(*state==3){
            if(stat_cycles>=*selected_stall){
                *state=2;
            }
            else{
                return 0;
            }     
    }
    if (*state == 2){
        for(int ways=0;ways<num_ways;ways++){
                    if(!selected_cache[ways].valid){
                        selected_cache[ways].valid=1;
                        selected_cache[ways].tag=tag;
                        selected_cache[ways].lru_counter=0;
                        selected_cache[ways].dirty=0;
                            for(int i=0;i<num_ways;i++){
                                if(!(i==ways)&&selected_cache[i].valid){
                                    selected_cache[i].lru_counter++;
                                }         
                }
                        if(option==1&&op->mem_write){
                            selected_cache[ways].dirty=1;
                            }
                        lru=0;
                        
                        break;
                            }
                        }
                if(lru){
                        int max=0;
                        int maxindex=0;
                        for(int ways=0;ways<num_ways;ways++){
                            if(selected_cache[ways].lru_counter>max){
                                max=selected_cache[ways].lru_counter;
                                maxindex=ways;
                            }
                            }
                            selected_cache[maxindex].lru_counter=0;
                            selected_cache[maxindex].tag=tag;
                            selected_cache[maxindex].dirty=0;
                            if(option==1&&op->mem_write){
                                selected_cache[maxindex].dirty=1;
                            }

                        for(int i=0;i<num_ways;i++){
                                if(!(i==maxindex)&&selected_cache[i].valid){
                                    selected_cache[i].lru_counter++;
                                }
                                
                }
                    }   
                    *state=4;
                    return 0;
    }
    else if(*state==4){
        *state=0;
        return 1;
    }


        return -1;
}

void pipe_stage_wb()
{
    /* if there is no instruction in this pipeline stage, we are done */
    if (!pipe.wb_op)
        return;

    /* grab the op out of our input slot */
    Pipe_Op *op = pipe.wb_op;
    pipe.wb_op = NULL;

    /* if this instruction writes a register, do so now */
    if (op->reg_dst != -1 && op->reg_dst != 0) {
        pipe.REGS[op->reg_dst] = op->reg_dst_value;
#ifdef DEBUG
        printf("R%d = %08x\n", op->reg_dst, op->reg_dst_value);
#endif
    }

    /* if this was a syscall, perform action */
    if (op->opcode == OP_SPECIAL && op->subop == SUBOP_SYSCALL) {
        if (op->reg_src1_value == 0xA) {
            pipe.PC = op->pc+4; 
            RUN_BIT = 0;
        }
    }

    /* free the op */
    free(op);

    stat_inst_retire++;
}

void pipe_stage_mem()
{
    int data_ready=1;
    /* if there is no instruction in this pipeline stage, we are done */
    if (!pipe.mem_op)
        return;
    
    /* grab the op out of our input slot */
    Pipe_Op *op = pipe.mem_op;
    if(op->is_mem){
        data_ready=cache_replacement(op->mem_addr, 1,op);
    }
     uint32_t val = 0;
    if(data_ready){
    if (op->is_mem)
        val = mem_read_32(op->mem_addr & ~3);

    switch (op->opcode) {
        case OP_LW:
        case OP_LH:
        case OP_LHU:
        case OP_LB:
        case OP_LBU:
            {
                /* extract needed value */
                op->reg_dst_value_ready = 1;
                if (op->opcode == OP_LW) {
                    op->reg_dst_value = val;
                }
                else if (op->opcode == OP_LH || op->opcode == OP_LHU) {
                    if (op->mem_addr & 2)
                        val = (val >> 16) & 0xFFFF;
                    else
                        val = val & 0xFFFF;

                    if (op->opcode == OP_LH)
                        val |= (val & 0x8000) ? 0xFFFF8000 : 0;

                    op->reg_dst_value = val;
                }
                else if (op->opcode == OP_LB || op->opcode == OP_LBU) {
                    switch (op->mem_addr & 3) {
                        case 0:
                            val = val & 0xFF;
                            break;
                        case 1:
                            val = (val >> 8) & 0xFF;
                            break;
                        case 2:
                            val = (val >> 16) & 0xFF;
                            break;
                        case 3:
                            val = (val >> 24) & 0xFF;
                            break;
                    }

                    if (op->opcode == OP_LB)
                        val |= (val & 0x80) ? 0xFFFFFF80 : 0;

                    op->reg_dst_value = val;
                }
            }
            break;

        case OP_SB:
            switch (op->mem_addr & 3) {
                case 0: val = (val & 0xFFFFFF00) | ((op->mem_value & 0xFF) << 0); break;
                case 1: val = (val & 0xFFFF00FF) | ((op->mem_value & 0xFF) << 8); break;
                case 2: val = (val & 0xFF00FFFF) | ((op->mem_value & 0xFF) << 16); break;
                case 3: val = (val & 0x00FFFFFF) | ((op->mem_value & 0xFF) << 24); break;
            }

            mem_write_32(op->mem_addr & ~3, val);
            break;

        case OP_SH:
#ifdef DEBUG
            printf("SH: addr %08x val %04x old word %08x\n", op->mem_addr, op->mem_value & 0xFFFF, val);
#endif
            if (op->mem_addr & 2)
                val = (val & 0x0000FFFF) | (op->mem_value) << 16;
            else
                val = (val & 0xFFFF0000) | (op->mem_value & 0xFFFF);
#ifdef DEBUG
            printf("new word %08x\n", val);
#endif

            mem_write_32(op->mem_addr & ~3, val);
            break;

        case OP_SW:
            val = op->mem_value;
            mem_write_32(op->mem_addr & ~3, val);
            break;
    }

    /* clear stage input and transfer to next stage */
    pipe.mem_op = NULL;
    pipe.wb_op = op;
    }
    else{
        return;
    }
   
}

void pipe_stage_execute()
{
    /* if a multiply/divide is in progress, decrement cycles until value is ready */
    if (pipe.multiplier_stall > 0)
        pipe.multiplier_stall--;

    /* if downstream stall, return (and leave any input we had) */
    if (pipe.mem_op != NULL)
        return;

    /* if no op to execute, return */
    if (pipe.execute_op == NULL)
        return;

    /* grab op and read sources */
    Pipe_Op *op = pipe.execute_op;

    /* read register values, and check for bypass; stall if necessary */
    int stall = 0;
    if (op->reg_src1 != -1) {
        if (op->reg_src1 == 0)
            op->reg_src1_value = 0;
        else if (pipe.mem_op && pipe.mem_op->reg_dst == op->reg_src1) {
            if (!pipe.mem_op->reg_dst_value_ready)
                stall = 1;
            else
                op->reg_src1_value = pipe.mem_op->reg_dst_value;
        }
        else if (pipe.wb_op && pipe.wb_op->reg_dst == op->reg_src1) {
            op->reg_src1_value = pipe.wb_op->reg_dst_value;
        }
        else
            op->reg_src1_value = pipe.REGS[op->reg_src1];
    }
    if (op->reg_src2 != -1) {
        if (op->reg_src2 == 0)
            op->reg_src2_value = 0;
        else if (pipe.mem_op && pipe.mem_op->reg_dst == op->reg_src2) {
            if (!pipe.mem_op->reg_dst_value_ready)
                stall = 1;
            else
                op->reg_src2_value = pipe.mem_op->reg_dst_value;
        }
        else if (pipe.wb_op && pipe.wb_op->reg_dst == op->reg_src2) {
            op->reg_src2_value = pipe.wb_op->reg_dst_value;
        }
        else
            op->reg_src2_value = pipe.REGS[op->reg_src2];
    }

    /* if bypassing requires a stall (e.g. use immediately after load),
     * return without clearing stage input */
    if (stall) 
        return;

    /* execute the op */
    switch (op->opcode) {
        case OP_SPECIAL:
            op->reg_dst_value_ready = 1;
            switch (op->subop) {
                case SUBOP_SLL:
                    op->reg_dst_value = op->reg_src2_value << op->shamt;
                    break;
                case SUBOP_SLLV:
                    op->reg_dst_value = op->reg_src2_value << op->reg_src1_value;
                    break;
                case SUBOP_SRL:
                    op->reg_dst_value = op->reg_src2_value >> op->shamt;
                    break;
                case SUBOP_SRLV:
                    op->reg_dst_value = op->reg_src2_value >> op->reg_src1_value;
                    break;
                case SUBOP_SRA:
                    op->reg_dst_value = (int32_t)op->reg_src2_value >> op->shamt;
                    break;
                case SUBOP_SRAV:
                    op->reg_dst_value = (int32_t)op->reg_src2_value >> op->reg_src1_value;
                    break;
                case SUBOP_JR:
                case SUBOP_JALR:
                    op->reg_dst_value = op->pc + 4;
                    op->branch_dest = op->reg_src1_value;
                    op->branch_taken = 1;
                    break;

                case SUBOP_MULT:
                    {
                        /* we set a result value right away; however, we will
                         * model a stall if the program tries to read the value
                         * before it's ready (or overwrite HI/LO). Also, if
                         * another multiply comes down the pipe later, it will
                         * update the values and re-set the stall cycle count
                         * for a new operation.
                         */
                        int64_t val = (int64_t)((int32_t)op->reg_src1_value) * (int64_t)((int32_t)op->reg_src2_value);
                        uint64_t uval = (uint64_t)val;
                        pipe.HI = (uval >> 32) & 0xFFFFFFFF;
                        pipe.LO = (uval >>  0) & 0xFFFFFFFF;

                        /* four-cycle multiplier latency */
                        pipe.multiplier_stall = 4;
                    }
                    break;
                case SUBOP_MULTU:
                    {
                        uint64_t val = (uint64_t)op->reg_src1_value * (uint64_t)op->reg_src2_value;
                        pipe.HI = (val >> 32) & 0xFFFFFFFF;
                        pipe.LO = (val >>  0) & 0xFFFFFFFF;

                        /* four-cycle multiplier latency */
                        pipe.multiplier_stall = 4;
                    }
                    break;

                case SUBOP_DIV:
                    if (op->reg_src2_value != 0) {

                        int32_t val1 = (int32_t)op->reg_src1_value;
                        int32_t val2 = (int32_t)op->reg_src2_value;
                        int32_t div, mod;

                        div = val1 / val2;
                        mod = val1 % val2;

                        pipe.LO = div;
                        pipe.HI = mod;
                    } else {
                        // really this would be a div-by-0 exception
                        pipe.HI = pipe.LO = 0;
                    }

                    /* 32-cycle divider latency */
                    pipe.multiplier_stall = 32;
                    break;

                case SUBOP_DIVU:
                    if (op->reg_src2_value != 0) {
                        pipe.HI = (uint32_t)op->reg_src1_value % (uint32_t)op->reg_src2_value;
                        pipe.LO = (uint32_t)op->reg_src1_value / (uint32_t)op->reg_src2_value;
                    } else {
                        /* really this would be a div-by-0 exception */
                        pipe.HI = pipe.LO = 0;
                    }

                    /* 32-cycle divider latency */
                    pipe.multiplier_stall = 32;
                    break;

                case SUBOP_MFHI:
                    /* stall until value is ready */
                    if (pipe.multiplier_stall > 0)
                        return;

                    op->reg_dst_value = pipe.HI;
                    break;
                case SUBOP_MTHI:
                    /* stall to respect WAW dependence */
                    if (pipe.multiplier_stall > 0)
                        return;

                    pipe.HI = op->reg_src1_value;
                    break;

                case SUBOP_MFLO:
                    /* stall until value is ready */
                    if (pipe.multiplier_stall > 0)
                        return;

                    op->reg_dst_value = pipe.LO;
                    break;
                case SUBOP_MTLO:
                    /* stall to respect WAW dependence */
                    if (pipe.multiplier_stall > 0)
                        return;

                    pipe.LO = op->reg_src1_value;
                    break;

                case SUBOP_ADD:
                case SUBOP_ADDU:
                    op->reg_dst_value = op->reg_src1_value + op->reg_src2_value;
                    break;
                case SUBOP_SUB:
                case SUBOP_SUBU:
                    op->reg_dst_value = op->reg_src1_value - op->reg_src2_value;
                    break;
                case SUBOP_AND:
                    op->reg_dst_value = op->reg_src1_value & op->reg_src2_value;
                    break;
                case SUBOP_OR:
                    op->reg_dst_value = op->reg_src1_value | op->reg_src2_value;
                    break;
                case SUBOP_NOR:
                    op->reg_dst_value = ~(op->reg_src1_value | op->reg_src2_value);
                    break;
                case SUBOP_XOR:
                    op->reg_dst_value = op->reg_src1_value ^ op->reg_src2_value;
                    break;
                case SUBOP_SLT:
                    op->reg_dst_value = ((int32_t)op->reg_src1_value <
                            (int32_t)op->reg_src2_value) ? 1 : 0;
                    break;
                case SUBOP_SLTU:
                    op->reg_dst_value = (op->reg_src1_value < op->reg_src2_value) ? 1 : 0;
                    break;
            }
            break;

        case OP_BRSPEC:
            switch (op->subop) {
                case BROP_BLTZ:
                case BROP_BLTZAL:
                    if ((int32_t)op->reg_src1_value < 0) op->branch_taken = 1;
                    break;

                case BROP_BGEZ:
                case BROP_BGEZAL:
                    if ((int32_t)op->reg_src1_value >= 0) op->branch_taken = 1;
                    break;
            }
            break;

        case OP_BEQ:
            if (op->reg_src1_value == op->reg_src2_value) op->branch_taken = 1;
            break;

        case OP_BNE:
            if (op->reg_src1_value != op->reg_src2_value) op->branch_taken = 1;
            break;

        case OP_BLEZ:
            if ((int32_t)op->reg_src1_value <= 0) op->branch_taken = 1;
            break;

        case OP_BGTZ:
            if ((int32_t)op->reg_src1_value > 0) op->branch_taken = 1;
            break;

        case OP_ADDI:
        case OP_ADDIU:
            op->reg_dst_value_ready = 1;
            op->reg_dst_value = op->reg_src1_value + op->se_imm16;
            break;
        case OP_SLTI:
            op->reg_dst_value_ready = 1;
            op->reg_dst_value = (int32_t)op->reg_src1_value < (int32_t)op->se_imm16 ? 1 : 0;
            break;
        case OP_SLTIU:
            op->reg_dst_value_ready = 1;
            op->reg_dst_value = (uint32_t)op->reg_src1_value < (uint32_t)op->se_imm16 ? 1 : 0;
            break;
        case OP_ANDI:
            op->reg_dst_value_ready = 1;
            op->reg_dst_value = op->reg_src1_value & op->imm16;
            break;
        case OP_ORI:
            op->reg_dst_value_ready = 1;
            op->reg_dst_value = op->reg_src1_value | op->imm16;
            break;
        case OP_XORI:
            op->reg_dst_value_ready = 1;
            op->reg_dst_value = op->reg_src1_value ^ op->imm16;
            break;
        case OP_LUI:
            op->reg_dst_value_ready = 1;
            op->reg_dst_value = op->imm16 << 16;
            break;

        case OP_LW:
        case OP_LH:
        case OP_LHU:
        case OP_LB:
        case OP_LBU:
            op->mem_addr = op->reg_src1_value + op->se_imm16;
            break;

        case OP_SW:
        case OP_SH:
        case OP_SB:
            op->mem_addr = op->reg_src1_value + op->se_imm16;
            op->mem_value = op->reg_src2_value;
            break;
    }

    /* handle branch recoveries at this point */
    if (op->branch_taken)
        pipe_recover(3, op->branch_dest);

    /* remove from upstream stage and place in downstream stage */
    pipe.execute_op = NULL;
    pipe.mem_op = op;
}

void pipe_stage_decode()
{
    /* if downstream stall, return (and leave any input we had) */
    if (pipe.execute_op != NULL)
        return;

    /* if no op to decode, return */
    if (pipe.decode_op == NULL)
        return;

    /* grab op and remove from stage input */
    Pipe_Op *op = pipe.decode_op;
    pipe.decode_op = NULL;

    /* set up info fields (source/dest regs, immediate, jump dest) as necessary */
    uint32_t opcode = (op->instruction >> 26) & 0x3F;
    uint32_t rs = (op->instruction >> 21) & 0x1F;
    uint32_t rt = (op->instruction >> 16) & 0x1F;
    uint32_t rd = (op->instruction >> 11) & 0x1F;
    uint32_t shamt = (op->instruction >> 6) & 0x1F;
    uint32_t funct1 = (op->instruction >> 0) & 0x1F;
    uint32_t funct2 = (op->instruction >> 0) & 0x3F;
    uint32_t imm16 = (op->instruction >> 0) & 0xFFFF;
    uint32_t se_imm16 = imm16 | ((imm16 & 0x8000) ? 0xFFFF8000 : 0);
    uint32_t targ = (op->instruction & ((1UL << 26) - 1)) << 2;

    op->opcode = opcode;
    op->imm16 = imm16;
    op->se_imm16 = se_imm16;
    op->shamt = shamt;

    switch (opcode) {
        case OP_SPECIAL:
            /* all "SPECIAL" insts are R-types that use the ALU and both source
             * regs. Set up source regs and immediate value. */
            op->reg_src1 = rs;
            op->reg_src2 = rt;
            op->reg_dst = rd;
            op->subop = funct2;
            if (funct2 == SUBOP_SYSCALL) {
                op->reg_src1 = 2; // v0
                op->reg_src2 = 3; // v1
            }
            if (funct2 == SUBOP_JR || funct2 == SUBOP_JALR) {
                op->is_branch = 1;
                op->branch_cond = 0;
            }

            break;

        case OP_BRSPEC:
            /* branches that have -and-link variants come here */
            op->is_branch = 1;
            op->reg_src1 = rs;
            op->reg_src2 = rt;
            op->is_branch = 1;
            op->branch_cond = 1; /* conditional branch */
            op->branch_dest = op->pc + 4 + (se_imm16 << 2);
            op->subop = rt;
            if (rt == BROP_BLTZAL || rt == BROP_BGEZAL) {
                /* link reg */
                op->reg_dst = 31;
                op->reg_dst_value = op->pc + 4;
                op->reg_dst_value_ready = 1;
            }
            break;

        case OP_JAL:
            op->reg_dst = 31;
            op->reg_dst_value = op->pc + 4;
            op->reg_dst_value_ready = 1;
            op->branch_taken = 1;
            /* fallthrough */
        case OP_J:
            op->is_branch = 1;
            op->branch_cond = 0;
            op->branch_taken = 1;
            op->branch_dest = (op->pc & 0xF0000000) | targ;
            break;

        case OP_BEQ:
        case OP_BNE:
        case OP_BLEZ:
        case OP_BGTZ:
            /* ordinary conditional branches (resolved after execute) */
            op->is_branch = 1;
            op->branch_cond = 1;
            op->branch_dest = op->pc + 4 + (se_imm16 << 2);
            op->reg_src1 = rs;
            op->reg_src2 = rt;
            break;

        case OP_ADDI:
        case OP_ADDIU:
        case OP_SLTI:
        case OP_SLTIU:
            /* I-type ALU ops with sign-extended immediates */
            op->reg_src1 = rs;
            op->reg_dst = rt;
            break;

        case OP_ANDI:
        case OP_ORI:
        case OP_XORI:
        case OP_LUI:
            /* I-type ALU ops with non-sign-extended immediates */
            op->reg_src1 = rs;
            op->reg_dst = rt;
            break;

        case OP_LW:
        case OP_LH:
        case OP_LHU:
        case OP_LB:
        case OP_LBU:
        case OP_SW:
        case OP_SH:
        case OP_SB:
            /* memory ops */
            op->is_mem = 1;
            op->reg_src1 = rs;
            if (opcode == OP_LW || opcode == OP_LH || opcode == OP_LHU || opcode == OP_LB || opcode == OP_LBU) {
                /* load */
                op->mem_write = 0;
                op->reg_dst = rt;
            }
            else {
                /* store */
                op->mem_write = 1;
                op->reg_src2 = rt;
            }
            break;
    }

    /* we will handle reg-read together with bypass in the execute stage */

    /* place op in downstream slot */
    pipe.execute_op = op;
}



void pipe_stage_fetch()
{  
    /* if pipeline is stalled (our output slot is not empty), return */
    if (pipe.decode_op != NULL)
        return;
            int data_ready=cache_replacement(pipe.PC, 0,NULL);
            if(data_ready){
                /* Allocate an op and send it down the pipeline. */
                Pipe_Op *op = malloc(sizeof(Pipe_Op));
                memset(op, 0, sizeof(Pipe_Op));
                op->reg_src1 = op->reg_src2 = op->reg_dst = -1;
                op->instruction = mem_read_32(pipe.PC);
                op->pc = pipe.PC;
                pipe.decode_op = op;

                /* update PC */
                pipe.PC += 4;

                stat_inst_fetch++;
            }
    else{
        return;
    }

}
