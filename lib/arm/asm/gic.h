/*
 * Copyright (C) 2016, Red Hat Inc, Andrew Jones <drjones@redhat.com>
 *
 * This work is licensed under the terms of the GNU LGPL, version 2.
 */
#ifndef _ASMARM_GIC_H_
#define _ASMARM_GIC_H_


/* Distributor registers */
#define GICD_CTLR			0x0000
#define GICD_TYPER			0x0004
#define GICD_IGROUPR			0x0080
#define GICD_ISENABLER			0x0100
#define GICD_IPRIORITYR			0x0400
#define GICD_SGIR			0x0f00

#define GICD_TYPER_IRQS(typer)		((((typer) & 0x1f) + 1) * 32)
#define GICD_INT_EN_SET_SGI		0x0000ffff
#define GICD_INT_DEF_PRI_X4		0xa0a0a0a0

/* CPU interface registers */
#define GICC_CTLR			0x0000
#define GICC_PMR			0x0004
#define GICC_IAR			0x000c
#define GICC_EOIR			0x0010

#define GICC_INT_PRI_THRESHOLD		0xf0
#define GICC_INT_SPURIOUS		0x3ff

#include <asm/gic-v2.h>
#include <asm/gic-v3.h>

#ifndef __ASSEMBLY__
#include <asm/cpumask.h>

/*
 * gic_init will try to find all known gics, and then
 * initialize the gic data for the one found.
 * returns
 *  0   : no gic was found
 *  > 0 : the gic version of the gic found
 */
extern int gic_init(void);

/*
 * gic_common_ops collects useful functions for unit tests which
 * aren't concerned with the gic version they're using.
 */
struct gic_common_ops {
	int gic_version;
	void (*enable_defaults)(void);
	u32 (*read_iar)(void);
	u32 (*iar_irqnr)(u32 iar);
	void (*write_eoir)(u32 irqstat);
	void (*ipi_send_single)(int irq, int cpu);
	void (*ipi_send_mask)(int irq, const cpumask_t *dest);
};

extern struct gic_common_ops *gic_common_ops;

static inline int gic_version(void)
{
	assert(gic_common_ops);
	return gic_common_ops->gic_version;
}

static inline void gic_enable_defaults(void)
{
	if (!gic_common_ops) {
		int ret = gic_init();
		assert(ret != 0);
	} else
		assert(gic_common_ops->enable_defaults);
	gic_common_ops->enable_defaults();
}

static inline u32 gic_read_iar(void)
{
	assert(gic_common_ops && gic_common_ops->read_iar);
	return gic_common_ops->read_iar();
}

static inline u32 gic_iar_irqnr(u32 iar)
{
	assert(gic_common_ops && gic_common_ops->iar_irqnr);
	return gic_common_ops->iar_irqnr(iar);
}

static inline void gic_write_eoir(u32 irqstat)
{
	assert(gic_common_ops && gic_common_ops->write_eoir);
	gic_common_ops->write_eoir(irqstat);
}

static inline void gic_ipi_send_single(int irq, int cpu)
{
	assert(gic_common_ops && gic_common_ops->ipi_send_single);
	gic_common_ops->ipi_send_single(irq, cpu);
}

static inline void gic_ipi_send_mask(int irq, const cpumask_t *dest)
{
	assert(gic_common_ops && gic_common_ops->ipi_send_mask);
	gic_common_ops->ipi_send_mask(irq, dest);
}

#endif /* !__ASSEMBLY__ */
#endif /* _ASMARM_GIC_H_ */
