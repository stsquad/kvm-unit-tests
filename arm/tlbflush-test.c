#include <libcflat.h>
#include <asm/smp.h>
#include <asm/cpumask.h>
#include <asm/barrier.h>
#include <asm/mmu.h>

#define SEQ_LENGTH 10
#define SEQ_HASH 0x7cd707fe

static cpumask_t smp_test_complete;
static int flush_count = 1000000;
static int flush_self = 0;
static int flush_page = 0;
static int flush_verbose = 0;

/* Work functions
 *
 * These work functions need to be:
 *
 *  - page aligned, so we can flush one function at a time
 *  - have branches, so QEMU TCG generates multiple basic blocks
 *  - call across pages, so we exercise the TCG basic block slow path
 */

/* Adler32 */
__attribute__((aligned(PAGE_SIZE))) uint32_t hash_array(const void *buf,
							size_t buflen)
{
	const uint8_t *data = (uint8_t *) buf;
	uint32_t s1 = 1;
	uint32_t s2 = 0;

	for (size_t n = 0; n < buflen; n++) {
		s1 = (s1 + data[n]) % 65521;
		s2 = (s2 + s1) % 65521;
	}
	return (s2 << 16) | s1;
}

__attribute__((aligned(PAGE_SIZE))) void create_fib_sequence(int length,
							unsigned int *array)
{
	int i;

	/* first two values */
	array[0] = 0;
	array[1] = 1;
	for (i=2; i<length; i++) {
		array[i] = array[i-2] + array[i-1];
	}
}

__attribute__((aligned(PAGE_SIZE))) unsigned long long factorial(unsigned int n)
{
	unsigned int i;
	unsigned long long fac = 1;
	for (i=1; i<=n; i++)
	{
		fac = fac * i;
	}
	return fac;
}

__attribute__((aligned(PAGE_SIZE))) void factorial_array
(unsigned int n, unsigned int *input, unsigned long long *output)
{
	unsigned int i;
	for (i=0; i<n; i++) {
		output[i] = factorial(input[i]);
	}
}

__attribute__((aligned(PAGE_SIZE))) unsigned int do_computation(void)
{
	unsigned int fib_array[SEQ_LENGTH];
	unsigned long long facfib_array[SEQ_LENGTH];
	uint32_t fib_hash, facfib_hash;

	create_fib_sequence(SEQ_LENGTH, &fib_array[0]);
	fib_hash = hash_array(&fib_array[0], sizeof(fib_array));
	factorial_array(SEQ_LENGTH, &fib_array[0], &facfib_array[0]);
	facfib_hash = hash_array(&facfib_array[0], sizeof(facfib_array));

	return (fib_hash ^ facfib_hash);
}

/* This provides a table of the work functions so we can flush each
 * page individually
 */
static void * pages[] = {&hash_array, &create_fib_sequence, &factorial,
			 &factorial_array, &do_computation};

static void do_flush(int i)
{
	if (flush_page) {
		flush_tlb_page((unsigned long)pages[i % ARRAY_SIZE(pages)]);
	} else {
		flush_tlb_all();
	}
}


static void just_compute(void)
{
	int i, errors = 0;
	int cpu = smp_processor_id();

	uint32_t result;

	printf("CPU%d online\n", cpu);

	for (i=0; i < flush_count; i++) {
		result = do_computation();

		if (result != SEQ_HASH) {
			errors++;
			printf("CPU%d: seq%d 0x%"PRIx32"!=0x%x\n",
				cpu, i, result, SEQ_HASH);
		}

		if (flush_verbose && (i % 1000) == 0) {
			printf("CPU%d: seq%d\n", cpu, i);
		}

		if (flush_self) {
			do_flush(i);
		}
	}

	report("CPU%d: Done - Errors: %d\n", errors == 0, cpu, errors);

	cpumask_set_cpu(cpu, &smp_test_complete);
	if (cpu != 0)
		halt();
}

static void just_flush(void)
{
	int cpu = smp_processor_id();
	int i = 0;

	/* set our CPU as done, keep flushing until everyone else
	   finished */
	cpumask_set_cpu(cpu, &smp_test_complete);

	while (!cpumask_full(&smp_test_complete)) {
		do_flush(i++);
	}

	report("CPU%d: Done - Triggered %d flushes\n", true, cpu, i);
}

int main(int argc, char **argv)
{
	int cpu, i;
	char prefix[100];

	for (i=0; i<argc; i++) {
		char *arg = argv[i];

		if (strcmp(arg, "page") == 0) {
			flush_page = 1;
                }

                if (strcmp(arg, "self") == 0) {
			flush_self = 1;
                }

		if (strcmp(arg, "verbose") == 0) {
			flush_verbose = 1;
                }
	}

	snprintf(prefix, sizeof(prefix), "tlbflush_%s_%s",
		flush_page?"page":"all",
		flush_self?"self":"other");
	report_prefix_push(prefix);

	for_each_present_cpu(cpu) {
		if (cpu == 0)
			continue;
		smp_boot_secondary(cpu, just_compute);
	}

	if (flush_self)
		just_compute();
	else
		just_flush();

	while (!cpumask_full(&smp_test_complete))
		cpu_relax();

	return report_summary();
}
