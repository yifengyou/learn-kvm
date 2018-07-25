
/*
 * smp_call_function_single() is not exported below 2.6.20.
 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)

#undef smp_call_function_single

#include <linux/spinlock.h>
#include <linux/smp.h>

struct scfs_thunk_info {
	int cpu;
	void (*func)(void *info);
	void *info;
};

static void scfs_thunk(void *_thunk)
{
	struct scfs_thunk_info *thunk = _thunk;

	if (raw_smp_processor_id() == thunk->cpu)
		thunk->func(thunk->info);
}

int kvm_smp_call_function_single(int cpu, void (*func)(void *info),
				 void *info, int wait)
{
	int r, this_cpu;
	struct scfs_thunk_info thunk;

	this_cpu = get_cpu();
	WARN_ON(irqs_disabled());
	if (cpu == this_cpu) {
		r = 0;
		local_irq_disable();
		func(info);
		local_irq_enable();
	} else {
		thunk.cpu = cpu;
		thunk.func = func;
		thunk.info = info;
		r = smp_call_function(scfs_thunk, &thunk, 0, 1);
	}
	put_cpu();
	return r;
}

#define smp_call_function_single kvm_smp_call_function_single

#elif LINUX_VERSION_CODE < KERNEL_VERSION(2,6,23)
/*
 * pre 2.6.23 doesn't handle smp_call_function_single on current cpu
 */

#undef smp_call_function_single

#include <linux/smp.h>

int kvm_smp_call_function_single(int cpu, void (*func)(void *info),
				 void *info, int wait)
{
	int this_cpu, r;

	this_cpu = get_cpu();
	WARN_ON(irqs_disabled());
	if (cpu == this_cpu) {
		r = 0;
		local_irq_disable();
		func(info);
		local_irq_enable();
	} else
		r = smp_call_function_single(cpu, func, info, 0, wait);
	put_cpu();
	return r;
}

#define smp_call_function_single kvm_smp_call_function_single

#elif LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)

/* The 'nonatomic' argument was removed in 2.6.27. */

#undef smp_call_function_single

#include <linux/smp.h>

#ifdef CONFIG_SMP
int kvm_smp_call_function_single(int cpu, void (*func)(void *info),
				 void *info, int wait)
{
	return smp_call_function_single(cpu, func, info, 0, wait);
}
#else /* !CONFIG_SMP */
int kvm_smp_call_function_single(int cpu, void (*func)(void *info),
				 void *info, int wait)
{
	WARN_ON(cpu != 0);
	local_irq_disable();
	func(info);
	local_irq_enable();
	return 0;

}
#endif /* !CONFIG_SMP */

#define smp_call_function_single kvm_smp_call_function_single

#endif

/* div64_u64 is fairly new */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)

#ifndef CONFIG_64BIT

/* 64bit divisor, dividend and result. dynamic precision */
uint64_t div64_u64(uint64_t dividend, uint64_t divisor)
{
	uint32_t high, d;

	high = divisor >> 32;
	if (high) {
		unsigned int shift = fls(high);

		d = divisor >> shift;
		dividend >>= shift;
	} else
		d = divisor;

	do_div(dividend, d);

	return dividend;
}

#endif

#endif

/*
 * smp_call_function_mask() is not defined/exported below 2.6.24 on all
 * targets and below 2.6.26 on x86-64
 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24) || \
    (defined CONFIG_X86_64 && LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26))

#include <linux/smp.h>

struct kvm_call_data_struct {
	void (*func) (void *info);
	void *info;
	atomic_t started;
	atomic_t finished;
	int wait;
};

static void kvm_ack_smp_call(void *_data)
{
	struct kvm_call_data_struct *data = _data;
	/* if wait == 0, data can be out of scope
	 * after atomic_inc(info->started)
	 */
	void (*func) (void *info) = data->func;
	void *info = data->info;
	int wait = data->wait;

	smp_mb();
	atomic_inc(&data->started);
	(*func)(info);
	if (wait) {
		smp_mb();
		atomic_inc(&data->finished);
	}
}

int kvm_smp_call_function_mask(cpumask_t mask,
			       void (*func) (void *info), void *info, int wait)
{
#ifdef CONFIG_SMP
	struct kvm_call_data_struct data;
	cpumask_t allbutself;
	int cpus;
	int cpu;
	int me;

	me = get_cpu();
	WARN_ON(irqs_disabled());
	allbutself = cpu_online_map;
	cpu_clear(me, allbutself);

	cpus_and(mask, mask, allbutself);
	cpus = cpus_weight(mask);

	if (!cpus)
		goto out;

	data.func = func;
	data.info = info;
	atomic_set(&data.started, 0);
	data.wait = wait;
	if (wait)
		atomic_set(&data.finished, 0);

	for (cpu = first_cpu(mask); cpu != NR_CPUS; cpu = next_cpu(cpu, mask))
		smp_call_function_single(cpu, kvm_ack_smp_call, &data, 0);

	while (atomic_read(&data.started) != cpus) {
		cpu_relax();
		barrier();
	}

	if (!wait)
		goto out;

	while (atomic_read(&data.finished) != cpus) {
		cpu_relax();
		barrier();
	}
out:
	put_cpu();
#endif /* CONFIG_SMP */
	return 0;
}

#include <linux/workqueue.h>

static void vcpu_kick_intr(void *info)
{
}

struct kvm_kick {
	int cpu;
	struct work_struct work;
};

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)
static void kvm_do_smp_call_function(void *data)
{
	int me;
	struct kvm_kick *kvm_kick = data;
#else
static void kvm_do_smp_call_function(struct work_struct *work)
{
	int me;
	struct kvm_kick *kvm_kick = container_of(work, struct kvm_kick, work);
#endif
	me = get_cpu();

	if (kvm_kick->cpu != me)
		smp_call_function_single(kvm_kick->cpu, vcpu_kick_intr,
					 NULL, 0);
	kfree(kvm_kick);
	put_cpu();
}

void kvm_queue_smp_call_function(int cpu)
{
	struct kvm_kick *kvm_kick = kmalloc(sizeof(struct kvm_kick), GFP_ATOMIC);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)
	INIT_WORK(&kvm_kick->work, kvm_do_smp_call_function, kvm_kick);
#else
	INIT_WORK(&kvm_kick->work, kvm_do_smp_call_function);
#endif

	schedule_work(&kvm_kick->work);
}

void kvm_smp_send_reschedule(int cpu)
{
	if (irqs_disabled()) {
		kvm_queue_smp_call_function(cpu);
		return;
	}
	smp_call_function_single(cpu, vcpu_kick_intr, NULL, 0);
}
#endif

/* manually export hrtimer_init/start/cancel */
void (*hrtimer_init_p)(struct hrtimer *timer, clockid_t which_clock,
		       enum hrtimer_mode mode);
int (*hrtimer_start_p)(struct hrtimer *timer, ktime_t tim,
		       const enum hrtimer_mode mode);
int (*hrtimer_cancel_p)(struct hrtimer *timer);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,22)

static void kvm_set_normalized_timespec(struct timespec *ts, time_t sec,
					long nsec)
{
        while (nsec >= NSEC_PER_SEC) {
                nsec -= NSEC_PER_SEC;
                ++sec;
        }
        while (nsec < 0) {
                nsec += NSEC_PER_SEC;
                --sec;
        }
        ts->tv_sec = sec;
        ts->tv_nsec = nsec;
}

struct timespec kvm_ns_to_timespec(const s64 nsec)
{
        struct timespec ts;

        if (!nsec)
                return (struct timespec) {0, 0};

        ts.tv_sec = div_long_long_rem_signed(nsec, NSEC_PER_SEC, &ts.tv_nsec);
        if (unlikely(nsec < 0))
                kvm_set_normalized_timespec(&ts, ts.tv_sec, ts.tv_nsec);

        return ts;
}

#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)

#include <linux/pci.h>

struct pci_dev *pci_get_bus_and_slot(unsigned int bus, unsigned int devfn)
{
	struct pci_dev *dev = NULL;

	while ((dev = pci_get_device(PCI_ANY_ID, PCI_ANY_ID, dev)) != NULL) {
		if (pci_domain_nr(dev->bus) == 0 &&
		    (dev->bus->number == bus && dev->devfn == devfn))
			return dev;
	}
	return NULL;
}

#endif

#include <linux/intel-iommu.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28)

int intel_iommu_found()
{
	return 0;
}

#endif


#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,21)

/* relay_open() interface has changed on 2.6.21 */

struct rchan *kvm_relay_open(const char *base_filename,
			 struct dentry *parent,
			 size_t subbuf_size,
			 size_t n_subbufs,
			 struct rchan_callbacks *cb,
			 void *private_data)
{
	struct rchan *chan = relay_open(base_filename, parent,
					subbuf_size, n_subbufs,
					cb);
	if (chan)
		chan->private_data = private_data;
	return chan;
}

#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,18)

#include <linux/pci.h>

int kvm_pcidev_msi_enabled(struct pci_dev *dev)
{
	int pos;
	u16 control;

	if (!(pos = pci_find_capability(dev, PCI_CAP_ID_MSI)))
		return 0;

	pci_read_config_word(dev, pos + PCI_MSI_FLAGS, &control);
	if (control & PCI_MSI_FLAGS_ENABLE)
		return 1;

	return 0;
}

#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,23)

extern unsigned tsc_khz;
static unsigned tsc_khz_dummy = 2000000;
static unsigned *tsc_khz_p;

unsigned kvm_get_tsc_khz(void)
{
	if (!tsc_khz_p) {
		tsc_khz_p = symbol_get(tsc_khz);
		if (!tsc_khz_p)
			tsc_khz_p = &tsc_khz_dummy;
	}
	return *tsc_khz_p;
}

#endif
