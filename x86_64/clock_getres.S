.text
.global clock_getres
#ifdef __PIE__
.hidden clock_getres
#endif
.type clock_getres,@function
clock_getres:
	movq cgt(%rip), %rax
	test %rax,%rax
	jz .L1
.L2:
	callq *%rax
	jmp __error_unified_syscall
.L1:
	push %rsi
	push %rdi
	movq __vdso(%rip),%rdi
#ifdef __PIC__
	leaq funcname(%rip), %rsi
#else
	movq $funcname, %rsi
#endif
	call vdso_dlsym
	pop %rdi
	pop %rsi
	jmp .L2

.Lhere:
	.size	 clock_getres,.Lhere-clock_getres
.local cgt
.comm cgt,8,8
.section .rodata.str1.1,"aMS",@progbits,1
.local funcname
funcname:
	.string "__vdso_clock_getres"
