/*
 * All ripped off from arch/arm/include/asm/arch_gicv3.h
 *
 * Copyright (C) 2016, Red Hat Inc, Andrew Jones <drjones@redhat.com>
 *
 * This work is licensed under the terms of the GNU LGPL, version 2.
 */
#ifndef _ASMARM_ARCH_GICV3_H_
#define _ASMARM_ARCH_GICV3_H_

#ifndef __ASSEMBLY__
#include <libcflat.h>
#include <asm/barrier.h>
#include <asm/io.h>

#define __stringify xstr

#define __ACCESS_CP15(CRn, Op1, CRm, Op2)	p15, Op1, %0, CRn, CRm, Op2

#define ICC_PMR				__ACCESS_CP15(c4, 0, c6, 0)
#define ICC_IGRPEN1			__ACCESS_CP15(c12, 0, c12, 7)

static inline void gicv3_write_pmr(u32 val)
{
	asm volatile("mcr " __stringify(ICC_PMR) : : "r" (val));
}

static inline void gicv3_write_grpen1(u32 val)
{
	asm volatile("mcr " __stringify(ICC_IGRPEN1) : : "r" (val));
	isb();
}

/*
 * We may access GICR_TYPER and GITS_TYPER by reading both the TYPER
 * offset and the following offset (+ 4) and then combining them to
 * form a 64-bit address.
 */
static inline u64 gicv3_read_typer(const volatile void __iomem *addr)
{
	u64 val = readl(addr);
	val |= (u64)readl(addr + 4) << 32;
	return val;
}

#endif /* !__ASSEMBLY__ */
#endif /* _ASMARM_ARCH_GICV3_H_ */
