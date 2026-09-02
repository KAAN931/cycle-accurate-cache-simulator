.text
    # Access a target word, delay its reuse with exactly eight other blocks
    # that map to the same cache set, and then access the target again.
    #
    # Exact eviction parameters for the specified data cache:
    #   associativity       = 8 ways
    #   number of sets      = 256
    #   block size          = 32 bytes
    #   same-set separation = 256 * 32 = 8,192 bytes (0x2000)
    #
    # The target plus seven conflicting blocks fill the set's eight ways.
    # The eighth conflicting block evicts the target because it is the LRU
    # block. Therefore, the final target access should miss. If the loop count
    # is changed from 8 to 7, the final target access should instead hit.
    lui   $s0, 0x1000       # target address: 0x10000000
    lw    $t2, 0($s0)       # initial target access: compulsory miss

    addiu $t1, $s0, 8192    # first same-set conflicting block
    addiu $t0, $zero, 8     # exact conflicts required to evict target

ConflictLoop:
    lw    $t3, 0($t1)
    addu  $t2, $t2, $t3    # checksum
    addiu $t1, $t1, 8192    # next block with the same set index
    addiu $t0, $t0, -1
    bne   $t0, $zero, ConflictLoop

    lw    $t4, 0($s0)       # delayed reuse: expected cache miss
    addu  $t2, $t2, $t4

    addiu $v0, $zero, 10
    syscall
