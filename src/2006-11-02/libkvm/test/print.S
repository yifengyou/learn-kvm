
#include "print.h"

#define PSEUDO_SERIAL_PORT 0xf1

	
.text
	PRINT "boo"
	hlt
1:	jmp 1b
	
.globl print
print:
	push %rax
	push %rsi
	push %rdx

	mov %rdi, %rsi
	mov $PSEUDO_SERIAL_PORT, %edx

putchar:
	cmpb $0, (%rsi)
	jz done
	outsb
	jmp putchar
done:
	
	pop %rdx
	pop %rsi
	pop %rax
	ret
