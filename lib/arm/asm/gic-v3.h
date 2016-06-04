/*
 * All GIC* defines are lifted from include/linux/irqchip/arm-gic-v3.h
 *
 * Copyright (C) 2016, Red Hat Inc, Andrew Jones <drjones@redhat.com>
 *
 * This work is licensed under the terms of the GNU LGPL, version 2.
 */
#ifndef _ASMARM_GIC_V3_H_
#define _ASMARM_GIC_V3_H_

/*
 * Distributor registers. We assume we're running non-secure, with ARE
 * being set. Secure-only and non-ARE registers are not described.
 */
#define GICD_CTLR			0x0000
#define GICD_TYPER			0x0004
#define GICD_IIDR			0x0008
#define GICD_STATUSR			0x0010
#define GICD_SETSPI_NSR			0x0040
#define GICD_CLRSPI_NSR			0x0048
#define GICD_SETSPI_SR			0x0050
#define GICD_CLRSPI_SR			0x0058
#define GICD_SEIR			0x0068
#define GICD_IGROUPR			0x0080
#define GICD_ISENABLER			0x0100
#define GICD_ICENABLER			0x0180
#define GICD_ISPENDR			0x0200
#define GICD_ICPENDR			0x0280
#define GICD_ISACTIVER			0x0300
#define GICD_ICACTIVER			0x0380
#define GICD_IPRIORITYR			0x0400
#define GICD_ICFGR			0x0C00
#define GICD_IGRPMODR			0x0D00
#define GICD_NSACR			0x0E00
#define GICD_IROUTER			0x6000
#define GICD_IDREGS			0xFFD0
#define GICD_PIDR2			0xFFE8

/*
 * Those registers are actually from GICv2, but the spec demands that they
 * are implemented as RES0 if ARE is 1 (which we do in KVM's emulated GICv3).
 */
#define GICD_ITARGETSR			0x0800
#define GICD_SGIR			0x0F00
#define GICD_CPENDSGIR			0x0F10
#define GICD_SPENDSGIR			0x0F20

#define GICD_CTLR_RWP			(1U << 31)
#define GICD_CTLR_DS			(1U << 6)
#define GICD_CTLR_ARE_NS		(1U << 4)
#define GICD_CTLR_ENABLE_G1A		(1U << 1)
#define GICD_CTLR_ENABLE_G1		(1U << 0)

/*
 * In systems with a single security state (what we emulate in KVM)
 * the meaning of the interrupt group enable bits is slightly different
 */
#define GICD_CTLR_ENABLE_SS_G1		(1U << 1)
#define GICD_CTLR_ENABLE_SS_G0		(1U << 0)

#define GICD_TYPER_LPIS			(1U << 17)
#define GICD_TYPER_MBIS			(1U << 16)

#define GICD_TYPER_ID_BITS(typer)	((((typer) >> 19) & 0x1f) + 1)
#define GICD_TYPER_IRQS(typer)		((((typer) & 0x1f) + 1) * 32)
#define GICD_TYPER_LPIS			(1U << 17)

#define GICD_IROUTER_SPI_MODE_ONE	(0U << 31)
#define GICD_IROUTER_SPI_MODE_ANY	(1U << 31)

#define GIC_PIDR2_ARCH_MASK		0xf0
#define GIC_PIDR2_ARCH_GICv3		0x30
#define GIC_PIDR2_ARCH_GICv4		0x40

#define GIC_V3_DIST_SIZE		0x10000

/*
 * Re-Distributor registers, offsets from RD_base
 */
#define GICR_CTLR			GICD_CTLR
#define GICR_IIDR			0x0004
#define GICR_TYPER			0x0008
#define GICR_STATUSR			GICD_STATUSR
#define GICR_WAKER			0x0014
#define GICR_SETLPIR			0x0040
#define GICR_CLRLPIR			0x0048
#define GICR_SEIR			GICD_SEIR
#define GICR_PROPBASER			0x0070
#define GICR_PENDBASER			0x0078
#define GICR_INVLPIR			0x00A0
#define GICR_INVALLR			0x00B0
#define GICR_SYNCR			0x00C0
#define GICR_MOVLPIR			0x0100
#define GICR_MOVALLR			0x0110
#define GICR_ISACTIVER			GICD_ISACTIVER
#define GICR_ICACTIVER			GICD_ICACTIVER
#define GICR_IDREGS			GICD_IDREGS
#define GICR_PIDR2			GICD_PIDR2

#define GICR_CTLR_ENABLE_LPIS		(1UL << 0)

#define GICR_TYPER_CPU_NUMBER(r)	(((r) >> 8) & 0xffff)

#define GICR_WAKER_ProcessorSleep	(1U << 1)
#define GICR_WAKER_ChildrenAsleep	(1U << 2)

#define GICR_PROPBASER_NonShareable	(0U << 10)
#define GICR_PROPBASER_InnerShareable	(1U << 10)
#define GICR_PROPBASER_OuterShareable	(2U << 10)
#define GICR_PROPBASER_SHAREABILITY_MASK (3UL << 10)
#define GICR_PROPBASER_nCnB		(0U << 7)
#define GICR_PROPBASER_nC		(1U << 7)
#define GICR_PROPBASER_RaWt		(2U << 7)
#define GICR_PROPBASER_RaWb		(3U << 7)
#define GICR_PROPBASER_WaWt		(4U << 7)
#define GICR_PROPBASER_WaWb		(5U << 7)
#define GICR_PROPBASER_RaWaWt		(6U << 7)
#define GICR_PROPBASER_RaWaWb		(7U << 7)
#define GICR_PROPBASER_CACHEABILITY_MASK (7U << 7)
#define GICR_PROPBASER_IDBITS_MASK	(0x1f)

#define GICR_PENDBASER_NonShareable	(0U << 10)
#define GICR_PENDBASER_InnerShareable	(1U << 10)
#define GICR_PENDBASER_OuterShareable	(2U << 10)
#define GICR_PENDBASER_SHAREABILITY_MASK (3UL << 10)
#define GICR_PENDBASER_nCnB		(0U << 7)
#define GICR_PENDBASER_nC		(1U << 7)
#define GICR_PENDBASER_RaWt		(2U << 7)
#define GICR_PENDBASER_RaWb		(3U << 7)
#define GICR_PENDBASER_WaWt		(4U << 7)
#define GICR_PENDBASER_WaWb		(5U << 7)
#define GICR_PENDBASER_RaWaWt		(6U << 7)
#define GICR_PENDBASER_RaWaWb		(7U << 7)
#define GICR_PENDBASER_CACHEABILITY_MASK (7U << 7)

/*
 * Re-Distributor registers, offsets from SGI_base
 */
#define GICR_IGROUPR0			GICD_IGROUPR
#define GICR_ISENABLER0			GICD_ISENABLER
#define GICR_ICENABLER0			GICD_ICENABLER
#define GICR_ISPENDR0			GICD_ISPENDR
#define GICR_ICPENDR0			GICD_ICPENDR
#define GICR_ISACTIVER0			GICD_ISACTIVER
#define GICR_ICACTIVER0			GICD_ICACTIVER
#define GICR_IPRIORITYR0		GICD_IPRIORITYR
#define GICR_ICFGR0			GICD_ICFGR
#define GICR_IGRPMODR0			GICD_IGRPMODR
#define GICR_NSACR			GICD_NSACR

#define GICR_TYPER_PLPIS		(1U << 0)
#define GICR_TYPER_VLPIS		(1U << 1)
#define GICR_TYPER_LAST			(1U << 4)

#define GIC_V3_REDIST_SIZE		0x20000

#define LPI_PROP_GROUP1			(1 << 1)
#define LPI_PROP_ENABLED		(1 << 0)

/*
 * ITS registers, offsets from ITS_base
 */
#define GITS_CTLR			0x0000
#define GITS_IIDR			0x0004
#define GITS_TYPER			0x0008
#define GITS_CBASER			0x0080
#define GITS_CWRITER			0x0088
#define GITS_CREADR			0x0090
#define GITS_BASER			0x0100
#define GITS_PIDR2			GICR_PIDR2

#define GITS_TRANSLATER			0x10040

#define GITS_CTLR_ENABLE		(1U << 0)
#define GITS_CTLR_QUIESCENT		(1U << 31)

#define GITS_TYPER_DEVBITS_SHIFT	13
#define GITS_TYPER_DEVBITS(r)		((((r) >> GITS_TYPER_DEVBITS_SHIFT) & 0x1f) + 1)
#define GITS_TYPER_PTA			(1UL << 19)

#define GITS_CBASER_VALID		(1UL << 63)
#define GITS_CBASER_nCnB		(0UL << 59)
#define GITS_CBASER_nC			(1UL << 59)
#define GITS_CBASER_RaWt		(2UL << 59)
#define GITS_CBASER_RaWb		(3UL << 59)
#define GITS_CBASER_WaWt		(4UL << 59)
#define GITS_CBASER_WaWb		(5UL << 59)
#define GITS_CBASER_RaWaWt		(6UL << 59)
#define GITS_CBASER_RaWaWb		(7UL << 59)
#define GITS_CBASER_CACHEABILITY_MASK	(7UL << 59)
#define GITS_CBASER_NonShareable	(0UL << 10)
#define GITS_CBASER_InnerShareable	(1UL << 10)
#define GITS_CBASER_OuterShareable	(2UL << 10)
#define GITS_CBASER_SHAREABILITY_MASK	(3UL << 10)

#define GITS_BASER_NR_REGS		8

#define GITS_BASER_VALID		(1UL << 63)
#define GITS_BASER_nCnB			(0UL << 59)
#define GITS_BASER_nC			(1UL << 59)
#define GITS_BASER_RaWt			(2UL << 59)
#define GITS_BASER_RaWb			(3UL << 59)
#define GITS_BASER_WaWt			(4UL << 59)
#define GITS_BASER_WaWb			(5UL << 59)
#define GITS_BASER_RaWaWt		(6UL << 59)
#define GITS_BASER_RaWaWb		(7UL << 59)
#define GITS_BASER_CACHEABILITY_MASK	(7UL << 59)
#define GITS_BASER_TYPE_SHIFT		(56)
#define GITS_BASER_TYPE(r)		(((r) >> GITS_BASER_TYPE_SHIFT) & 7)
#define GITS_BASER_ENTRY_SIZE_SHIFT	(48)
#define GITS_BASER_ENTRY_SIZE(r)	((((r) >> GITS_BASER_ENTRY_SIZE_SHIFT) & 0xff) + 1)
#define GITS_BASER_NonShareable		(0UL << 10)
#define GITS_BASER_InnerShareable	(1UL << 10)
#define GITS_BASER_OuterShareable	(2UL << 10)
#define GITS_BASER_SHAREABILITY_SHIFT	(10)
#define GITS_BASER_SHAREABILITY_MASK	(3UL << GITS_BASER_SHAREABILITY_SHIFT)
#define GITS_BASER_PAGE_SIZE_SHIFT	(8)
#define GITS_BASER_PAGE_SIZE_4K		(0UL << GITS_BASER_PAGE_SIZE_SHIFT)
#define GITS_BASER_PAGE_SIZE_16K	(1UL << GITS_BASER_PAGE_SIZE_SHIFT)
#define GITS_BASER_PAGE_SIZE_64K	(2UL << GITS_BASER_PAGE_SIZE_SHIFT)
#define GITS_BASER_PAGE_SIZE_MASK	(3UL << GITS_BASER_PAGE_SIZE_SHIFT)
#define GITS_BASER_PAGES_MAX		256

#define GITS_BASER_TYPE_NONE		0
#define GITS_BASER_TYPE_DEVICE		1
#define GITS_BASER_TYPE_VCPU		2
#define GITS_BASER_TYPE_CPU		3
#define GITS_BASER_TYPE_COLLECTION	4
#define GITS_BASER_TYPE_RESERVED5	5
#define GITS_BASER_TYPE_RESERVED6	6
#define GITS_BASER_TYPE_RESERVED7	7

/*
 * ITS commands
 */
#define GITS_CMD_MAPD			0x08
#define GITS_CMD_MAPC			0x09
#define GITS_CMD_MAPVI			0x0a
#define GITS_CMD_MOVI			0x01
#define GITS_CMD_DISCARD		0x0f
#define GITS_CMD_INV			0x0c
#define GITS_CMD_MOVALL			0x0e
#define GITS_CMD_INVALL			0x0d
#define GITS_CMD_INT			0x03
#define GITS_CMD_CLEAR			0x04
#define GITS_CMD_SYNC			0x05

/*
 * CPU interface registers
 */
#define ICC_CTLR_EL1_EOImode_drop_dir	(0U << 1)
#define ICC_CTLR_EL1_EOImode_drop	(1U << 1)
#define ICC_SRE_EL1_SRE			(1U << 0)

#include <asm/arch_gicv3.h>

#define SZ_64K 0x10000

#ifndef __ASSEMBLY__
#include <libcflat.h>
#include <asm/processor.h>
#include <asm/setup.h>
#include <asm/smp.h>
#include <asm/io.h>

struct gicv3_data {
	void *dist_base;
	void *redist_base[NR_CPUS];
	unsigned int irq_nr;
};
extern struct gicv3_data gicv3_data;

#define gicv3_dist_base()		(gicv3_data.dist_base)
#define gicv3_redist_base()		(gicv3_data.redist_base[smp_processor_id()])
#define gicv3_sgi_base()		(gicv3_data.redist_base[smp_processor_id()] + SZ_64K)

extern int gicv3_init(void);
extern void gicv3_enable_defaults(void);

static inline void gicv3_do_wait_for_rwp(void *base)
{
	int count = 100000;	/* 1s */

	while (readl(base + GICD_CTLR) & GICD_CTLR_RWP) {
		if (!--count) {
			printf("GICv3: RWP timeout!\n");
			abort();
		}
		cpu_relax();
		udelay(10);
	};
}

static inline void gicv3_dist_wait_for_rwp(void)
{
	gicv3_do_wait_for_rwp(gicv3_dist_base());
}

static inline void gicv3_redist_wait_for_rwp(void)
{
	gicv3_do_wait_for_rwp(gicv3_redist_base());
}

static inline u32 mpidr_compress(u64 mpidr)
{
	u64 compressed = mpidr & MPIDR_HWID_BITMASK;

	compressed = (((compressed >> 32) & 0xff) << 24) | compressed;
	return compressed;
}

static inline u64 mpidr_uncompress(u32 compressed)
{
	u64 mpidr = ((u64)compressed >> 24) << 32;

	mpidr |= compressed & MPIDR_HWID_BITMASK;
	return mpidr;
}

#endif /* !__ASSEMBLY__ */
#endif /* _ASMARM_GIC_V3_H_ */
