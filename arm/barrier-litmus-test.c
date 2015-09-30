/*
 * ARM Barrier Litmus Tests
 *
 * This test provides a framework for testing barrier conditions on
 * the processor. It's simpler than the more involved barrier testing
 * frameworks as we are looking for simple failures of QEMU's TCG not
 * weird edge cases the silicon gets wrong.
 */

#include <libcflat.h>
#include <asm/smp.h>
#include <asm/cpumask.h>
#include <asm/barrier.h>
#include <asm/mmu.h>

#define MAX_CPUS 8

/* How wide is the array */
static int array_size = 100000;
static int array_stride = 1;

/*
 * These test_array_* structures are a contiguous array modified by two or more
 * competing CPUs. The padding is to ensure the variables do not share
 * cache lines.
 *
 * All structures start zeroed.
 */

/* typedef struct test_array_shared */
/* { */
/* 	volatile int x; */
/* 	volatile int y; */
/* 	volatile int z; */
/* } test_array_shared_t; */

typedef struct test_array
{
	volatile int x;
	uint8_t dummy[64];
	volatile int y;
	uint8_t dummy2[64];
	volatile int z;
} test_array;

volatile test_array *array;

/* Test definition structure
 *
 * The first function will always run on the primary CPU, it is
 * usually the one that will detect any weirdness and trigger the
 * failure of the test.
 */

typedef void (*test_fn)(void);

typedef struct {
	const char *test_name;
	bool  should_pass;
	test_fn main_fn;
	test_fn secondary_fns[MAX_CPUS-1];
} test_descr_t;

/* Litmus tests */

/* Simple Message Passing
 *
 * x is the message data
 * y is the flag to indicate the data is ready
 *
 * Reading x == 0 when y == 1 is a failure.
 */

void message_passing_write(void)
{
	int i;
	for (i=0; i< array_size; i++) {
		volatile test_array *entry = &array[i];
		entry->x = 1;
		entry->y = 1;
	}

	halt();
}

void message_passing_read(void)
{
	int i;
	int errors = 0, ready = 0;

	for (i=0; i< array_size; i++) {
		volatile test_array *entry = &array[i];
		int x,y;
		y = entry->y;
		x = entry->x;

		if (y && !x)
			errors++;
		ready += y;
	}

	report("mp: %d errors, %d ready", errors == 0, errors, ready);
}

void message_passing_write_barrier(void)
{
	int i;
	for (i=0; i< array_size; i++) {
		volatile test_array *entry = &array[i];
		entry->x = 1;
		smp_wmb();
		entry->y = 1;
	}

	halt();
}

void message_passing_read_barrier(void)
{
	int i;
	int errors = 0, ready = 0;

	for (i=0; i< array_size; i++) {
		volatile test_array *entry = &array[i];
		int x, y;
		y = entry->y;
		smp_rmb();
		x = entry->x;

		if (y && !x)
			errors++;
		ready += y;
	}

	report("mp barrier: %d errors, %d ready", errors == 0, errors, ready);
}

/* void write_values(void) */
/* { */
/* 	int i; */
/* 	for (i=0; i< array_size; i++) { */
/* 		volatile test_array *entry = &array[i]; */
/* 		entry->x = 1; */
/* 		smp_wmb(); */
/* 		entry->y = 1; /\* smp_store_release(entry->y, 1) *\/ */
/* 	} */

/* 	halt(); */

/* } */

/* void read_values(void) */
/* { */
/* 	int i; */
/* 	for (i=0; i< array_size; i++) { */
/* 		volatile test_array *entry = &array[i]; */
/* 		int x,y; */
/* 		y = entry->y; /\* smp_load_acquire(entry->y) *\/ */
/* 		smp_rmb(); */
/* 		x = entry->x; */
/* 		if (y && !x) { */
/* 			report("that was odd", false); */
/* 			return; */
/* 		} */
/* 	} */

/* 	report("all ok", true); */
/* } */

/* Test array */
static test_descr_t tests[] = {

	{ "mp",         false,
	  message_passing_read,
	  { message_passing_write }
	},

	{ "mp_barrier", true,
	  message_passing_read_barrier,
	  { message_passing_write_barrier }
	}
};


void setup_and_run_litmus(test_descr_t *test)
{
	array = calloc(array_size, sizeof(test_array));

	if (array) {
		int i = 0;
		printf("Allocated test array @ %p\n", array);

		while (test->secondary_fns[i]) {
			smp_boot_secondary(i+1, test->secondary_fns[i]);
			i++;
		}

		test->main_fn();
	} else {
		report("%s: failed to allocate memory",false, test->test_name);
	}
}

int main(int argc, char **argv)
{
	int cpu, i;
	unsigned int j;
	test_descr_t *test = NULL;

	for (i=0; i<argc; i++) {
		char *arg = argv[i];

		for (j = 0; j < ARRAY_SIZE(tests); j++) {
			if (strcmp(arg, tests[j].test_name) == 0)
				test = & tests[j];
		}

		/* Test modifiers */
		if (strstr(arg, "count=") != NULL) {
			char *p = strstr(arg, "=");
			array_size = atol(p+1);
		}
	}

	if (test) {
		setup_and_run_litmus(test);
	} else {
		report("Unknown test", false);
	}

	return report_summary();
}
