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
#define __ACCESS_CP15_64(Op1, CRm)		p15, Op1, %Q0, %R0, CRm

#define ICC_EOIR1			__ACCESS_CP15(c12, 0, c12, 1)
#define ICC_DIR				__ACCESS_CP15(c12, 0, c11, 1)
#define ICC_IAR1			__ACCESS_CP15(c12, 0, c12, 0)
#define ICC_SGI1R			__ACCESS_CP15_64(0, c12)
#define ICC_PMR				__ACCESS_CP15(c4, 0, c6, 0)
#define ICC_CTLR			__ACCESS_CP15(c12, 0, c12, 4)
#define ICC_SRE				__ACCESS_CP15(c12, 0, c12, 5)
#define ICC_IGRPEN1			__ACCESS_CP15(c12, 0, c12, 7)

#define ICC_HSRE			__ACCESS_CP15(c12, 4, c9, 5)

#define ICH_VSEIR			__ACCESS_CP15(c12, 4, c9, 4)
#define ICH_HCR				__ACCESS_CP15(c12, 4, c11, 0)
#define ICH_VTR				__ACCESS_CP15(c12, 4, c11, 1)
#define ICH_MISR			__ACCESS_CP15(c12, 4, c11, 2)
#define ICH_EISR			__ACCESS_CP15(c12, 4, c11, 3)
#define ICH_ELSR			__ACCESS_CP15(c12, 4, c11, 5)
#define ICH_VMCR			__ACCESS_CP15(c12, 4, c11, 7)

#define __LR0(x)			__ACCESS_CP15(c12, 4, c12, x)
#define __LR8(x)			__ACCESS_CP15(c12, 4, c13, x)

#define ICH_LR0				__LR0(0)
#define ICH_LR1				__LR0(1)
#define ICH_LR2				__LR0(2)
#define ICH_LR3				__LR0(3)
#define ICH_LR4				__LR0(4)
#define ICH_LR5				__LR0(5)
#define ICH_LR6				__LR0(6)
#define ICH_LR7				__LR0(7)
#define ICH_LR8				__LR8(0)
#define ICH_LR9				__LR8(1)
#define ICH_LR10			__LR8(2)
#define ICH_LR11			__LR8(3)
#define ICH_LR12			__LR8(4)
#define ICH_LR13			__LR8(5)
#define ICH_LR14			__LR8(6)
#define ICH_LR15			__LR8(7)

/* LR top half */
#define __LRC0(x)			__ACCESS_CP15(c12, 4, c14, x)
#define __LRC8(x)			__ACCESS_CP15(c12, 4, c15, x)

#define ICH_LRC0			__LRC0(0)
#define ICH_LRC1			__LRC0(1)
#define ICH_LRC2			__LRC0(2)
#define ICH_LRC3			__LRC0(3)
#define ICH_LRC4			__LRC0(4)
#define ICH_LRC5			__LRC0(5)
#define ICH_LRC6			__LRC0(6)
#define ICH_LRC7			__LRC0(7)
#define ICH_LRC8			__LRC8(0)
#define ICH_LRC9			__LRC8(1)
#define ICH_LRC10			__LRC8(2)
#define ICH_LRC11			__LRC8(3)
#define ICH_LRC12			__LRC8(4)
#define ICH_LRC13			__LRC8(5)
#define ICH_LRC14			__LRC8(6)
#define ICH_LRC15			__LRC8(7)

#define __AP0Rx(x)			__ACCESS_CP15(c12, 4, c8, x)
#define ICH_AP0R0			__AP0Rx(0)
#define ICH_AP0R1			__AP0Rx(1)
#define ICH_AP0R2			__AP0Rx(2)
#define ICH_AP0R3			__AP0Rx(3)

#define __AP1Rx(x)			__ACCESS_CP15(c12, 4, c9, x)
#define ICH_AP1R0			__AP1Rx(0)
#define ICH_AP1R1			__AP1Rx(1)
#define ICH_AP1R2			__AP1Rx(2)
#define ICH_AP1R3			__AP1Rx(3)

/* Low-level accessors */

static inline void gicv3_write_eoir(u32 irq)
{
	asm volatile("mcr " __stringify(ICC_EOIR1) : : "r" (irq));
	isb();
}

static inline void gicv3_write_dir(u32 val)
{
	asm volatile("mcr " __stringify(ICC_DIR) : : "r" (val));
	isb();
}

static inline u32 gicv3_read_iar(void)
{
	u32 irqstat;

	asm volatile("mrc " __stringify(ICC_IAR1) : "=r" (irqstat));
	dsb(sy);
	return irqstat;
}

static inline void gicv3_write_pmr(u32 val)
{
	asm volatile("mcr " __stringify(ICC_PMR) : : "r" (val));
}

static inline void gicv3_write_ctlr(u32 val)
{
	asm volatile("mcr " __stringify(ICC_CTLR) : : "r" (val));
	isb();
}

static inline void gicv3_write_grpen1(u32 val)
{
	asm volatile("mcr " __stringify(ICC_IGRPEN1) : : "r" (val));
	isb();
}

static inline void gicv3_write_sgi1r(u64 val)
{
	asm volatile("mcrr " __stringify(ICC_SGI1R) : : "r" (val));
}

static inline u32 gicv3_read_sre(void)
{
	u32 val;

	asm volatile("mrc " __stringify(ICC_SRE) : "=r" (val));
	return val;
}

static inline void gicv3_write_sre(u32 val)
{
	asm volatile("mcr " __stringify(ICC_SRE) : : "r" (val));
	isb();
}

/*
 * Even in 32bit systems that use LPAE, there is no guarantee that the I/O
 * interface provides true 64bit atomic accesses, so using strd/ldrd doesn't
 * make much sense.
 * Moreover, 64bit I/O emulation is extremely difficult to implement on
 * AArch32, since the syndrome register doesn't provide any information for
 * them.
 * Consequently, the following IO helpers use 32bit accesses.
 *
 * There are only two registers that need 64bit accesses in this driver:
 * - GICD_IROUTERn, contain the affinity values associated to each interrupt.
 *   The upper-word (aff3) will always be 0, so there is no need for a lock.
 * - GICR_TYPER is an ID register and doesn't need atomicity.
 */
static inline void gicv3_write_irouter(u64 val, volatile void __iomem *addr)
{
	writel((u32)val, addr);
	writel((u32)(val >> 32), addr + 4);
}

static inline u64 gicv3_read_typer(const volatile void __iomem *addr)
{
	u64 val;

	val = readl(addr);
	val |= (u64)readl(addr + 4) << 32;
	return val;
}

#endif /* !__ASSEMBLY__ */
#endif /* _ASMARM_ARCH_GICV3_H_ */
