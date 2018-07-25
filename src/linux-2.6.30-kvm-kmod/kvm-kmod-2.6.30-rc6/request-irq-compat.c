/*
 * compat for request_irq
 */

#include <linux/interrupt.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)

static kvm_irq_handler_t kvm_irq_handlers[NR_IRQS];
static DEFINE_MUTEX(kvm_irq_handlers_mutex);

static irqreturn_t kvm_irq_thunk(int irq, void *dev_id, struct pt_regs *regs)
{
	kvm_irq_handler_t handler = kvm_irq_handlers[irq];
	return handler(irq, dev_id);
}

int kvm_request_irq(unsigned int a, kvm_irq_handler_t handler,
		    unsigned long c, const char *d, void *e)
{
	int rc = -EBUSY;
	kvm_irq_handler_t old;

	mutex_lock(&kvm_irq_handlers_mutex);
	old = kvm_irq_handlers[a];
	if (old)
		goto out;
	kvm_irq_handlers[a] = handler;
	rc = request_irq(a, kvm_irq_thunk, c, d, e);
	if (rc)
		kvm_irq_handlers[a] = NULL;
out:
	mutex_unlock(&kvm_irq_handlers_mutex);
	return rc;
}

void kvm_free_irq(unsigned int irq, void *dev_id)
{
	mutex_lock(&kvm_irq_handlers_mutex);
	free_irq(irq, dev_id);
	kvm_irq_handlers[irq] = NULL;
	mutex_unlock(&kvm_irq_handlers_mutex);
}

#endif
