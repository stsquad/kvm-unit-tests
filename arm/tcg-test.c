/*
 * ARM TCG Tests
 *
 * These tests are explicitly aimed at stretching the QEMU TCG engine.
 */

#include <libcflat.h>
#include <asm/smp.h>
#include <asm/cpumask.h>
#include <asm/barrier.h>
#include <asm/mmu.h>
#include <asm/gic.h>

#include <prng.h>

#define MAX_CPUS 8

/* These entry points are in the assembly code */
extern int tight_start(uint32_t count, uint32_t pattern);
extern int computed_start(uint32_t count, uint32_t pattern);
extern int paged_start(uint32_t count, uint32_t pattern);
extern uint32_t tight_end;
extern uint32_t computed_end;
extern uint32_t paged_end;
extern unsigned long test_code_end;

typedef int (*test_fn)(uint32_t count, uint32_t pattern);

typedef struct {
	const char *test_name;
	bool       should_pass;
	test_fn    start_fn;
	uint32_t   *code_end;
} test_descr_t;

/* Test array */
static test_descr_t tests[] = {
       /*
	* Tight chain.
	*
	* These are a bunch of basic blocks that have fixed branches in
	* a page aligned space. The branches taken are decided by a
	* psuedo-random bitmap for each CPU.
	*
	* Once the basic blocks have been chained together by the TCG they
	* should run until they reach their block count. This will be the
	* most efficient mode in which generated code is run. The only other
	* exits will be caused by interrupts or TB invalidation.
	*/
	{ "tight", true, tight_start, &tight_end },
	/*
	 * Computed jumps.
	 *
	 * A bunch of basic blocks which just do computed jumps so the basic
	 * block is never chained but they are all within a page (maybe not
	 * required). This will exercise the cache lookup but not the new
	 * generation.
	 */
	{ "computed", true, computed_start, &computed_end },
        /*
	 * Page ping pong.
	 *
	 * Have the blocks are separated by PAGE_SIZE so they can never
	 * be chained together.
	 *
	 */
	{ "paged", true, paged_start, &paged_end}
};

static test_descr_t *test = NULL;

static int iterations = 100000;
static int rounds = 1000;
static int mod_freq = 5;
static uint32_t pattern[MAX_CPUS];

static int smc = 0;
static int irq = 0;
static int irq_cnt[MAX_CPUS];
static int errors[MAX_CPUS];

static cpumask_t smp_test_complete;


/* This triggers TCGs SMC detection by writing values to the executing
 * code pages. We are not actually modifying the instructions and the
 * underlying code will remain unchanged. However this should trigger
 * invalidation of the Translation Blocks
 */

void trigger_smc_detection(uint32_t *start, uint32_t *end)
{
	volatile uint32_t *ptr = start;
	while (ptr < end) {
		uint32_t inst = *ptr;
		*ptr++ = inst;
	}
}

/* Handler for receiving IRQs */

static void irq_handler(struct pt_regs *regs __unused)
{
	int cpu = smp_processor_id();
	irq_cnt[cpu]++;
	gic_irq_ack();
}

/* This triggers cross-CPU IRQs. Each IRQ should cause the basic block
 * execution to finish the main run-loop get entered again.
 */
int send_cross_cpu_irqs(int this_cpu)
{
	int cpu, sent = 0;

	for_each_present_cpu(cpu) {
		if (cpu != this_cpu) {
			gic_send_sgi(cpu, 1);
			sent++;
		}
	}

	return sent;
}


void do_test(void)
{
	int cpu = smp_processor_id();
	int i;
	int sent_irqs = 0;

	printf("CPU%d: online and setting up with pattern 0x%x\n", cpu, pattern[cpu]);

	if (irq) {
		gic_enable();
#ifdef __arm__
		install_exception_handler(EXCPTN_IRQ, irq_handler);
#else
		install_irq_handler(EL1H_IRQ, irq_handler);
#endif
		local_irq_enable();
	}

	for (i=0; i<rounds; i++)
	{
		/* Enter the blocks */
		errors[cpu] += test->start_fn(iterations, pattern[cpu]);

		if ((i + cpu) % mod_freq == 0)
		{
			if (smc) {
				trigger_smc_detection((uint32_t *) test->start_fn,
						test->code_end);
			}
			if (irq) {
				sent_irqs += send_cross_cpu_irqs(cpu);
			}
		}
	}

	if (irq) {
		printf("CPU%d: Done with %d irqs sent and %d received\n", cpu, sent_irqs, irq_cnt[cpu]);
	} else {
		printf("CPU%d: Done with %d errors\n", cpu, errors[cpu]);
	}

	cpumask_set_cpu(cpu, &smp_test_complete);
	if (cpu != 0)
		halt();
}


void setup_and_run_tcg_test(void)
{
	static const unsigned char seed[] = "tcg-test";
	struct isaac_ctx prng_context;
	int cpu;

	isaac_init(&prng_context, &seed[0], sizeof(seed));

	if (irq) {
		gic_enable();
	}

	/* boot other CPUs */
	for_each_present_cpu(cpu) {
		pattern[cpu] = isaac_next_uint32(&prng_context);

		if (cpu == 0)
			continue;

		smp_boot_secondary(cpu, do_test);
	}

	do_test();

	while (!cpumask_full(&smp_test_complete))
		cpu_relax();

	/* how do we detect errors other than not crashing? */
	report("passed", true);
}

int main(int argc, char **argv)
{
	int i;
	unsigned int j;

	for (i=0; i<argc; i++) {
		char *arg = argv[i];

		for (j = 0; j < ARRAY_SIZE(tests); j++) {
			if (strcmp(arg, tests[j].test_name) == 0)
				test = & tests[j];
		}

		/* Test modifiers */
		if (strstr(arg, "mod=") != NULL) {
			char *p = strstr(arg, "=");
			mod_freq = atol(p+1);
		}

		if (strcmp(arg, "smc") == 0) {
			unsigned long test_start = (unsigned long) &tight_start;
			unsigned long test_end = (unsigned long) &test_code_end;

			smc = 1;
			mmu_set_range_ptes(mmu_idmap, test_start, test_start, test_end,
					__pgprot(PTE_WBWA));

			report_prefix_push("smc");
		}

		if (strcmp(arg, "irq") == 0) {
			irq = 1;
			report_prefix_push("irq");
		}
	}

	if (test) {
		setup_and_run_tcg_test();
	} else {
		report("Unknown test", false);
	}

	return report_summary();
}
