#include <libcflat.h>
#include <asm/smp.h>
#include <asm/cpumask.h>
#include <asm/barrier.h>
#include <asm/mmu.h>

#include <prng.h>

#define MAX_CPUS 8

/* Test definition structure
 *
 * A simple structure that describes the test name, expected pass and
 * increment function.
 */

/* Function pointers for test */
typedef void (*inc_fn)(int cpu);

typedef struct {
	const char *test_name;
	bool  should_pass;
	inc_fn main_fn;
} test_descr_t;

/* How many increments to do */
static int increment_count = 1000000;
static bool do_shuffle = true;

/* Shared value all the tests attempt to safely increment using
 * various forms of atomic locking and exclusive behaviour.
 */
static unsigned int shared_value;

/* PAGE_SIZE * uint32_t means we span several pages */
__attribute__((aligned(PAGE_SIZE))) static uint32_t memory_array[PAGE_SIZE];

/* We use the alignment of the following to ensure accesses to locking
 * and synchronisation primatives don't interfere with the page of the
 * shared value
 */
__attribute__((aligned(PAGE_SIZE))) static unsigned int per_cpu_value[MAX_CPUS];
__attribute__((aligned(PAGE_SIZE))) static cpumask_t smp_test_complete;
__attribute__((aligned(PAGE_SIZE))) struct isaac_ctx prng_context[MAX_CPUS];

/* Some of the approaches use a global lock to prevent contention. */
static int global_lock;

/* In any SMP setting this *should* fail due to cores stepping on
 * each other updating the shared variable
 */
static void increment_shared(int cpu)
{
	(void)cpu;

	shared_value++;
}

/* GCC __sync primitives are deprecated in favour of __atomic */
static void increment_shared_with_lock(int cpu)
{
	(void)cpu;

	while (__sync_lock_test_and_set(&global_lock, 1));
	shared_value++;
	__sync_lock_release(&global_lock);
}

/* In practice even __ATOMIC_RELAXED uses ARM's ldxr/stex exclusive
 * semantics */
static void increment_shared_with_atomic(int cpu)
{
	(void)cpu;

	__atomic_add_fetch(&shared_value, 1, __ATOMIC_SEQ_CST);
}


/*
 * Load/store exclusive with WFE (wait-for-event)
 *
 * See ARMv8 ARM examples:
 *   Use of Wait For Event (WFE) and Send Event (SEV) with locks
 */

static void increment_shared_with_wfelock(int cpu)
{
	(void)cpu;

#if defined(__aarch64__)
	asm volatile(
	"	mov     w1, #1\n"
	"       sevl\n"
	"       prfm PSTL1KEEP, [%[lock]]\n"
	"1:     wfe\n"
	"	ldaxr	w0, [%[lock]]\n"
	"	cbnz    w0, 1b\n"
	"	stxr    w0, w1, [%[lock]]\n"
	"	cbnz	w0, 1b\n"
	/* lock held */
	"	ldr	w0, [%[sptr]]\n"
	"	add	w0, w0, #0x1\n"
	"	str	w0, [%[sptr]]\n"
	/* now release */
	"	stlr	wzr, [%[lock]]\n"
	: /* out */
	: [lock] "r" (&global_lock), [sptr] "r" (&shared_value) /* in */
	: "w0", "w1", "cc");
#else
	asm volatile(
	"	mov     r1, #1\n"
	"1:	ldrex	r0, [%[lock]]\n"
	"	cmp     r0, #0\n"
	"	wfene\n"
	"	strexeq r0, r1, [%[lock]]\n"
	"	cmpeq	r0, #0\n"
	"	bne	1b\n"
	"	dmb\n"
	/* lock held */
	"	ldr	r0, [%[sptr]]\n"
	"	add	r0, r0, #0x1\n"
	"	str	r0, [%[sptr]]\n"
	/* now release */
	"	mov	r0, #0\n"
	"	dmb\n"
	"	str	r0, [%[lock]]\n"
	"	dsb\n"
	"	sev\n"
	: /* out */
	: [lock] "r" (&global_lock), [sptr] "r" (&shared_value) /* in */
	: "r0", "r1", "cc");
#endif
}


/*
 * Hand-written version of the load/store exclusive
 */
static void increment_shared_with_excl(int cpu)
{
	(void)cpu;

#if defined(__aarch64__)
        asm volatile(
	"1:	ldxr	w0, [%[sptr]]\n"
	"	add     w0, w0, #0x1\n"
	"	stxr	w1, w0, [%[sptr]]\n"
	"	cbnz	w1, 1b\n"
	: /* out */
	: [sptr] "r" (&shared_value) /* in */
	: "w0", "w1", "cc");
#else
	asm volatile(
	"1:	ldrex	r0, [%[sptr]]\n"
	"	add     r0, r0, #0x1\n"
	"	strex	r1, r0, [%[sptr]]\n"
	"	cmp	r1, #0\n"
	"	bne	1b\n"
	: /* out */
	: [sptr] "r" (&shared_value) /* in */
	: "r0", "r1", "cc");
#endif
}

/* Test array */
static test_descr_t tests[] = {
	{ "none", false, increment_shared },
	{ "lock", true, increment_shared_with_lock },
	{ "atomic", true, increment_shared_with_atomic },
	{ "wfelock", true, increment_shared_with_wfelock },
	{ "excl", true, increment_shared_with_excl }
};

/* The idea of this is just to generate some random load/store
 * activity which may or may not race with an un-barried incremented
 * of the shared counter
 */
static void shuffle_memory(int cpu)
{
	int i;
	uint32_t lspat = isaac_next_uint32(&prng_context[cpu]);
	uint32_t seq = isaac_next_uint32(&prng_context[cpu]);
	int count = seq & 0x1f;
	uint32_t val=0;

	seq >>= 5;

	for (i=0; i<count; i++) {
		int index = seq & ~PAGE_MASK;
		if (lspat & 1) {
			val ^= memory_array[index];
		} else {
			memory_array[index] = val;
		}
		seq >>= PAGE_SHIFT;
		seq ^= lspat;
		lspat >>= 1;
	}

}

static inc_fn increment_function;

static void do_increment(void)
{
	int i;
	int cpu = smp_processor_id();

	printf("CPU%d: online and ++ing\n", cpu);

	for (i=0; i < increment_count; i++) {
		per_cpu_value[cpu]++;
		increment_function(cpu);

		if (do_shuffle)
			shuffle_memory(cpu);
	}

	printf("CPU%d: Done, %d incs\n", cpu, per_cpu_value[cpu]);

	cpumask_set_cpu(cpu, &smp_test_complete);
	if (cpu != 0)
		halt();
}

static void setup_and_run_test(test_descr_t *test)
{
	unsigned int i, sum = 0;
	int cpu, cpu_cnt = 0;

	increment_function = test->main_fn;

	/* fill our random page */
        for (i=0; i<PAGE_SIZE; i++) {
		memory_array[i] = isaac_next_uint32(&prng_context[0]);
	}

	for_each_present_cpu(cpu) {
		uint32_t seed2 = isaac_next_uint32(&prng_context[0]);
		cpu_cnt++;
		if (cpu == 0)
			continue;

		isaac_init(&prng_context[cpu], (unsigned char *) &seed2, sizeof(seed2));
		smp_boot_secondary(cpu, do_increment);
	}

	do_increment();

	while (!cpumask_full(&smp_test_complete))
		cpu_relax();

	/* All CPUs done, do we add up */
	for_each_present_cpu(cpu) {
		sum += per_cpu_value[cpu];
	}

	if (test->should_pass) {
		report("total incs %d", sum == shared_value, shared_value);
	} else {
		report_xfail("total incs %d", true, sum == shared_value, shared_value);
	}
}

int main(int argc, char **argv)
{
	static const unsigned char seed[] = "myseed";
	test_descr_t *test = &tests[0];
	int i;
	unsigned int j;

	isaac_init(&prng_context[0], &seed[0], sizeof(seed));

	for (i=0; i<argc; i++) {
		char *arg = argv[i];

		/* Check for test name */
		for (j = 0; j < ARRAY_SIZE(tests); j++) {
			if (strcmp(arg, tests[j].test_name) == 0)
				test = &tests[j];
		}

		/* Test modifiers */
		if (strcmp(arg, "noshuffle") == 0) {
			do_shuffle = false;
			report_prefix_push("noshuffle");
		} else if (strstr(arg, "count=") != NULL) {
			char *p = strstr(arg, "=");
			increment_count = atol(p+1);
		} else {
			isaac_reseed(&prng_context[0], (unsigned char *) arg, strlen(arg));
		}
	}

	if (test) {
		setup_and_run_test(test);
	} else {
		report("Unknown test", false);
	}

	return report_summary();
}
