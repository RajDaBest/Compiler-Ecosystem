section .data
L0: dq 10.9
L1: dq 11.8

section .text
global _start
_start:
movsd xmm0, [L0]
sub rsp, 8
movsd [rsp], xmm0
movsd xmm0, [L1]
sub rsp, 8
movsd [rsp], xmm0
movsd xmm0, [rsp]
addsd xmm0, [rsp+8]
add rsp, 8
movsd [rsp], xmm0
mov rax, 60
cvttsd2si edi, [rsp]
syscall
