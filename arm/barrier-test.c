#include <libcflat.h>
#include <asm/smp.h>
#include <asm/cpumask.h>
#include <asm/barrier.h>
#include <asm/mmu.h>

#include <prng.h>

#define MAX_CPUS 4

/* How many increments to do */
static int increment_count = 10000000;


/* shared value, we use the alignment to ensure the global_lock value
 * doesn't share a page */
static unsigned int shared_value;

/* PAGE_SIZE * uint32_t means we span several pages */
static uint32_t memory_array[PAGE_SIZE];

__attribute__((aligned(PAGE_SIZE))) static unsigned int per_cpu_value[MAX_CPUS];
__attribute__((aligned(PAGE_SIZE))) static cpumask_t smp_test_complete;
__attribute__((aligned(PAGE_SIZE))) static int global_lock;

struct isaac_ctx prng_context[MAX_CPUS];

void (*inc_fn)(void);

static void lock(int *lock_var)
{
	while (__sync_lock_test_and_set(lock_var, 1));
}
static void unlock(int *lock_var)
{
	__sync_lock_release(lock_var);
}

static void increment_shared(void)
{
	shared_value++;
}

static void increment_shared_with_lock(void)
{
	lock(&global_lock);
	shared_value++;
	unlock(&global_lock);
}

static void increment_shared_with_acqrel(void)
{
	uint32_t tmp, res;

	asm volatile(
	"1:	ldxr	w0, [%[sptr]]\n"
	"	add     w0, w0, #0x1\n"
	"	stxr	w1, w0, [%[sptr]]\n"
	"	cbnz	w1, 1b\n"
	: /* out */
	: [sptr] "r" (&shared_value) /* in */
	: "w0", "w1", "cc");
}

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

static void do_increment(void)
{
	int i;
	int cpu = smp_processor_id();

	printf("CPU%d online\n", cpu);

	for (i=0; i < increment_count; i++) {
		per_cpu_value[cpu]++;
		inc_fn();

		shuffle_memory(cpu);
	}

	printf("CPU%d: Done, %d incs\n", cpu, per_cpu_value[cpu]);

	cpumask_set_cpu(cpu, &smp_test_complete);
	if (cpu != 0)
		halt();
}

int main(int argc, char **argv)
{
	int cpu;
	unsigned int i, sum = 0;
	static const unsigned char seed[] = "myseed";

	inc_fn = &increment_shared;

	isaac_init(&prng_context[0], &seed[0], sizeof(seed));

	for (i=0; i<argc; i++) {
		char *arg = argv[i];

		if (strcmp(arg, "lock") == 0) {
			inc_fn = &increment_shared_with_lock;
			report_prefix_push("lock");
		} else if (strcmp(arg, "acqrel") == 0) {
			inc_fn = &increment_shared_with_acqrel;
			report_prefix_push("acqrel");
		} else {
			isaac_reseed(&prng_context[0], (unsigned char *) arg, strlen(arg));
		}
	}

	/* fill our random page */
	for (i=0; i<PAGE_SIZE; i++) {
		memory_array[i] = isaac_next_uint32(&prng_context[0]);
	}

	for_each_present_cpu(cpu) {
		uint32_t seed2 = isaac_next_uint32(&prng_context[0]);
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
	report("total incs %d", sum == shared_value, shared_value);

	return report_summary();
}
