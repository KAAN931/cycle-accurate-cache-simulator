.text
    # Access the same word 8,192 times to exercise strong temporal locality.
    # With an initially empty data cache, the first load is a compulsory miss
    # and the remaining 8,191 loads should hit.
    lui   $s0, 0x1000       # target address: 0x10000000
    addiu $t0, $zero, 8192  # number of accesses

Loop:
    lw    $t1, 0($s0)       # repeatedly access the same word
    addiu $t0, $t0, -1
    bne   $t0, $zero, Loop

    addiu $v0, $zero, 10
    syscall
