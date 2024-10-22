#define _VM_IMPLEMENTATION
#define _SV_IMPLEMENATION
#include "../non_nanboxed/virt_mach.h"
#include "../non_nanboxed/String_View.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>

#define alloc 0
#define free_vm 1
#define print_f64 2
#define print_s64 3
#define print_u64 4
#define dump_static 5
#define print_string 6
#define read 7
#define write 8

// the swap instruction basically converts the top of the VM stack into an implicit register; we can bring any value in the stack to the top of the
// stack (the implicit register) using swap and work on it and put it right back into it's original place using swap again.

// the thing is, i did not put any fucking checks on instruction operands that help access some VM stack memory to detect stack overflow/underflows and all
// like i did when a .vasm file converts to VM bytecode and runs on the VM; this prolly doesn't matter since this is how actual assembly is

#define TYPE_INVALID ((uint8_t)10)
#define ERROR_BUFFER_SIZE 256

#define PROG_TEXT(inst) fprintf(ctx->program_file, "    "##inst "\n")
#define PROG_DATA(inst) fprintf(ctx->data_file, "   "##inst "\n")

typedef struct label_hashnode_
{
    String_View label;
    struct label_hashnode_ *next;
} label_hashnode;

typedef struct
{
    String_View label;
    size_t line_no;
} unresolved_label_;

typedef struct
{
    FILE *program_file;
    FILE *data_file;
    size_t code_section_offset;
    size_t data_section_offset;
    bool is_code;
    bool is_data;
    bool compilation_successful;
    char error_buffer[ERROR_BUFFER_SIZE];
    size_t line_no;
    size_t l_num;
    unresolved_label_ *unresolved_labels;
    size_t unresolved_labels_counter;
} CompilerContext;

label_hashnode *label_table[MAX_HASHTABLE_SIZE] = {NULL};

// function prototypes
bool init_compiler_context(CompilerContext *ctx, const char *output_file);
void cleanup_compiler_context(CompilerContext *ctx);
label_hashnode *search_in_label_table(String_View label);
bool push_to_label_table(String_View label, CompilerContext *ctx);
uint8_t check_operand_type(String_View *operand);
bool handle_instruction(CompilerContext *ctx, size_t inst_number, String_View *operand);
bool handle_code_line(CompilerContext *ctx, String_View *line);
bool handle_data_line(CompilerContext *ctx, String_View *line);
bool process_source_file(CompilerContext *ctx, const char *input_file);

// initialize compiler context
bool init_compiler_context(CompilerContext *ctx, const char *output_file)
{
    ctx->program_file = fopen("temp.asm", "w+");
    if (!ctx->program_file)
    {
        snprintf(ctx->error_buffer, ERROR_BUFFER_SIZE, "Failed to open temporary program file: %s", strerror(errno));
        return false;
    }

    ctx->data_file = fopen(output_file, "w+");
    if (!ctx->data_file)
    {
        snprintf(ctx->error_buffer, ERROR_BUFFER_SIZE, "Failed to open output file: %s", strerror(errno));
        fclose(ctx->program_file);
        return false;
    }

    fprintf(ctx->data_file, "section .bss\nstack: resq %zu\nsection .data\n", vm_stack_capacity);

    fprintf(ctx->program_file, "section .text\nglobal _start\n\n");
    fprintf(ctx->program_file, "; VASM Library Functions are currently statically linked\n\n");

    // VASM Library functions are linked statically, i.e, they are implemented (resolved) directly into the assembly file

    fprintf(ctx->program_file, "print_number:\n"
                               "    mov rax, [r15]\n"
                               "    add r15, 8\n\n"
                               "    cmp rax, 0\n"
                               "    jnl is_non_negative\n\n"
                               "    neg rax\n"
                               "    mov r12, 0 ; 0 for negative\n"
                               "    jmp intermediate\n\n"
                               "is_non_negative:\n"
                               "    mov r12, 1 ; 1 for non-negative\n\n"
                               "intermediate:\n"
                               "    dec rsp\n"
                               "    mov byte [rsp], 10\n"
                               "    mov r14, 1 ; counter for number of characters\n"
                               "    mov r13, 10 ; divisor for decimal\n"
                               "div_loop:\n"
                               "    xor edx, edx ; zero rdx before using the division instruction\n"
                               "    div r13\n"
                               "    add dl, 48 ; convert numeric digit to ASCII equivalent\n"
                               "    dec rsp\n"
                               "    mov byte [rsp], dl\n"
                               "    inc r14\n\n"
                               "    test rax, rax ; test if rax is zero\n"
                               "    jnz div_loop\n\n"
                               "    test r12, r12 ; check if the number was negative\n"
                               "    jz handle_negative\n"
                               "    jmp write\n\n"
                               "handle_negative:\n"
                               "    dec rsp\n"
                               "    mov byte [rsp], 45 ; add '-' for negative number\n"
                               "    inc r14\n\n"
                               "write:\n"
                               "    mov rax, 1\n"
                               "    mov rdi, 1\n"
                               "    mov rsi, rsp\n"
                               "    mov rdx, r14\n"
                               "    syscall\n\n"
                               "    add rsp, r14 ; restore the stack pointer\n"
                               "    ret\n");

    // the call instruction places the return address on the stack itself, so the called function must ensure that the stack it uses is cleaned up before it returns using ret

    ctx->code_section_offset = 0;
    ctx->data_section_offset = 0;
    ctx->is_code = true;
    ctx->is_data = false;
    ctx->compilation_successful = true;
    ctx->line_no = 0;
    ctx->l_num = 0;
    ctx->unresolved_labels_counter = 0;

    ctx->unresolved_labels = malloc(sizeof(unresolved_label_) * VM_LABEL_CAPACITY);
    if (!ctx->unresolved_labels)
    {
        snprintf(ctx->error_buffer, ERROR_BUFFER_SIZE, "Failed to allocate memory for unresolved labels");
        fclose(ctx->program_file);
        fclose(ctx->data_file);
        return false;
    }

    return true;
}

void cleanup_compiler_context(CompilerContext *ctx)
{
    if (ctx->program_file)
        fclose(ctx->program_file);
    if (ctx->data_file)
        fclose(ctx->data_file);
    free(ctx->unresolved_labels);
}

label_hashnode *search_in_label_table(String_View label)
{
    uint32_t key = hash_sv(label) % MAX_HASHTABLE_SIZE;
    label_hashnode *current_node = label_table[key];

    while (current_node != NULL)
    {
        if (sv_eq(current_node->label, label))
        {
            return current_node;
        }
        current_node = current_node->next;
    }

    return NULL;
}

bool push_to_label_table(String_View label, CompilerContext *ctx)
{
    if (search_in_label_table(label))
    {
        snprintf(ctx->error_buffer, ERROR_BUFFER_SIZE,
                 "Line Number %zu -> ERROR: Label '%.*s' redefinition",
                 ctx->line_no, (int)label.count, label.data);
        ctx->compilation_successful = false;
        return false;
    }

    uint32_t key = hash_sv(label) % MAX_HASHTABLE_SIZE;
    label_hashnode *node = malloc(sizeof(label_hashnode));
    if (!node)
    {
        snprintf(ctx->error_buffer, ERROR_BUFFER_SIZE,
                 "ERROR: Memory allocation failed for label '%.*s'",
                 (int)label.count, label.data);
        return false;
    }

    node->label = label;
    node->next = label_table[key];
    label_table[key] = node;
    return true;
}

uint8_t check_operand_type(String_View *operand)
{
    bool point_found = false;
    bool is_negative = false;

    for (size_t i = 0; i < operand->count; i++)
    {
        char data = operand->data[i];

        if (i == 0 && (data == '-' || data == '+'))
        {
            is_negative = (data == '-');
            continue;
        }

        if (data == '.' && !point_found)
        {
            point_found = true;
            continue;
        }

        if (data < '0' || data > '9')
        {
            return TYPE_INVALID;
        }
    }

    if (point_found)
        return TYPE_DOUBLE;
    if (is_negative)
        return TYPE_SIGNED_64INT;
    return TYPE_UNSIGNED_64INT;
}

bool handle_instruction(CompilerContext *ctx, size_t inst_number, String_View *operand)
{
    switch (inst_number)
    {
    case INST_UPUSH:
    case INST_SPUSH:
        fprintf(ctx->program_file, "    sub r15, 8\n" // r15 is my personal stack pointer
                                   "    mov QWORD [r15], %.*s\n\n",
                (int)operand->count,
                operand->data);
        break;
    case INST_FPUSH:
        fprintf(ctx->data_file, "L%zu: dq %.*s\n", ctx->l_num,
                (int)operand->count, operand->data);
        fprintf(ctx->program_file, "    sub r15, 8\n"
                                   "    movsd xmm0, [L%zu]\n"
                                   "    movsd [r15], xmm0\n\n",
                ctx->l_num);
        ctx->l_num++;
        break;
    case INST_HALT:
        fprintf(ctx->program_file, "    mov rax, 60\n"
                                   "    mov rdi, [r15]\n" // the current program exit code is the value at the top of the VM stack
                                   "    syscall\n");
        break;
    case INST_SPLUS:
    case INST_UPLUS:
        fprintf(ctx->program_file, "    mov rax, [r15]\n"
                                   "    add r15, 8\n"
                                   "    add [r15], rax\n\n"); // stack is the pointer to the stack; stack_top is the pointer to the address of stack top
        break;
    case INST_FPLUS:
        fprintf(ctx->program_file, "    movsd xmm0, [r15]\n"
                                   "    add r15, 8\n"
                                   "    vaddsd xmm0, [r15]\n"
                                   "    vmovsd [r15], xmm0\n\n");
        break;
    case INST_NATIVE:
    {
        uint8_t lib_function_no = sv_to_unsigned64(operand);
        switch (lib_function_no)
        {
        case alloc:
            fprintf(ctx->program_file, "    call alloc\n\n");
            break;
        case free_vm:
            fprintf(ctx->program_file, "    call free\n\n");
            break;
        case print_f64:
            fprintf(ctx->program_file, "    call print_f64\n\n");
            break;
        case print_s64:
            fprintf(ctx->program_file, "    call print_number\n\n");
            break;
        case print_u64:
            fprintf(ctx->program_file, "    call print_number\n\n");
            break;
        case dump_static:
            fprintf(ctx->program_file, "    call dump_static\n\n");
            break;
        case print_string:
            fprintf(ctx->program_file, "    call print_string\n\n");
            break;
        case read:
            fprintf(ctx->program_file, "    call read\n\n");
            break;
        case write:
            fprintf(ctx->program_file, "    call write\n\n");
            break;
        }
        break;
    }

    case INST_RSWAP:
    {
        uint64_t operand_conv = sv_to_unsigned64(operand);
        fprintf(ctx->program_file, "    mov rax, qword [r15 + %llu]\n"
                                   "    mov rbx, [r15]\n"
                                   "    mov [r15 + %llu], rbx\n"
                                   "    mov [r15], rax\n\n",
                (operand_conv) * 8, (operand_conv) * 8);
        break;
    }

    case INST_ASWAP:
    {
        uint64_t operand_conv = sv_to_unsigned64(operand);
        fprintf(ctx->program_file, "    mov rax, [stack + 8184 - %llu]\n"
                                   "    mov rbx, [r15]\n"
                                   "    mov [stack + 8184 - %llu], rbx\n"
                                   "    mov [r15], rax\n\n",
                (operand_conv) * 8, (operand_conv) * 8);
        break;
    }

    case INST_RDUP:
    {
        uint64_t operand_conv = sv_to_unsigned64(operand);
        fprintf(ctx->program_file, "    mov rax, qword [r15 + %llu]\n"
                                   "    sub r15, 8\n"
                                   "    mov [r15], rax\n\n",
                (operand_conv) * 8);
        break;
    }

    case INST_ADUP:
    {
        uint64_t operand_conv = sv_to_unsigned64(operand);
        fprintf(ctx->program_file, "    mov rax, qword [stack + 8184 - %llu]\n"
                                   "    sub r15, 8\n"
                                   "    mov [r15], rax\n\n",
                (operand_conv) * 8);
        break;
    }

    case INST_JMP:
    {
        fprintf(ctx->program_file, "    jmp %.*s\n\n", (int)operand->count, operand->data);
        break;
    }

    case INST_CALL:
    {
        fprintf(ctx->program_file, "    call %.*s\n\n", (int)operand->count, operand->data);
        break;
    }

    case INST_RET:
    {
        fprintf(ctx->program_file, "    ret\n\n");
        break;
    }

    case INST_EQU:
    case INST_EQS:
    {
        fprintf(ctx->program_file, "    mov rax, [r15]\n"
                                   "    add r15, 8\n"
                                   "    cmp rax, [r15]"
                                   "    setz byte [r15]\n\n");
        break;
    }

    case INST_GEU:
    {
        fprintf(ctx->program_file, "    mov rax, [r15]\n"
                                   "    add r15, 8\n"
                                   "    cmp rax, [r15]\n"
                                   "    setae byte [r15]\n\n");
        break;
    }

    case INST_GES:
    {
        fprintf(ctx->program_file, "    mov rax, [r15]\n"
                                   "    add r15, 8\n"
                                   "    cmp rax, [r15]\n"
                                   "    setge byte [r15]\n\n");
        break;
    }

    case INST_GU:
    {
        fprintf(ctx->program_file, "    mov rax, [r15]\n"
                                   "    add r15, 8\n"
                                   "    cmp rax, [r15]\n"
                                   "    seta byte [r15]\n\n");
        break;
    }

    case INST_GS:
    {
        fprintf(ctx->program_file, "    mov rax, [r15]\n"
                                   "    add r15, 8\n"
                                   "    cmp rax, [r15]\n"
                                   "    setg byte [r15]\n\n");
        break;
    }

    case INST_LEU:
    {
        fprintf(ctx->program_file, "    mov rax, [r15]\n"
                                   "    add r15, 8\n"
                                   "    cmp rax, [r15]\n"
                                   "    setbe byte [r15]\n\n");
        break;
    }

    case INST_LES:
    {
        fprintf(ctx->program_file, "    mov rax, [r15]\n"
                                   "    add r15, 8\n"
                                   "    cmp rax, [r15]\n"
                                   "    setle byte [r15]\n\n");
        break;
    }

    case INST_LU:
    {
        fprintf(ctx->program_file, "    mov rax, [r15]\n"
                                   "    add r15, 8\n"
                                   "    cmp rax, [r15]\n"
                                   "    setb byte [r15]\n\n");
        break;
    }

    case INST_LS:
    {
        fprintf(ctx->program_file, "    mov rax, [r15]\n"
                                   "    add r15, 8\n"
                                   "    cmp rax, [r15]\n"
                                   "    setl byte [r15]\n\n");
        break;
    }

    case INST_EQF:
    {
        fprintf(ctx->program_file, "    movsd xmm0, [r15]\n"
                                   "    add r15, 8\n"
                                   "    vucomisd xmm0, [r15]\n"
                                   "    setz byte [r15]\n");
        break;
    }

    case INST_GEF:
    {
        fprintf(ctx->program_file, "    movsd xmm0, [r15]\n"
                                   "    add r15, 8\n"
                                   "    vucomisd xmm0, [r15]\n"
                                   "    setae byte [r15]\n");
        break;
    }

    case INST_LEF:
    {
        fprintf(ctx->program_file, "    movsd xmm0, [r15]\n"
                                   "    add r15, 8\n"
                                   "    vucomisd xmm0, [r15]\n"
                                   "    setbe byte [r15]\n");
        break;
    }

    case INST_GF:
    {
        fprintf(ctx->program_file, "    movsd xmm0, [r15]\n"
                                   "    add r15, 8\n"
                                   "    vucomisd xmm0, [r15]\n"
                                   "    seta byte [r15]\n");
        break;
    }

    case INST_LF:
    {
        fprintf(ctx->program_file, "    movsd xmm0, [r15]\n"
                                   "    add r15, 8\n"
                                   "    vucomisd xmm0, [r15]\n"
                                   "    setb byte [r15]\n");
        break;
    }

    default:
        snprintf(ctx->error_buffer, ERROR_BUFFER_SIZE,
                 "Line Number %zu -> ERROR: Unknown instruction number %zu",
                 ctx->line_no, inst_number);
        return false;
    }
    return true;
}

bool handle_data_line(CompilerContext *ctx, String_View *line)
{
    String_View label = sv_chop_by_delim(line, ':');
    if (*(line->data - 1) == ':')
    { // if there's a label
        if (!push_to_label_table(label, ctx))
        {
            return false; // Error already set in push_to_label_table
        }
        fprintf(ctx->data_file, "%.*s: ", (int)label.count, label.data);
        sv_trim_left(line);
    }
    else
    {
        // Reset line if no label found
        line->count = label.count;
        line->data = label.data;
    }

    if (line->count == 0)
    {
        snprintf(ctx->error_buffer, ERROR_BUFFER_SIZE,
                 "Line Number %zu -> ERROR: Data directive expected after label",
                 ctx->line_no);
        return false;
    }

    String_View data_type = sv_chop_by_delim(line, ' ');
    sv_trim_left(line);

    if (line->count == 0)
    {
        snprintf(ctx->error_buffer, ERROR_BUFFER_SIZE,
                 "Line Number %zu -> ERROR: Data value expected after directive %.*s",
                 ctx->line_no, (int)data_type.count, data_type.data);
        return false;
    }

    if (sv_eq(data_type, cstr_as_sv(".byte")))
    {
        if (ctx->data_section_offset >= vm_default_memory_size - sizeof(int8_t))
        {
            snprintf(ctx->error_buffer, ERROR_BUFFER_SIZE,
                     "Line Number %zu -> ERROR: Not enough default static memory for .byte %.*s",
                     ctx->line_no, (int)line->count, line->data);
            return false;
        }
        fprintf(ctx->data_file, "db %.*s\n", (int)line->count, line->data);
        ctx->data_section_offset += sizeof(int8_t);
    }
    else if (sv_eq(data_type, cstr_as_sv(".word")))
    {
        if (ctx->data_section_offset >= vm_default_memory_size - sizeof(int16_t))
        {
            snprintf(ctx->error_buffer, ERROR_BUFFER_SIZE,
                     "Line Number %zu -> ERROR: Not enough default static memory for .word %.*s",
                     ctx->line_no, (int)line->count, line->data);
            return false;
        }
        fprintf(ctx->data_file, "dw %.*s\n", (int)line->count, line->data);
        ctx->data_section_offset += sizeof(int16_t);
    }
    else if (sv_eq(data_type, cstr_as_sv(".doubleword")))
    {
        if (ctx->data_section_offset >= vm_default_memory_size - sizeof(int32_t))
        {
            snprintf(ctx->error_buffer, ERROR_BUFFER_SIZE,
                     "Line Number %zu -> ERROR: Not enough default static memory for .doubleword %.*s",
                     ctx->line_no, (int)line->count, line->data);
            return false;
        }
        fprintf(ctx->data_file, "dd %.*s\n", (int)line->count, line->data);
        ctx->data_section_offset += sizeof(int32_t);
    }
    else if (sv_eq(data_type, cstr_as_sv(".quadword")))
    {
        if (ctx->data_section_offset >= vm_default_memory_size - sizeof(int64_t))
        {
            snprintf(ctx->error_buffer, ERROR_BUFFER_SIZE,
                     "Line Number %zu -> ERROR: Not enough default static memory for .quadword %.*s",
                     ctx->line_no, (int)line->count, line->data);
            return false;
        }
        fprintf(ctx->data_file, "dq %.*s\n", (int)line->count, line->data);
        ctx->data_section_offset += sizeof(int64_t);
    }
    else if (sv_eq(data_type, cstr_as_sv(".double")))
    {
        if (ctx->data_section_offset >= vm_default_memory_size - sizeof(double))
        {
            snprintf(ctx->error_buffer, ERROR_BUFFER_SIZE,
                     "Line Number %zu -> ERROR: Not enough default static memory for .double %.*s",
                     ctx->line_no, (int)line->count, line->data);
            return false;
        }
        fprintf(ctx->data_file, "dq %.*s\n", (int)line->count, line->data);
        ctx->data_section_offset += sizeof(double);
    }
    else if (sv_eq(data_type, cstr_as_sv(".string")))
    {
        if (line->count >= 2 && line->data[0] == '"' && line->data[line->count - 1] == '"')
        {
            line->data++;
            line->count -= 2;
        }
        else
        {
            snprintf(ctx->error_buffer, ERROR_BUFFER_SIZE,
                     "Line Number %zu -> ERROR: %.*s not a valid vasm string",
                     ctx->line_no, (int)line->count, line->data);
            return false;
        }

        if (ctx->data_section_offset >= vm_default_memory_size - line->count - 1)
        {
            snprintf(ctx->error_buffer, ERROR_BUFFER_SIZE,
                     "Line Number %zu -> ERROR: Not enough default static memory for .string %.*s",
                     ctx->line_no, (int)line->count, line->data);
            return false;
        }

        fprintf(ctx->data_file, "db '%.*s'\n", (int)line->count, line->data);
        ctx->data_section_offset += line->count + 1; // +1 for null terminator
    }
    else
    {
        snprintf(ctx->error_buffer, ERROR_BUFFER_SIZE,
                 "Line Number %zu -> ERROR: invalid data type '%.*s'",
                 ctx->line_no, (int)data_type.count, data_type.data);
        return false;
    }

    return true;
}

bool handle_code_line(CompilerContext *ctx, String_View *line)
{
    if (ctx->code_section_offset >= vm_program_capacity)
    {
        snprintf(ctx->error_buffer, ERROR_BUFFER_SIZE, "Program Too Big");
        return false;
    }

    String_View inst_name = sv_chop_by_delim(line, ' ');
    sv_trim_left(line);
    bool has_operand_value = line->count > 0;

    for (size_t i = 1; i < (size_t)INST_COUNT; i++)
    {
        if (sv_eq(inst_name, cstr_as_sv(get_inst_name(i))))
        {
            if (has_operand_function(i) != has_operand_value)
            {
                snprintf(ctx->error_buffer, ERROR_BUFFER_SIZE,
                         "Line Number %zu -> ERROR: %s %s an operand",
                         ctx->line_no, get_inst_name(i),
                         has_operand_function(i) ? "requires" : "doesn't require");
                return false;
            }

            if (!has_operand_function(i))
            {
                return handle_instruction(ctx, i, &(String_View){0});
            }

            uint8_t operand_type = check_operand_type(line);

            switch (get_operand_type(i))
            {
            case TYPE_SIGNED_64INT:
            {
                if (operand_type == TYPE_UNSIGNED_64INT || operand_type == TYPE_SIGNED_64INT)
                {
                    if (!handle_instruction(ctx, i, line))
                        return false;
                }
                else if (operand_type == TYPE_DOUBLE)
                {
                    snprintf(ctx->error_buffer, ERROR_BUFFER_SIZE,
                             "Line Number %zu -> ERROR: illegal operand value for %s instruction: %.*s\n"
                             "Must be a signed integral value",
                             ctx->line_no, get_inst_name(i), (int)line->count, line->data);
                    return false;
                }
                else if (operand_type == TYPE_INVALID)
                {
                    if (ctx->unresolved_labels_counter >= VM_LABEL_CAPACITY)
                    {
                        snprintf(ctx->error_buffer, ERROR_BUFFER_SIZE,
                                 "Line Number %zu -> ERROR: Too many unresolved labels",
                                 ctx->line_no);
                        return false;
                    }
                    ctx->unresolved_labels[ctx->unresolved_labels_counter].label = *line;
                    ctx->unresolved_labels[ctx->unresolved_labels_counter].line_no = ctx->line_no;
                    ctx->unresolved_labels_counter++;
                }
                return true;
            }
            case TYPE_UNSIGNED_64INT:
            {
                if (operand_type == TYPE_UNSIGNED_64INT)
                {
                    if (!handle_instruction(ctx, i, line))
                        return false;
                }
                else if (operand_type == TYPE_DOUBLE || operand_type == TYPE_SIGNED_64INT)
                {
                    snprintf(ctx->error_buffer, ERROR_BUFFER_SIZE,
                             "Line Number %zu -> ERROR: illegal operand value for %s instruction: %.*s\n"
                             "Must be an unsigned integral value",
                             ctx->line_no, get_inst_name(i), (int)line->count, line->data);
                    return false;
                }
                else if (operand_type == TYPE_INVALID)
                {
                    if (ctx->unresolved_labels_counter >= VM_LABEL_CAPACITY)
                    {
                        snprintf(ctx->error_buffer, ERROR_BUFFER_SIZE,
                                 "Line Number %zu -> ERROR: Too many unresolved labels",
                                 ctx->line_no);
                        return false;
                    }
                    ctx->unresolved_labels[ctx->unresolved_labels_counter].label = *line;
                    ctx->unresolved_labels[ctx->unresolved_labels_counter].line_no = ctx->line_no;
                    ctx->unresolved_labels_counter++;

                    if (i == INST_CALL || i == INST_JMP || i == INST_FJMP_IF || i == INST_UJMP_IF)
                    {
                        if (!handle_instruction(ctx, i, line))
                            return false;
                    }
                }
                return true;
            }
            case TYPE_DOUBLE:
            {
                if (operand_type == TYPE_INVALID)
                {
                    if (ctx->unresolved_labels_counter >= VM_LABEL_CAPACITY)
                    {
                        snprintf(ctx->error_buffer, ERROR_BUFFER_SIZE,
                                 "Line Number %zu -> ERROR: Too many unresolved labels",
                                 ctx->line_no);
                        return false;
                    }
                    ctx->unresolved_labels[ctx->unresolved_labels_counter].label = *line;
                    ctx->unresolved_labels[ctx->unresolved_labels_counter].line_no = ctx->line_no;
                    ctx->unresolved_labels_counter++;
                }
                else
                {
                    if (!handle_instruction(ctx, i, line))
                        return false;
                }
                return true;
            }
            default:
                snprintf(ctx->error_buffer, ERROR_BUFFER_SIZE,
                         "Line Number %zu -> ERROR: Unknown operand type for instruction %s",
                         ctx->line_no, get_inst_name(i));
                return false;
            }

            return handle_instruction(ctx, i, line);
        }
    }

    snprintf(ctx->error_buffer, ERROR_BUFFER_SIZE,
             "Line Number %zu -> ERROR: Invalid instruction %.*s",
             ctx->line_no, (int)inst_name.count, inst_name.data);
    return false;
}

bool process_source_file(CompilerContext *ctx, const char *input_file)
{
    String_View source = slurp_file(input_file);
    if (!source.data)
    {
        snprintf(ctx->error_buffer, ERROR_BUFFER_SIZE, "Failed to read input file: %s", input_file);
        return false;
    }

    while (source.count > 0)
    {
        ctx->line_no++;
        String_View line = sv_chop_by_delim(&source, '\n');
        line = sv_chop_by_delim(&line, ';');
        sv_trim_left(&line);
        sv_trim_right(&line);

        if (line.count == 0)
            continue;

        String_View section = line;
        String_View directive = sv_chop_by_delim(&section, ' ');

        if (sv_eq(directive, cstr_as_sv(".text")))
        {
            ctx->is_code = true;
            ctx->is_data = false;
        }
        else if (sv_eq(directive, cstr_as_sv(".data")))
        {
            ctx->is_code = false;
            ctx->is_data = true;
        }
        else if (ctx->is_code)
        {
            String_View label = sv_chop_by_delim(&line, ':');
            if (*(line.data - 1) == ':')
            {
                if (!push_to_label_table(label, ctx))
                    return false;
                fprintf(ctx->program_file, "%.*s:\n", (int)label.count, label.data);
                sv_trim_left(&line);
                if (line.count > 0 && !handle_code_line(ctx, &line))
                    return false;
            }
            else
            {
                if (!handle_code_line(ctx, &label))
                    return false;
            }

            if (sv_eq(label, cstr_as_sv("_start")))
            {
                size_t offset = vm_stack_capacity * sizeof(uint64_t);
                fprintf(ctx->program_file, "    mov r15, stack + %zu\n", offset);
            }
            ctx->code_section_offset++;
        }
        else if (ctx->is_data)
        {
            if (!handle_data_line(ctx, &line))
                return false;
        }
    }

    // Check for unresolved labels
    for (size_t i = 0; i < ctx->unresolved_labels_counter; i++)
    {
        if (!search_in_label_table(ctx->unresolved_labels[i].label))
        {
            snprintf(ctx->error_buffer, ERROR_BUFFER_SIZE,
                     "Line No: %zu -> ERROR: Undefined label: '%.*s'",
                     ctx->unresolved_labels[i].line_no,
                     (int)ctx->unresolved_labels[i].label.count,
                     ctx->unresolved_labels[i].label.data);
            return false;
        }
    }

    return true;
}

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <input_file> <output_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    CompilerContext ctx = {0};
    if (!init_compiler_context(&ctx, argv[2]))
    {
        fprintf(stderr, "Initialization failed: %s\n", ctx.error_buffer);
        return EXIT_FAILURE;
    }

    if (!process_source_file(&ctx, argv[1]))
    {
        fprintf(stderr, "Compilation failed: %s\n", ctx.error_buffer);
        cleanup_compiler_context(&ctx);
        remove("temp.asm");
        remove(argv[2]);
        return EXIT_FAILURE;
    }

    // Merge program file into data file
    fseek(ctx.program_file, 0, SEEK_END);
    long program_size = ftell(ctx.program_file);
    fseek(ctx.program_file, 0, SEEK_SET);

    char *program_buffer = malloc(program_size);
    if (!program_buffer)
    {
        fprintf(stderr, "Failed to allocate memory for program buffer\n");
        cleanup_compiler_context(&ctx);
        return EXIT_FAILURE;
    }

    fread(program_buffer, 1, program_size, ctx.program_file);
    fprintf(ctx.data_file, "\n");
    fwrite(program_buffer, 1, program_size, ctx.data_file);

    free(program_buffer);
    cleanup_compiler_context(&ctx);
    remove("temp.asm");

    printf("Compilation successful!\n");
    return EXIT_SUCCESS;
}