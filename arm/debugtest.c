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

/* From Linux cputype.h */
#define read_sysreg(reg) ({						\
			u64 __val;					\
			asm("mrs	%0, " #reg : "=r" (__val));	\
			__val;						\
		})

#define write_sysreg(reg, val) ({					\
			asm("msr	" #reg ", %0" :: "r" (val));    \
		})

#define MIN(a, b)		((a) < (b) ? (a) : (b))

#define DBGBCR_RES0_MASK ~((0xff << 24) & (0xf << 9) & (0x3 << 3))
#define DBGBVR_RES0_MASK ~(0x3)
#define DBGWCR_RES0_MASK ~((0x7 << 29) & (0x7 << 21))
#define DBGWVR_RES0_MASK ~(0x3)

typedef struct {
	uint32_t dbgbcr;
	uint64_t dbgbvr;
	uint32_t dbgwcr;
	uint64_t dbgwvr;
} dbgregs_t;

static cpumask_t smp_test_complete;
static int nbp, nwp;

static void read_dbgb(int n, dbgregs_t *array)
{
	switch (n-1) {
	case 15:
		array[15].dbgbcr = read_sysreg(dbgbcr15_el1);
		array[15].dbgbvr = read_sysreg(dbgbcr15_el1);
	case 14:
		array[14].dbgbcr = read_sysreg(dbgbcr14_el1);
		array[14].dbgbvr = read_sysreg(dbgbcr14_el1);
	case 13:
		array[13].dbgbcr = read_sysreg(dbgbcr13_el1);
		array[13].dbgbvr = read_sysreg(dbgbcr13_el1);
	case 12:
		array[12].dbgbcr = read_sysreg(dbgbcr12_el1);
		array[12].dbgbvr = read_sysreg(dbgbcr12_el1);
	case 11:
		array[11].dbgbcr = read_sysreg(dbgbcr11_el1);
		array[11].dbgbvr = read_sysreg(dbgbcr11_el1);
	case 10:
		array[10].dbgbcr = read_sysreg(dbgbcr10_el1);
		array[10].dbgbvr = read_sysreg(dbgbcr10_el1);
	case 9:
		array[9].dbgbcr = read_sysreg(dbgbcr9_el1);
		array[9].dbgbvr = read_sysreg(dbgbcr9_el1);
	case 8:
		array[8].dbgbcr = read_sysreg(dbgbcr8_el1);
		array[8].dbgbvr = read_sysreg(dbgbcr8_el1);
	case 7:
		array[7].dbgbcr = read_sysreg(dbgbcr7_el1);
		array[7].dbgbvr = read_sysreg(dbgbcr7_el1);
	case 6:
		array[6].dbgbcr = read_sysreg(dbgbcr6_el1);
		array[6].dbgbvr = read_sysreg(dbgbcr6_el1);
	case 5:
		array[5].dbgbcr = read_sysreg(dbgbcr5_el1);
		array[5].dbgbvr = read_sysreg(dbgbcr5_el1);
	case 4:
		array[4].dbgbcr = read_sysreg(dbgbcr4_el1);
		array[4].dbgbvr = read_sysreg(dbgbvr4_el1);
	case 3:
		array[3].dbgbcr = read_sysreg(dbgbcr3_el1);
		array[3].dbgbvr = read_sysreg(dbgbvr3_el1);
	case 2:
		array[2].dbgbcr = read_sysreg(dbgbcr2_el1);
		array[2].dbgbvr = read_sysreg(dbgbvr2_el1);
	case 1:
		array[1].dbgbcr = read_sysreg(dbgbcr1_el1);
		array[1].dbgbvr = read_sysreg(dbgbvr1_el1);
	case 0:
		array[0].dbgbcr = read_sysreg(dbgbcr0_el1);
		array[0].dbgbvr = read_sysreg(dbgbvr0_el1);
		break;
	default:
		break;
	}
}

static void write_dbgb(int n, dbgregs_t *array)
{
	switch (n-1) {
	case 15:
		write_sysreg(dbgbcr15_el1, array[15].dbgbcr);
		write_sysreg(dbgbvr15_el1, array[15].dbgbvr);
	case 14:
		write_sysreg(dbgbcr14_el1, array[14].dbgbcr);
		write_sysreg(dbgbvr14_el1, array[14].dbgbvr);
	case 13:
		write_sysreg(dbgbcr13_el1, array[13].dbgbcr);
		write_sysreg(dbgbvr13_el1, array[13].dbgbvr);
	case 12:
		write_sysreg(dbgbcr12_el1, array[12].dbgbcr);
		write_sysreg(dbgbvr12_el1, array[12].dbgbvr);
	case 11:
		write_sysreg(dbgbcr11_el1, array[11].dbgbcr);
		write_sysreg(dbgbvr11_el1, array[11].dbgbvr);
	case 10:
		write_sysreg(dbgbcr10_el1, array[10].dbgbcr);
		write_sysreg(dbgbvr10_el1, array[10].dbgbvr);
	case 9:
		write_sysreg(dbgbcr9_el1, array[9].dbgbcr);
		write_sysreg(dbgbvr9_el1, array[9].dbgbvr);
	case 8:
		write_sysreg(dbgbcr8_el1, array[8].dbgbcr);
		write_sysreg(dbgbvr8_el1, array[8].dbgbvr);
	case 7:
		write_sysreg(dbgbcr7_el1, array[7].dbgbcr);
		write_sysreg(dbgbvr7_el1, array[7].dbgbvr);
	case 6:
		write_sysreg(dbgbcr6_el1, array[6].dbgbcr);
		write_sysreg(dbgbvr6_el1, array[6].dbgbvr);
	case 5:
		write_sysreg(dbgbcr5_el1, array[5].dbgbcr);
		write_sysreg(dbgbvr5_el1, array[5].dbgbvr);
	case 4:
		write_sysreg(dbgbcr4_el1, array[4].dbgbcr);
		write_sysreg(dbgbvr4_el1, array[4].dbgbvr);
	case 3:
		write_sysreg(dbgbcr3_el1, array[3].dbgbcr);
		write_sysreg(dbgbvr3_el1, array[3].dbgbvr);
	case 2:
		write_sysreg(dbgbcr2_el1, array[2].dbgbcr);
		write_sysreg(dbgbvr2_el1, array[2].dbgbvr);
	case 1:
		write_sysreg(dbgbcr1_el1, array[1].dbgbcr);
		write_sysreg(dbgbvr1_el1, array[1].dbgbvr);
	case 0:
		write_sysreg(dbgbcr0_el1, array[0].dbgbcr);
		write_sysreg(dbgbvr0_el1, array[0].dbgbvr);
		break;
	default:
		break;
	}
}

static void read_dbgw(int n, dbgregs_t *array)
{
	switch (n-1) {
	case 15:
		array[15].dbgwcr = read_sysreg(dbgwcr15_el1);
		array[15].dbgwvr = read_sysreg(dbgwcr15_el1);
	case 14:
		array[14].dbgwcr = read_sysreg(dbgwcr14_el1);
		array[14].dbgwvr = read_sysreg(dbgwcr14_el1);
	case 13:
		array[13].dbgwcr = read_sysreg(dbgwcr13_el1);
		array[13].dbgwvr = read_sysreg(dbgwcr13_el1);
	case 12:
		array[12].dbgwcr = read_sysreg(dbgwcr12_el1);
		array[12].dbgwvr = read_sysreg(dbgwcr12_el1);
	case 11:
		array[11].dbgwcr = read_sysreg(dbgwcr11_el1);
		array[11].dbgwvr = read_sysreg(dbgwcr11_el1);
	case 10:
		array[10].dbgwcr = read_sysreg(dbgwcr10_el1);
		array[10].dbgwvr = read_sysreg(dbgwcr10_el1);
	case 9:
		array[9].dbgwcr = read_sysreg(dbgwcr9_el1);
		array[9].dbgwvr = read_sysreg(dbgwcr9_el1);
	case 8:
		array[8].dbgwcr = read_sysreg(dbgwcr8_el1);
		array[8].dbgwvr = read_sysreg(dbgwcr8_el1);
	case 7:
		array[7].dbgwcr = read_sysreg(dbgwcr7_el1);
		array[7].dbgwvr = read_sysreg(dbgwcr7_el1);
	case 6:
		array[6].dbgwcr = read_sysreg(dbgwcr6_el1);
		array[6].dbgwvr = read_sysreg(dbgwcr6_el1);
	case 5:
		array[5].dbgwcr = read_sysreg(dbgwcr5_el1);
		array[5].dbgwvr = read_sysreg(dbgwcr5_el1);
	case 4:
		array[4].dbgwcr = read_sysreg(dbgwcr4_el1);
		array[4].dbgwvr = read_sysreg(dbgwvr4_el1);
	case 3:
		array[3].dbgwcr = read_sysreg(dbgwcr3_el1);
		array[3].dbgwvr = read_sysreg(dbgwvr3_el1);
	case 2:
		array[2].dbgwcr = read_sysreg(dbgwcr2_el1);
		array[2].dbgwvr = read_sysreg(dbgwvr2_el1);
	case 1:
		array[1].dbgwcr = read_sysreg(dbgwcr1_el1);
		array[1].dbgwvr = read_sysreg(dbgwvr1_el1);
	case 0:
		array[0].dbgwcr = read_sysreg(dbgwcr0_el1);
		array[0].dbgwvr = read_sysreg(dbgwvr0_el1);
		break;
	default:
		break;
	}
}

static void write_dbgw(int n, dbgregs_t *array)
{
	switch (n-1) {
	case 15:
		write_sysreg(dbgwcr15_el1, array[15].dbgwcr);
		write_sysreg(dbgwvr15_el1, array[15].dbgwvr);
	case 14:
		write_sysreg(dbgwcr14_el1, array[14].dbgwcr);
		write_sysreg(dbgwvr14_el1, array[14].dbgwvr);
	case 13:
		write_sysreg(dbgwcr13_el1, array[13].dbgwcr);
		write_sysreg(dbgwvr13_el1, array[13].dbgwvr);
	case 12:
		write_sysreg(dbgwcr12_el1, array[12].dbgwcr);
		write_sysreg(dbgwvr12_el1, array[12].dbgwvr);
	case 11:
		write_sysreg(dbgwcr11_el1, array[11].dbgwcr);
		write_sysreg(dbgwvr11_el1, array[11].dbgwvr);
	case 10:
		write_sysreg(dbgwcr10_el1, array[10].dbgwcr);
		write_sysreg(dbgwvr10_el1, array[10].dbgwvr);
	case 9:
		write_sysreg(dbgwcr9_el1, array[9].dbgwcr);
		write_sysreg(dbgwvr9_el1, array[9].dbgwvr);
	case 8:
		write_sysreg(dbgwcr8_el1, array[8].dbgwcr);
		write_sysreg(dbgwvr8_el1, array[8].dbgwvr);
	case 7:
		write_sysreg(dbgwcr7_el1, array[7].dbgwcr);
		write_sysreg(dbgwvr7_el1, array[7].dbgwvr);
	case 6:
		write_sysreg(dbgwcr6_el1, array[6].dbgwcr);
		write_sysreg(dbgwvr6_el1, array[6].dbgwvr);
	case 5:
		write_sysreg(dbgwcr5_el1, array[5].dbgwcr);
		write_sysreg(dbgwvr5_el1, array[5].dbgwvr);
	case 4:
		write_sysreg(dbgwcr4_el1, array[4].dbgwcr);
		write_sysreg(dbgwvr4_el1, array[4].dbgwvr);
	case 3:
		write_sysreg(dbgwcr3_el1, array[3].dbgwcr);
		write_sysreg(dbgwvr3_el1, array[3].dbgwvr);
	case 2:
		write_sysreg(dbgwcr2_el1, array[2].dbgwcr);
		write_sysreg(dbgwvr2_el1, array[2].dbgwvr);
	case 1:
		write_sysreg(dbgwcr1_el1, array[1].dbgwcr);
		write_sysreg(dbgwvr1_el1, array[1].dbgwvr);
	case 0:
		write_sysreg(dbgwcr0_el1, array[0].dbgwcr);
		write_sysreg(dbgwvr0_el1, array[0].dbgwvr);
		break;
	default:
		break;
	}
}

static void dump_regs(int cpu, dbgregs_t *array)
{
	int i;
	for (i=0; i<nbp; i++) {
		printf("CPU%d: B%d 0x%08x:0x%016llx\n", cpu, i, array[i].dbgbcr, array[i].dbgbvr);
	}
	for (i=0; i<nwp; i++) {
		printf("CPU%d: W%d 0x%08x:0x%016llx\n", cpu, i, array[i].dbgwcr, array[i].dbgwvr);
	}
}

static int compare_regs(int cpu, dbgregs_t *initial, dbgregs_t *current)
{
	int i, errors=0;
        for (i=0; i<nbp; i++) {
		if ((initial[i].dbgbcr & DBGBCR_RES0_MASK) != current[i].dbgbcr) {
			printf("ERROR DBGBCR%d: 0x%08x!=0x%08x\n", i, initial[i].dbgbcr & DBGBCR_RES0_MASK, current[i].dbgbcr);
			errors++;
		}
		if ((initial[i].dbgbvr & DBGBVR_RES0_MASK) != current[i].dbgbvr) {
			printf("ERROR DBGBVR%d: 0x%16lx!=0x%16lx\n", i, initial[i].dbgbvr & DBGBVR_RES0_MASK, current[i].dbgbvr);
			errors++;
		}
	}

	for (i=0; i<nwp; i++) {
		if ((initial[i].dbgwcr & DBGWCR_RES0_MASK) != current[i].dbgwcr) {
			printf("ERROR DBGWCR%d: 0x%08x!=0x%08x\n", i, initial[i].dbgwcr & DBGWCR_RES0_MASK, current[i].dbgwcr);
			errors++;
		}
		if ((initial[i].dbgwvr & DBGWVR_RES0_MASK) != current[i].dbgwvr) {
			printf("ERROR DBWVR%d: 0x%16x!=0x%16lx\n", i, initial[i].dbgwvr & DBGWVR_RES0_MASK, current[i].dbgwvr);
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
        dbgregs_t initial[16], current[16];

	printf("CPU%d online\n", cpu);

        printf("Reading Initial Values\n");
        read_dbgb(nbp, &initial[0]);
        read_dbgw(nwp, &initial[0]);
        dump_regs(cpu, &initial[0]);

        printf("Setting all 1s\n");
        for (i=0; i<nbp; i++) {
		initial[i].dbgbcr = 0xfffffffe; /* E=0 */
		initial[i].dbgbvr = 0xfffffffffffffffd;
        }
        for (i=0; i<nwp; i++) {
		initial[i].dbgwcr = 0xfffffffe; /* E=0 */
		initial[i].dbgwvr = 0xffffffffffffffff;
        }
        write_dbgb(nbp, &initial[0]);
        write_dbgw(nwp, &initial[0]);

        printf("Reading checking for RES0\n");
        read_dbgb(nbp, &current[0]);
        read_dbgw(nwp, &current[0]);

	errors += compare_regs(cpu, &initial[0], &current[0]);
        dump_regs(cpu, &current[0]);

	/* Take care not to write unsupported values */
        printf("Writing differrent values\n");
        for (i=0; i<nbp; i++) {
		initial[i].dbgbcr = (i << 20) | (i << 16);
		initial[i].dbgbvr = (i << 24) | (i << 16) | (i << 8);
        }
        for (i=0; i<nwp; i++) {
		initial[i].dbgwcr = (i << 24) | (i << 16) | (i << 5);
		initial[i].dbgwvr = (i << 24) | (i << 16) | (i << 8);
        }
        write_dbgb(nbp, &initial[0]);
        write_dbgw(nwp, &initial[0]);

        printf("Reading checking for DIFF\n");
        read_dbgb(nbp, &current[0]);
        read_dbgw(nwp, &current[0]);

	errors += compare_regs(cpu, &initial[0], &current[0]);
        dump_regs(cpu, &current[0]);

	report("CPU%d: Done - Errors: %d\n", errors == 0, cpu, errors);

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

int main(int argc, char **argv)
{
        int cpu, i;
	unsigned int id = read_sysreg(ID_AA64DFR0_EL1);;
	/* Check the number of break/watch points */
	nbp = ((id >> 12) & 0xf) + 1;
	nwp = ((id >> 20) & 0xf) + 1;

	for (i = 0; i < argc; ++i) {
		char *var;
		long val;
		var = split_var(argv[i], &val);
		if (var) {
			int tmp;
			if ( strcmp(var,"nbp") == 0 ) {
				tmp = MIN(nbp, val);
				nbp = tmp;
			}
			if ( strcmp(var,"nwp") == 0 ) {
				tmp = MIN(nwp, val);
				nwp = tmp;
			}
		}
        }

	printf("CPU%d: %d BPS, %d WPS\n", cpu, nbp, nwp);

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
