.text
    # Perform the same 8,192 loads as stride1.s, but use a stride of 16 words
    # (64 bytes). Consecutive accesses skip an entire 32-byte cache block, so
    # every data access is expected to miss when streaming through the array.
    # The final address remains inside the simulator's 1 MiB data region.
    lui   $s0, 0x1000       # data-memory base: 0x10000000
    addiu $t0, $zero, 8192  # number of memory accesses
    addu  $t1, $s0, $zero   # current address
    addu  $t2, $zero, $zero # checksum

Loop:
    lw    $t3, 0($t1)
    addu  $t2, $t2, $t3
    addiu $t1, $t1, 64      # stride = 16 words = 64 bytes
    addiu $t0, $t0, -1
    bne   $t0, $zero, Loop

    addiu $v0, $zero, 10
    syscall
