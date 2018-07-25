/*
 * Kernel-based Virtual Machine driver for Linux
 *
 * This module enables machines with Intel VT-x extensions to run virtual
 * machines without emulation or binary translation.
 *
 * Debug support
 *
 * Copyright (C) 2006 Qumranet, Inc.
 *
 * Authors:
 *   Yaniv Kamay  <yaniv@qumranet.com>
 *   Avi Kivity   <avi@qumranet.com>
 *
 */

#include <linux/highmem.h>

#include "kvm.h"
#include "debug.h"

#ifdef KVM_DEBUG

static const char *vmx_msr_name[] = {
	"MSR_EFER", "MSR_STAR", "MSR_CSTAR",
	"MSR_KERNEL_GS_BASE", "MSR_SYSCALL_MASK", "MSR_LSTAR"
};

#define NR_VMX_MSR (sizeof(vmx_msr_name) / sizeof(char*))

void show_msrs(struct kvm_vcpu *vcpu)
{
	int i;

	for (i = 0; i < NR_VMX_MSR; ++i) {
		vcpu_printf(vcpu, "%s: %s=0x%llx\n",
		       __FUNCTION__,
		       vmx_msr_name[i],
		       vcpu->guest_msrs[i].data);
	}
}

void show_code(struct kvm_vcpu *vcpu)
{
	gva_t rip = vmcs_readl(GUEST_RIP);
	u8 code[50];
	char buf[30 + 3 * sizeof code];
	int i;

	if (!is_long_mode())
		rip += vmcs_readl(GUEST_CS_BASE);

	kvm_read_guest(vcpu, rip, sizeof code, code);
	for (i = 0; i < sizeof code; ++i)
		sprintf(buf + i * 3, " %02x", code[i]);
	vcpu_printf(vcpu, "code: %lx%s\n", rip, buf);
}

struct gate_struct {
	u16 offset_low;
	u16 segment;
	unsigned ist : 3, zero0 : 5, type : 5, dpl : 2, p : 1;
	u16 offset_middle;
	u32 offset_high;
	u32 zero1;
} __attribute__((packed));

void show_irq(struct kvm_vcpu *vcpu,  int irq)
{
	unsigned long idt_base = vmcs_readl(GUEST_IDTR_BASE);
	unsigned long idt_limit = vmcs_readl(GUEST_IDTR_LIMIT);
	struct gate_struct gate;

	if (!is_long_mode())
		vcpu_printf(vcpu, "%s: not in long mode\n", __FUNCTION__);

	if (!is_long_mode() || idt_limit < irq * sizeof(gate)) {
		vcpu_printf(vcpu, "%s: 0x%x read_guest err\n",
			   __FUNCTION__,
			   irq);
		return;
	}

	if (kvm_read_guest(vcpu, idt_base + irq * sizeof(gate), sizeof(gate), &gate) != sizeof(gate)) {
		vcpu_printf(vcpu, "%s: 0x%x read_guest err\n",
			   __FUNCTION__,
			   irq);
		return;
	}
	vcpu_printf(vcpu, "%s: 0x%x handler 0x%llx\n",
		   __FUNCTION__,
		   irq,
		   ((u64)gate.offset_high << 32) |
		   ((u64)gate.offset_middle << 16) |
		   gate.offset_low);
}

void show_page(struct kvm_vcpu *vcpu,
			     gva_t addr)
{
	u64 *buf = kmalloc(PAGE_SIZE, GFP_KERNEL);

	if (!buf)
		return;

	addr &= PAGE_MASK;
	if (kvm_read_guest(vcpu, addr, PAGE_SIZE, buf)) {
		int i;
		for (i = 0; i <  PAGE_SIZE / sizeof(u64) ; i++) {
			u8 *ptr = (u8*)&buf[i];
			int j;
			vcpu_printf(vcpu, " 0x%16.16lx:",
				   addr + i * sizeof(u64));
			for (j = 0; j < sizeof(u64) ; j++)
				vcpu_printf(vcpu, " 0x%2.2x", ptr[j]);
			vcpu_printf(vcpu, "\n");
		}
	}
	kfree(buf);
}

void show_u64(struct kvm_vcpu *vcpu, gva_t addr)
{
	u64 buf;

	if (kvm_read_guest(vcpu, addr, sizeof(u64), &buf) == sizeof(u64)) {
		u8 *ptr = (u8*)&buf;
		int j;
		vcpu_printf(vcpu, " 0x%16.16lx:", addr);
		for (j = 0; j < sizeof(u64) ; j++)
			vcpu_printf(vcpu, " 0x%2.2x", ptr[j]);
		vcpu_printf(vcpu, "\n");
	}
}

#define IA32_DEBUGCTL_RESERVED_BITS 0xfffffffffffffe3cULL

static int is_canonical(unsigned long addr)
{
       return  addr == ((long)addr << 16) >> 16;
}

int vm_entry_test_guest(struct kvm_vcpu *vcpu)
{
	unsigned long cr0;
	unsigned long cr4;
	unsigned long cr3;
	unsigned long dr7;
	u64 ia32_debugctl;
	unsigned long sysenter_esp;
	unsigned long sysenter_eip;
	unsigned long rflags;

	int long_mode;
	int virtual8086;

	#define RFLAGS_VM (1 << 17)
	#define RFLAGS_RF (1 << 9)


	#define VIR8086_SEG_BASE_TEST(seg)\
		if (vmcs_readl(GUEST_##seg##_BASE) != \
		    (unsigned long)vmcs_read16(GUEST_##seg##_SELECTOR) << 4) {\
			vcpu_printf(vcpu, "%s: "#seg" base 0x%lx in "\
				   "virtual8086 is not "#seg" selector 0x%x"\
				   " shifted right 4 bits\n",\
			   __FUNCTION__,\
			   vmcs_readl(GUEST_##seg##_BASE),\
			   vmcs_read16(GUEST_##seg##_SELECTOR));\
			return 0;\
		}

	#define VIR8086_SEG_LIMIT_TEST(seg)\
		if (vmcs_readl(GUEST_##seg##_LIMIT) != 0x0ffff) { \
			vcpu_printf(vcpu, "%s: "#seg" limit 0x%lx in "\
				   "virtual8086 is not 0xffff\n",\
			   __FUNCTION__,\
			   vmcs_readl(GUEST_##seg##_LIMIT));\
			return 0;\
		}

	#define VIR8086_SEG_AR_TEST(seg)\
		if (vmcs_read32(GUEST_##seg##_AR_BYTES) != 0x0f3) { \
			vcpu_printf(vcpu, "%s: "#seg" AR 0x%x in "\
				   "virtual8086 is not 0xf3\n",\
			   __FUNCTION__,\
			   vmcs_read32(GUEST_##seg##_AR_BYTES));\
			return 0;\
		}


	cr0 = vmcs_readl(GUEST_CR0);

	if (!(cr0 & CR0_PG_MASK)) {
		vcpu_printf(vcpu, "%s: cr0 0x%lx, PG is not set\n",
			   __FUNCTION__, cr0);
		return 0;
	}

	if (!(cr0 & CR0_PE_MASK)) {
		vcpu_printf(vcpu, "%s: cr0 0x%lx, PE is not set\n",
			   __FUNCTION__, cr0);
		return 0;
	}

	if (!(cr0 & CR0_NE_MASK)) {
		vcpu_printf(vcpu, "%s: cr0 0x%lx, NE is not set\n",
			   __FUNCTION__, cr0);
		return 0;
	}

	if (!(cr0 & CR0_WP_MASK)) {
		vcpu_printf(vcpu, "%s: cr0 0x%lx, WP is not set\n",
			   __FUNCTION__, cr0);
	}

	cr4 = vmcs_readl(GUEST_CR4);

	if (!(cr4 & CR4_VMXE_MASK)) {
		vcpu_printf(vcpu, "%s: cr4 0x%lx, VMXE is not set\n",
			   __FUNCTION__, cr4);
		return 0;
	}

	if (!(cr4 & CR4_PAE_MASK)) {
		vcpu_printf(vcpu, "%s: cr4 0x%lx, PAE is not set\n",
			   __FUNCTION__, cr4);
	}

	ia32_debugctl = vmcs_read64(GUEST_IA32_DEBUGCTL);

	if (ia32_debugctl & IA32_DEBUGCTL_RESERVED_BITS ) {
		vcpu_printf(vcpu, "%s: ia32_debugctl 0x%llx, reserve bits\n",
			   __FUNCTION__, ia32_debugctl);
		return 0;
	}

	long_mode = is_long_mode();

	if (long_mode) {
	}

	if ( long_mode && !(cr4 & CR4_PAE_MASK)) {
		vcpu_printf(vcpu, "%s: long mode and not PAE\n",
			   __FUNCTION__);
		return 0;
	}

	cr3 = vmcs_readl(GUEST_CR3);

	if (cr3 & CR3_L_MODE_RESEVED_BITS) {
		vcpu_printf(vcpu, "%s: cr3 0x%lx, reserved bits\n",
			   __FUNCTION__, cr3);
		return 0;
	}

	if ( !long_mode && (cr4 & CR4_PAE_MASK)) {
		/* check the 4 PDPTEs for reserved bits */
		unsigned long pdpt_pfn = cr3 >> PAGE_SHIFT;
		int i;
		u64 pdpte;
		unsigned offset = (cr3 & (PAGE_SIZE-1)) >> 5;
		u64 *pdpt = kmap_atomic(pfn_to_page(pdpt_pfn), KM_USER0);

		for (i = 0; i < 4; ++i) {
			pdpte = pdpt[offset + i];
			if ((pdpte & 1) && (pdpte & 0xfffffff0000001e6ull))
				break;
		}

		kunmap_atomic(pdpt, KM_USER0);

		if (i != 4) {
			vcpu_printf(vcpu, "%s: pae cr3[%d] 0x%llx, reserved bits\n",
				   __FUNCTION__, i, pdpte);
			return 0;
		}
	}

	dr7 = vmcs_readl(GUEST_DR7);

	if (dr7 & ~((1ULL << 32) - 1)) {
		vcpu_printf(vcpu, "%s: dr7 0x%lx, reserved bits\n",
			   __FUNCTION__, dr7);
		return 0;
	}

	sysenter_esp = vmcs_readl(GUEST_SYSENTER_ESP);

	if (!is_canonical(sysenter_esp)) {
		vcpu_printf(vcpu, "%s: sysenter_esp 0x%lx, not canonical\n",
			   __FUNCTION__, sysenter_esp);
		return 0;
	}

	sysenter_eip = vmcs_readl(GUEST_SYSENTER_EIP);

	if (!is_canonical(sysenter_eip)) {
		vcpu_printf(vcpu, "%s: sysenter_eip 0x%lx, not canonical\n",
			   __FUNCTION__, sysenter_eip);
		return 0;
	}

	rflags = vmcs_readl(GUEST_RFLAGS);
	virtual8086 = rflags & RFLAGS_VM;


	if (vmcs_read16(GUEST_TR_SELECTOR) & SELECTOR_TI_MASK) {
	       vcpu_printf(vcpu, "%s: tr selctor 0x%x, TI is set\n",
			   __FUNCTION__, vmcs_read16(GUEST_TR_SELECTOR));
	       return 0;
	}

	if (!(vmcs_read32(GUEST_LDTR_AR_BYTES) & AR_UNUSABLE_MASK) &&
	      vmcs_read16(GUEST_LDTR_SELECTOR) & SELECTOR_TI_MASK) {
	       vcpu_printf(vcpu, "%s: ldtr selctor 0x%x,"
				     " is usable and TI is set\n",
			   __FUNCTION__, vmcs_read16(GUEST_LDTR_SELECTOR));
	       return 0;
	}

	if (!virtual8086 &&
	    (vmcs_read16(GUEST_SS_SELECTOR) & SELECTOR_RPL_MASK) !=
	    (vmcs_read16(GUEST_CS_SELECTOR) & SELECTOR_RPL_MASK)) {
		vcpu_printf(vcpu, "%s: ss selctor 0x%x cs selctor 0x%x,"
				     " not same RPL\n",
			   __FUNCTION__,
			   vmcs_read16(GUEST_SS_SELECTOR),
			   vmcs_read16(GUEST_CS_SELECTOR));
		return 0;
	}

	if (virtual8086) {
		VIR8086_SEG_BASE_TEST(CS);
		VIR8086_SEG_BASE_TEST(SS);
		VIR8086_SEG_BASE_TEST(DS);
		VIR8086_SEG_BASE_TEST(ES);
		VIR8086_SEG_BASE_TEST(FS);
		VIR8086_SEG_BASE_TEST(GS);
	}

	if (!is_canonical(vmcs_readl(GUEST_TR_BASE)) ||
	    !is_canonical(vmcs_readl(GUEST_FS_BASE)) ||
	    !is_canonical(vmcs_readl(GUEST_GS_BASE)) ) {
		vcpu_printf(vcpu, "%s: TR 0x%lx FS 0x%lx or GS 0x%lx base"
				      " is not canonical\n",
			   __FUNCTION__,
			   vmcs_readl(GUEST_TR_BASE),
			   vmcs_readl(GUEST_FS_BASE),
			   vmcs_readl(GUEST_GS_BASE));
		return 0;

	}

	if (!(vmcs_read32(GUEST_LDTR_AR_BYTES) & AR_UNUSABLE_MASK) &&
	    !is_canonical(vmcs_readl(GUEST_LDTR_BASE))) {
		vcpu_printf(vcpu, "%s: LDTR base 0x%lx, usable and is not"
				      " canonical\n",
			   __FUNCTION__,
			   vmcs_readl(GUEST_LDTR_BASE));
		return 0;
	}

	if ((vmcs_readl(GUEST_CS_BASE) & ~((1ULL << 32) - 1))) {
		vcpu_printf(vcpu, "%s: CS base 0x%lx, not all bits 63-32"
				      " are zero\n",
			   __FUNCTION__,
			   vmcs_readl(GUEST_CS_BASE));
		return 0;
	}

	#define SEG_BASE_TEST(seg)\
	if ( !(vmcs_read32(GUEST_##seg##_AR_BYTES) & AR_UNUSABLE_MASK) &&\
	     (vmcs_readl(GUEST_##seg##_BASE) & ~((1ULL << 32) - 1))) {\
		vcpu_printf(vcpu, "%s: "#seg" base 0x%lx, is usable and not"\
						" all bits 63-32 are zero\n",\
			   __FUNCTION__,\
			   vmcs_readl(GUEST_##seg##_BASE));\
		return 0;\
	}
	SEG_BASE_TEST(SS);
	SEG_BASE_TEST(DS);
	SEG_BASE_TEST(ES);

	if (virtual8086) {
		VIR8086_SEG_LIMIT_TEST(CS);
		VIR8086_SEG_LIMIT_TEST(SS);
		VIR8086_SEG_LIMIT_TEST(DS);
		VIR8086_SEG_LIMIT_TEST(ES);
		VIR8086_SEG_LIMIT_TEST(FS);
		VIR8086_SEG_LIMIT_TEST(GS);
	}

	if (virtual8086) {
		VIR8086_SEG_AR_TEST(CS);
		VIR8086_SEG_AR_TEST(SS);
		VIR8086_SEG_AR_TEST(DS);
		VIR8086_SEG_AR_TEST(ES);
		VIR8086_SEG_AR_TEST(FS);
		VIR8086_SEG_AR_TEST(GS);
	} else {

		u32 cs_ar = vmcs_read32(GUEST_CS_AR_BYTES);
		u32 ss_ar = vmcs_read32(GUEST_SS_AR_BYTES);
		u32 tr_ar = vmcs_read32(GUEST_TR_AR_BYTES);
		u32 ldtr_ar = vmcs_read32(GUEST_LDTR_AR_BYTES);

		#define SEG_G_TEST(seg) {					\
		u32 lim = vmcs_read32(GUEST_##seg##_LIMIT);		\
		u32 ar = vmcs_read32(GUEST_##seg##_AR_BYTES);		\
		int err = 0;							\
		if (((lim & ~PAGE_MASK) != ~PAGE_MASK) && (ar & AR_G_MASK))	\
			err = 1;						\
		if ((lim & ~((1u << 20) - 1)) && !(ar & AR_G_MASK))		\
			err = 1;						\
		if (err) {							\
			vcpu_printf(vcpu, "%s: "#seg" AR 0x%x, G err. lim"	\
							" is 0x%x\n",		\
						   __FUNCTION__,		\
						   ar, lim);			\
			return 0;						\
		}								\
		}


		if (!(cs_ar & AR_TYPE_ACCESSES_MASK)) {
			vcpu_printf(vcpu, "%s: cs AR 0x%x, accesses is clear\n",
			   __FUNCTION__,
			   cs_ar);
			return 0;
		}

		if (!(cs_ar & AR_TYPE_CODE_MASK)) {
			vcpu_printf(vcpu, "%s: cs AR 0x%x, code is clear\n",
			   __FUNCTION__,
			   cs_ar);
			return 0;
		}

		if (!(cs_ar & AR_S_MASK)) {
			vcpu_printf(vcpu, "%s: cs AR 0x%x, type is sys\n",
			   __FUNCTION__,
			   cs_ar);
			return 0;
		}

		if ((cs_ar & AR_TYPE_MASK) >= 8 && (cs_ar & AR_TYPE_MASK) < 12 &&
		    AR_DPL(cs_ar) !=
		    (vmcs_read16(GUEST_CS_SELECTOR) & SELECTOR_RPL_MASK) ) {
			vcpu_printf(vcpu, "%s: cs AR 0x%x, "
					      "DPL not as RPL\n",
				   __FUNCTION__,
				   cs_ar);
			return 0;
		}

		if ((cs_ar & AR_TYPE_MASK) >= 13 && (cs_ar & AR_TYPE_MASK) < 16 &&
		    AR_DPL(cs_ar) >
		    (vmcs_read16(GUEST_CS_SELECTOR) & SELECTOR_RPL_MASK) ) {
			vcpu_printf(vcpu, "%s: cs AR 0x%x, "
					      "DPL greater than RPL\n",
				   __FUNCTION__,
				   cs_ar);
			return 0;
		}

		if (!(cs_ar & AR_P_MASK)) {
				vcpu_printf(vcpu, "%s: CS AR 0x%x, not "
						      "present\n",
					   __FUNCTION__,
					   cs_ar);
				return 0;
		}

		if ((cs_ar & AR_RESERVD_MASK)) {
				vcpu_printf(vcpu, "%s: CS AR 0x%x, reseved"
						      " bits are set\n",
					   __FUNCTION__,
					   cs_ar);
				return 0;
		}

		if (long_mode & (cs_ar & AR_L_MASK) && (cs_ar & AR_DB_MASK)) {
			vcpu_printf(vcpu, "%s: CS AR 0x%x, DB and L are set"
					      " in long mode\n",
					   __FUNCTION__,
					   cs_ar);
			return 0;

		}

		SEG_G_TEST(CS);

		if (!(ss_ar & AR_UNUSABLE_MASK)) {
		    if ((ss_ar & AR_TYPE_MASK) != 3 &&
			(ss_ar & AR_TYPE_MASK) != 7 ) {
			vcpu_printf(vcpu, "%s: ss AR 0x%x, usable and type"
					      " is not 3 or 7\n",
			   __FUNCTION__,
			   ss_ar);
			return 0;
		    }

		    if (!(ss_ar & AR_S_MASK)) {
			vcpu_printf(vcpu, "%s: ss AR 0x%x, usable and"
					      " is sys\n",
			   __FUNCTION__,
			   ss_ar);
			return 0;
		    }
		    if (!(ss_ar & AR_P_MASK)) {
				vcpu_printf(vcpu, "%s: SS AR 0x%x, usable"
						      " and  not present\n",
					   __FUNCTION__,
					   ss_ar);
				return 0;
		    }

		    if ((ss_ar & AR_RESERVD_MASK)) {
					vcpu_printf(vcpu, "%s: SS AR 0x%x, reseved"
							      " bits are set\n",
						   __FUNCTION__,
						   ss_ar);
					return 0;
		    }

		    SEG_G_TEST(SS);

		}

		if (AR_DPL(ss_ar) !=
		    (vmcs_read16(GUEST_SS_SELECTOR) & SELECTOR_RPL_MASK) ) {
			vcpu_printf(vcpu, "%s: SS AR 0x%x, "
					      "DPL not as RPL\n",
				   __FUNCTION__,
				   ss_ar);
			return 0;
		}

		#define SEG_AR_TEST(seg) {\
		u32 ar = vmcs_read32(GUEST_##seg##_AR_BYTES);\
		if (!(ar & AR_UNUSABLE_MASK)) {\
			if (!(ar & AR_TYPE_ACCESSES_MASK)) {\
				vcpu_printf(vcpu, "%s: "#seg" AR 0x%x, "\
						"usable and not accesses\n",\
					   __FUNCTION__,\
					   ar);\
				return 0;\
			}\
			if ((ar & AR_TYPE_CODE_MASK) &&\
			    !(ar & AR_TYPE_READABLE_MASK)) {\
				vcpu_printf(vcpu, "%s: "#seg" AR 0x%x, "\
						"code and not readable\n",\
					   __FUNCTION__,\
					   ar);\
				return 0;\
			}\
			if (!(ar & AR_S_MASK)) {\
				vcpu_printf(vcpu, "%s: "#seg" AR 0x%x, usable and"\
					      " is sys\n",\
					   __FUNCTION__,\
					   ar);\
				return 0;\
			}\
			if ((ar & AR_TYPE_MASK) >= 0 && \
			    (ar & AR_TYPE_MASK) < 12 && \
			    AR_DPL(ar) < (vmcs_read16(GUEST_##seg##_SELECTOR) & \
					  SELECTOR_RPL_MASK) ) {\
				    vcpu_printf(vcpu, "%s: "#seg" AR 0x%x, "\
					      "DPL less than RPL\n",\
					       __FUNCTION__,\
					       ar);\
				    return 0;\
			}\
			if (!(ar & AR_P_MASK)) {\
				vcpu_printf(vcpu, "%s: "#seg" AR 0x%x, usable and"\
					      " not present\n",\
					   __FUNCTION__,\
					   ar);\
				return 0;\
			}\
			if ((ar & AR_RESERVD_MASK)) {\
					vcpu_printf(vcpu, "%s: "#seg" AR"\
							" 0x%x, reseved"\
							" bits are set\n",\
						   __FUNCTION__,\
						   ar);\
					return 0;\
			}\
			SEG_G_TEST(seg)\
		}\
		}

#undef DS
#undef ES
#undef FS
#undef GS

		SEG_AR_TEST(DS);
		SEG_AR_TEST(ES);
		SEG_AR_TEST(FS);
		SEG_AR_TEST(GS);

		// TR test
		if (long_mode) {
			if ((tr_ar & AR_TYPE_MASK) != AR_TYPE_BUSY_64_TSS) {
				vcpu_printf(vcpu, "%s: TR AR 0x%x, long"
						      " mode and not 64bit busy"
						      " tss\n",
				   __FUNCTION__,
				   tr_ar);
				return 0;
			}
		} else {
			if ((tr_ar & AR_TYPE_MASK) != AR_TYPE_BUSY_32_TSS &&
			    (tr_ar & AR_TYPE_MASK) != AR_TYPE_BUSY_16_TSS) {
				vcpu_printf(vcpu, "%s: TR AR 0x%x, legacy"
						      " mode and not 16/32bit "
						      "busy tss\n",
				   __FUNCTION__,
				   tr_ar);
				return 0;
			}

		}
		if ((tr_ar & AR_S_MASK)) {
			vcpu_printf(vcpu, "%s: TR AR 0x%x, S is set\n",
				   __FUNCTION__,
				   tr_ar);
			return 0;
		}
		if (!(tr_ar & AR_P_MASK)) {
			vcpu_printf(vcpu, "%s: TR AR 0x%x, P is not set\n",
				   __FUNCTION__,
				   tr_ar);
			return 0;
		}

		if ((tr_ar & (AR_RESERVD_MASK| AR_UNUSABLE_MASK))) {
			vcpu_printf(vcpu, "%s: TR AR 0x%x, reserved bit are"
					      " set\n",
				   __FUNCTION__,
				   tr_ar);
			return 0;
		}
		SEG_G_TEST(TR);

		// TR test
		if (!(ldtr_ar & AR_UNUSABLE_MASK)) {

			if ((ldtr_ar & AR_TYPE_MASK) != AR_TYPE_LDT) {
				vcpu_printf(vcpu, "%s: LDTR AR 0x%x,"
						      " bad type\n",
					   __FUNCTION__,
					   ldtr_ar);
			    return 0;
			}

			if ((ldtr_ar & AR_S_MASK)) {
				vcpu_printf(vcpu, "%s: LDTR AR 0x%x,"
						      " S is set\n",
					   __FUNCTION__,
					   ldtr_ar);
				return 0;
			}

			if (!(ldtr_ar & AR_P_MASK)) {
				vcpu_printf(vcpu, "%s: LDTR AR 0x%x,"
						      " P is not set\n",
					   __FUNCTION__,
					   ldtr_ar);
				return 0;
			}
			if ((ldtr_ar & AR_RESERVD_MASK)) {
				vcpu_printf(vcpu, "%s: LDTR AR 0x%x,"
						      " reserved bit are  set\n",
					   __FUNCTION__,
					   ldtr_ar);
				return 0;
			}
			SEG_G_TEST(LDTR);
		}
	}

	// GDTR and IDTR


	#define IDT_GDT_TEST(reg)\
	if (!is_canonical(vmcs_readl(GUEST_##reg##_BASE))) {\
		vcpu_printf(vcpu, "%s: "#reg" BASE 0x%lx, not canonical\n",\
					   __FUNCTION__,\
					   vmcs_readl(GUEST_##reg##_BASE));\
		return 0;\
	}\
	if (vmcs_read32(GUEST_##reg##_LIMIT) >> 16) {\
		vcpu_printf(vcpu, "%s: "#reg" LIMIT 0x%x, size err\n",\
				   __FUNCTION__,\
				   vmcs_read32(GUEST_##reg##_LIMIT));\
		return 0;\
	}\

	IDT_GDT_TEST(GDTR);
	IDT_GDT_TEST(IDTR);


	// RIP

	if ((!long_mode || !(vmcs_read32(GUEST_CS_AR_BYTES) & AR_L_MASK)) &&
	    vmcs_readl(GUEST_RIP) & ~((1ULL << 32) - 1) ){
		vcpu_printf(vcpu, "%s: RIP 0x%lx, size err\n",
				   __FUNCTION__,
				   vmcs_readl(GUEST_RIP));
		return 0;
	}

	if (!is_canonical(vmcs_readl(GUEST_RIP))) {
		vcpu_printf(vcpu, "%s: RIP 0x%lx, not canonical\n",
				   __FUNCTION__,
				   vmcs_readl(GUEST_RIP));
		return 0;
	}

	// RFLAGS
	#define RFLAGS_RESEVED_CLEAR_BITS\
		(~((1ULL << 22) - 1) | (1ULL << 15) | (1ULL << 5) | (1ULL << 3))
	#define RFLAGS_RESEVED_SET_BITS (1 << 1)

	if ((rflags & RFLAGS_RESEVED_CLEAR_BITS) ||
	    !(rflags & RFLAGS_RESEVED_SET_BITS)) {
		vcpu_printf(vcpu, "%s: RFLAGS 0x%lx, reserved bits 0x%llx 0x%x\n",
			   __FUNCTION__,
			   rflags,
			   RFLAGS_RESEVED_CLEAR_BITS,
			   RFLAGS_RESEVED_SET_BITS);
		return 0;
	}

	if (long_mode && virtual8086) {
		vcpu_printf(vcpu, "%s: RFLAGS 0x%lx, vm and long mode\n",
				   __FUNCTION__,
				   rflags);
		return 0;
	}


	if (!(rflags & RFLAGS_RF)) {
		u32 vm_entry_info = vmcs_read32(VM_ENTRY_INTR_INFO_FIELD);
		if ((vm_entry_info & INTR_INFO_VALID_MASK) &&
		    (vm_entry_info & INTR_INFO_INTR_TYPE_MASK) ==
		    INTR_TYPE_EXT_INTR) {
			vcpu_printf(vcpu, "%s: RFLAGS 0x%lx, external"
					      " interrupt and RF is clear\n",
				   __FUNCTION__,
				   rflags);
			return 0;
		}

	}

	// to be continued from Checks on Guest Non-Register State (22.3.1.5)
	return 1;
}

static int check_fixed_bits(struct kvm_vcpu *vcpu, const char *reg,
			    unsigned long cr,
			    u32 msr_fixed_0, u32 msr_fixed_1)
{
	u64 fixed_bits_0, fixed_bits_1;

	rdmsrl(msr_fixed_0, fixed_bits_0);
	rdmsrl(msr_fixed_1, fixed_bits_1);
	if ((cr & fixed_bits_0) != fixed_bits_0) {
		vcpu_printf(vcpu, "%s: %s (%lx) has one of %llx unset\n",
			   __FUNCTION__, reg, cr, fixed_bits_0);
		return 0;
	}
	if ((~cr & ~fixed_bits_1) != ~fixed_bits_1) {
		vcpu_printf(vcpu, "%s: %s (%lx) has one of %llx set\n",
			   __FUNCTION__, reg, cr, ~fixed_bits_1);
		return 0;
	}
	return 1;
}

static int phys_addr_width(void)
{
	unsigned eax, ebx, ecx, edx;

	cpuid(0x80000008, &eax, &ebx, &ecx, &edx);
	return eax & 0xff;
}

static int check_canonical(struct kvm_vcpu *vcpu, const char *name,
			   unsigned long reg)
{
#ifdef __x86_64__
	unsigned long x;

	if (sizeof(reg) == 4)
		return 1;
	x = (long)reg >> 48;
	if (!(x == 0 || x == ~0UL)) {
		vcpu_printf(vcpu, "%s: %s (%lx) not canonical\n",
			    __FUNCTION__, name, reg);
		return 0;
	}
#endif
	return 1;
}

static int check_selector(struct kvm_vcpu *vcpu, const char *name,
			  int rpl_ti, int null,
			  u16 sel)
{
	if (rpl_ti && (sel & 7)) {
		vcpu_printf(vcpu, "%s: %s (%x) nonzero rpl or ti\n",
			    __FUNCTION__, name, sel);
		return 0;
	}
	if (null && !sel) {
		vcpu_printf(vcpu, "%s: %s (%x) zero\n",
			    __FUNCTION__, name, sel);
		return 0;
	}
	return 1;
}

#define MSR_IA32_VMX_CR0_FIXED0 0x486
#define MSR_IA32_VMX_CR0_FIXED1 0x487

#define MSR_IA32_VMX_CR4_FIXED0 0x488
#define MSR_IA32_VMX_CR4_FIXED1 0x489

int vm_entry_test_host(struct kvm_vcpu *vcpu)
{
	int r = 0;
	unsigned long cr0 = vmcs_readl(HOST_CR0);
	unsigned long cr4 = vmcs_readl(HOST_CR4);
	unsigned long cr3 = vmcs_readl(HOST_CR3);
	int host_64;

	host_64 = vmcs_read32(VM_EXIT_CONTROLS) & VM_EXIT_HOST_ADD_SPACE_SIZE;

	/* 22.2.2 */
	r &= check_fixed_bits(vcpu, "host cr0", cr0, MSR_IA32_VMX_CR0_FIXED0,
			      MSR_IA32_VMX_CR0_FIXED1);

	r &= check_fixed_bits(vcpu, "host cr0", cr4, MSR_IA32_VMX_CR4_FIXED0,
			      MSR_IA32_VMX_CR4_FIXED1);
	if ((u64)cr3 >> phys_addr_width()) {
		vcpu_printf(vcpu, "%s: cr3 (%lx) vs phys addr width\n",
			    __FUNCTION__, cr3);
		r = 0;
	}

	r &= check_canonical(vcpu, "host ia32_sysenter_eip",
			     vmcs_readl(HOST_IA32_SYSENTER_EIP));
	r &= check_canonical(vcpu, "host ia32_sysenter_esp",
			     vmcs_readl(HOST_IA32_SYSENTER_ESP));

	/* 22.2.3 */
	r &= check_selector(vcpu, "host cs", 1, 1,
			    vmcs_read16(HOST_CS_SELECTOR));
	r &= check_selector(vcpu, "host ss", 1, !host_64,
			    vmcs_read16(HOST_SS_SELECTOR));
	r &= check_selector(vcpu, "host ds", 1, 0,
			    vmcs_read16(HOST_DS_SELECTOR));
	r &= check_selector(vcpu, "host es", 1, 0,
			    vmcs_read16(HOST_ES_SELECTOR));
	r &= check_selector(vcpu, "host fs", 1, 0,
			    vmcs_read16(HOST_FS_SELECTOR));
	r &= check_selector(vcpu, "host gs", 1, 0,
			    vmcs_read16(HOST_GS_SELECTOR));
	r &= check_selector(vcpu, "host tr", 1, 1,
			    vmcs_read16(HOST_TR_SELECTOR));

#ifdef __x86_64__
	r &= check_canonical(vcpu, "host fs base",
			     vmcs_readl(HOST_FS_BASE));
	r &= check_canonical(vcpu, "host gs base",
			     vmcs_readl(HOST_GS_BASE));
	r &= check_canonical(vcpu, "host gdtr base",
			     vmcs_readl(HOST_GDTR_BASE));
	r &= check_canonical(vcpu, "host idtr base",
			     vmcs_readl(HOST_IDTR_BASE));
#endif

	/* 22.2.4 */
#ifdef __x86_64__
	if (!host_64) {
		vcpu_printf(vcpu, "%s: vm exit controls: !64 bit host\n",
			    __FUNCTION__);
		r = 0;
	}
	if (!(cr4 & CR4_PAE_MASK)) {
		vcpu_printf(vcpu, "%s: cr4 (%lx): !pae\n",
			    __FUNCTION__, cr4);
		r = 0;
	}
	r &= check_canonical(vcpu, "host rip", vmcs_readl(HOST_RIP));
#endif

	return r;
}

int vm_entry_test(struct kvm_vcpu *vcpu)
{
	int rg, rh;

	rg = vm_entry_test_guest(vcpu);
	rh = vm_entry_test_host(vcpu);
	return rg && rh;
}

void vmcs_dump(struct kvm_vcpu *vcpu)
{
	vcpu_printf(vcpu, "************************ vmcs_dump ************************\n");
	vcpu_printf(vcpu, "VM_ENTRY_CONTROLS 0x%x\n", vmcs_read32(VM_ENTRY_CONTROLS));

	vcpu_printf(vcpu, "GUEST_CR0 0x%lx\n", vmcs_readl(GUEST_CR0));
	vcpu_printf(vcpu, "GUEST_CR3 0x%lx\n", vmcs_readl(GUEST_CR3));
	vcpu_printf(vcpu, "GUEST_CR4 0x%lx\n", vmcs_readl(GUEST_CR4));

	vcpu_printf(vcpu, "GUEST_SYSENTER_ESP 0x%lx\n", vmcs_readl(GUEST_SYSENTER_ESP));
	vcpu_printf(vcpu, "GUEST_SYSENTER_EIP 0x%lx\n", vmcs_readl(GUEST_SYSENTER_EIP));


	vcpu_printf(vcpu, "GUEST_IA32_DEBUGCTL 0x%llx\n", vmcs_read64(GUEST_IA32_DEBUGCTL));
	vcpu_printf(vcpu, "GUEST_DR7 0x%lx\n", vmcs_readl(GUEST_DR7));

	vcpu_printf(vcpu, "GUEST_RFLAGS 0x%lx\n", vmcs_readl(GUEST_RFLAGS));
	vcpu_printf(vcpu, "GUEST_RIP 0x%lx\n", vmcs_readl(GUEST_RIP));

	vcpu_printf(vcpu, "GUEST_CS_SELECTOR 0x%x\n", vmcs_read16(GUEST_CS_SELECTOR));
	vcpu_printf(vcpu, "GUEST_DS_SELECTOR 0x%x\n", vmcs_read16(GUEST_DS_SELECTOR));
	vcpu_printf(vcpu, "GUEST_ES_SELECTOR 0x%x\n", vmcs_read16(GUEST_ES_SELECTOR));
	vcpu_printf(vcpu, "GUEST_FS_SELECTOR 0x%x\n", vmcs_read16(GUEST_FS_SELECTOR));
	vcpu_printf(vcpu, "GUEST_GS_SELECTOR 0x%x\n", vmcs_read16(GUEST_GS_SELECTOR));
	vcpu_printf(vcpu, "GUEST_SS_SELECTOR 0x%x\n", vmcs_read16(GUEST_SS_SELECTOR));

	vcpu_printf(vcpu, "GUEST_TR_SELECTOR 0x%x\n", vmcs_read16(GUEST_TR_SELECTOR));
	vcpu_printf(vcpu, "GUEST_LDTR_SELECTOR 0x%x\n", vmcs_read16(GUEST_LDTR_SELECTOR));

	vcpu_printf(vcpu, "GUEST_CS_AR_BYTES 0x%x\n", vmcs_read32(GUEST_CS_AR_BYTES));
	vcpu_printf(vcpu, "GUEST_DS_AR_BYTES 0x%x\n", vmcs_read32(GUEST_DS_AR_BYTES));
	vcpu_printf(vcpu, "GUEST_ES_AR_BYTES 0x%x\n", vmcs_read32(GUEST_ES_AR_BYTES));
	vcpu_printf(vcpu, "GUEST_FS_AR_BYTES 0x%x\n", vmcs_read32(GUEST_FS_AR_BYTES));
	vcpu_printf(vcpu, "GUEST_GS_AR_BYTES 0x%x\n", vmcs_read32(GUEST_GS_AR_BYTES));
	vcpu_printf(vcpu, "GUEST_SS_AR_BYTES 0x%x\n", vmcs_read32(GUEST_SS_AR_BYTES));

	vcpu_printf(vcpu, "GUEST_LDTR_AR_BYTES 0x%x\n", vmcs_read32(GUEST_LDTR_AR_BYTES));
	vcpu_printf(vcpu, "GUEST_TR_AR_BYTES 0x%x\n", vmcs_read32(GUEST_TR_AR_BYTES));

	vcpu_printf(vcpu, "GUEST_CS_BASE 0x%lx\n", vmcs_readl(GUEST_CS_BASE));
	vcpu_printf(vcpu, "GUEST_DS_BASE 0x%lx\n", vmcs_readl(GUEST_DS_BASE));
	vcpu_printf(vcpu, "GUEST_ES_BASE 0x%lx\n", vmcs_readl(GUEST_ES_BASE));
	vcpu_printf(vcpu, "GUEST_FS_BASE 0x%lx\n", vmcs_readl(GUEST_FS_BASE));
	vcpu_printf(vcpu, "GUEST_GS_BASE 0x%lx\n", vmcs_readl(GUEST_GS_BASE));
	vcpu_printf(vcpu, "GUEST_SS_BASE 0x%lx\n", vmcs_readl(GUEST_SS_BASE));


	vcpu_printf(vcpu, "GUEST_LDTR_BASE 0x%lx\n", vmcs_readl(GUEST_LDTR_BASE));
	vcpu_printf(vcpu, "GUEST_TR_BASE 0x%lx\n", vmcs_readl(GUEST_TR_BASE));

	vcpu_printf(vcpu, "GUEST_CS_LIMIT 0x%x\n", vmcs_read32(GUEST_CS_LIMIT));
	vcpu_printf(vcpu, "GUEST_DS_LIMIT 0x%x\n", vmcs_read32(GUEST_DS_LIMIT));
	vcpu_printf(vcpu, "GUEST_ES_LIMIT 0x%x\n", vmcs_read32(GUEST_ES_LIMIT));
	vcpu_printf(vcpu, "GUEST_FS_LIMIT 0x%x\n", vmcs_read32(GUEST_FS_LIMIT));
	vcpu_printf(vcpu, "GUEST_GS_LIMIT 0x%x\n", vmcs_read32(GUEST_GS_LIMIT));
	vcpu_printf(vcpu, "GUEST_SS_LIMIT 0x%x\n", vmcs_read32(GUEST_SS_LIMIT));

	vcpu_printf(vcpu, "GUEST_LDTR_LIMIT 0x%x\n", vmcs_read32(GUEST_LDTR_LIMIT));
	vcpu_printf(vcpu, "GUEST_TR_LIMIT 0x%x\n", vmcs_read32(GUEST_TR_LIMIT));

	vcpu_printf(vcpu, "GUEST_GDTR_BASE 0x%lx\n", vmcs_readl(GUEST_GDTR_BASE));
	vcpu_printf(vcpu, "GUEST_IDTR_BASE 0x%lx\n", vmcs_readl(GUEST_IDTR_BASE));

	vcpu_printf(vcpu, "GUEST_GDTR_LIMIT 0x%x\n", vmcs_read32(GUEST_GDTR_LIMIT));
	vcpu_printf(vcpu, "GUEST_IDTR_LIMIT 0x%x\n", vmcs_read32(GUEST_IDTR_LIMIT));
	vcpu_printf(vcpu, "***********************************************************\n");
}

void regs_dump(struct kvm_vcpu *vcpu)
{
	#define REG_DUMP(reg) \
		vcpu_printf(vcpu, #reg" = 0x%lx\n", vcpu->regs[VCPU_REGS_##reg])
	#define VMCS_REG_DUMP(reg) \
		vcpu_printf(vcpu, #reg" = 0x%lx\n", vmcs_readl(GUEST_##reg))

	vcpu_printf(vcpu, "************************ regs_dump ************************\n");
	REG_DUMP(RAX);
	REG_DUMP(RBX);
	REG_DUMP(RCX);
	REG_DUMP(RDX);
	REG_DUMP(RSP);
	REG_DUMP(RBP);
	REG_DUMP(RSI);
	REG_DUMP(RDI);
	REG_DUMP(R8);
	REG_DUMP(R9);
	REG_DUMP(R10);
	REG_DUMP(R11);
	REG_DUMP(R12);
	REG_DUMP(R13);
	REG_DUMP(R14);
	REG_DUMP(R15);

	VMCS_REG_DUMP(RSP);
	VMCS_REG_DUMP(RIP);
	VMCS_REG_DUMP(RFLAGS);

	vcpu_printf(vcpu, "***********************************************************\n");
}

void sregs_dump(struct kvm_vcpu *vcpu)
{
	vcpu_printf(vcpu, "************************ sregs_dump ************************\n");
	vcpu_printf(vcpu, "cr0 = 0x%lx\n", guest_cr0());
	vcpu_printf(vcpu, "cr2 = 0x%lx\n", vcpu->cr2);
	vcpu_printf(vcpu, "cr3 = 0x%lx\n", vcpu->cr3);
	vcpu_printf(vcpu, "cr4 = 0x%lx\n", guest_cr4());
	vcpu_printf(vcpu, "cr8 = 0x%lx\n", vcpu->cr8);
	vcpu_printf(vcpu, "shadow_efer = 0x%llx\n", vcpu->shadow_efer);
	vmcs_dump(vcpu);
	vcpu_printf(vcpu, "***********************************************************\n");
}

#endif

