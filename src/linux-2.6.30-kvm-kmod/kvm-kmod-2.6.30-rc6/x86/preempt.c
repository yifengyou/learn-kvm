
#ifdef CONFIG_PREEMPT_NOTIFIERS_COMPAT

#include <linux/sched.h>
#include <linux/percpu.h>

static DEFINE_SPINLOCK(pn_lock);
static LIST_HEAD(pn_list);

#define dprintk(fmt) do {						\
		if (0)							\
			printk("%s (%d/%d): " fmt, __FUNCTION__,	\
			       current->pid, raw_smp_processor_id());	\
	} while (0)

#if !defined(CONFIG_X86_64) && (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,25))
#define debugreg(x) debugreg[x]
#else
#define debugreg(x) debugreg##x
#endif

static void preempt_enable_sched_out_notifiers(void)
{
	asm volatile ("mov %0, %%db0" : : "r"(schedule));
	asm volatile ("mov %0, %%db7" : : "r"(0x701ul));
	current->thread.debugreg(7) = 0ul;
#ifdef TIF_DEBUG
	clear_tsk_thread_flag(current, TIF_DEBUG);
#endif
}

static void preempt_enable_sched_in_notifiers(void * addr)
{
	asm volatile ("mov %0, %%db0" : : "r"(addr));
	asm volatile ("mov %0, %%db7" : : "r"(0x701ul));
	current->thread.debugreg(0) = (unsigned long) addr;
	current->thread.debugreg(7) = 0x701ul;
#ifdef TIF_DEBUG
	set_tsk_thread_flag(current, TIF_DEBUG);
#endif
}

static void __preempt_disable_notifiers(void)
{
	asm volatile ("mov %0, %%db7" : : "r"(0ul));
}

static void preempt_disable_notifiers(void)
{
	__preempt_disable_notifiers();
	current->thread.debugreg(7) = 0ul;
#ifdef TIF_DEBUG
	clear_tsk_thread_flag(current, TIF_DEBUG);
#endif
}

static void fastcall  __attribute__((used)) preempt_notifier_trigger(void *** ip)
{
	struct preempt_notifier *pn;
	int cpu = raw_smp_processor_id();
	int found = 0;

	dprintk(" - in\n");
	//dump_stack();
	spin_lock(&pn_lock);
	list_for_each_entry(pn, &pn_list, link)
		if (pn->tsk == current) {
			found = 1;
			break;
		}
	spin_unlock(&pn_lock);

	if (found) {
		if ((void *) *ip != schedule) {
			dprintk("sched_in\n");
			preempt_enable_sched_out_notifiers();

			preempt_disable();
			local_irq_enable();
			pn->ops->sched_in(pn, cpu);
			local_irq_disable();
			preempt_enable_no_resched();
		} else {
			void * sched_in_addr;
			dprintk("sched_out\n");
#ifdef CONFIG_X86_64
			sched_in_addr = **(ip+3);
#else
			/* no special debug stack switch on x86 */
			sched_in_addr = (void *) *(ip+3);
#endif
			preempt_enable_sched_in_notifiers(sched_in_addr);

			preempt_disable();
			local_irq_enable();
			pn->ops->sched_out(pn, NULL);
			local_irq_disable();
			preempt_enable_no_resched();
		}
	} else
		__preempt_disable_notifiers();
	dprintk(" - out\n");
}

unsigned long orig_int1_handler;

#ifdef CONFIG_X86_64

#define SAVE_REGS \
	"push %rax; push %rbx; push %rcx; push %rdx; " \
	"push %rsi; push %rdi; push %rbp; " \
	"push %r8;  push %r9;  push %r10; push %r11; " \
	"push %r12; push %r13; push %r14; push %r15"

#define RESTORE_REGS \
	"pop %r15; pop %r14; pop %r13; pop %r12; " \
	"pop %r11; pop %r10; pop %r9;  pop %r8; " \
	"pop %rbp; pop %rdi; pop %rsi; " \
	"pop %rdx; pop %rcx; pop %rbx; pop %rax "

#define TMP "%rax"

#else

#define SAVE_REGS "pusha"
#define RESTORE_REGS "popa"
#define TMP "%eax"

#endif

asm ("pn_int1_handler:  \n\t"
     "push "  TMP " \n\t"
     "mov %db7, " TMP " \n\t"
     "cmp $0x701, " TMP " \n\t"
     "pop "  TMP " \n\t"
     "jnz .Lnotme \n\t"
     "push "  TMP " \n\t"
     "mov %db6, " TMP " \n\t"
     "test $0x1, " TMP " \n\t"
     "pop "  TMP " \n\t"
     "jz .Lnotme \n\t"
     SAVE_REGS "\n\t"
#ifdef CONFIG_X86_64
     "leaq 120(%rsp),%rdi\n\t"
#else
     "leal 32(%esp),%eax\n\t"
#endif
     "call preempt_notifier_trigger \n\t"
     RESTORE_REGS "\n\t"
#ifdef CONFIG_X86_64
     "orq $0x10000, 16(%rsp) \n\t"
     "iretq \n\t"
#else
     "orl $0x10000, 8(%esp) \n\t"
     "iret \n\t"
#endif
     ".Lnotme: \n\t"
#ifdef CONFIG_X86_64
     "jmpq *orig_int1_handler\n\t"
#else
     "jmpl *orig_int1_handler\n\t"
#endif
	);

void preempt_notifier_register(struct preempt_notifier *notifier)
{
	unsigned long flags;

	dprintk(" - in\n");
	spin_lock_irqsave(&pn_lock, flags);
	preempt_enable_sched_out_notifiers();
	notifier->tsk = current;
	list_add(&notifier->link, &pn_list);
	spin_unlock_irqrestore(&pn_lock, flags);
	dprintk(" - out\n");
}

void preempt_notifier_unregister(struct preempt_notifier *notifier)
{
	unsigned long flags;

	dprintk(" - in\n");
	spin_lock_irqsave(&pn_lock, flags);
	list_del(&notifier->link);
	spin_unlock_irqrestore(&pn_lock, flags);
	preempt_disable_notifiers();
	dprintk(" - out\n");
}

struct intr_gate {
	u16 offset0;
	u16 segment;
	u16 junk;
	u16 offset1;
#ifdef CONFIG_X86_64
	u32 offset2;
	u32 blah;
#endif
} __attribute__((packed));

struct idt_desc {
	u16 limit;
	struct intr_gate *gates;
} __attribute__((packed));

static struct intr_gate orig_int1_gate;

void pn_int1_handler(void);

void preempt_notifier_sys_init(void)
{
	struct idt_desc idt_desc;
	struct intr_gate *int1_gate;

	printk("kvm: emulating preempt notifiers;"
	       " do not benchmark on this machine\n");
	dprintk("\n");
	asm ("sidt %0" : "=m"(idt_desc));
	int1_gate = &idt_desc.gates[1];
	orig_int1_gate = *int1_gate;
	orig_int1_handler = int1_gate->offset0
		| ((u32)int1_gate->offset1 << 16);
#ifdef CONFIG_X86_64
	orig_int1_handler |= (u64)int1_gate->offset2 << 32;
#endif
	int1_gate->offset0 = (unsigned long)pn_int1_handler;
	int1_gate->offset1 = (unsigned long)pn_int1_handler >> 16;
#ifdef CONFIG_X86_64
	int1_gate->offset2 = (unsigned long)pn_int1_handler >> 32;
#endif
}

static void do_disable(void *blah)
{
#ifdef TIF_DEBUG
	if (!test_tsk_thread_flag(current, TIF_DEBUG))
#else
	if (!current->thread.debugreg(7))
#endif
		__preempt_disable_notifiers();
}

void preempt_notifier_sys_exit(void)
{
	struct idt_desc idt_desc;

	dprintk("\n");
	kvm_on_each_cpu(do_disable, NULL, 1);
	asm ("sidt %0" : "=m"(idt_desc));
	idt_desc.gates[1] = orig_int1_gate;
}

#endif
