

.bss

	. = . + 4096
	.align 16
stacktop:

.data
		
.align 4096
ptl2:
i = 0
	.rept 512
	.quad 0x1e3 | (i << 21)
	i = i + 1
	.endr

.align 4096
ptl3:
	.quad ptl2 + 3

.align 4096
ptl4:
	.quad ptl3 + 3
	
.align 4096

gdt64_desc:
	.word gdt64_end - gdt64 - 1
	.quad gdt64

gdt64:
	.quad 0
	.quad 0x00af9b000000ffff # 64-bit code segment
gdt64_end:
	
.section .init

.code32
	lgdt gdt64_desc
	
	mov %cr4, %eax
	bts $5, %eax  # pae
	mov %eax, %cr4

	mov $ptl4, %eax
	mov %eax, %cr3

efer = 0xc0000080
	mov $efer, %ecx
	rdmsr
	bts $8, %eax
	wrmsr

	mov %cr0, %eax
	bts $0, %eax
	bts $31, %eax
	mov %eax, %cr0
	
	jmpl $8, $start64

.code64
start64:	
	lea stacktop, %rsp
	call main
1:	hlt
	jmp 1b
	

