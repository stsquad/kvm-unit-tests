#ifndef _ASMARM_BARRIER_H_
#define _ASMARM_BARRIER_H_
/*
 * Adapted from arch/arm/include/asm/barrier.h
 */

#include <stdint.h>

#define sev()		asm volatile("sev" : : : "memory")
#define wfe()		asm volatile("wfe" : : : "memory")
#define wfi()		asm volatile("wfi" : : : "memory")
#define cpu_relax()	asm volatile(""    : : : "memory")

#define isb(option) __asm__ __volatile__ ("isb " #option : : : "memory")
#define dsb(option) __asm__ __volatile__ ("dsb " #option : : : "memory")
#define dmb(option) __asm__ __volatile__ ("dmb " #option : : : "memory")

#define mb()		dsb()
#define rmb()		dsb()
#define wmb()		dsb(st)
#define smp_mb()	dmb(ish)
#define smp_rmb()	smp_mb()
#define smp_wmb()	dmb(ishst)

extern void abort(void);

static inline void __write_once_size(volatile void *p, void *res, int size)
{
	switch (size) {
	case 1: *(volatile uint8_t *)p = *(uint8_t *)res; break;
	case 2: *(volatile uint16_t *)p = *(uint16_t *)res; break;
	case 4: *(volatile uint32_t *)p = *(uint32_t *)res; break;
	case 8: *(volatile uint64_t *)p = *(uint64_t *)res; break;
	default:
		/* unhandled case */
		abort();
	}
}

#define WRITE_ONCE(x, val) \
({							\
	union { typeof(x) __val; char __c[1]; } __u =	\
		{ .__val = (typeof(x)) (val) }; \
	__write_once_size(&(x), __u.__c, sizeof(x));	\
	__u.__val;					\
})

#define smp_store_release(p, v)						\
do {									\
	smp_mb();							\
	WRITE_ONCE(*p, v);						\
} while (0)


static inline
void __read_once_size(const volatile void *p, void *res, int size)
{
	switch (size) {
	case 1: *(uint8_t *)res = *(volatile uint8_t *)p; break;
	case 2: *(uint16_t *)res = *(volatile uint16_t *)p; break;
	case 4: *(uint32_t *)res = *(volatile uint32_t *)p; break;
	case 8: *(uint64_t *)res = *(volatile uint64_t *)p; break;
	default:
		/* unhandled case */
		abort();
	}
}

#define READ_ONCE(x)							\
({									\
	union { typeof(x) __val; char __c[1]; } __u;			\
	__read_once_size(&(x), __u.__c, sizeof(x));			\
	__u.__val;							\
})


#define smp_load_acquire(p)						\
({									\
	typeof(*p) ___p1 = READ_ONCE(*p);				\
	smp_mb();							\
	___p1;								\
})

#endif /* _ASMARM_BARRIER_H_ */
