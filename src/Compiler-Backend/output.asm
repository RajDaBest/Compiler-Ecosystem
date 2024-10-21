section .bss
stack: resq 1024
section .data
print_u64_buffer: db 20 dup(0), 10

section .text
global _start

; VASM Library Functions are currently statically linked

print_u64:

    mov rax, [r15]
    add r15, 8
    mov r14, print_u64_buffer + 19

    mov r13, 10div_loop:
    xor edx,   edx ; zero rdx before using the division instruction
    div r13
    add dl,   48 ; 48 is the ASCII for '0', i added it to convert the numeric digit to it's ASCII equivalent
    mov byte [r14], dl
    sub r14,   1

    test rax, rax ; tests if rax is zero
    jnz  div_loop

    mov rax, 1
    mov rdi, 1
    mov rsi, print_u64_buffer
    mov rdx, 21
    syscall

    ret

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
    sub r15, 8
    mov QWORD [r15], 10
    mov rax, [r15]
    add r15, 8
    add [r15], rax
    call print_u64
    mov rax, 60
    mov rdi, [r15]
    syscall
