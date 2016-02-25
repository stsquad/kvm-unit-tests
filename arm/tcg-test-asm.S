/*
 * TCG Test assembler functions for armv7 tests.
 *
 * Copyright (C) 2016, Linaro Ltd, Alex Bennée <alex.bennee@linaro.org>
 *
 * This work is licensed under the terms of the GNU LGPL, version 2.
 *
 * These helper functions are written in pure asm to control the size
 * of the basic blocks and ensure they fit neatly into page
 * aligned chunks. The pattern of branches they follow is determined by
 * the 32 bit seed they are passed. It should be the same for each set.
 *
 * Calling convention
 *  - r0, iterations
 *  - r1, jump pattern
 *  - r2-r3, scratch
 *
 * Returns r0
 */

.arm

.section .text

/* Tight - all blocks should quickly be patched and should run
 * very fast unless irqs or smc gets in the way
 */

.global tight_start
tight_start:
        subs    r0, r0, #1
        beq     tight_end

        ror     r1, r1, #1
        tst     r1, #1
        beq     tightA
        b       tight_start

tightA:
        subs    r0, r0, #1
        beq     tight_end

        ror     r1, r1, #1
        tst     r1, #1
        beq     tightB
        b       tight_start

tightB:
        subs    r0, r0, #1
        beq     tight_end

        ror     r1, r1, #1
        tst     r1, #1
        beq     tight_start
        b       tightA

.global tight_end
tight_end:
        mov     pc, lr

/*
 * Computed jumps cannot be hardwired into the basic blocks so each one
 * will cause an exit for the main execution loop to look up the next block.
 *
 * There is some caching which should ameliorate the cost a little.
 */

        /* Align << 13 == 4096 byte alignment */
        .align 13
        .global computed_start
computed_start:
        subs    r0, r0, #1
        beq     computed_end

        /* Jump table */
        ror     r1, r1, #1
        and     r2, r1, #1
        adr     r3, computed_jump_table
        ldr     r2, [r3, r2, lsl #2]
        mov     pc, r2

        b       computed_err

computed_jump_table:
        .word   computed_start
        .word   computedA

computedA:
        subs    r0, r0, #1
        beq     computed_end

        /* Jump into code */
        ror     r1, r1, #1
        and     r2, r1, #1
        adr     r3, 1f
        add	r3, r2, lsl #2
        mov     pc, r3
1:      b       computed_start
        b       computedB

        b       computed_err


computedB:
        subs    r0, r0, #1
        beq     computed_end
        ror     r1, r1, #1

        /* Conditional register load */
        adr     r3, computedA
        tst     r1, #1
        adreq   r3, computed_start
        mov     pc, r3

        b       computed_err

computed_err:
        mov     r0, #1
        .global computed_end
computed_end:
        mov     pc, lr


/*
 * Page hoping
 *
 * Each block is in a different page, hence the blocks never get joined
 */
        /* Align << 13 == 4096 byte alignment */
        .align 13
        .global paged_start
paged_start:
        subs    r0, r0, #1
        beq     paged_end

        ror     r1, r1, #1
        tst     r1, #1
        beq     pagedA
        b       paged_start

        /* Align << 13 == 4096 byte alignment */
        .align 13
pagedA:
        subs    r0, r0, #1
        beq     paged_end

        ror     r1, r1, #1
        tst     r1, #1
        beq     pagedB
        b       paged_start

        /* Align << 13 == 4096 byte alignment */
        .align 13
pagedB:
        subs    r0, r0, #1
        beq     paged_end

        ror     r1, r1, #1
        tst     r1, #1
        beq     paged_start
        b       pagedA

        /* Align << 13 == 4096 byte alignment */
        .align 13
.global paged_end
paged_end:
        mov     pc, lr

.global test_code_end
test_code_end:
