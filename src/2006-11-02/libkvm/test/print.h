#ifndef PRINT_H
#define PRINT_H

.macro PRINT text

.data

333: .asciz "\text\n"

.previous

	push %rdi
	lea 333b, %rdi
	call print
	pop %rdi

.endm

#endif
