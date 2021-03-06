/* RDPRU test */

#include "libcflat.h"
#include "processor.h"
#include "desc.h"

static int rdpru_checking(void)
{
	asm volatile (ASM_TRY("1f")
		      ".byte 0x0f,0x01,0xfd \n\t" /* rdpru */
		      "1:" : : "c" (0) : "eax", "edx");
	return exception_vector();
}

int main(int ac, char **av)
{
	setup_idt();

	if (this_cpu_has(X86_FEATURE_RDPRU))
		report_skip("RDPRU raises #UD");
	else
		report("RDPRU raises #UD", rdpru_checking() == UD_VECTOR);

	return report_summary();
}
