.include "o/libc/nt/codegen.inc"
.imp	kernel32,__imp_TlsFree,TlsFree,0

	.text.windows
__TlsFree:
	push	%rbp
	mov	%rsp,%rbp
	.profilable
	mov	%rdi,%rcx
	sub	$32,%rsp
	call	*__imp_TlsFree(%rip)
	leave
	ret
	.endfn	__TlsFree,globl
	.previous
