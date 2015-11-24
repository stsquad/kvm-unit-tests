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

/* Array size and access controls */
static int array_size = 100000;
static int wait_if_ahead = 0;

static cpumask_t cpu_mask;

/*
 * These test_array_* structures are a contiguous array modified by two or more
 * competing CPUs. The padding is to ensure the variables do not share
 * cache lines.
 *
 * All structures start zeroed.
 */

typedef struct test_array
{
	volatile unsigned int x;
	uint8_t dummy[64];
	volatile unsigned int y;
	uint8_t dummy2[64];
	volatile unsigned int r[MAX_CPUS];
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

static unsigned long sync_start(void)
{
	const unsigned long gate_mask = ~0x3ffff;
	unsigned long gate, now;
	gate = get_cntvct() & gate_mask;
	do {
		now =get_cntvct();
	} while ((now & gate_mask)==gate);

	return now;
}

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

	sync_start();
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

	sync_start();
	for (i=0; i< array_size; i++) {
		volatile test_array *entry = &array[i];
		unsigned int x,y;
		y = entry->y;
		x = entry->x;

		if (y && !x)
			errors++;
		ready += y;
	}

	report_xfail("mp: %d errors, %d ready", true, errors == 0, errors, ready);
}

/* Simple Message Passing with barriers */
void message_passing_write_barrier(void)
{
	int i;
	sync_start();
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
	int errors = 0, ready = 0, not_ready = 0;

	sync_start();
	for (i=0; i< array_size; i++) {
		volatile test_array *entry = &array[i];
		unsigned int x, y;
		y = entry->y;
		smp_rmb();
		x = entry->x;

		if (y && !x)
			errors++;

		if (y) {
			ready++;
		} else {
			not_ready++;

			if (not_ready > 2) {
				entry = &array[i+1];
				do {
					not_ready = 0;
				} while (wait_if_ahead && !entry->y);
			}
		}
	}

	report("mp barrier: %d errors, %d ready", errors == 0, errors, ready);
}

/* Simple Message Passing with Acquire/Release */
void message_passing_write_release(void)
{
	int i;
	for (i=0; i< array_size; i++) {
		volatile test_array *entry = &array[i];
		entry->x = 1;
		smp_store_release(&entry->y, 1);
	}

	halt();
}

void message_passing_read_acquire(void)
{
	int i;
	int errors = 0, ready = 0, not_ready = 0;

	for (i=0; i< array_size; i++) {
		volatile test_array *entry = &array[i];
		unsigned int x, y;
		y = smp_load_acquire(&entry->y);
		x = entry->x;

		if (y && !x)
			errors++;

		if (y) {
			ready++;
		} else {
			not_ready++;

			if (not_ready > 2) {
				entry = &array[i+1];
				do {
					not_ready = 0;
				} while (wait_if_ahead && !entry->y);
			}
		}
	}

	report("mp acqrel: %d errors, %d ready", errors == 0, errors, ready);
}

/*
 * Store after load
 *
 * T1: write 1 to x, load r from y
 * T2: write 1 to y, load r from x
 *
 * Without memory fence r[0] && r[1] == 0
 * With memory fence both == 0 should be impossible
 */

static void check_store_and_load_results(char *name, int thread, bool xfail,
					unsigned long start, unsigned long end)
{
	int i;
	int neither = 0;
	int only_first = 0;
	int only_second = 0;
	int both = 0;

	for (i=0; i< array_size; i++) {
		volatile test_array *entry = &array[i];
		if (entry->r[0] == 0 &&
		    entry->r[1] == 0) {
			neither++;
		} else if (entry->r[0] &&
			entry->r[1]) {
			both++;
		} else if (entry->r[0]) {
			only_first++;
		} else {
			only_second++;
		}
	}

	printf("T%d: %08lx->%08lx neither=%d only_t1=%d only_t2=%d both=%d\n", thread,
		start, end, neither, only_first, only_second, both);

	if (thread == 1) {
		if (xfail) {
			report_xfail("%s: errors=%d", true, neither==0,
				name, neither);
		} else {
			report("%s: errors=%d", neither==0, name, neither);
		}

	}
}

/*
 * This attempts to synchronise the start of both threads to roughly
 * the same time. On real hardware there is a little latency as the
 * secondary vCPUs are powered up however this effect it much more
 * exaggerated on a TCG host.
 *
 * Busy waits until the we pass a future point in time, returns final
 * start time.
 */

void store_and_load_1(void)
{
	int i;
	unsigned long start, end;

	start = sync_start();
	for (i=0; i<array_size; i++) {
		volatile test_array *entry = &array[i];
		unsigned int r;
		entry->x = 1;
		r = entry->y;
		entry->r[0] = r;
	}
	end = get_cntvct();

	smp_mb();

	while (!cpumask_test_cpu(1, &cpu_mask))
		cpu_relax();

	check_store_and_load_results("sal", 1, true, start, end);
}

void store_and_load_2(void)
{
	int i;
	unsigned long start, end;

	start = sync_start();
	for (i=0; i<array_size; i++) {
		volatile test_array *entry = &array[i];
		unsigned int r;
		entry->y = 1;
		r = entry->x;
		entry->r[1] = r;
	}
	end = get_cntvct();

	check_store_and_load_results("sal", 2, true, start, end);

	cpumask_set_cpu(1, &cpu_mask);

	halt();
}

void store_and_load_barrier_1(void)
{
	int i;
	unsigned long start, end;

	start = sync_start();
	for (i=0; i< array_size; i++) {
		volatile test_array *entry = &array[i];
		unsigned int r;
		entry->x = 1;
		smp_mb();
		r = entry->y;
		entry->r[0] = r;
	}
	end = get_cntvct();

	smp_mb();

	while (!cpumask_test_cpu(1, &cpu_mask))
		cpu_relax();

	check_store_and_load_results("sal_barrier", 1, false, start, end);
}

void store_and_load_barrier_2(void)
{
	int i;
	unsigned long start, end;

	start = sync_start();
	for (i=0; i< array_size; i++) {
		volatile test_array *entry = &array[i];
		unsigned int r;
		entry->y = 1;
		smp_mb();
		r = entry->x;
		entry->r[1] = r;
	}
	end = get_cntvct();

	check_store_and_load_results("sal_barrier", 2, false, start, end);

	cpumask_set_cpu(1, &cpu_mask);

	halt();
}


/* Test array */
static test_descr_t tests[] = {

	{ "mp",         false,
	  message_passing_read,
	  { message_passing_write }
	},

	{ "mp_barrier", true,
	  message_passing_read_barrier,
	  { message_passing_write_barrier }
	},

	{ "mp_acqrel", true,
	  message_passing_read_acquire,
	  { message_passing_write_release }
	},

	{ "sal",       false,
	  store_and_load_1,
	  { store_and_load_2 }
	},

	{ "sal_barrier", true,
	  store_and_load_barrier_1,
	  { store_and_load_barrier_2 }
	},
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
	int i;
	unsigned int j;
	test_descr_t *test = NULL;

	for (i=0; i<argc; i++) {
		char *arg = argv[i];

		for (j = 0; j < ARRAY_SIZE(tests); j++) {
			if (strcmp(arg, tests[j].test_name) == 0)
				test = &tests[j];
		}

		/* Test modifiers */
		if (strstr(arg, "count=") != NULL) {
			char *p = strstr(arg, "=");
			array_size = atol(p+1);
		} else if (strcmp (arg, "wait") == 0) {
			wait_if_ahead = 1;
		}
	}

	if (test) {
		setup_and_run_litmus(test);
	} else {
		report("Unknown test", false);
	}

	return report_summary();
}
