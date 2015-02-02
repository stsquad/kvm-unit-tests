/*
 * Each architecture must implement puts() and exit() with the I/O
 * devices exposed from QEMU, e.g. pl011 and chr-testdev. That's
 * what's done here, along with initialization functions for those
 * devices.
 *
 * Copyright (C) 2014, Red Hat Inc, Andrew Jones <drjones@redhat.com>
 *
 * This work is licensed under the terms of the GNU LGPL, version 2.
 */
#include <libcflat.h>
#include <devicetree.h>
#include <chr-testdev.h>
#include <asm/spinlock.h>
#include <asm/gic.h>
#include <asm/io.h>

extern void halt(int code);

/*
 * Use this guess for the pl011 base in order to make an attempt at
 * having earlier printf support. We'll overwrite it with the real
 * base address that we read from the device tree later. This is
 * the address we expect QEMU's mach-virt machine type to put in
 * its generated device tree.
 */
#define UART_EARLY_BASE 0x09000000UL

static struct spinlock uart_lock;
static volatile u8 *uart0_base = (u8 *)UART_EARLY_BASE;

static void uart0_init(void)
{
	const char *compatible = "arm,pl011";
	struct dt_pbus_reg base;
	int ret;

	ret = dt_get_default_console_node();
	assert(ret >= 0 || ret == -FDT_ERR_NOTFOUND);

	if (ret == -FDT_ERR_NOTFOUND) {

		ret = dt_pbus_get_base_compatible(compatible, &base);
		assert(ret == 0 || ret == -FDT_ERR_NOTFOUND);

		if (ret) {
			printf("%s: %s not found in the device tree, "
				"aborting...\n",
				__func__, compatible);
			abort();
		}

	} else {
		ret = dt_pbus_translate_node(ret, 0, &base);
		assert(ret == 0);
	}

	uart0_base = ioremap(base.addr, base.size);

	if (uart0_base != (u8 *)UART_EARLY_BASE) {
		printf("WARNING: early print support may not work. "
		       "Found uart at %p, but early base is %p.\n",
			uart0_base, (u8 *)UART_EARLY_BASE);
	}
}

struct gicv2_data gicv2_data;
static int gicv2_init(void)
{
	const char *compatible = "arm,cortex-a15-gic";
	struct dt_pbus_reg reg;
	struct dt_device gic;
	struct dt_bus bus;
	int node;

	dt_bus_init_defaults(&bus);
	dt_device_init(&gic, &bus, NULL);

	node = dt_device_find_compatible(&gic, compatible);
	assert(node >= 0 || node == -FDT_ERR_NOTFOUND);

	if (node == -FDT_ERR_NOTFOUND)
		return node;

	assert(dt_pbus_translate_node(node, 0, &reg) == 0);

	gicv2_data.dist_base = ioremap(reg.addr, reg.size);

	assert(dt_pbus_translate_node(node, 1, &reg) == 0);

	gicv2_data.cpu_base = ioremap(reg.addr, reg.size);

	return 0;
}

void io_init(void)
{
	uart0_init();
	chr_testdev_init();
	assert(gicv2_init() == 0);
}

void puts(const char *s)
{
	spin_lock(&uart_lock);
	while (*s)
		writeb(*s++, uart0_base);
	spin_unlock(&uart_lock);
}

void exit(int code)
{
	chr_testdev_exit(code);
	halt(code);
}
