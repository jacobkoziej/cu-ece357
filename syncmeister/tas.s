.text

.global	tas
.type	tas, @function
tas:
	pushq		%rbp
	movq		%rsp, %rbp
	movq		$1,   %rax
	lock;xchgb	%al,  (%rdi)
	movsbq		%al,  %rax
	pop 		%rbp
	ret

.section .note.GNU-stack, "", %progbits
