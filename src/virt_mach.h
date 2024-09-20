#ifndef _VM
#define _VM
#define _SV_IMPLEMENTATION

#include "String_View.h"
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>

#define VM_STACK_CAPACITY 1024
#define VM_PROGRAM_CAPACITY 1024
#define VM_LABEL_CAPACITY 128
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define MAKE_INST_PUSH(value) {.type = INST_PUSH, .operand = (value)}
#define MAKE_INST_DUP(rel_addr) {.type = INST_DUP, .operand = (rel_addr)}
#define MAKE_INST_PLUS { \
    .type = INST_PLUS,   \
}
#define MAKE_INST_MINUS { \
    .type = INST_MINUS,   \
}
#define MAKE_INST_MULT { \
    .type = INST_MULT,   \
}
#define MAKE_INST_DIV {.type = INST_DIV}
#define MAKE_INST_JMP(addr) {.type = INST_JMP, .operand = (addr)}
#define MAKE_INST_HALT {.type = INST_HALT}
#define MAKE_INST_JMP_IF(addr) {.type = INST_JMP_IF, .operand = (addr)}
#define MAKE_INST_EQ {.type = INST_EQ}
#define MAKE_INST_NOP {.type = INST_NOP}

size_t vm_stack_capacity = VM_STACK_CAPACITY;
size_t vm_program_capacity = VM_PROGRAM_CAPACITY;
size_t label_capacity = VM_LABEL_CAPACITY;

typedef struct
{
    String_View label;
    bool resolved;
    size_t inst_location;
} label_inst_location;

typedef struct
{
    String_View label;
    size_t label_pointing_location;
} label_point_location;

size_t label_array_counter = 0;
size_t not_resolved_yet_counter = 0;

label_point_location *label_array;
label_inst_location *not_resolved_yet;

typedef enum
{
    TRAP_OK = 0,
    TRAP_STACK_OVERFLOW,
    TRAP_STACK_UNDERFLOW,
    TRAP_ILLEGAL_INSTRUCTION,
    TRAP_DIV_BY_ZERO,
    TRAP_ILLEGAL_JMP,
    TRAP_ILLEGAL_INST_ACCESS,
    TRAP_NO_HALT_FOUND,
    TRAP_ILLEGAL_OPERAND,
} Trap; // exceptions that stop the execution on the virtual machine

typedef enum
{
    INST_NOP = 0, // does nothing but increment the instruction pointer; if the program array is just zero, it will all be no-ops;
    INST_PUSH,    // push a word to the stack top; we assume that our stack grows downwards
    INST_DUP,     // duplicates the element at the position stack_top - addr at the top of the stack; stack_top = stack_size - 1
    INST_PLUS,    // add the last element on stack onto the second last element, and remove the last element from the stack
    INST_MINUS,   // subtract the last element on the stack from the second last element, and remove the last element from the stack
    INST_MULT,    // multiply the last element on the stack to the second last element, and remove the last element from the stack
    INST_DIV,     // integer divide the second last element on the stack by the last element and store the result in the second last element, and then remove the last element from the stack
    INST_JMP,     // unconditional jump
    INST_HALT,    // halt the machine
    INST_JMP_IF,  // jump to an address if the last element on the stack is non-zero; do not jump otherwise
    INST_EQ,      // checks if the second last stack element is equal to the last stack element; sets the second last element to one if true, and 0 otherwise; removes the last element from the stack
} Inst_Type;      // enum for the instruction types

typedef struct
{
    Inst_Type type; // type of the instruction
    word operand;   // operand of the instruction
} Inst;             // structure for the actual instruction

typedef struct // structure defining the actual virtual machine
{
    word *stack;              // the stack of the virtual machine; the stack top is the end of the array
    size_t stack_size;        // current stack size
    word instruction_pointer; // the address of the next instruction to be executed
    int halt;
    Inst *program;       // the actual instruction array
    size_t program_size; // number of instructions in the program
} VirtualMachine;

const char *trap_as_cstr(Trap trap);
const char *inst_type_as_asm_str(Inst_Type type);
const char *inst_type_as_cstr(Inst_Type type);
void vm_dump_stack(FILE *stream, const VirtualMachine *vm);
int vm_execute_at_inst_pointer(VirtualMachine *vm); // executes the instruction inst on vm
int vm_load_program_from_memory(VirtualMachine *vm, Inst *program, size_t program_size);
void vm_init(VirtualMachine *vm);
int vm_exec_program(VirtualMachine *vm, __int64_t limit);
void vm_push_inst(VirtualMachine *vm, Inst *inst);
void vm_save_program_to_file(Inst *program, size_t program_size, const char *file_path);
size_t vm_load_program_from_file(Inst *program, const char *file_path);
Inst vm_translate_line(String_View line, size_t current_program_counter);
size_t vm_translate_source(String_View source, Inst *program, size_t program_capacity);
String_View slurp_file(const char *file_path);

#ifdef _VM_IMPLEMENTATION

const char *trap_as_cstr(Trap trap)
{
    switch (trap)
    {
    case TRAP_OK:
        return "TRAP_OK";
    case TRAP_ILLEGAL_INSTRUCTION:
        return "TRAP_ILLEGAL_INSTRUCTION";
    case TRAP_STACK_OVERFLOW:
        return "TRAP_STACK_OVERFLOW";
    case TRAP_STACK_UNDERFLOW:
        return "TRAP_STACK_UNDERFLOW";
    case TRAP_DIV_BY_ZERO:
        return "TRAP_DIV_BY_ZERO";
    case TRAP_ILLEGAL_JMP:
        return "TRAP_ILLEGAL_JMP";
    case TRAP_ILLEGAL_INST_ACCESS:
        return "TRAP_ILLEGAL_INST_ACCESS";
    case TRAP_NO_HALT_FOUND:
        return "TRAP_NO_HALT_FOUND";
    case TRAP_ILLEGAL_OPERAND:
        return "TRAP_ILLEGAL_OPERAND";
    default:
        assert(00 && "trap_as_cstr: Unreachnable");
    }
}

const char *inst_type_as_cstr(Inst_Type type)
{
    switch (type)
    {
    case INST_PUSH:
        return "INST_PUSH";
    case INST_NOP:
        return "INST_NOP";
    case INST_PLUS:
        return "INST_PLUS";
    case INST_MULT:
        return "INST_MULT";
    case INST_DIV:
        return "INST_DIV";
    case INST_MINUS:
        return "INST_MINUS";
    case INST_HALT:
        return "INST_HALT";
    case INST_JMP:
        return "INST_JMP";
    case INST_JMP_IF:
        return "INST_JMP_IF";
    case INST_EQ:
        return "INST_EQU";
    case INST_DUP:
        return "INST_DUP";
    default:
        assert(0 && "inst_type_as_cstr: unreachable");
        break;
    }
}

const char *inst_type_as_asm_str(Inst_Type type)
{
    switch (type)
    {
    case INST_PUSH:
        return "push";
    case INST_NOP:
        return "nop";
    case INST_PLUS:
        return "plus";
    case INST_MULT:
        return "mult";
    case INST_DIV:
        return "div";
    case INST_MINUS:
        return "minus";
    case INST_HALT:
        return "halt";
    case INST_JMP:
        return "jmp";
    case INST_JMP_IF:
        return "jmp_if";
    case INST_EQ:
        return "equ";
    case INST_DUP:
        return "dup";
    default:
        assert(0 && "inst_type_as_asm_str: unreachable");
        break;
    }
}

void vm_dump_stack(FILE *stream, const VirtualMachine *vm)
{
    fprintf(stream, "Stack:\n");
    if (vm->stack_size > 0)
    {
        for (size_t i = 0; i < vm->stack_size; i++)
        {
            fprintf(stream, " %ld\n", vm->stack[i]); // prints the stack, starting from 0 position to the end, in that order
        }
    }
    else
    {
        fprintf(stream, " [empty]\n");
    }
}

int vm_execute_at_inst_pointer(VirtualMachine *vm) // executes the instruction inst on vm
{
    Inst inst = vm->program[vm->instruction_pointer];
    switch (inst.type)
    {
    case INST_NOP:
        vm->instruction_pointer++;
        break;
    case INST_PUSH:
        if (vm->stack_size >= vm_stack_capacity)
        {
            return TRAP_STACK_OVERFLOW;
        }
        vm->stack[vm->stack_size++] = inst.operand; // push one word onto the stack
        vm->instruction_pointer++;
        break;
    case INST_PLUS:
        if (vm->stack_size < 2)
        {
            return TRAP_STACK_UNDERFLOW;
        }
        vm->stack[vm->stack_size - 2] += vm->stack[vm->stack_size - 1]; // add the last element on the stack to the second last element
        vm->stack_size -= 1;                                            // and remove the last element from the stack
        vm->instruction_pointer++;
        break;
    case INST_MINUS:
        if (vm->stack_size < 2)
        {
            return TRAP_STACK_UNDERFLOW;
        }
        vm->stack[vm->stack_size - 2] -= vm->stack[vm->stack_size - 1]; // add the last element on the stack to the second last element
        vm->stack_size -= 1;
        vm->instruction_pointer++;
        break;
    case INST_MULT:
        if (vm->stack_size < 2)
        {
            return TRAP_STACK_UNDERFLOW;
        }
        vm->stack[vm->stack_size - 2] *= vm->stack[vm->stack_size - 1]; // add the last element on the stack to the second last element
        vm->stack_size -= 1;
        vm->instruction_pointer++;
        break;
    case INST_DIV:
        if (vm->stack_size < 2)
        {
            return TRAP_STACK_UNDERFLOW;
        }
        if (!vm->stack[vm->stack_size - 1])
        {
            return TRAP_DIV_BY_ZERO;
        }
        vm->stack[vm->stack_size - 2] /= vm->stack[vm->stack_size - 1]; // add the last element on the stack to the second last element
        vm->stack_size -= 1;
        vm->instruction_pointer++;
        break;
    case INST_HALT:
        vm->halt = 1;
        break;
    case INST_JMP:
        if (inst.operand < 0 || (size_t)inst.operand >= vm->program_size)
        {
            return TRAP_ILLEGAL_JMP;
        }
        vm->instruction_pointer = inst.operand;
        break;
    case INST_EQ:
        if (vm->stack_size < 2)
        {
            return TRAP_STACK_UNDERFLOW;
        }
        vm->stack[vm->stack_size - 2] = (vm->stack[vm->stack_size - 1] == vm->stack[vm->stack_size - 2]); // add the last element on the stack to the second last element
        vm->stack_size -= 1;
        vm->instruction_pointer++;
        break;
    case INST_JMP_IF:
        if (vm->stack_size < 1)
        {
            return TRAP_STACK_UNDERFLOW;
        }
        if (inst.operand < 0 || (size_t)inst.operand >= vm->program_size)
        {
            return TRAP_ILLEGAL_JMP;
        }
        if (vm->stack[vm->stack_size - 1])
        {
            vm->stack_size--;
            vm->instruction_pointer = inst.operand;
        }
        else
        {
            vm->instruction_pointer++;
        }
        break;
    case INST_DUP:
        if (vm->stack_size >= vm_stack_capacity)
        {
            return TRAP_STACK_OVERFLOW;
        }
        if (inst.operand < 0)
        {
            return TRAP_ILLEGAL_OPERAND;
        }
        if ((__int64_t)vm->stack_size - 1 - inst.operand < 0)
        {
            return TRAP_STACK_UNDERFLOW;
        }
        vm->stack_size++;
        vm->stack[vm->stack_size - 1] = vm->stack[vm->stack_size - 2 - inst.operand];
        vm->instruction_pointer++;
        break;
    default:
        return TRAP_ILLEGAL_INSTRUCTION;
        break;
    }
    return TRAP_OK;
}

int vm_load_program_from_memory(VirtualMachine *vm, Inst *program, size_t program_size)
{
    if (!program)
    {
        fprintf(stderr, "Invalid Pointer to the Instruction Array\n");
        return FAILURE;
    }
    memcpy(vm->program, program, program_size * sizeof(program[0]));
    vm->program_size = program_size;

    return SUCCESS;
}

void vm_init(VirtualMachine *vm)
{
    label_array = malloc(sizeof(label_point_location) * label_capacity);
    if (!label_array)
    {
        fprintf(stderr, "ERROR: label array allocation failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    not_resolved_yet = malloc(sizeof(label_inst_location) * label_capacity);
    if (!not_resolved_yet)
    {
        fprintf(stderr, "ERROR: label resolution array allocation failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    vm->stack = malloc(sizeof(word) * vm_stack_capacity);
    if (!vm->stack)
    {
        fprintf(stderr, "ERROR: stack allocation failed: %s\n", strerror(errno));
    }

    vm->program = malloc(sizeof(Inst) * vm_program_capacity);
    if (!vm->program)
    {
        fprintf(stderr, "ERROR: code section allocation failed: %s\n", strerror(errno));
    }

    vm->stack_size = 0;
    vm->instruction_pointer = 0;
    vm->halt = 0;
}

int vm_exec_program(VirtualMachine *vm, __int64_t limit)
{
    int ret;
    if (vm->program[vm->program_size - 1].type != INST_HALT)
    {
        return TRAP_NO_HALT_FOUND;
    }
    while (!vm->halt && limit != 0)
    {
        fprintf(stdout, "%s\n", inst_type_as_cstr(vm->program[vm->instruction_pointer].type));
        ret = vm_execute_at_inst_pointer(vm);
        vm_dump_stack(stdout, vm);
        if (ret != TRAP_OK)
        {
            fprintf(stderr, "Trap activated: %s\n", trap_as_cstr(ret));
            return ret;
            // vm_dump_stack(stderr, &vm);
        }
        // vm_dump_stack(stdout, &vm);
        if (limit > 0)
        {
            limit--;
        }
    }
    return SUCCESS;
}

void vm_push_inst(VirtualMachine *vm, Inst *inst)
{
    if (vm->program_size == vm_program_capacity)
    {
        fprintf(stderr, "Program Size Limit Exceeded");
        exit(EXIT_FAILURE);
    }

    vm->program[vm->program_size++ - 1] = *inst;
}

void vm_save_program_to_file(Inst *program, size_t program_size, const char *file_path)
{
    FILE *f = fopen(file_path, "wb");
    if (!f)
    {
        fprintf(stderr, "ERROR: Could not open file '%s': %s\n", file_path, strerror(errno));
        exit(EXIT_FAILURE);
    }

    fwrite(program, sizeof(Inst), program_size, f);
    if (ferror(f)) // did some error occur due to the last stdio function call on f?
    {
        fprintf(stderr, "ERROR: Could not write to file '%s': %s\n", file_path, strerror(errno));
        fclose(f);
        exit(EXIT_FAILURE);
    }

    fclose(f);
}

size_t vm_load_program_from_file(Inst *program, const char *file_path)
{
    long ret = 0;
    FILE *f = fopen(file_path, "rb");
    if (!f)
    {
        fprintf(stderr, "ERROR: Could not open file '%s': %s\n", file_path, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (fseek(f, 0, SEEK_END))
    {
        fclose(f);
        fprintf(stderr, "ERROR: Could not read file '%s': %s\n", file_path, strerror(errno));
        exit(EXIT_FAILURE);
    }

    ret = ftell(f);
    if (ret == -1)
    {
        fclose(f);
        fprintf(stderr, "ERROR: Could not read file '%s': %s\n", file_path, strerror(errno));
        exit(EXIT_FAILURE);
    }

    // ret now contains the number of bytes in the file

    assert(ret % sizeof(Inst) == 0);
    assert((size_t)ret <= vm_program_capacity * sizeof(Inst));

    if (fseek(f, 0, SEEK_SET))
    {
        fclose(f);
        fprintf(stderr, "ERROR: Could not read file '%s': %s\n", file_path, strerror(errno));
        exit(EXIT_FAILURE);
    }

    int ret_val = fread(program, sizeof(Inst), ret / sizeof(Inst), f);
    if (ferror(f))
    {
        fclose(f);
        fprintf(stderr, "ERROR: Could not read file '%s': %s\n", file_path, strerror(errno));
        exit(EXIT_FAILURE);
    }
    fclose(f);

    return (size_t)(ret / sizeof(Inst));
}

Inst vm_translate_line(String_View line, size_t current_program_counter)
{
    String_View inst_name = sv_chop_by_delim(&line, ' ');
    // printf("%d\n", line.count);
    sv_trim_left(&line);
    sv_trim_right(&line);
    bool has_operand = line.count > 0;
    // line should point to a null byte now if the instruction doesn't have any operands

    if (sv_eq(inst_name, cstr_as_sv("push")))
    {
        if (!has_operand)
        {
            fprintf(stderr, "push requires an operand\n");
            exit(EXIT_FAILURE);
        }
        word operand = sv_to_int(&line);
        if (str_errno == FAILURE)
        {
            fprintf(stderr, "ERROR: %.*s is not a valid integer\n", (int)line.count, line.data);
            exit(EXIT_FAILURE);
        }
        else if (str_errno == OPERAND_OVERFLOW)
        {
            fprintf(stderr, "ERROR: %.*s overflows a 64 bit signed integer\n", (int)line.count, line.data);
            exit(EXIT_FAILURE);
        }

        return (Inst){.type = INST_PUSH, .operand = operand};
    }
    else if (sv_eq(inst_name, cstr_as_sv("halt")))
    {
        if (has_operand)
        {
            fprintf(stderr, "halt doesn't require an operand\n");
            exit(EXIT_FAILURE);
        }

        // printf("yes\n");
        return (Inst){.type = INST_HALT};
    }
    else if (sv_eq(inst_name, cstr_as_sv("dup")))
    {
        if (!has_operand)
        {
            fprintf(stderr, "dup requires an operand\n");
            exit(EXIT_FAILURE);
        }
        word operand = sv_to_int(&line);
        if (str_errno == FAILURE)
        {
            fprintf(stderr, "ERROR: %.*s is not a valid integer\n", (int)line.count, line.data);
            exit(EXIT_FAILURE);
        }
        else if (str_errno == OPERAND_OVERFLOW)
        {
            fprintf(stderr, "ERROR: %.*s overflows a 64 bit signed integer\n", (int)line.count, line.data);
            exit(EXIT_FAILURE);
        }

        return (Inst){.type = INST_DUP, .operand = operand};
    }
    else if (sv_eq(inst_name, cstr_as_sv("jmp")))
    {
        if (!has_operand)
        {
            fprintf(stderr, "jmp requires an operand\n");
            exit(EXIT_FAILURE);
        }
        word operand = sv_to_int(&line);
        if (str_errno == FAILURE)
        {
            not_resolved_yet[not_resolved_yet_counter].inst_location = current_program_counter;
            not_resolved_yet[not_resolved_yet_counter].label = line;
            not_resolved_yet[not_resolved_yet_counter].resolved = false;
            not_resolved_yet_counter++;
            return (Inst){.type = INST_JMP, .operand = 0};
        }
        else if (str_errno == OPERAND_OVERFLOW)
        {
            fprintf(stderr, "ERROR: %.*s overflows a 64 bit signed integer\n", (int)line.count, line.data);
            exit(EXIT_FAILURE);
        }

        return (Inst){.type = INST_JMP, .operand = operand};
    }
    else if (sv_eq(inst_name, cstr_as_sv("jmp_if")))
    {
        if (!has_operand)
        {
            fprintf(stderr, "jmp_if requires an operand\n");
            exit(EXIT_FAILURE);
        }
        word operand = sv_to_int(&line);
        if (str_errno == FAILURE)
        {
            fprintf(stderr, "%d\n", (int)line.count);
            not_resolved_yet[not_resolved_yet_counter].inst_location = current_program_counter;
            not_resolved_yet[not_resolved_yet_counter].label = line;
            not_resolved_yet[not_resolved_yet_counter].resolved = false;
            not_resolved_yet_counter++;
            return (Inst){.type = INST_JMP, .operand = 0};
        }
        else if (str_errno == OPERAND_OVERFLOW)
        {
            fprintf(stderr, "ERROR: %.*s overflows a 64 bit signed integer\n", (int)line.count, line.data);
            exit(EXIT_FAILURE);
        }

        return (Inst){.type = INST_JMP_IF, .operand = operand};
    }
    else if (sv_eq(inst_name, cstr_as_sv("plus")))
    {
        if (has_operand)
        {
            fprintf(stderr, "plus doesn't require an operand\n");
            exit(EXIT_FAILURE);
        }

        // printf("yes\n");
        return (Inst){.type = INST_PLUS};
    }
    else if (sv_eq(inst_name, cstr_as_sv("minus")))
    {
        if (has_operand)
        {
            fprintf(stderr, "minus doesn't require an operand\n");
            exit(EXIT_FAILURE);
        }

        // printf("yes\n");
        return (Inst){.type = INST_MINUS};
    }
    else if (sv_eq(inst_name, cstr_as_sv("mult")))
    {
        if (has_operand)
        {
            fprintf(stderr, "mult doesn't require an operand\n");
            exit(EXIT_FAILURE);
        }

        // printf("yes\n");
        return (Inst){.type = INST_MULT};
    }
    else if (sv_eq(inst_name, cstr_as_sv("div")))
    {
        if (has_operand)
        {
            fprintf(stderr, "div doesn't require an operand\n");
            exit(EXIT_FAILURE);
        }

        // printf("yes\n");
        return (Inst){.type = INST_DIV};
    }
    else if (sv_eq(inst_name, cstr_as_sv("eq")))
    {
        if (has_operand)
        {
            fprintf(stderr, "eq doesn't require an operand\n");
            exit(EXIT_FAILURE);
        }

        // printf("yes\n");
        return (Inst){.type = INST_EQ};
    }
    else if (sv_eq(inst_name, cstr_as_sv("nop")))
    {
        if (has_operand)
        {
            fprintf(stderr, "nop doesn't require an operand\n");
            exit(EXIT_FAILURE);
        }

        // printf("yes\n");
        return (Inst){.type = INST_NOP};
    }
    else
    {
        fprintf(stderr, "invalid instruction %.*s\n", (int)inst_name.count, inst_name.data);
        exit(EXIT_FAILURE);
    }
}

size_t vm_translate_source(String_View source, Inst *program, size_t program_capacity)
{
    size_t program_size = 0;

    while (source.count > 0) // Process each line
    {
        printf("Processing a new line...\n");

        if (program_size >= program_capacity)
        {
            fprintf(stderr, "Program Too Big\n");
            exit(EXIT_FAILURE);
        }

        String_View line = sv_chop_by_delim(&source, '\n');
        printf("Line after chop: '%.*s'\n", (int)line.count, line.data);

        line = sv_chop_by_delim(&line, '#'); // Remove comments
        printf("Line after removing comments: '%.*s'\n", (int)line.count, line.data);

        sv_trim_left(&line);
        sv_trim_right(&line);

        if (line.count == 0) // Ignore empty lines
        {
            printf("Ignoring empty line\n");
            continue;
        }

        // Check for a label
        String_View label = sv_chop_by_delim(&line, ':');
        printf("%d\n", line.count);
        if (*(line.data - 1) == ':') // If there's a label
        {
            printf("Found label: '%.*s', pointing to address %zu\n", (int)label.count, label.data, program_size);
            label_array[label_array_counter].label_pointing_location = program_size;
            label_array[label_array_counter].label = label;
            label_array_counter++;

            if (line.count > 0) // instruction remaining after the label
            {
                sv_trim_left(&line);
                printf("Translating instruction: '%.*s'\n", (int)line.count, line.data);
                program[program_size] = vm_translate_line(line, program_size);
                program_size++;
            }
            continue;
        }

        line = label;
        printf("Translating instruction: '%.*s'\n", (int)line.count, line.data);
        program[program_size] = vm_translate_line(line, program_size);
        program_size++;
    }

    // Resolve labels
    printf("Resolving labels...\n");
    for (size_t i = 0; i < not_resolved_yet_counter; i++)
    {
        for (size_t j = 0; j < label_array_counter; j++)
        {
            if (sv_eq(not_resolved_yet[i].label, label_array[j].label))
            {
                printf("Resolving label '%.*s' to location %zu\n", (int)label_array[j].label.count, label_array[j].label.data, label_array[j].label_pointing_location);
                program[not_resolved_yet[i].inst_location].operand = label_array[j].label_pointing_location;
                not_resolved_yet[i].resolved = true;
            }
        }
    }

    // Check if any labels couldn't be resolved
    bool flag = false;
    for (size_t i = 0; i < not_resolved_yet_counter; i++)
    {
        if (!not_resolved_yet[i].resolved)
        {
            fprintf(stderr, "ERROR: cannot resolve label: %.*s\n", (int)not_resolved_yet[i].label.count, not_resolved_yet[i].label.data);
            flag = true;
        }
    }

    if (flag)
    {
        exit(EXIT_FAILURE);
    }

    printf("Translation complete. Program size: %zu\n", program_size);
    return program_size;
}

String_View slurp_file(const char *file_path)
{
    int ret = 0;
    FILE *f = fopen(file_path, "r");
    if (!f)
    {
        fprintf(stderr, "ERROR: Couldn't open file: '%s': %s\n", file_path, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (fseek(f, 0, SEEK_END))
    {
        fclose(f);
        fprintf(stderr, "ERROR: Could not read file '%s': %s\n", file_path, strerror(errno));
        exit(EXIT_FAILURE);
    }

    ret = ftell(f);
    if (ret == -1)
    {
        fclose(f);
        fprintf(stderr, "ERROR: Could not read file '%s': %s\n", file_path, strerror(errno));
        exit(EXIT_FAILURE);
    }

    // ret now contains the number of bytes in the file
    if (fseek(f, 0, SEEK_SET))
    {
        fclose(f);
        fprintf(stderr, "ERROR: Could not read file '%s': %s\n", file_path, strerror(errno));
        exit(EXIT_FAILURE);
    }

    char *buffer = malloc(ret);
    if (!buffer)
    {
        fprintf(stderr, "ERROR: Couldn't allocate memory for file '%s': %s\n", file_path, strerror(errno));
        exit(EXIT_FAILURE);
    }

    int ret_val = fread(buffer, 1, ret, f);
    if (ferror(f))
    {
        fclose(f);
        fprintf(stderr, "Could not read the file '%s': %s\n", file_path, strerror(errno));
        exit(EXIT_FAILURE);
    }

    fclose(f);

    return (String_View){
        .count = ret,
        .data = buffer,
    };
}

#endif // _VM_IMPLEMENTATION
#endif // _VM