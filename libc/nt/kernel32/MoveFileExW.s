.include "o/libc/nt/codegen.inc"
.imp	kernel32,__imp_MoveFileExW,MoveFileExW,0

	.text.windows
__MoveFileEx:
	push	%rbp
	mov	%rsp,%rbp
	.profilable
	mov	__imp_MoveFileExW(%rip),%rax
	jmp	__sysv2nt
	.endfn	__MoveFileEx,globl
	.previous
