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

static u32 gicv2_read_iar(void)
{
	return readl(gicv2_cpu_base() + GICC_IAR);
}

static u32 gicv2_iar_irqnr(u32 iar)
{
	return iar & GICC_IAR_INT_ID_MASK;
}

static void gicv2_write_eoir(u32 irqstat)
{
	writel(irqstat, gicv2_cpu_base() + GICC_EOIR);
}

static void gicv2_ipi_send(int cpu, int irq)
{
	assert(cpu < 8);
	assert(irq < 16);
	writel(1 << (cpu + 16) | irq, gicv2_dist_base() + GICD_SGIR);
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

static u32 gicv3_iar_irqnr(u32 iar)
{
	return iar;
}

void gicv3_ipi_send_tlist(cpumask_t *mask, int irq)
{
	u16 tlist;
	int cpu;

	assert(irq < 16);

	/*
	 * For each cpu in the mask collect its peers, which are also in
	 * the mask, in order to form target lists.
	 */
	for_each_cpu(cpu, mask) {
		u64 mpidr = cpus[cpu], sgi1r;
		u64 cluster_id;

		/*
		 * GICv3 can send IPIs to up 16 peer cpus with a single
		 * write to ICC_SGI1R_EL1 (using the target list). Peers
		 * are cpus that have nearly identical MPIDRs, the only
		 * difference being Aff0. The matching upper affinity
		 * levels form the cluster ID.
		 */
		cluster_id = mpidr & ~0xffUL;
		tlist = 0;

		/*
		 * Sort of open code for_each_cpu in order to have a
		 * nested for_each_cpu loop.
		 */
		while (cpu < nr_cpus) {
			if ((mpidr & 0xff) >= 16) {
				printf("cpu%d MPIDR:aff0 is %d (>= 16)!\n",
					cpu, (int)(mpidr & 0xff));
				break;
			}

			tlist |= 1 << (mpidr & 0xf);

			cpu = cpumask_next(cpu, mask);
			if (cpu >= nr_cpus)
				break;

			mpidr = cpus[cpu];

			if (cluster_id != (mpidr & ~0xffUL)) {
				/*
				 * The next cpu isn't in our cluster. Roll
				 * back the cpu index allowing the outer
				 * for_each_cpu to find it again with
				 * cpumask_next
				 */
				--cpu;
				break;
			}
		}

		/* Send the IPIs for the target list of this cluster */
		sgi1r = (MPIDR_TO_SGI_AFFINITY(cluster_id, 3)	|
			 MPIDR_TO_SGI_AFFINITY(cluster_id, 2)	|
			 irq << 24				|
			 MPIDR_TO_SGI_AFFINITY(cluster_id, 1)	|
			 tlist);

		gicv3_write_sgi1r(sgi1r);
	}

	/* Force the above writes to ICC_SGI1R_EL1 to be executed */
	isb();
}

static void gicv3_ipi_send(int cpu, int irq)
{
	cpumask_t mask;

	cpumask_clear(&mask);
	cpumask_set_cpu(cpu, &mask);
	gicv3_ipi_send_tlist(&mask, irq);
}

static struct gic_common_ops gicv2_common_ops = {
	.gic_version = 2,
	.read_iar = gicv2_read_iar,
	.iar_irqnr = gicv2_iar_irqnr,
	.write_eoir = gicv2_write_eoir,
	.ipi_send = gicv2_ipi_send,
};

static struct gic_common_ops gicv3_common_ops = {
	.gic_version = 3,
	.read_iar = gicv3_read_iar,
	.iar_irqnr = gicv3_iar_irqnr,
	.write_eoir = gicv3_write_eoir,
	.ipi_send = gicv3_ipi_send,
};

struct gic_common_ops *gic_common_ops;

int gic_init(void)
{
	if (gicv2_init()) {
		gic_common_ops = &gicv2_common_ops;
		return 2;
	} else if (gicv3_init()) {
		gic_common_ops = &gicv3_common_ops;
		return 3;
	}
	return 0;
}
