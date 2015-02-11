/*
 * Test XN
 *
 * Copyright (C) 2014, Red Hat Inc, Andrew Jones <drjones@redhat.com>
 *
 * This work is licensed under the terms of the GNU LGPL, version 2.
 */
#include <libcflat.h>
#include <alloc.h>
#include <asm/processor.h>
#ifdef __aarch64__
#include <asm/esr.h>
#endif
#include <asm/mmu.h>

#ifdef __arm__
#define is_user() (current_mode() == USR_MODE)
#define is_aa64() (false)
#define PTE_PXN (_AT(pteval_t, 1) << 53)
#define PTE_UXN (_AT(pteval_t, 1) << 54)
#define R0 "r0"
#define R1 "r1"

static inline void flush_dcache_page(unsigned long addr)
{
	unsigned long i;
	dmb();
	for (i = 0; i < PAGE_SIZE; i += L1_CACHE_BYTES)
		asm volatile("mcr p15, 0, %0, c7, c14, 1" :: "r" (addr + i));
	dsb();
}

#else
#define is_user() (user_mode)
#define is_aa64() (true)
#define R0 "x0"
#define R1 "x1"

static inline void flush_dcache_page(unsigned long addr)
{
	unsigned long i;
	for (i = 0; i < PAGE_SIZE; i += L1_CACHE_BYTES)
		asm volatile("dc civac, %0" :: "r" (addr + i));
	mb();
}
#endif

#define SCTLR_WXN	19
#define SCTLR_UWXN	20

static pte_t *get_pte(pgd_t *pgtable, unsigned long vaddr)
{
	pgd_t *pgd = pgd_offset(pgtable, vaddr);
	pud_t *pud = pud_offset(pgd, vaddr);
	pmd_t *pmd = pmd_offset(pud, vaddr);
	pte_t *pte = pte_offset(pmd, vaddr);
	return pte;
}

static bool faulted;

struct xntest {
	char name[32];
	bool wxn;
	bool uwxn;
	pteval_t flags;
	bool (*pass)(void);
	bool (*pass64)(void);
};

static bool none_fault(void)
{
	return !faulted;
}
static bool el1_fault(void)
{
	return (!is_user() && faulted) || (is_user() && !faulted);
}
static bool el0_fault(void)
{
	return (is_user() && faulted) || (!is_user() && !faulted);
}
static bool all_fault(void)
{
	return faulted;
}

static struct xntest xntests[] = {
{ "<none>",			0, 0, 0,					el0_fault, none_fault, },
{ "ro",				0, 0, PTE_RDONLY,				el0_fault, none_fault, },
{ "usr",			0, 0, PTE_USER,					none_fault, el1_fault, },
{ "ro,usr",			0, 0, PTE_RDONLY|PTE_USER,			none_fault, none_fault,},

{ "[u]xn,<none>",		0, 0, PTE_UXN,					all_fault, el0_fault,  },
{ "[u]xn,ro",			0, 0, PTE_UXN|PTE_RDONLY,			all_fault, el0_fault,  },
{ "[u]xn,usr",			0, 0, PTE_UXN|PTE_USER,				all_fault, all_fault,  },
{ "[u]xn,ro,usr",		0, 0, PTE_UXN|PTE_RDONLY|PTE_USER,		all_fault, el0_fault,  },

{ "pxn,<none>",			0, 0, PTE_PXN,					all_fault, el1_fault,  },
{ "pxn,ro",			0, 0, PTE_PXN|PTE_RDONLY,			all_fault, el1_fault,  },
{ "pxn,usr",			0, 0, PTE_PXN|PTE_USER,				el1_fault, el1_fault,  },
{ "pxn,ro,usr",			0, 0, PTE_PXN|PTE_RDONLY|PTE_USER,		el1_fault, el1_fault,  },

{ "[u]xn,pxn,<none>",		0, 0, PTE_UXN|PTE_PXN,				all_fault, all_fault,  },
{ "[u]xn,pxn,ro",		0, 0, PTE_UXN|PTE_PXN|PTE_RDONLY,		all_fault, all_fault,  },
{ "[u]xn,pxn,usr",		0, 0, PTE_UXN|PTE_PXN|PTE_USER,			all_fault, all_fault,  },
{ "[u]xn,pxn,ro,usr",		0, 0, PTE_UXN|PTE_PXN|PTE_RDONLY|PTE_USER,	all_fault, all_fault,  },

{ "wxn,<none>",			1, 0, 0,					all_fault, el1_fault,  },
{ "wxn,ro",			1, 0, PTE_RDONLY,				el0_fault, none_fault, },
{ "wxn,usr",			1, 0, PTE_USER,					all_fault, all_fault,  },
{ "wxn,ro,usr",			1, 0, PTE_RDONLY|PTE_USER,			none_fault, none_fault,},

{ "wxn,[u]xn,<none>",		1, 0, PTE_UXN,					all_fault, all_fault,  },
{ "wxn,[u]xn,ro",		1, 0, PTE_UXN|PTE_RDONLY,			all_fault, el0_fault,  },
{ "wxn,[u]xn,usr",		1, 0, PTE_UXN|PTE_USER,				all_fault, all_fault,  },
{ "wxn,[u]xn,ro,usr",		1, 0, PTE_UXN|PTE_RDONLY|PTE_USER,		all_fault, el0_fault,  },

{ "wxn,pxn,<none>",		1, 0, PTE_PXN,					all_fault, el1_fault,  },
{ "wxn,pxn,ro",			1, 0, PTE_PXN|PTE_RDONLY,			all_fault, el1_fault,  },
{ "wxn,pxn,usr",		1, 0, PTE_PXN|PTE_USER,				all_fault, all_fault,  },
{ "wxn,pxn,ro,usr",		1, 0, PTE_PXN|PTE_RDONLY|PTE_USER,		el1_fault, el1_fault,  },

{ "wxn,[u]xn,pxn,<none>",	1, 0, PTE_UXN|PTE_PXN,				all_fault, all_fault,  },
{ "wxn,[u]xn,pxn,ro",		1, 0, PTE_UXN|PTE_PXN|PTE_RDONLY,		all_fault, all_fault,  },
{ "wxn,[u]xn,pxn,usr",		1, 0, PTE_UXN|PTE_PXN|PTE_USER,			all_fault, all_fault,  },
{ "wxn,[u]xn,pxn,ro,usr",	1, 0, PTE_UXN|PTE_PXN|PTE_RDONLY|PTE_USER,	all_fault, all_fault,  },

{ "uwxn,<none>",		0, 1, 0,					el0_fault, none_fault, },
{ "uwxn,ro",			0, 1, PTE_RDONLY,				el0_fault, none_fault, },
{ "uwxn,usr",			0, 1, PTE_USER,					el1_fault, el1_fault,  },
{ "uwxn,ro,usr",		0, 1, PTE_RDONLY|PTE_USER,			none_fault, none_fault,},

{ "uwxn,[u]xn,<none>",		0, 1, PTE_UXN,					all_fault, el0_fault,  },
{ "uwxn,[u]xn,ro",		0, 1, PTE_UXN|PTE_RDONLY,			all_fault, el0_fault,  },
{ "uwxn,[u]xn,usr",		0, 1, PTE_UXN|PTE_USER,				all_fault, all_fault,  },
{ "uwxn,[u]xn,ro,usr",		0, 1, PTE_UXN|PTE_RDONLY|PTE_USER,		all_fault, el0_fault,  },

{ "uwxn,pxn,<none>",		0, 1, PTE_PXN,					all_fault, el1_fault,  },
{ "uwxn,pxn,ro",		0, 1, PTE_PXN|PTE_RDONLY,			all_fault, el1_fault,  },
{ "uwxn,pxn,usr",		0, 1, PTE_PXN|PTE_USER,				el1_fault, el1_fault,  },
{ "uwxn,pxn,ro,usr",		0, 1, PTE_PXN|PTE_RDONLY|PTE_USER,		el1_fault, el1_fault,  },

{ "uwxn,[u]xn,pxn,<none>",	0, 1, PTE_UXN|PTE_PXN,				all_fault, all_fault,  },
{ "uwxn,[u]xn,pxn,ro",		0, 1, PTE_UXN|PTE_PXN|PTE_RDONLY,		all_fault, all_fault,  },
{ "uwxn,[u]xn,pxn,usr",		0, 1, PTE_UXN|PTE_PXN|PTE_USER,			all_fault, all_fault,  },
{ "uwxn,[u]xn,pxn,ro,usr",	0, 1, PTE_UXN|PTE_PXN|PTE_RDONLY|PTE_USER,	all_fault, all_fault,  },

{ "uwxn,wxn,<none>",		1, 1, 0,					all_fault, el1_fault,  },
{ "uwxn,wxn,ro",		1, 1, PTE_RDONLY,				el0_fault, none_fault, },
{ "uwxn,wxn,usr",		1, 1, PTE_USER,					all_fault, all_fault,  },
{ "uwxn,wxn,ro,usr",		1, 1, PTE_RDONLY|PTE_USER,			none_fault, none_fault,},

{ "uwxn,wxn,[u]xn,<none>",	1, 1, PTE_UXN,					all_fault, all_fault,  },
{ "uwxn,wxn,[u]xn,ro",		1, 1, PTE_UXN|PTE_RDONLY,			all_fault, el0_fault,  },
{ "uwxn,wxn,[u]xn,usr",		1, 1, PTE_UXN|PTE_USER,				all_fault, all_fault,  },
{ "uwxn,wxn,[u]xn,ro,usr",	1, 1, PTE_UXN|PTE_RDONLY|PTE_USER,		all_fault, el0_fault,  },

{ "uwxn,wxn,pxn,<none>",	1, 1, PTE_PXN,					all_fault, el1_fault,  },
{ "uwxn,wxn,pxn,ro",		1, 1, PTE_PXN|PTE_RDONLY,			all_fault, el1_fault,  },
{ "uwxn,wxn,pxn,usr",		1, 1, PTE_PXN|PTE_USER,				all_fault, all_fault,  },
{ "uwxn,wxn,pxn,ro,usr",	1, 1, PTE_PXN|PTE_RDONLY|PTE_USER,		el1_fault, el1_fault,  },

{ "uwxn,wxn,[u]xn,pxn,<none>",	1, 1, PTE_UXN|PTE_PXN,				all_fault, all_fault,  },
{ "uwxn,wxn,[u]xn,pxn,ro",	1, 1, PTE_UXN|PTE_PXN|PTE_RDONLY,		all_fault, all_fault,  },
{ "uwxn,wxn,[u]xn,pxn,usr",	1, 1, PTE_UXN|PTE_PXN|PTE_USER,			all_fault, all_fault,  },
{ "uwxn,wxn,[u]xn,pxn,ro,usr",	1, 1, PTE_UXN|PTE_PXN|PTE_RDONLY|PTE_USER,	all_fault, all_fault,  },
};

static void __do_flush_page(unsigned long addr)
{
	flush_dcache_page(addr);

	/*
	 * flush all tlb entries to also flush the entry
	 * for the code page we're trying to execute
	 */
	flush_tlb_all();
}
static void do_flush_page(unsigned long addr)
{
	addr &= PAGE_MASK;

	if (is_user())
		asm volatile(
			"mov " R0 ", %0\n"
			"svc #0\n"
		:: "r" (addr) : R0);
	else
		__do_flush_page(addr);
}

#ifdef __arm__
static void __set_sctlr(int v, int b)
{
	unsigned int sctlr;
	asm volatile("mrc p15, 0, %0, c1, c0, 0" : "=r" (sctlr));
	if (v)
		sctlr |= (1 << b);
	else
		sctlr &= ~(1 << b);
	asm volatile("mcr p15, 0, %0, c1, c0, 0" :: "r" (sctlr));
}
#else
static void __set_sctlr(int v, int b)
{
	unsigned int sctlr;
	asm volatile("mrs %0, sctlr_el1" : "=r" (sctlr));
	if (v)
		sctlr |= (1 << b);
	else
		sctlr &= ~(1 << b);
	asm volatile("msr sctlr_el1, %0" :: "r" (sctlr));
}
#endif

static void set_sctlr(int v, int b)
{
#if 1 /* AArch64 doesn't define UWXN, comment this out to test anyway. */
	if (is_aa64() && b == SCTLR_UWXN)
		return;
#endif

	if (is_user()) {
		asm volatile(
			"mov " R0 ", %0\n"
			"mov " R1 ", %1\n"
			"svc #1\n"
		:: "r" (v), "r" (b) : R0, R1);
	} else
		__set_sctlr(v, b);
}

static bool do_test(struct xntest *test, pte_t *pte, void *page)
{
	typedef void (*func_t)(void);
	func_t f = (func_t)page;

	faulted = false;

	set_sctlr(test->wxn, SCTLR_WXN);
	set_sctlr(test->uwxn, SCTLR_UWXN);

	pte_val(*pte) &= ~(PTE_UXN | PTE_PXN | PTE_RDONLY | PTE_USER);
	pte_val(*pte) |= test->flags;
	do_flush_page((unsigned long)pte);

	f();

	return is_aa64() ? test->pass64() : test->pass();
}

#ifdef __arm__
static void pabt_handler(struct pt_regs *regs)
{
	faulted = true;
	regs->ARM_pc = regs->ARM_lr;
}
static void svc_handler(struct pt_regs *regs)
{
	u32 svc = *(u32 *)(regs->ARM_pc - 4) & 0xffffff;

	switch (svc) {
	case 0:
		__do_flush_page(regs->ARM_r0);
		break;
	case 1:
		__set_sctlr(regs->ARM_r0, regs->ARM_r1);
		break;
	}
}
#else
static void iabt_handler(struct pt_regs *regs, unsigned int esr __unused)
{
	faulted = true;
	regs->pc = regs->regs[30];
}
static void svc_handler(struct pt_regs *regs, unsigned int esr)
{
	u16 svc = esr & 0xffff;

	switch (svc) {
	case 0:
		__do_flush_page(regs->regs[0]);
		break;
	case 1:
		__set_sctlr(regs->regs[0], regs->regs[1]);
		break;
	}
}
#endif

static void check_xn(void *arg __unused)
{
	void *page = memalign(PAGE_SIZE, PAGE_SIZE);
	pte_t *pte = get_pte(mmu_idmap, (unsigned long)page);
	unsigned i;

	*((unsigned int *)page) = is_aa64() ? 0xd65f03c0 : 0xe1a0f00e; /* ret */

	report_prefix_push(is_user() ? "el0" : "el1");

#ifdef __arm__
	if (is_user())
		install_exception_handler(EXCPTN_SVC, svc_handler);
	install_exception_handler(EXCPTN_PABT, pabt_handler);
#else
	if (is_user())
		install_exception_handler(EL0_SYNC_64, ESR_EL1_EC_SVC64,
					  svc_handler);
	install_exception_handler(
		is_user() ? EL0_SYNC_64 : EL1H_SYNC,
		is_user() ? ESR_EL1_EC_IABT_EL0 : ESR_EL1_EC_IABT_EL1,
		iabt_handler);
#endif
	do_flush_page((unsigned long)page);

	for (i = 0; i < ARRAY_SIZE(xntests); ++i)
		report("%s", do_test(&xntests[i], pte, page), xntests[i].name);

	report_prefix_pop();
	if (is_user())
		exit(report_summary());
}

int main(void)
{
	void *sp = memalign(PAGE_SIZE, 16384) + 16384;

	report_prefix_push("xntest");
	check_xn(NULL);
	start_usr(check_xn, NULL, (unsigned long)sp);
	return 0;
}
