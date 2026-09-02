.text
    # Stream through 8,192 words (32 KiB) with stride 1.
    # A stride of one word advances the address by 4 bytes. Since a cache
    # block is 32 bytes, each miss is followed by seven accesses to the same
    # block. With an initially empty cache, this gives 1,024 data-cache misses.
    lui   $s0, 0x1000       # data-memory base: 0x10000000
    addiu $t0, $zero, 8192  # number of memory accesses
    addu  $t1, $s0, $zero   # current address
    addu  $t2, $zero, $zero # checksum

Loop:
    lw    $t3, 0($t1)
    addu  $t2, $t2, $t3
    addiu $t1, $t1, 4       # stride = 1 word
    addiu $t0, $t0, -1
    bne   $t0, $zero, Loop

    addiu $v0, $zero, 10
    syscall
