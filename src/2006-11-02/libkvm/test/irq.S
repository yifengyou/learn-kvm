// irq test program.  assumes outb $irq, $0xff generates an interrupt $irq.

#include "print.h"
	
.text
	PRINT "irq test"
	mov $stack_top, %rsp

	call setup_gdt
	
	mov %ds, %ax
	mov %ax, %ds  // check ds descriptor is okay

	mov $irq_handler, %rdx
	mov $0x20, %eax
	call setup_idt_entry

	lidt idt_descriptor

	PRINT "software interrupt"
	int $0x20

	sti
	nop

	PRINT "injecting interrupt with interrupts enabled"
	
	mov $0x20, %al
	outb %al, $0xff // inject interrupt

	nop
	nop
	nop
	PRINT "after injection"

	cli

	PRINT "injecting interrupt with interrupts disabled"
	
	mov $0x20, %al
	outb %al, $0xff // inject interrupt

	// no interrupt here (disabled)
	nop
	nop
	PRINT "enabling interrupts"
	nop
	nop
	sti
	out %al, $0x80 // blocked by sti
	// interrupt here
	out %al, $0x80 

	PRINT "after injection"
	nop
	nop
	
	hlt
	
irq_handler:
	PRINT "interrupt handler"
	iretq
	
setup_idt_entry:	// %rax: irq %rdx: handler
	shl $4, %rax
	mov %dx, idt(%rax)
	shr $16, %rdx
	mov %cs, 2+idt(%rax)
	mov %dx, 6+idt(%rax)
	shr $16, %rdx
	mov %edx, 8+idt(%rax)
	movw $0x8e00, 4+idt(%rax)
	ret

setup_gdt:
	mov $0, %eax
	mov %cs, %ax
	andl $~7, %eax
	movl $0xffff, gdt(%rax)
	movl $0xaf9b00, 4+gdt(%rax)

	mov $0, %eax
	mov %ds, %ax
	andl $~7, %eax
	movl $0xffff, gdt(%rax)
	movl $0x8f9300, 4+gdt(%rax)

	lgdt gdt_descriptor
	ret
	
.data
	
.align 16

idt:	
	. = . + 256 * 16

idt_descriptor:
	.word . - idt - 1
	.quad idt

.align 8
	
gdt:
	. = . + 256 * 8

gdt_descriptor:
	.word . - gdt - 1
	.quad gdt 

	
.align 4096
stack_base:
	. = . + 4096
stack_top:
	


