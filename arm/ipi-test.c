/*
 * Test sending an IPI and handling IRQ exceptions.
 *
 * Copyright (C) 2015, Red Hat Inc, Andrew Jones <drjones@redhat.com>
 *
 * This work is licensed under the terms of the GNU LGPL, version 2.
 */
#include <libcflat.h>
#include <asm/setup.h>
#include <asm/processor.h>
#include <asm/gic.h>
#include <asm/smp.h>
#include <asm/barrier.h>

static volatile bool ready, acked, done;

static void irq_handler(struct pt_regs *regs __unused)
{
	gic_irq_ack();
	acked = true;
}

static void ipi_test(void)
{
	gic_enable();
#ifdef __arm__
	install_exception_handler(EXCPTN_IRQ, irq_handler);
#else
	install_irq_handler(EL1H_IRQ, irq_handler);
#endif
	local_irq_enable();
	ready = true;
	wfi();
	report("IPI", acked);
	done = true;
	halt();
}

int main(void)
{
	if (nr_cpus < 2) {
		printf("ipi-test requires '-smp 2'\n");
		abort();
	}

	gic_enable();
	smp_boot_secondary(1, ipi_test);

	while (!ready)
		cpu_relax();

	gic_send_sgi(1, 1);

	while (!done)
		cpu_relax();

	return report_summary();
}
