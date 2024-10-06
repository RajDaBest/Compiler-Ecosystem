section .bss
stack: resq 1024
section .data

section .text
global _start
_start:
mov r15, stack + 8192
sub r15, 8
mov QWORD [r15], 10
sub r15, 8
mov QWORD [r15], 18
mov rax, [r15]
add r15, 8
add [r15], rax
sub r15, 8
mov QWORD [r15], 90
mov rax, 60
mov rdi, [r15]
syscall
