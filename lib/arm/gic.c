/*
 * Copyright (C) 2016, Red Hat Inc, Andrew Jones <drjones@redhat.com>
 *
 * This work is licensed under the terms of the GNU LGPL, version 2.
 */
#include <devicetree.h>
#include <asm/gic.h>
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
	void *dist = gicv2_dist_base();
	void *cpu_base = gicv2_cpu_base();
	unsigned int i;

	gicv2_data.irq_nr = GICD_TYPER_IRQS(readl(dist + GICD_TYPER));
	if (gicv2_data.irq_nr > 1020)
		gicv2_data.irq_nr = 1020;

	for (i = 0; i < gicv2_data.irq_nr; i += 4)
		writel(GICD_INT_DEF_PRI_X4, dist + GICD_IPRIORITYR + i);

	writel(GICD_INT_EN_SET_SGI, dist + GICD_ISENABLER + 0);
	writel(GICD_ENABLE, dist + GICD_CTLR);

	writel(GICC_INT_PRI_THRESHOLD, cpu_base + GICC_PMR);
	writel(GICC_ENABLE, cpu_base + GICC_CTLR);
}

void gicv3_set_redist_base(size_t stride)
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
		ptr += stride; /* skip RD_base, SGI_base, etc. */
	} while (!(typer & GICR_TYPER_LAST));

	/* should never reach here */
	assert(0);
}

void gicv3_enable_defaults(void)
{
	void *dist = gicv3_dist_base();
	void *sgi_base;
	unsigned int i;

	gicv3_data.irq_nr = GICD_TYPER_IRQS(readl(dist + GICD_TYPER));
	if (gicv3_data.irq_nr > 1020)
		gicv3_data.irq_nr = 1020;

	writel(0, dist + GICD_CTLR);
	gicv3_dist_wait_for_rwp();

	writel(GICD_CTLR_ARE_NS | GICD_CTLR_ENABLE_G1A | GICD_CTLR_ENABLE_G1,
	       dist + GICD_CTLR);
	gicv3_dist_wait_for_rwp();

	for (i = 0; i < gicv3_data.irq_nr; i += 4)
		writel(~0, dist + GICD_IGROUPR + i);

	if (!gicv3_redist_base())
		gicv3_set_redist_base(SZ_64K * 2);
	sgi_base = gicv3_sgi_base();

	writel(~0, sgi_base + GICR_IGROUPR0);

	for (i = 0; i < 16; i += 4)
		writel(GICD_INT_DEF_PRI_X4, sgi_base + GICR_IPRIORITYR0 + i);

	writel(GICD_INT_EN_SET_SGI, sgi_base + GICR_ISENABLER0);

	gicv3_write_pmr(GICC_INT_PRI_THRESHOLD);
	gicv3_write_grpen1(1);
}
