section .bss
stack: resq 1024
section .data
stack_top: dq (stack + 8192)

section .text
global _start
_start:
mov rsi, [stack_top]
sub rsi, 8
mov QWORD [rsi], 10
mov [stack_top], rsi
mov rsi, [stack_top]
sub rsi, 8
mov QWORD [rsi], 18
mov [stack_top], rsi
mov rsi, [stack_top]
mov rax, [rsi]
add rsi, 8
add [rsi], rax
mov [stack_top], rsi
mov rsi, [stack_top]
sub rsi, 8
mov QWORD [rsi], 90
mov [stack_top], rsi
mov rbx, [stack_top]
mov rax, 60
mov rdi, [rbx]
syscall
