.text
    # Scan a 48 KiB working set four times, touching one word per 32-byte block.
    # There are 1,536 blocks per pass and 6,144 total data accesses.
    #
    # A cache smaller than 48 KiB cannot retain the full working set, so the
    # repeated sequential scans miss continually. A cache of at least 48 KiB
    # incurs compulsory misses on the first pass and hits on later passes.
    lui   $s0, 0x1000       # working-set base: 0x10000000
    addiu $t0, $zero, 4     # number of complete passes
    sll   $zero, $zero, 0   # padding keeps the hot loops within cache blocks
    sll   $zero, $zero, 0
    sll   $zero, $zero, 0
    sll   $zero, $zero, 0

OuterLoop:
    addu  $t1, $s0, $zero
    addiu $t2, $zero, 1536  # 48 KiB / 32 bytes per block

InnerLoop:
    lw    $t3, 0($t1)
    addiu $t1, $t1, 32      # access the next cache block
    addiu $t2, $t2, -1
    bne   $t2, $zero, InnerLoop

    addiu $t0, $t0, -1
    bne   $t0, $zero, OuterLoop

    addiu $v0, $zero, 10
    syscall
