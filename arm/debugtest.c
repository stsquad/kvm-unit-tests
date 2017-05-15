/*
 * Test guest access to debug registers.
 *
 * Copyright (C) 2015, Linaro Ltd, Alex Bennée <alex.bennee@linaro.org>
 *
 * This work is licensed under the terms of the GNU LGPL, version 2.
 */

#include <libcflat.h>
#include <asm/psci.h>
#include <asm/smp.h>
#include <asm/cpumask.h>
#include <asm/barrier.h>
#include "asm/sysreg.h"
#include "asm/processor.h"



#if defined(__arm__)

#define ID_DFR0	__ACCESS_CP15(c0, 0, c1, 2)

/* __ACCESS_CP14(CRn, Op1, CRm, Op2) */
#define DBGDIDR	__ACCESS_CP14(c0, 0, c0, 0)
#define DBGDSCR	__ACCESS_CP14(c0, 0, c2, 2)

#define DBGBCR0	__ACCESS_CP14(c0, 0, c0, 5)
#define DBGBCR1	__ACCESS_CP14(c0, 0, c1, 5)
#define DBGBCR2	__ACCESS_CP14(c0, 0, c2, 5)
#define DBGBCR3	__ACCESS_CP14(c0, 0, c3, 5)
#define DBGBCR4	__ACCESS_CP14(c0, 0, c4, 5)
#define DBGBCR5	__ACCESS_CP14(c0, 0, c5, 5)
#define DBGBCR6	__ACCESS_CP14(c0, 0, c6, 5)
#define DBGBCR7	__ACCESS_CP14(c0, 0, c7, 5)
#define DBGBCR8	__ACCESS_CP14(c0, 0, c8, 5)
#define DBGBCR9	__ACCESS_CP14(c0, 0, c9, 5)
#define DBGBCR10	__ACCESS_CP14(c0, 0, c10, 5)
#define DBGBCR11	__ACCESS_CP14(c0, 0, c11, 5)
#define DBGBCR12	__ACCESS_CP14(c0, 0, c12, 5)
#define DBGBCR13	__ACCESS_CP14(c0, 0, c13, 5)
#define DBGBCR14	__ACCESS_CP14(c0, 0, c14, 5)
#define DBGBCR15	__ACCESS_CP14(c0, 0, c15, 5)
#define DBGBVR0	__ACCESS_CP14(c0, 0, c0, 4)
#define DBGBVR1	__ACCESS_CP14(c0, 0, c1, 4)
#define DBGBVR2	__ACCESS_CP14(c0, 0, c2, 4)
#define DBGBVR3	__ACCESS_CP14(c0, 0, c3, 4)
#define DBGBVR4	__ACCESS_CP14(c0, 0, c4, 4)
#define DBGBVR5	__ACCESS_CP14(c0, 0, c5, 4)
#define DBGBVR6	__ACCESS_CP14(c0, 0, c6, 4)
#define DBGBVR7	__ACCESS_CP14(c0, 0, c7, 4)
#define DBGBVR8	__ACCESS_CP14(c0, 0, c8, 4)
#define DBGBVR9	__ACCESS_CP14(c0, 0, c9, 4)
#define DBGBVR10	__ACCESS_CP14(c0, 0, c10, 4)
#define DBGBVR11	__ACCESS_CP14(c0, 0, c11, 4)
#define DBGBVR12	__ACCESS_CP14(c0, 0, c12, 4)
#define DBGBVR13	__ACCESS_CP14(c0, 0, c13, 4)
#define DBGBVR14	__ACCESS_CP14(c0, 0, c14, 4)
#define DBGBVR15	__ACCESS_CP14(c0, 0, c15, 4)
#define DBGWCR0	__ACCESS_CP14(c0, 0, c0, 7)
#define DBGWCR1	__ACCESS_CP14(c0, 0, c1, 7)
#define DBGWCR2	__ACCESS_CP14(c0, 0, c2, 7)
#define DBGWCR3	__ACCESS_CP14(c0, 0, c3, 7)
#define DBGWCR4	__ACCESS_CP14(c0, 0, c4, 7)
#define DBGWCR5	__ACCESS_CP14(c0, 0, c5, 7)
#define DBGWCR6	__ACCESS_CP14(c0, 0, c6, 7)
#define DBGWCR7	__ACCESS_CP14(c0, 0, c7, 7)
#define DBGWCR8	__ACCESS_CP14(c0, 0, c8, 7)
#define DBGWCR9	__ACCESS_CP14(c0, 0, c9, 7)
#define DBGWCR10	__ACCESS_CP14(c0, 0, c10, 7)
#define DBGWCR11	__ACCESS_CP14(c0, 0, c11, 7)
#define DBGWCR12	__ACCESS_CP14(c0, 0, c12, 7)
#define DBGWCR13	__ACCESS_CP14(c0, 0, c13, 7)
#define DBGWCR14	__ACCESS_CP14(c0, 0, c14, 7)
#define DBGWCR15	__ACCESS_CP14(c0, 0, c15, 7)
#define DBGWVR0	__ACCESS_CP14(c0, 0, c0, 6)
#define DBGWVR1	__ACCESS_CP14(c0, 0, c1, 6)
#define DBGWVR2	__ACCESS_CP14(c0, 0, c2, 6)
#define DBGWVR3	__ACCESS_CP14(c0, 0, c3, 6)
#define DBGWVR4	__ACCESS_CP14(c0, 0, c4, 6)
#define DBGWVR5	__ACCESS_CP14(c0, 0, c5, 6)
#define DBGWVR6	__ACCESS_CP14(c0, 0, c6, 6)
#define DBGWVR7	__ACCESS_CP14(c0, 0, c7, 6)
#define DBGWVR8	__ACCESS_CP14(c0, 0, c8, 6)
#define DBGWVR9	__ACCESS_CP14(c0, 0, c9, 6)
#define DBGWVR10	__ACCESS_CP14(c0, 0, c10, 6)
#define DBGWVR11	__ACCESS_CP14(c0, 0, c11, 6)
#define DBGWVR12	__ACCESS_CP14(c0, 0, c12, 6)
#define DBGWVR13	__ACCESS_CP14(c0, 0, c13, 6)
#define DBGWVR14	__ACCESS_CP14(c0, 0, c14, 6)
#define DBGWVR15	__ACCESS_CP14(c0, 0, c15, 6)

#define read_debug_bcr(reg)	read_sysreg(DBGBCR ## reg)
#define read_debug_bvr(reg)	read_sysreg(DBGBVR ## reg)
#define read_debug_wcr(reg)	read_sysreg(DBGWCR ## reg)
#define read_debug_wvr(reg)	read_sysreg(DBGWVR ## reg)

#define write_debug_bcr(val, reg) write_sysreg(val, DBGBCR ## reg)
#define write_debug_bvr(val, reg) write_sysreg(val, DBGBVR ## reg)
#define write_debug_wcr(val, reg) write_sysreg(val, DBGWCR ## reg)
#define write_debug_wvr(val, reg) write_sysreg(val, DBGWVR ## reg)

#define PRIxDBGADDR PRIx32

struct dbgregs {
	uint32_t dbgbcr;
	uint32_t dbgbvr;
	uint32_t dbgwcr;
	uint32_t dbgwvr;
};

#elif defined(__aarch64__)

#define read_debug_bcr(reg)	read_sysreg(dbgbcr ## reg ## _el1)
#define read_debug_bvr(reg)	read_sysreg(dbgbvr ## reg ## _el1)
#define read_debug_wcr(reg)	read_sysreg(dbgwcr ## reg ## _el1)
#define read_debug_wvr(reg)	read_sysreg(dbgwvr ## reg ## _el1)

#define write_debug_bcr(val, reg) write_sysreg(val, dbgbcr ## reg ## _el1)
#define write_debug_bvr(val, reg) write_sysreg(val, dbgbvr ## reg ## _el1)
#define write_debug_wcr(val, reg) write_sysreg(val, dbgwcr ## reg ## _el1)
#define write_debug_wvr(val, reg) write_sysreg(val, dbgwvr ## reg ## _el1)

#define PRIxDBGADDR PRIx64

struct dbgregs {
	uint32_t dbgbcr;
	uint64_t dbgbvr;
	uint32_t dbgwcr;
	uint64_t dbgwvr;
};

#endif

#define MIN(a, b)		((a) < (b) ? (a) : (b))

static cpumask_t smp_test_complete;
static int nbp, nwp;

static void read_dbgb(int n, struct dbgregs *array)
{
	switch (n-1) {
	case 15:
		array[15].dbgbcr = read_debug_bcr(15);
		array[15].dbgbvr = read_debug_bvr(15);
	case 14:
		array[14].dbgbcr = read_debug_bcr(14);
		array[14].dbgbvr = read_debug_bvr(14);
	case 13:
		array[13].dbgbcr = read_debug_bcr(13);
		array[13].dbgbvr = read_debug_bvr(13);
	case 12:
		array[12].dbgbcr = read_debug_bcr(12);
		array[12].dbgbvr = read_debug_bvr(12);
	case 11:
		array[11].dbgbcr = read_debug_bcr(11);
		array[11].dbgbvr = read_debug_bvr(11);
	case 10:
		array[10].dbgbcr = read_debug_bcr(10);
		array[10].dbgbvr = read_debug_bvr(10);
	case 9:
		array[9].dbgbcr = read_debug_bcr(9);
		array[9].dbgbvr = read_debug_bvr(9);
	case 8:
		array[8].dbgbcr = read_debug_bcr(8);
		array[8].dbgbvr = read_debug_bvr(8);
	case 7:
		array[7].dbgbcr = read_debug_bcr(7);
		array[7].dbgbvr = read_debug_bvr(7);
	case 6:
		array[6].dbgbcr = read_debug_bcr(6);
		array[6].dbgbvr = read_debug_bvr(6);
	case 5:
		array[5].dbgbcr = read_debug_bcr(5);
		array[5].dbgbvr = read_debug_bvr(5);
	case 4:
		array[4].dbgbcr = read_debug_bcr(4);
		array[4].dbgbvr = read_debug_bvr(4);
	case 3:
		array[3].dbgbcr = read_debug_bcr(3);
		array[3].dbgbvr = read_debug_bvr(3);
	case 2:
		array[2].dbgbcr = read_debug_bcr(2);
		array[2].dbgbvr = read_debug_bvr(2);
	case 1:
		array[1].dbgbcr = read_debug_bcr(1);
		array[1].dbgbvr = read_debug_bvr(1);
	case 0:
		array[0].dbgbcr = read_debug_bcr(0);
		array[0].dbgbvr = read_debug_bvr(0);
		break;
	default:
		break;
	}
}

static void write_dbgb(int n, struct dbgregs *array)
{
	switch (n-1) {
	case 15:
		write_debug_bcr(array[15].dbgbcr, 15);
		write_debug_bvr(array[15].dbgbvr, 15);
	case 14:
		write_debug_bcr(array[14].dbgbcr, 14);
		write_debug_bvr(array[14].dbgbvr, 14);
	case 13:
		write_debug_bcr(array[13].dbgbcr, 13);
		write_debug_bvr(array[13].dbgbvr, 13);
	case 12:
		write_debug_bcr(array[12].dbgbcr, 12);
		write_debug_bvr(array[12].dbgbvr, 12);
	case 11:
		write_debug_bcr(array[11].dbgbcr, 11);
		write_debug_bvr(array[11].dbgbvr, 11);
	case 10:
		write_debug_bcr(array[10].dbgbcr, 10);
		write_debug_bvr(array[10].dbgbvr, 10);
	case 9:
		write_debug_bcr(array[9].dbgbcr, 9);
		write_debug_bvr(array[9].dbgbvr, 9);
	case 8:
		write_debug_bcr(array[8].dbgbcr, 8);
		write_debug_bvr(array[8].dbgbvr, 8);
	case 7:
		write_debug_bcr(array[7].dbgbcr, 7);
		write_debug_bvr(array[7].dbgbvr, 7);
	case 6:
		write_debug_bcr(array[6].dbgbcr, 6);
		write_debug_bvr(array[6].dbgbvr, 6);
	case 5:
		write_debug_bcr(array[5].dbgbcr, 5);
		write_debug_bvr(array[5].dbgbvr, 5);
	case 4:
		write_debug_bcr(array[4].dbgbcr, 4);
		write_debug_bvr(array[4].dbgbvr, 4);
	case 3:
		write_debug_bcr(array[3].dbgbcr, 3);
		write_debug_bvr(array[3].dbgbvr, 3);
	case 2:
		write_debug_bcr(array[2].dbgbcr, 2);
		write_debug_bvr(array[2].dbgbvr, 2);
	case 1:
		write_debug_bcr(array[1].dbgbcr, 1);
		write_debug_bvr(array[1].dbgbvr, 1);
	case 0:
		write_debug_bcr(array[0].dbgbcr, 0);
		write_debug_bvr(array[0].dbgbvr, 0);
		break;
	default:
		break;
	}
}

static void read_dbgw(int n, struct dbgregs *array)
{
	switch (n-1) {
	case 15:
		array[15].dbgwcr = read_debug_wcr(15);
		array[15].dbgwvr = read_debug_wvr(15);
	case 14:
		array[14].dbgwcr = read_debug_wcr(14);
		array[14].dbgwvr = read_debug_wvr(14);
	case 13:
		array[13].dbgwcr = read_debug_wcr(13);
		array[13].dbgwvr = read_debug_wvr(13);
	case 12:
		array[12].dbgwcr = read_debug_wcr(12);
		array[12].dbgwvr = read_debug_wvr(12);
	case 11:
		array[11].dbgwcr = read_debug_wcr(11);
		array[11].dbgwvr = read_debug_wvr(11);
	case 10:
		array[10].dbgwcr = read_debug_wcr(10);
		array[10].dbgwvr = read_debug_wvr(10);
	case 9:
		array[9].dbgwcr = read_debug_wcr(9);
		array[9].dbgwvr = read_debug_wvr(9);
	case 8:
		array[8].dbgwcr = read_debug_wcr(8);
		array[8].dbgwvr = read_debug_wvr(8);
	case 7:
		array[7].dbgwcr = read_debug_wcr(7);
		array[7].dbgwvr = read_debug_wvr(7);
	case 6:
		array[6].dbgwcr = read_debug_wcr(6);
		array[6].dbgwvr = read_debug_wvr(6);
	case 5:
		array[5].dbgwcr = read_debug_wcr(5);
		array[5].dbgwvr = read_debug_wvr(5);
	case 4:
		array[4].dbgwcr = read_debug_wcr(4);
		array[4].dbgwvr = read_debug_wvr(4);
	case 3:
		array[3].dbgwcr = read_debug_wcr(3);
		array[3].dbgwvr = read_debug_wvr(3);
	case 2:
		array[2].dbgwcr = read_debug_wcr(2);
		array[2].dbgwvr = read_debug_wvr(2);
	case 1:
		array[1].dbgwcr = read_debug_wcr(1);
		array[1].dbgwvr = read_debug_wvr(1);
	case 0:
		array[0].dbgwcr = read_debug_wcr(0);
		array[0].dbgwvr = read_debug_wvr(0);
		break;
	default:
		break;
	}
}

static void write_dbgw(int n, struct dbgregs *array)
{
	switch (n-1) {
	case 15:
		write_debug_wcr(array[15].dbgwcr, 15);
		write_debug_wvr(array[15].dbgwvr, 15);
	case 14:
		write_debug_wcr(array[14].dbgwcr, 14);
		write_debug_wvr(array[14].dbgwvr, 14);
	case 13:
		write_debug_wcr(array[13].dbgwcr, 13);
		write_debug_wvr(array[13].dbgwvr, 13);
	case 12:
		write_debug_wcr(array[12].dbgwcr, 12);
		write_debug_wvr(array[12].dbgwvr, 12);
	case 11:
		write_debug_wcr(array[11].dbgwcr, 11);
		write_debug_wvr(array[11].dbgwvr, 11);
	case 10:
		write_debug_wcr(array[10].dbgwcr, 10);
		write_debug_wvr(array[10].dbgwvr, 10);
	case 9:
		write_debug_wcr(array[9].dbgwcr, 9);
		write_debug_wvr(array[9].dbgwvr, 9);
	case 8:
		write_debug_wcr(array[8].dbgwcr, 8);
		write_debug_wvr(array[8].dbgwvr, 8);
	case 7:
		write_debug_wcr(array[7].dbgwcr, 7);
		write_debug_wvr(array[7].dbgwvr, 7);
	case 6:
		write_debug_wcr(array[6].dbgwcr, 6);
		write_debug_wvr(array[6].dbgwvr, 6);
	case 5:
		write_debug_wcr(array[5].dbgwcr, 5);
		write_debug_wvr(array[5].dbgwvr, 5);
	case 4:
		write_debug_wcr(array[4].dbgwcr, 4);
		write_debug_wvr(array[4].dbgwvr, 4);
	case 3:
		write_debug_wcr(array[3].dbgwcr, 3);
		write_debug_wvr(array[3].dbgwvr, 3);
	case 2:
		write_debug_wcr(array[2].dbgwcr, 2);
		write_debug_wvr(array[2].dbgwvr, 2);
	case 1:
		write_debug_wcr(array[1].dbgwcr, 1);
		write_debug_wvr(array[1].dbgwvr, 1);
	case 0:
		write_debug_wcr(array[0].dbgwcr, 0);
		write_debug_wvr(array[0].dbgwvr, 0);
		break;
	default:
		break;
	}
}

static void dump_regs(int cpu, struct dbgregs *array)
{
	int i;

	for (i = 0; i < nbp; i++)
		printf("CPU%d: B%d 0x%08x:0x%" PRIxDBGADDR "\n", cpu, i,
			array[i].dbgbcr, array[i].dbgbvr);
	for (i = 0; i < nwp; i++)
		printf("CPU%d: W%d 0x%08x:0x%" PRIxDBGADDR "\n", cpu, i,
			array[i].dbgwcr, array[i].dbgwvr);
}

static int compare_regs(struct dbgregs *initial, struct dbgregs *current)
{
	int i, errors = 0;

	for (i = 0; i < nbp; i++) {
		if (initial[i].dbgbcr != current[i].dbgbcr) {
			printf("ERROR DBGBCR%d: %" PRIx32 "!=%" PRIx32 "\n",
				i, initial[i].dbgbcr, current[i].dbgbcr);
			errors++;
		}
		if (initial[i].dbgbvr != current[i].dbgbvr) {
			printf("ERROR DBGBVR%d: %" PRIxDBGADDR "!=%" PRIxDBGADDR "\n",
				i, initial[i].dbgbvr, current[i].dbgbvr);
			errors++;
		}
	}

	for (i = 0; i < nwp; i++) {
		if (initial[i].dbgwcr != current[i].dbgwcr) {
			printf("ERROR DBGWCR%d: %" PRIx32 "!=%" PRIx32 "\n",
				i, initial[i].dbgwcr, current[i].dbgwcr);
			errors++;
		}
		if (initial[i].dbgwvr != current[i].dbgwvr) {
			printf("ERROR DBWVR%d: %" PRIxDBGADDR "!=%" PRIxDBGADDR "\n",
				i, initial[i].dbgwvr, current[i].dbgwvr);
			errors++;
		}
	}

	return errors;
}

static void test_debug_regs(void)
{
	int errors = 0;
	int cpu = smp_processor_id();
	int i;
	struct dbgregs initial[16], current[16];

	printf("CPU%d online\n", cpu);

	printf("Reading Initial Values\n");
	read_dbgb(nbp, &initial[0]);
	read_dbgw(nwp, &initial[0]);
	dump_regs(cpu, &initial[0]);

	printf("Setting simple BPs and WPs\n");
	for (i = 0; i < nbp; i++) {
		initial[i].dbgbcr = (0x3 << 1) | (0xf << 5) ; /* E=0 */
		initial[i].dbgbvr = ~0x3;
	}
	for (i = 0; i < nwp; i++) {
		initial[i].dbgwcr = (0x3 << 1) | (0x3 << 3); /* E=0 */
		initial[i].dbgwvr = ~0x7;
	}
	write_dbgb(nbp, &initial[0]);
	write_dbgw(nwp, &initial[0]);

	printf("Reading values back\n");
	read_dbgb(nbp, &current[0]);
	read_dbgw(nwp, &current[0]);

	errors += compare_regs(&initial[0], &current[0]);

	report("CPU%d: Done - Errors: %d\n", errors == 0, cpu, errors);

	if (errors)
		dump_regs(cpu, &current[0]);

	cpumask_set_cpu(cpu, &smp_test_complete);
	if (cpu != 0)
		halt();
}

static char *split_var(char *s, long *val)
{
	char *p;

	p = strchr(s, '=');
	if (!p)
		return NULL;

	*val = atol(p+1);
	*p = '\0';

	return s;
}

#if defined(__arm__)
static void read_debug_feature_regs(void)
{
	uint32_t id_dfr0 = read_sysreg(ID_DFR0);
	uint32_t dbgdidr = read_sysreg(DBGDIDR);
	uint32_t dbgdscr = read_sysreg(DBGDSCR);
	char *pmu, *dm_m, *mmt_m, *cpt_m, *mmd_m, *cpsd_m, *cpd_m;

	printf("ID_DFR0: 0x%" PRIx32 "\n", id_dfr0);

	/* PMU support */
	switch ((id_dfr0 >> 24) & 0xf) {
	case 0:
		pmu = "No PMUv2";
		break;
	case 1:
		pmu = "PMUv1";
		break;
	case 2:
		pmu = "PMUv2";
		break;
	case 3:
		pmu = "No PMU Extensions";
		break;
	default:
		pmu = "Invalid";
		break;
	}
	printf("  PMU: %s\n", pmu);

	/* M-profile debug model */
	switch ((id_dfr0 >> 20) & 0xf) {
	case 0:
		dm_m = "Not supported";
		break;
	case 1:
		dm_m = "M-profile debug with memory mapped access";
		break;
	default:
		dm_m = "Invalid";
		break;
	}
	printf("  M-profile debug: %s\n", dm_m);

	/* Memory mapped trace model */
	switch ((id_dfr0 >> 16) & 0xf) {
	case 0:
		mmt_m = "Not supported";
		break;
	case 1:
		mmt_m = "Supported with memory mapped access";
		break;
	default:
		mmt_m = "Invalid";
		break;
	}
	printf("  Trace Model: %s\n", mmt_m);

	/* Co-processor trace model */
	switch ((id_dfr0 >> 12) & 0xf) {
	case 0:
		cpt_m = "Not supported";
		break;
	case 1:
		cpt_m = "Supported with CP14access";
		break;
	default:
		cpt_m = "Invalid";
		break;
	}
	printf("  Trace Model: %s\n", cpt_m);

	/* Memory mapped debug model, A and R profiles */
	switch ((id_dfr0 >> 8) & 0xf) {
	case 0:
		mmd_m = "Not supported or pre-ARMv6";
		break;
	case 4:
		mmd_m = "Supported v7 Debug Architecture";
		break;
	case 5:
		mmd_m = "Supported v7.1 Debug Architecture";
		break;
	default:
		mmd_m = "Invalid";
		break;
	}
	printf("  Debug Model: %s\n", mmd_m);

	/* Co-processor secure debug model */
	switch ((id_dfr0 >> 4) & 0xf) {
	case 0:
		cpsd_m = "Not supported";
		break;
	case 2:
		cpsd_m = "Supported v6 Debug Architecture";
		break;
	case 3:
		cpsd_m = "Supported v6.1 Debug Architecture";
		break;
	case 4:
		cpsd_m = "Supported v7 Debug Architecture";
		break;
	case 5:
		cpsd_m = "Supported v7.1 Debug Architecture";
		break;
	default:
		cpsd_m = "Invalid";
		break;
	}
	printf("  Co-processor Secure Debug Model: %s\n", cpsd_m);

	/* Co-processor debug model */
	switch ((id_dfr0 >> 0) & 0xf) {
	case 0:
		cpd_m = "Not supported";
		break;
	case 2:
		cpd_m = "Supported v6 Debug Architecture";
		break;
	case 3:
		cpd_m = "Supported v6.1 Debug Architecture";
		break;
	case 4:
		cpd_m = "Supported v7 Debug Architecture";
		break;
	case 5:
		cpd_m = "Supported v7.1 Debug Architecture";
		break;
	default:
		cpd_m = "Invalid";
		break;
	}
	printf("  Co-processor Debug Model: %s\n", cpd_m);

	/* DBGIDR, num registers */
	nwp = (dbgdidr >> 28) + 1;
	/* BRPs == 0 is reserved, else it is BRPs + 1 */
	nbp = ((dbgdidr >> 24) & 0xf);
	if (nbp > 0)
		nbp += 1;

	printf("DBGDIDR: 0x%" PRIx32 " nwp:%d nbp:%d\n", dbgdidr, nwp, nbp);

	printf("DBGDSCR: 0x%" PRIx32 "\n", dbgdscr);

}
#elif defined(__aarch64__)
static void read_debug_feature_regs(void)
{
        uint64_t id = read_sysreg(id_aa64dfr0_el1);

	/* Check the number of break/watch points */
	nbp = ((id >> 12) & 0xf);
	if (nbp > 0)
		nbp -= 1;

	nwp = ((id >> 20) & 0xf);
	if (nwp > 0)
		nwp -= 1;

	printf("ID_AA64_DFR0_EL1: 0x%" PRIx64 " nwp:%d nbp:%d\n", id, nwp, nbp);
}
#endif

int main(int argc, char **argv)
{
	int cpu, i;

	read_debug_feature_regs();

	for (i = 0; i < argc; ++i) {
		char *var;
		long val;

		var = split_var(argv[i], &val);

		if (var) {
			int tmp;

			if (strcmp(var, "nbp") == 0) {
				tmp = MIN(nbp, val);
				nbp = tmp;
			}
			if (strcmp(var, "nwp") == 0) {
				tmp = MIN(nwp, val);
				nwp = tmp;
			}
		}
	}

	for_each_present_cpu(cpu) {
		if (cpu == 0)
			continue;
		smp_boot_secondary(cpu, test_debug_regs);
	}

	test_debug_regs();

	while (!cpumask_full(&smp_test_complete))
		cpu_relax();

	return report_summary();
}
