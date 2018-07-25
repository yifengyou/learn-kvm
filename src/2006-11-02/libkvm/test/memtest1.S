.text
	
start:
	mov    $0x1000,%r8
	mov    $0x0a,%ecx

init_page:
	dec    %ecx
	jne    no_io
	mov    $0x0,%al
	out    %al,$0x80
	mov    $0x0a,%ecx

no_io:
	mov    %r8,(%r8)
	add    $0x1000,%r8
	cmp    $0x8000000,%r8
	jne    init_page
	mov    $0x1000,%r8
	mov    $0x0a,%ecx

test_loop:
	dec    %ecx
	jne    no_io2
	mov    $0x0,%al
	out    %al,$0x80
	mov    $0x0a,%ecx

no_io2:
	mov    (%r8),%r9
	cmp    %r8,%r9
	jne    err
	add    $0x1000,%r8
	cmp    $0x8000000,%r8
	jne    test_loop
	mov    $0x1000,%r8
	jmp    test_loop

err:
	mov    $0xffffffffffffffff,%r12
	mov    $0xffffffffffffffff,%r13
	mov    $0x0,%al
	out    %al,$0x80
	jmp    err
