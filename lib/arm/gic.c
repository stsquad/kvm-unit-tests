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

/*
 * Documentation/devicetree/bindings/interrupt-controller/arm,gic.txt
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

int gic_init(void)
{
	if (gicv2_init())
		return 2;
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
