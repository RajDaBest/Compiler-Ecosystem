section .data

buf: dq 0

section .text
    global _start

_start:

    mov rax, 50

print_u64:

    xor edx, edx


