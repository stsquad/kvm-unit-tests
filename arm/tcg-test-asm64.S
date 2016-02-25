/*
 * TCG Test assembler functions for armv8 tests.
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
 *  - x0, iterations
 *  - x1, jump pattern
 *  - x2-x3, scratch
 *
 * Returns x0
 */

.section .text

/* Tight - all blocks should quickly be patched and should run
 * very fast unless irqs or smc gets in the way
 */

.global tight_start
tight_start:
        subs    x0, x0, #1
        beq     tight_end

        ror     x1, x1, #1
        tst     x1, #1
        beq     tightA
        b       tight_start

tightA:
        subs    x0, x0, #1
        beq     tight_end

        ror     x1, x1, #1
        tst     x1, #1
        beq     tightB
        b       tight_start

tightB:
        subs    x0, x0, #1
        beq     tight_end

        ror     x1, x1, #1
        tst     x1, #1
        beq     tight_start
        b       tightA

.global tight_end
tight_end:
        ret

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
        subs    x0, x0, #1
        beq     computed_end

        /* Jump table */
        ror     x1, x1, #1
        and     x2, x1, #1
        adr     x3, computed_jump_table
        ldr     x2, [x3, x2, lsl #3]
        br      x2

        b       computed_err

computed_jump_table:
        .quad   computed_start
        .quad   computedA

computedA:
        subs    x0, x0, #1
        beq     computed_end

        /* Jump into code */
        ror     x1, x1, #1
        and     x2, x1, #1
        adr     x3, 1f
        add	x3, x3, x2, lsl #2
        br      x3
1:      b       computed_start
        b       computedB

        b       computed_err


computedB:
        subs    x0, x0, #1
        beq     computed_end
        ror     x1, x1, #1

        /* Conditional register load */
        adr     x2, computedA
        adr     x3, computed_start
        tst     x1, #1
        csel    x2, x3, x2, eq
        br      x2

        b       computed_err

computed_err:
        mov     x0, #1
        .global computed_end
computed_end:
        ret


/*
 * Page hoping
 *
 * Each block is in a different page, hence the blocks never get joined
 */
        /* Align << 13 == 4096 byte alignment */
        .align 13
        .global paged_start
paged_start:
        subs    x0, x0, #1
        beq     paged_end

        ror     x1, x1, #1
        tst     x1, #1
        beq     pagedA
        b       paged_start

        /* Align << 13 == 4096 byte alignment */
        .align 13
pagedA:
        subs    x0, x0, #1
        beq     paged_end

        ror     x1, x1, #1
        tst     x1, #1
        beq     pagedB
        b       paged_start

        /* Align << 13 == 4096 byte alignment */
        .align 13
pagedB:
        subs    x0, x0, #1
        beq     paged_end

        ror     x1, x1, #1
        tst     x1, #1
        beq     paged_start
        b       pagedA

        /* Align << 13 == 4096 byte alignment */
        .align 13
.global paged_end
paged_end:
        ret

.global test_code_end
test_code_end:
