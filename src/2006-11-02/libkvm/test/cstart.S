

.bss

	. = . + 4096
	.align 16
stacktop:

.section .init
	lea stacktop, %rsp
	call main
1:	hlt
	jmp 1b
	

