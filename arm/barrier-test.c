#include <libcflat.h>
#include <asm/smp.h>
#include <asm/cpumask.h>
#include <asm/barrier.h>
#include <asm/mmu.h>

#define MAX_CPUS 4

/* How many increments to do */
static int increment_count = 10000000;


/* shared value, we use the alignment to ensure the global_lock value
 * doesn't share a page */
static unsigned int shared_value;

__attribute__((aligned(PAGE_SIZE))) static unsigned int per_cpu_value[MAX_CPUS];
__attribute__((aligned(PAGE_SIZE))) static cpumask_t smp_test_complete;
__attribute__((aligned(PAGE_SIZE))) static int global_lock;

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

static void increment_shared_with_smp_mb(void)
{
	smp_rmb();
	shared_value++;
	smp_wmb();
}

static void do_increment(void)
{
	int i;
	int cpu = smp_processor_id();

	printf("CPU%d online\n", cpu);

	for (i=0; i < increment_count; i++) {
		per_cpu_value[cpu]++;
		inc_fn();
	}

	printf("CPU%d: Done, %d incs\n", cpu, per_cpu_value[cpu]);

	cpumask_set_cpu(cpu, &smp_test_complete);
	if (cpu != 0)
		halt();
}

int main(int argc, char **argv)
{
	int cpu, i;
	unsigned int sum = 0;

	inc_fn = &increment_shared;
	
	for (i=0; i<argc; i++) {
		char *arg = argv[i];

		if (strcmp(arg, "lock") == 0) {
			inc_fn = &increment_shared_with_lock;
			report_prefix_push("lock");
		};

		if (strcmp(arg, "smp_mb") == 0) {
			inc_fn = &increment_shared_with_smp_mb;
			report_prefix_push("smp_mb");
		};
	}

	for_each_present_cpu(cpu) {
		if (cpu == 0)
			continue;
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
