/*
 * GIC tests
 *
 * GICv2
 *   . test sending/receiving IPIs
 * GICv3
 *   . test sending/receiving IPIs
 *
 * Copyright (C) 2016, Red Hat Inc, Andrew Jones <drjones@redhat.com>
 *
 * This work is licensed under the terms of the GNU LGPL, version 2.
 */
#include <libcflat.h>
#include <util.h>
#include <asm/setup.h>
#include <asm/processor.h>
#include <asm/gic.h>
#include <asm/smp.h>
#include <asm/barrier.h>
#include <asm/io.h>

struct gic {
	struct {
		void (*enable)(void);
		void (*send_self)(void);
		void (*send_tlist)(cpumask_t *);
		void (*send_broadcast)(void);
	} ipi;
	u32 (*read_iar)(void);
	void (*write_eoi)(u32);
};

static struct gic *gic;
static int gic_version;
static int acked[NR_CPUS], spurious[NR_CPUS];
static cpumask_t ready;
static int sender;
static u32 irq;

static void nr_cpu_check(int nr)
{
	if (nr_cpus < nr)
		report_abort("At least %d cpus required", nr);
}

static void wait_on_ready(void)
{
	cpumask_set_cpu(smp_processor_id(), &ready);
	while (!cpumask_full(&ready))
		cpu_relax();
}

static void check_acked(cpumask_t *mask)
{
	int missing = 0, extra = 0, unexpected = 0;
	int nr_pass, cpu, i;

	/* Wait up to 5s for all interrupts to be delivered */
	for (i = 0; i < 50; ++i) {
		mdelay(100);
		nr_pass = 0;
		for_each_present_cpu(cpu) {
			smp_rmb();
			nr_pass += cpumask_test_cpu(cpu, mask) ?
				acked[cpu] == 1 : acked[cpu] == 0;
		}
		if (nr_pass == nr_cpus) {
			report("Completed in %d ms", true, ++i * 100);
			return;
		}
	}

	for_each_present_cpu(cpu) {
		if (cpumask_test_cpu(cpu, mask)) {
			if (!acked[cpu])
				++missing;
			else if (acked[cpu] > 1)
				++extra;
		} else {
			if (acked[cpu])
				++unexpected;
		}
	}

	report("Timed-out (5s). ACKS: missing=%d extra=%d unexpected=%d",
	       false, missing, extra, unexpected);
}

static u32 gicv2_read_iar(void)
{
	u32 iar = readl(gicv2_cpu_base() + GIC_CPU_INTACK);
	int src = (iar >> 10) & 7;

	if (src != sender) {
		report("cpu%d received IPI from unexpected source cpu%d "
		       "(expected cpu%d)",
		       false, smp_processor_id(), src, sender);
	}

	return iar & 0x3ff;
}

static void gicv2_write_eoi(u32 irq)
{
	writel(irq, gicv2_cpu_base() + GIC_CPU_EOI);
}

static void ipi_handler(struct pt_regs *regs __unused)
{
	u32 iar = gic->read_iar();

	if (iar != GICC_INT_SPURIOUS) {
		gic->write_eoi(iar);
		if (iar == irq) {
			smp_rmb(); /* pairs with wmb in ipi_test functions */
			++acked[smp_processor_id()];
			smp_wmb(); /* pairs with rmb in check_acked */
		} else {
			report("cpu%d received unexpected irq %u "
			       "(expected %u)",
			       false, smp_processor_id(), iar, irq);
		}
	} else {
		++spurious[smp_processor_id()];
		smp_wmb();
	}
}

static void gicv2_ipi_send_self(void)
{
	writel(2 << 24 | irq, gicv2_dist_base() + GIC_DIST_SOFTINT);
}

static void gicv2_ipi_send_tlist(cpumask_t *mask)
{
	u8 tlist = (u8)cpumask_bits(mask)[0];

	writel(tlist << 16 | irq, gicv2_dist_base() + GIC_DIST_SOFTINT);
}

static void gicv2_ipi_send_broadcast(void)
{
	writel(1 << 24 | irq, gicv2_dist_base() + GIC_DIST_SOFTINT);
}

#define ICC_SGI1R_AFFINITY_1_SHIFT	16
#define ICC_SGI1R_AFFINITY_2_SHIFT	32
#define ICC_SGI1R_AFFINITY_3_SHIFT	48
#define MPIDR_TO_SGI_AFFINITY(cluster_id, level) \
	(MPIDR_AFFINITY_LEVEL(cluster_id, level) << ICC_SGI1R_AFFINITY_## level ## _SHIFT)

static void gicv3_ipi_send_tlist(cpumask_t *mask)
{
	u16 tlist;
	int cpu;

	for_each_cpu(cpu, mask) {
		u64 mpidr = cpus[cpu], sgi1r;
		u64 cluster_id = mpidr & ~0xffUL;

		tlist = 0;

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
				--cpu;
				break;
			}
		}

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

static void gicv3_ipi_send_self(void)
{
	cpumask_t mask;

	cpumask_clear(&mask);
	cpumask_set_cpu(smp_processor_id(), &mask);
	gicv3_ipi_send_tlist(&mask);
}

static void gicv3_ipi_send_broadcast(void)
{
	gicv3_write_sgi1r(1ULL << 40 | irq << 24);
	isb();
}

static void ipi_test_self(void)
{
	cpumask_t mask;

	report_prefix_push("self");
	memset(acked, 0, sizeof(acked));
	smp_wmb();
	cpumask_clear(&mask);
	cpumask_set_cpu(smp_processor_id(), &mask);
	gic->ipi.send_self();
	check_acked(&mask);
	report_prefix_pop();
}

static void ipi_test_smp(void)
{
	cpumask_t mask;
	int i;

	report_prefix_push("target-list");
	memset(acked, 0, sizeof(acked));
	smp_wmb();
	cpumask_copy(&mask, &cpu_present_mask);
	for (i = smp_processor_id() & 1; i < nr_cpus; i += 2)
		cpumask_clear_cpu(i, &mask);
	gic->ipi.send_tlist(&mask);
	check_acked(&mask);
	report_prefix_pop();

	report_prefix_push("broadcast");
	memset(acked, 0, sizeof(acked));
	smp_wmb();
	cpumask_copy(&mask, &cpu_present_mask);
	cpumask_clear_cpu(smp_processor_id(), &mask);
	gic->ipi.send_broadcast();
	check_acked(&mask);
	report_prefix_pop();
}

static void ipi_enable(void)
{
	gic->ipi.enable();
#ifdef __arm__
	install_exception_handler(EXCPTN_IRQ, ipi_handler);
#else
	install_irq_handler(EL1H_IRQ, ipi_handler);
#endif
	local_irq_enable();
}

static void ipi_send(void)
{
	ipi_enable();
	wait_on_ready();
	ipi_test_self();
	ipi_test_smp();
	exit(report_summary());
}

static void ipi_recv(void)
{
	ipi_enable();
	cpumask_set_cpu(smp_processor_id(), &ready);
	while (1)
		wfi();
}

struct gic gicv2 = {
	.ipi = {
		.enable = gicv2_enable_defaults,
		.send_self = gicv2_ipi_send_self,
		.send_tlist = gicv2_ipi_send_tlist,
		.send_broadcast = gicv2_ipi_send_broadcast,
	},
	.read_iar = gicv2_read_iar,
	.write_eoi = gicv2_write_eoi,
};

struct gic gicv3 = {
	.ipi = {
		.enable = gicv3_enable_defaults,
		.send_self = gicv3_ipi_send_self,
		.send_tlist = gicv3_ipi_send_tlist,
		.send_broadcast = gicv3_ipi_send_broadcast,
	},
	.read_iar = gicv3_read_iar,
	.write_eoi = gicv3_write_eoir,
};

int main(int argc, char **argv)
{
	char pfx[8];
	int cpu;

	gic_version = gic_init();
	if (!gic_version)
		report_abort("No gic present!");

	snprintf(pfx, 8, "gicv%d", gic_version);
	report_prefix_push(pfx);

	switch (gic_version) {
	case 2:
		gic = &gicv2;
		break;
	case 3:
		gic = &gicv3;
		break;
	}

	if (argc < 2) {

		report_prefix_push("ipi");
		ipi_enable();
		ipi_test_self();
		report_prefix_pop();

	} else if (!strcmp(argv[1], "ipi")) {
		int off, i = 1;
		long val;

		report_prefix_push(argv[1]);

		while (--argc != 1) {
			off = parse_keyval(argv[++i], &val);
			if (off == -1)
				continue;
			argv[i][off] = '\0';
			if (strcmp(argv[i], "sender") == 0)
				sender = val;
			else if (strcmp(argv[i], "irq") == 0)
				irq = val;
		}

		nr_cpu_check(2);
		ipi_enable();

		for_each_present_cpu(cpu) {
			if (cpu == 0)
				continue;
			if (cpu == sender)
				smp_boot_secondary(cpu, ipi_send);
			else
				smp_boot_secondary(cpu, ipi_recv);
		}
		if (sender == 0)  {
			wait_on_ready();
			ipi_test_self();
			ipi_test_smp();
		} else {
			ipi_recv();
		}

		smp_rmb();
		for_each_present_cpu(cpu) {
			if (spurious[cpu]) {
				printf("ipi: WARN: cpu%d got %d spurious "
				       "interrupts\n",
				       spurious[cpu], smp_processor_id());
			}
		}

		report_prefix_pop();

	} else {
		report_abort("Unknown subtest '%s'", argv[1]);
	}

	return report_summary();
}
