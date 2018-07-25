#include "kvmctl.h"

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

kvm_context_t kvm;

static int test_cpuid(void *opaque, uint64_t *rax, uint64_t *rbx, 
		       uint64_t *rcx, uint64_t *rdx)
{
    printf("cpuid 0x%lx\n", (uint32_t)*rax);
    return 0;
}

static int test_inb(void *opaque, uint16_t addr, uint8_t *value)
{
    printf("inb 0x%x\n", addr);
    return 0;
}

static int test_inw(void *opaque, uint16_t addr, uint16_t *value)
{
    printf("inw 0x%x\n", addr);
    return 0;
}

static int test_inl(void *opaque, uint16_t addr, uint32_t *value)
{
    printf("inl 0x%x\n", addr);
    return 0;
}

static int test_outb(void *opaque, uint16_t addr, uint8_t value)
{
    static int newline = 1;

    switch (addr) {
    case 0xff: // irq injector
	printf("injecting interrupt 0x%x\n", value);
	kvm_inject_irq(kvm, 0, value);
	break;
    case 0xf1: // serial
	if (newline)
	    fputs("GUEST: ", stdout);
	putchar(value);
	newline = value == '\n';
	break;
    default:
	printf("outb $0x%x, 0x%x\n", value, addr);
    }
    return 0;
}

static int test_outw(void *opaque, uint16_t addr, uint16_t value)
{
    printf("outw $0x%x, 0x%x\n", value, addr);
    return 0;
}

static int test_outl(void *opaque, uint16_t addr, uint32_t value)
{
    printf("outl $0x%x, 0x%x\n", value, addr);
    return 0;
}

static int test_debug(void *opaque, int vcpu)
{
    printf("test_debug\n");
    return 0;
}

static int test_halt(void *opaque, int vcpu)
{
    printf("test_halt\n");
    return 0;
}

static int test_io_window(void *opaque)
{
    printf("test_io_window\n");
    return 0;
}

static struct kvm_callbacks test_callbacks = {
    .cpuid       = test_cpuid,
    .inb         = test_inb,
    .inw         = test_inw,
    .inl         = test_inl,
    .outb        = test_outb,
    .outw        = test_outw,
    .outl        = test_outl,
    .debug       = test_debug,
    .halt        = test_halt,
    .io_window = test_io_window,
};
 

static void load_file(void *mem, const char *fname)
{
    int r;
    int fd;

    fd = open(fname, O_RDONLY);
    if (fd == -1) {
	perror("open");
	exit(1);
    }
    while ((r = read(fd, mem, 4096)) != -1 && r != 0)
	mem += r;
    if (r == -1) {
	perror("read");
	exit(1);
    }
}

int main(int ac, char **av)
{
	void *vm_mem;

	kvm = kvm_init(&test_callbacks, 0);
	kvm_create(kvm, 128 * 1024 * 1024, &vm_mem);
	if (ac > 1)
	    load_file(vm_mem + 0xf0000, av[1]);
	if (ac > 2)
	    load_file(vm_mem + 0x100000, av[2]);
	kvm_show_regs(kvm, 0);

	kvm_run(kvm, 0);
}
