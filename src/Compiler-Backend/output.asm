section .data

    hello: db 'hello, world', 10, 0

section .text

    global _start

_start:

    mov rax, 10
    push rax

    ;mov rax, 60
    ;mov rdi, 0
    ;syscall

    