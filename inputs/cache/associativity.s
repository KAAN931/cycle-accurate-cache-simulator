.text
    # Repeatedly access eight blocks that map to the same data-cache set.
    # A 64 KiB separation maps to the same set for the 1-, 2-, 4-, and 8-way
    # configurations when total cache size is fixed at 64 KiB.
    #
    # With 8 ways: the first round misses, then all eight blocks remain cached.
    # With fewer than 8 ways: the cyclic accesses continually evict each other.
    lui   $s0, 0x1000       # first address: 0x10000000
    lui   $s1, 0x0001       # same-set separation: 64 KiB
    addiu $t0, $zero, 1024  # repeat the eight-block sequence 1,024 times
    sll   $zero, $zero, 0   # padding keeps the hot loops within cache blocks
    sll   $zero, $zero, 0
    sll   $zero, $zero, 0

OuterLoop:
    addu  $t1, $s0, $zero
    addiu $t2, $zero, 8

InnerLoop:
    lw    $t3, 0($t1)
    addu  $t1, $t1, $s1    # advance by 64 KiB to another same-set block
    addiu $t2, $t2, -1
    bne   $t2, $zero, InnerLoop

    addiu $t0, $t0, -1
    bne   $t0, $zero, OuterLoop

    addiu $v0, $zero, 10
    syscall
