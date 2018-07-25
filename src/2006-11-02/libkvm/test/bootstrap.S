#
# minimal bootstrap to set up flat 32-bit protected mode
#
	
bstart = 0xf0000
	
.code16

start:
	cs lidtl idt_desc
	cs lgdtl gdt_desc
	mov %cr0, %eax
	or $1, %eax
	mov %eax, %cr0
	ljmpl $8, $pmode + bstart
pmode:
.code32	
	mov $0x10, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	mov %ax, %ss
	mov $0xa0000, %esp
	ljmp $8, $0x100000

.align 16
	
idt_desc:
	.word 0
	.long 0

gdt_desc:
	.word gdt_end - gdt - 1
	.long gdt + bstart

.align 16

gdt:
	.quad 0
	.quad 0x00cf9b000000ffff # flat 32-bit code segment
	.quad 0x00cf93000000ffff # flat 32-bit data segment
gdt_end:
	
. = 0xfff0
	.code16
	ljmp $0xf000, $start
.align 65536
