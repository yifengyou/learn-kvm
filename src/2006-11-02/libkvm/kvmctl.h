#ifndef KVMCTL_H
#define KVMCTL_H

#define __user /* temporary, until installed via make headers_install */
#include <linux/kvm.h>
#include <stdint.h>

struct kvm_context;

typedef struct kvm_context *kvm_context_t;

struct kvm_callbacks {
    int (*cpuid)(void *opaque, 
		  uint64_t *rax, uint64_t *rbx, uint64_t *rcx, uint64_t *rdx);
    int (*inb)(void *opaque, uint16_t addr, uint8_t *data);
    int (*inw)(void *opaque, uint16_t addr, uint16_t *data);
    int (*inl)(void *opaque, uint16_t addr, uint32_t *data);
    int (*outb)(void *opaque, uint16_t addr, uint8_t data);
    int (*outw)(void *opaque, uint16_t addr, uint16_t data);
    int (*outl)(void *opaque, uint16_t addr, uint32_t data);
    int (*readb)(void *opaque, uint64_t addr, uint8_t *data);
    int (*readw)(void *opaque, uint64_t addr, uint16_t *data);
    int (*readl)(void *opaque, uint64_t addr, uint32_t *data);
    int (*readq)(void *opaque, uint64_t addr, uint64_t *data);
    int (*writeb)(void *opaque, uint64_t addr, uint8_t data);
    int (*writew)(void *opaque, uint64_t addr, uint16_t data);
    int (*writel)(void *opaque, uint64_t addr, uint32_t data);
    int (*writeq)(void *opaque, uint64_t addr, uint64_t data);
    int (*debug)(void *opaque, int vcpu);
    int (*halt)(void *opaque, int vcpu);
    int (*io_window)(void *opaque);
};

kvm_context_t kvm_init(struct kvm_callbacks *callbacks,
		       void *opaque);
int kvm_create(kvm_context_t kvm,
	       unsigned long phys_mem_bytes,
	       void **phys_mem);
int kvm_run(kvm_context_t kvm, int vcpu);
int kvm_get_regs(kvm_context_t, int vcpu, struct kvm_regs *regs);
int kvm_set_regs(kvm_context_t, int vcpu, struct kvm_regs *regs);
int kvm_get_sregs(kvm_context_t, int vcpu, struct kvm_sregs *regs);
int kvm_set_sregs(kvm_context_t, int vcpu, struct kvm_sregs *regs);
int kvm_inject_irq(kvm_context_t, int vcpu, unsigned irq);
int kvm_guest_debug(kvm_context_t, int vcpu, struct kvm_debug_guest *dbg);
void kvm_show_regs(kvm_context_t, int vcpu);
void *kvm_create_phys_mem(kvm_context_t, unsigned long phys_start, 
			  unsigned long len, int slot, int log, int writable);
void kvm_destroy_phys_mem(kvm_context_t, unsigned long phys_start, 
			  unsigned long len);
void kvm_get_dirty_pages(kvm_context_t, int slot, void *buf);

#endif
