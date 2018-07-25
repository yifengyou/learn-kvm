
	.text

	mov $0, %al
	mov $10000, %ebx
1:
	mov %rbx, %rcx
2:	
	loop 2b
	out %al, $0x80
	inc %al
	add $10000, %rbx
	jmp 1b
