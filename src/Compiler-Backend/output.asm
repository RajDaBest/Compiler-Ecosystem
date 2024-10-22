section .bss
stack: resq 1024
section .data

section .text
global _start

; VASM Library Functions are currently statically linked

print_number:
    mov rax, [r15]
    add r15, 8

    cmp rax, 0
    jnl is_non_negative

    neg rax
    mov r12, 0 ; 0 for negative
    jmp intermediate

is_non_negative:
    mov r12, 1 ; 1 for non-negative

intermediate:
    dec rsp
    mov byte [rsp], 10
    mov r14, 1 ; counter for number of characters
    mov r13, 10 ; divisor for decimal
div_loop:
    xor edx, edx ; zero rdx before using the division instruction
    div r13
    add dl, 48 ; convert numeric digit to ASCII equivalent
    dec rsp
    mov byte [rsp], dl
    inc r14

    test rax, rax ; test if rax is zero
    jnz div_loop

    test r12, r12 ; check if the number was negative
    jz handle_negative
    jmp write

handle_negative:
    dec rsp
    mov byte [rsp], 45 ; add '-' for negative number
    inc r14

write:
    mov rax, 1
    mov rdi, 1
    mov rsi, rsp
    mov rdx, r14
    syscall

    add rsp, r14 ; restore the stack pointer
    ret
hello:
    sub r15, 8
    mov QWORD [r15], 10

    sub r15, 8
    mov QWORD [r15], 20

    mov rax, [r15]
    add r15, 8
    add [r15], rax

    ret
_start:
    mov r15, stack + 8192
    sub r15, 8
    mov QWORD [r15], -9

    sub r15, 8
    mov QWORD [r15], 10

    mov rax, [stack + 8184 - 8]
    mov rbx, [r15]
    mov [stack + 8184 - 8], rbx
    mov [r15], rax
    call hello
    mov rax, qword [r15 + 8]
    sub r15, 8
    vucomisd xmm0, [r15]
    mov [r15], rax
    mov rax, qword [r15 + 16]
    sub r15, 8
    mov [r15], rax
    mov rax, qword [stack + 8184 - 0]
    sub r15, 8
    mov [r15], rax
    call print_number

    mov rax, 60
    mov rdi, [r15]
    syscall
