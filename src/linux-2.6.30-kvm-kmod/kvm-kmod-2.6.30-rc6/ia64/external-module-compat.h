/*
 * Compatibility header for building as an external module.
 */

#ifndef __ASSEMBLY__
#include <linux/version.h>

#include "../external-module-compat-comm.h"

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
#error "KVM/IA-64 Can't be compiled if kernel version < 2.6.26"
#endif

#ifndef CONFIG_PREEMPT_NOTIFIERS
/*Now, Just print an error message if no preempt notifiers configured!!
  TODO: Implement it later! */
#error "KVM/IA-64 depends on preempt notifiers in kernel."
#endif

/* smp_call_function() lost an argument in 2.6.27. */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)

#define kvm_smp_call_function(func, info, wait) smp_call_function(func, info, 0, wait)

#else

#define kvm_smp_call_function(func, info, wait) smp_call_function(func, info, wait)

#endif

/*There is no struct fdesc definition <2.6.27*/
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
struct fdesc {
	uint64_t ip;
	uint64_t gp;
};
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28)

typedef u64 phys_addr_t;

#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)

#define PAGE_KERNEL_UC __pgprot(__DIRTY_BITS  | _PAGE_PL_0 | _PAGE_AR_RWX | \
                                       _PAGE_MA_UC)
#endif

#endif

#ifndef CONFIG_HAVE_KVM_IRQCHIP
#define CONFIG_HAVE_KVM_IRQCHIP 1
#endif
