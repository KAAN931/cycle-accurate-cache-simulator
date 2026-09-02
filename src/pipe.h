/*
 * Computer Architecture - Professor Onur Mutlu
 *
 * MIPS pipeline timing simulator
 *
 * Chris Fallin, 2012
 */

#ifndef _PIPE_H_
#define _PIPE_H_

#include "shell.h"

/* Pipeline ops (instances of this structure) are high-level representations of
 * the instructions that actually flow through the pipeline. This struct does
 * not correspond 1-to-1 with the control signals that would actually pass
 * through the pipeline. Rather, it carries the original instruction, operand
 * information and values as they are collected, and destination information. */
typedef struct Pipe_Op {
    /* PC of this instruction */
    uint32_t pc;
    /* raw instruction */
    uint32_t instruction;
    /* decoded opcode and subopcode fields */
    int opcode, subop;

    /* immediate value, if any, for ALU immediates */
    uint32_t imm16, se_imm16;
    /* shift amount */
    int shamt;

    /* register source values */
    int reg_src1, reg_src2; /* 0 -- 31 if this inst has register source(s), or
                               -1 otherwise */
    uint32_t reg_src1_value, reg_src2_value; /* values of operands from source
                                                regs */

    /* memory access information */
    int is_mem;       /* is this a load/store? */
    uint32_t mem_addr; /* address if applicable */
    int mem_write; /* is this a write to memory? */
    uint32_t mem_value; /* value loaded from memory or to be written to memory */

    /* register destination information */
    int reg_dst; /* 0 -- 31 if this inst has a destination register, -1
                    otherwise */
    uint32_t reg_dst_value; /* value to write into dest reg. */
    int reg_dst_value_ready; /* destination value produced yet? */

    /* branch information */
    int is_branch;        /* is this a branch? */
    uint32_t branch_dest; /* branch destination (if taken) */
    int branch_cond;      /* is this a conditional branch? */
    int branch_taken;     /* branch taken? (set as soon as resolved: in decode
                             for unconditional, execute for conditional) */
    int is_link;          /* jump-and-link or branch-and-link inst? */
    int link_reg;         /* register to place link into? */

} Pipe_Op;

/* The pipe state represents the current state of the pipeline. It holds a
 * pointer to the op that is currently at the input of each stage. As stages
 * execute, they remove the op from their input (set the pointer to NULL) and
 * place an op at their output. If the pointer that represents a stage's output
 * is not null when that stage executes, then this represents a pipeline stall,
 * and the stage must not overwrite its output (otherwise an instruction would
 * be lost).
 */

typedef struct Pipe_State {
    /* pipe op currently at the input of the given stage (NULL for none) */
    Pipe_Op *decode_op, *execute_op, *mem_op, *wb_op;

    /* register file state */
    uint32_t REGS[32];
    uint32_t HI, LO;

    /* program counter in fetch stage */
    uint32_t PC;

    /* information for PC update (branch recovery). Branches should use this
     * mechanism to redirect the fetch stage, and flush the ops that came after
     * the branch as necessary. */
    int branch_recover; /* set to '1' to load a new PC */
    uint32_t branch_dest; /* next fetch will be from this PC */
    int branch_flush; /* how many stages to flush during recover? (1 = fetch, 2 = fetch/decode, ...) */

    /* multiplier stall info */
    int multiplier_stall; /* number of remaining cycles until HI/LO are ready */

    /* place other information here as necessary */
    int fetch_stall_cycles;
    int mem_stall_cycles;

} Pipe_State;

// Data-cache geometry. These defaults preserve the lab specification.
// Individual parameters can be overridden at compile time with -D flags.
#ifndef DATA_CACHE_SIZE_BYTES
#define DATA_CACHE_SIZE_BYTES (64 * 1024)
#endif

#ifndef DATA_CACHE_BLOCK_SIZE_BYTES
#define DATA_CACHE_BLOCK_SIZE_BYTES 32
#endif

#ifndef DATA_CACHE_ASSOCIATIVITY
#define DATA_CACHE_ASSOCIATIVITY 8
#endif

#define DATA_CACHE_NUM_SETS \
    (DATA_CACHE_SIZE_BYTES / \
     (DATA_CACHE_BLOCK_SIZE_BYTES * DATA_CACHE_ASSOCIATIVITY))

#if DATA_CACHE_SIZE_BYTES % \
    (DATA_CACHE_BLOCK_SIZE_BYTES * DATA_CACHE_ASSOCIATIVITY) != 0
#error "Data-cache size must be divisible by block size times associativity"
#endif

#if DATA_CACHE_NUM_SETS < 1
#error "Data-cache configuration must contain at least one set"
#endif

//cache struct

typedef struct cache_block{
    uint32_t tag;
    int valid;
    int dirty;
    int lru_counter;
}cache_block;

typedef struct cache_set_I{
    cache_block cache_block[4];
}cache_set_I;

typedef struct cache_I{
    cache_set_I cache_set[64];
}cache_I;

typedef struct cache_set_D{
    cache_block cache_block[DATA_CACHE_ASSOCIATIVITY];
}cache_set_D;

typedef struct cache_D{
    cache_set_D cache_set[DATA_CACHE_NUM_SETS];
}cache_D;

/* global variable -- pipeline state */
extern Pipe_State pipe;

extern cache_I cache;
extern cache_D data_cache;

/* called during simulator startup */
void pipe_init();

/* this function calls the others */
void pipe_cycle();

/* helper: pipe stages can call this to schedule a branch recovery */
/* flushes 'flush' stages (1 = execute only, 2 = fetch/decode, ...) and then
 * sets the fetch PC to the given destination. */
void pipe_recover(int flush, uint32_t dest);

/* each of these functions implements one stage of the pipeline */
void pipe_stage_fetch();
void pipe_stage_decode();
void pipe_stage_execute();
void pipe_stage_mem();
void pipe_stage_wb();

#endif
