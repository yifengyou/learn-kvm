
.data

.macro str name, value

\name :	.long 1f-2f
2:	.ascii "\value"
1:	
.endm	

	str "forward", "forward"
	str "backward", "backward"
		
.text


	cld
	movl forward, %ecx
	lea  4+forward, %rsi
	movw $1, %dx
	rep outsb

	std
	movl backward, %ecx
	lea 4+backward-1(%rcx), %rsi
	movw $2, %dx
	rep outsb
	
	hlt


