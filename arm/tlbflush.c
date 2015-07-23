#include <libcflat.h>
#include <asm/smp.h>
#include <asm/cpumask.h>
#include <asm/barrier.h>
#include <asm/mmu.h>

#define SEQ_LENGTH 10

static cpumask_t smp_test_complete;
static int flush_count = 100000;
static int flush_self = 1;
static int flush_page = 0;

__attribute__((aligned(0x1000))) unsigned int hash_array(int length, unsigned int *array)
{
	int i;
	unsigned int sum=0;
	for (i=0; i<length; i++)
	{
		unsigned int val = *array++;
		sum ^= val;
		sum ^= (val >> (val % 16));
		sum ^= (val << (val % 32));
	}

	return sum;
}

__attribute__((aligned(0x1000))) void create_fib_sequence(int length, unsigned int *array)
{
	int i;

	/* first two values */
	array[0] = 0;
	array[1] = 1;
	for (i=2; i<length; i++)
	{
		array[i] = array[i-2] + array[i-1];
	}
}

__attribute__((aligned(0x1000))) unsigned long long factorial(unsigned int n)
{
	unsigned int i;
	unsigned long long fac = 1;
	for (i=1; i<=n; i++)
	{
		fac = fac * i;
	}
	return fac;
}

/* do some computationally expensive stuff, return a checksum of the
 * results */
__attribute__((aligned(0x1000))) unsigned int do_computation(void)
{
	unsigned int fib_array[SEQ_LENGTH];
	unsigned long long facfib_array[SEQ_LENGTH];
	unsigned int fib_hash, facfib_hash;
	int cpu = smp_processor_id();
	int i, j;
	
	create_fib_sequence(SEQ_LENGTH, &fib_array[0]);
	fib_hash = hash_array(SEQ_LENGTH, &fib_array[0]);
	for (i=0; i<SEQ_LENGTH; i++) {
		for (j=0; j<fib_array[i]; j++) {
			facfib_array[i] = factorial(fib_array[i]+j);
		}
	}
	facfib_hash = 0;
	for (i=0; i<SEQ_LENGTH; i++) {
		for (j=0; j<fib_array[i]; j++) {
			facfib_hash ^= hash_array(sizeof(facfib_array)/sizeof(unsigned int), (unsigned int *)&facfib_array[0]);
		}
	}

#if 0
	printf("CPU:%d FIBSEQ ", cpu);
	for (i=0; i<SEQ_LENGTH; i++)
		printf("%u,", fib_array[i]);
	printf("\n");

	printf("CPU:%d FACFIB ", cpu);
	for (i=0; i<SEQ_LENGTH; i++)
		printf("%llu,", facfib_array[i]);
	printf("\n");
#endif
	
	return (fib_hash ^ facfib_hash);
}

static void * pages[] = {&hash_array, &create_fib_sequence, &factorial, &do_computation};

static void test_flush(void)
{
	int i, errors = 0;
	int cpu = smp_processor_id();

	unsigned int ref;

	printf("CPU%d online\n", cpu);

	ref = do_computation();

	for (i=0; i < flush_count; i++) {
		unsigned int this_ref = do_computation();

		if (this_ref != ref) {
			errors++;
			printf("CPU%d: seq%d 0x%x!=0x%x\n",
				cpu, i, ref, this_ref);
		}

		if ((i % 1000) == 0) {
			printf("CPU%d: seq%d\n", cpu, i);
		}
		
		if (flush_self) {
			if (flush_page) {
				int j = (i % (sizeof(pages)/sizeof(void *)));
				flush_tlb_page((unsigned long)pages[j]);
			} else {
				flush_tlb_all();
			}
		}
	}

	report("CPU%d: Done - Errors: %d\n", errors == 0, cpu, errors);

	cpumask_set_cpu(cpu, &smp_test_complete);
	if (cpu != 0)
		halt();
}

int main(int argc, char **argv)
{
	int cpu, i;
	
	report_prefix_push("tlbflush");

	for (i=0; i<argc; i++) {
		char *arg = argv[i];
/* 		printf("arg:%d:%s\n", i, arg); */

		if (strcmp(arg, "page") == 0) {
			report_prefix_push("page");
			flush_page = 1;
		}
	}

	for_each_present_cpu(cpu) {
		if (cpu == 0)
			continue;
		smp_boot_secondary(cpu, test_flush);
	}

	test_flush();

	while (!cpumask_full(&smp_test_complete))
		cpu_relax();

	return report_summary();
}
