/*
 * Copyright (C) 2016, Red Hat Inc, Andrew Jones <drjones@redhat.com>
 *
 * This work is licensed under the terms of the GNU LGPL, version 2.
 */
#include <libcflat.h>
#include <devicetree.h>
#include <asm/gic.h>
#include <asm/smp.h>
#include <asm/io.h>

struct gicv2_data gicv2_data;
struct gicv3_data gicv3_data;

/*
 * Documentation/devicetree/bindings/interrupt-controller/arm,gic.txt
 * Documentation/devicetree/bindings/interrupt-controller/arm,gic-v3.txt
 */
static bool
gic_get_dt_bases(const char *compatible, void **base1, void **base2)
{
	struct dt_pbus_reg reg;
	struct dt_device gic;
	struct dt_bus bus;
	int node, ret;

	dt_bus_init_defaults(&bus);
	dt_device_init(&gic, &bus, NULL);

	node = dt_device_find_compatible(&gic, compatible);
	assert(node >= 0 || node == -FDT_ERR_NOTFOUND);

	if (node == -FDT_ERR_NOTFOUND)
		return false;

	dt_device_bind_node(&gic, node);

	ret = dt_pbus_translate(&gic, 0, &reg);
	assert(ret == 0);
	*base1 = ioremap(reg.addr, reg.size);

	ret = dt_pbus_translate(&gic, 1, &reg);
	assert(ret == 0);
	*base2 = ioremap(reg.addr, reg.size);

	return true;
}

int gicv2_init(void)
{
	return gic_get_dt_bases("arm,cortex-a15-gic",
			&gicv2_data.dist_base, &gicv2_data.cpu_base);
}

int gicv3_init(void)
{
	return gic_get_dt_bases("arm,gic-v3", &gicv3_data.dist_base,
			&gicv3_data.redist_base[0]);
}

int gic_init(void)
{
	if (gicv2_init())
		return 2;
	else if (gicv3_init())
		return 3;
	return 0;
}

void gicv2_enable_defaults(void)
{
	if (smp_processor_id() == 0) {
		writel(GICD_INT_DEF_PRI_X4, gicv2_dist_base() + GIC_DIST_PRI);
		writel(GICD_INT_EN_SET_SGI, gicv2_dist_base() + GIC_DIST_ENABLE_SET);
		writel(GICD_ENABLE, gicv2_dist_base() + GIC_DIST_CTRL);
	}
	writel(GICC_INT_PRI_THRESHOLD, gicv2_cpu_base() + GIC_CPU_PRIMASK);
	writel(GICC_ENABLE, gicv2_cpu_base() + GIC_CPU_CTRL);
}

void gicv3_set_redist_base(void)
{
	u32 aff = mpidr_compress(get_mpidr());
	void *ptr = gicv3_data.redist_base[0];
	u64 typer;

	do {
		typer = gicv3_read_typer(ptr + GICR_TYPER);
		if ((typer >> 32) == aff) {
			gicv3_redist_base() = ptr;
			return;
		}
		ptr += SZ_64K * 2; /* skip RD_base and SGI_base */
	} while (!(typer & GICR_TYPER_LAST));
	assert(0);
}

void gicv3_enable_defaults(void)
{
	void *dist = gicv3_dist_base();
	void *sgi_base;
	unsigned int i;

	if (smp_processor_id() == 0) {
		u32 typer = readl(dist + GICD_TYPER);

		gicv3_data.irq_nr = GICD_TYPER_IRQS(typer);
		if (gicv3_data.irq_nr > 1020) {
			printf("GICD_TYPER_IRQS reported %d! "
			       "Clamping to max=1020.\n", 1020);
			gicv3_data.irq_nr = 1020;
		}

		writel(0, dist + GICD_CTLR);
		gicv3_dist_wait_for_rwp();

		for (i = 32; i < gicv3_data.irq_nr; i += 32)
			writel(~0, dist + GICD_IGROUPR + i / 8);

		writel(GICD_CTLR_ARE_NS | GICD_CTLR_ENABLE_G1A | GICD_CTLR_ENABLE_G1,
		       dist + GICD_CTLR);
		gicv3_dist_wait_for_rwp();
	}

	if (!gicv3_redist_base())
		gicv3_set_redist_base();
	sgi_base = gicv3_sgi_base();

	writel(~0, sgi_base + GICR_IGROUPR0);

	writel(GICD_INT_EN_CLR_X32, sgi_base + GIC_DIST_ACTIVE_CLEAR);
	writel(GICD_INT_EN_CLR_PPI, sgi_base + GIC_DIST_ENABLE_CLEAR);
	writel(GICD_INT_EN_SET_SGI, sgi_base + GIC_DIST_ENABLE_SET);

	for (i = 0; i < 32; i += 4)
		writel(GICD_INT_DEF_PRI_X4, sgi_base + GIC_DIST_PRI + i);
	gicv3_redist_wait_for_rwp();

	gicv3_write_pmr(0xf0);
	gicv3_write_ctlr(ICC_CTLR_EL1_EOImode_drop_dir);
	gicv3_write_grpen1(1);
}
