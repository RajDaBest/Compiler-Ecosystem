#ifndef _VM
#define _VM
#define _SV_IMPLEMENTATION
#define _NAN_IMPLEMENTATION

#include "String_View.h"
#include "nan_boxing.h"
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
    double label_pointing_location;
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
    TRAP_ILLEGAL_OPERATION, // doing arithmetic on floating points and ints together
} Trap;                     // exceptions that stop the execution on the virtual machine

typedef enum
{
    INST_NOP = 0, // does nothing but increment the instruction pointer; if the program array is just zero, it will all be no-ops;
    INST_SPUSH,    // push a word to the stack top; we assume that our stack grows downwards
    INST_UPUSH,
    INST_FPUSH,
    INST_DUP,     // duplicates the element at the position stack_top - addr at the top of the stack; stack_top = stack_size - 1
    INST_SPLUS,   // add the last element on stack onto the second last element, and remove the last element from the stack
    INST_UPLUS,
    INST_FPLUS,
    INST_SMINUS, // subtract the last element on the stack from the second last element, and remove the last element from the stack
    INST_UMINUS,
    INST_FMINUS,
    INST_SMULT, // multiply the last element on the stack to the second last element, and remove the last element from the stack
    INST_UMULT,
    INST_FMULT,
    INST_SDIV, // integer divide the second last element on the stack by the last element and store the result in the second last element, and then remove the last element from the stack
    INST_UDIV,
    INST_FDIV,
    INST_JMP,    // unconditional jump
    INST_HALT,   // halt the machine
    INST_JMP_IF, // jump to an address if the last element on the stack is non-zero; do not jump otherwise
    INST_EQ,     // checks if the second last stack element is equal to the last stack element; sets the second last element to one if true, and 0 otherwise; removes the last element from the stack
} Inst_Type;     // enum for the instruction types

typedef struct
{
    Inst_Type type; // type of the instruction
    double operand; // operand of the instruction
} Inst;             // structure for the actual instruction

typedef struct // structure defining the actual virtual machine
{
    double *stack;            // the stack of the virtual machine; the stack top is the end of the array
    size_t stack_size;        // current stack size
    word instruction_pointer; // the address of the next instruction to be executed
    int halt;
    Inst *program;       // the actual instruction array
    size_t program_size; // number of instructions in the program
} VirtualMachine;

void push_to_not_resolved_yet(String_View label, size_t inst_location);
void push_to_label_array(String_View label, double pointing_location);
const char *trap_as_cstr(Trap trap);
const char *inst_type_as_asm_str(Inst_Type type);
const char *inst_type_as_cstr(Inst_Type type);
void vm_dump_stack(FILE *stream, const VirtualMachine *vm);
int vm_execute_at_inst_pointer(VirtualMachine *vm); // executes the instruction inst on vm
int vm_load_program_from_memory(VirtualMachine *vm, Inst *program, size_t program_size);
void label_init();
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
    case TRAP_ILLEGAL_OPERATION:
        return "TRAP_ILLEGAL_OPERATION";
    default:
        assert(00 && "trap_as_cstr: Unreachnable");
    }
}

const char *inst_type_as_cstr(Inst_Type type)
{
    switch (type)
    {
    case INST_FPUSH:
        return "INST_FPUSH";
    case INST_UPUSH:
        return "INST_UPUSH";
    case INST_SPUSH:
        return "INST_SPUSH";
    case INST_NOP:
        return "INST_NOP";
    case INST_SPLUS:
        return "INST_SPLUS";
    case INST_SMULT:
        return "INST_SMULT";
    case INST_SDIV:
        return "INST_IDIV";
    case INST_SMINUS:
        return "INST_IMINUS";
    case INST_UPLUS:
        return "INST_UPLUS";
    case INST_UMULT:
        return "INST_UMULT";
    case INST_UDIV:
        return "INST_UDIV";
    case INST_UMINUS:
        return "INST_UMINUS";
    case INST_FPLUS:
        return "INST_FPLUS";
    case INST_FMULT:
        return "INST_FMULT";
    case INST_FDIV:
        return "INST_FDIV";
    case INST_FMINUS:
        return "INST_FMINUS";
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
    case INST_SPUSH:
        return "spush";
    case INST_UPUSH:
        return "upush";
    case INST_FPUSH:
        return "fpush";
    case INST_NOP:
        return "nop";
    case INST_SPLUS:
        return "splus";
    case INST_SMULT:
        return "smult";
    case INST_SDIV:
        return "sdiv";
    case INST_SMINUS:
        return "sminus";
    case INST_UPLUS:
        return "uplus";
    case INST_UMULT:
        return "umult";
    case INST_UDIV:
        return "udiv";
    case INST_UMINUS:
        return "uminus";
    case INST_FPLUS:
        return "fplus";
    case INST_FDIV:
        return "fdiv";
    case INST_FMINUS:
        return "fminus";
    case INST_FMULT:
        return "fmult";
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

void push_to_not_resolved_yet(String_View label, size_t inst_location)
{
    not_resolved_yet[not_resolved_yet_counter].inst_location = inst_location;
    not_resolved_yet[not_resolved_yet_counter].label = label;
    not_resolved_yet[not_resolved_yet_counter].resolved = false;
    not_resolved_yet_counter++;
}

void push_to_label_array(String_View label, double pointing_location)
{
    label_array[label_array_counter].label_pointing_location = pointing_location;
    label_array[label_array_counter].label = label;
    label_array_counter++;
}

void vm_dump_stack(FILE *stream, const VirtualMachine *vm)
{
    fprintf(stream, "Stack:\n");
    if (vm->stack_size > 0)
    {
        // prints the stack, starting from 0 position to the end, in that order
        for (size_t i = 0; i < vm->stack_size; i++)
        {
            if (is_nan(vm->stack[i]))
            {

                __int64_t element = return_value_signed(vm->stack[i]);
                fprintf(stream, "%ld\n", element);
            }
            else
            {
                // printf("%lf\n", vm->stack[i]);
                // print_bits(vm->stack[i]);
                fprintf(stream, "%lf\n", vm->stack[i]);
            }
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
    case INST_FPUSH:
        if (vm->stack_size >= vm_stack_capacity)
        {
            return TRAP_STACK_OVERFLOW;
        }
        // print_bits(inst.operand);
        vm->stack[vm->stack_size++] = inst.operand; // push one word onto the stack
        vm->instruction_pointer++;
        break;
    case INST_SPUSH:
        if (vm->stack_size >= vm_stack_capacity)
        {
            return TRAP_STACK_OVERFLOW;
        }
        // print_bits(inst.operand);
        vm->stack[vm->stack_size++] = inst.operand; // push one word onto the stack
        vm->instruction_pointer++;
        break;
    case INST_UPUSH:
        if (vm->stack_size >= vm_stack_capacity)
        {
            return TRAP_STACK_OVERFLOW;
        }
        // print_bits(inst.operand);
        vm->stack[vm->stack_size++] = inst.operand; // push one word onto the stack
        vm->instruction_pointer++;
        break;
    case INST_FPLUS:
        if (vm->stack_size < 2)
        {
            return TRAP_STACK_UNDERFLOW;
        }

        vm->stack[vm->stack_size - 2] += vm->stack[vm->stack_size - 1]; // add the last element on the stack to the second last element
        vm->stack_size -= 1;                                            // and remove the last element from the stack
        vm->instruction_pointer++;
        break;
    case INST_SPLUS:
        if (vm->stack_size < 2)
        {
            return TRAP_STACK_UNDERFLOW;
        }

        __int64_t operand1 = return_value_signed(vm->stack[vm->stack_size - 2]);
        __int64_t operand2 = return_value_signed(vm->stack[vm->stack_size - 1]);
        __int64_t ans = operand1 + operand2;

        double ans_ = 0;
        set_signed_64int(&ans_, ans);
        vm->stack[vm->stack_size - 2] = ans_;

        vm->stack_size -= 1; // and remove the last element from the stack
        vm->instruction_pointer++;
        break;
    case INST_UPLUS:
        if (vm->stack_size < 2)
        {
            return TRAP_STACK_UNDERFLOW;
        }

        __uint64_t opernd11 = return_value_unsigned(vm->stack[vm->stack_size - 2]);
        __uint64_t opernd12 = return_value_unsigned(vm->stack[vm->stack_size - 1]);
        __uint64_t ns = opernd11 + opernd12;

        double ns_ = 0;
        set_unsigned_64int(&ns_, ns);
        vm->stack[vm->stack_size - 2] = ns_;

        vm->stack_size -= 1; // and remove the last element from the stack
        vm->instruction_pointer++;
        break;
    case INST_FMINUS:
        if (vm->stack_size < 2)
        {
            return TRAP_STACK_UNDERFLOW;
        }

        vm->stack[vm->stack_size - 2] -= vm->stack[vm->stack_size - 1]; // subtract the last element on the stack to the second last element

        vm->stack_size -= 1;
        vm->instruction_pointer++;
        break;
    case INST_SMINUS:
        if (vm->stack_size < 2)
        {
            return TRAP_STACK_UNDERFLOW;
        }

        __int64_t operand3 = return_value_signed(vm->stack[vm->stack_size - 2]);
        __int64_t operand4 = return_value_signed(vm->stack[vm->stack_size - 1]);
        __int64_t ans2 = operand3 - operand4;

        double ans_2 = 0;
        set_signed_64int(&ans_2, ans2);
        vm->stack[vm->stack_size - 2] = ans_2;

        vm->stack_size -= 1;
        vm->instruction_pointer++;
        break;
    case INST_UMINUS:
        if (vm->stack_size < 2)
        {
            return TRAP_STACK_UNDERFLOW;
        }

        __uint64_t opernd3 = return_value_unsigned(vm->stack[vm->stack_size - 2]);
        __uint64_t opernd4 = return_value_unsigned(vm->stack[vm->stack_size - 1]);
        __uint64_t ns2 = opernd3 - opernd4;

        double ns_2 = 0;
        set_unsigned_64int(&ns_2, ns2);
        vm->stack[vm->stack_size - 2] = ns_2;

        vm->stack_size -= 1;
        vm->instruction_pointer++;
        break;
    case INST_FMULT:
        if (vm->stack_size < 2)
        {
            return TRAP_STACK_UNDERFLOW;
        }

        vm->stack[vm->stack_size - 2] *= vm->stack[vm->stack_size - 1]; // add the last element on the stack to the second last element

        vm->stack_size -= 1;
        vm->instruction_pointer++;
        break;
    case INST_SMULT:
        if (vm->stack_size < 2)
        {
            return TRAP_STACK_UNDERFLOW;
        }

        __int64_t operand5 = return_value_signed(vm->stack[vm->stack_size - 2]);
        __int64_t operand6 = return_value_signed(vm->stack[vm->stack_size - 1]);
        __int64_t ans3 = operand5 * operand6;

        double ans_3 = 0;
        set_signed_64int(&ans_3, ans3);
        vm->stack[vm->stack_size - 2] = ans_3;

        vm->stack_size -= 1;
        vm->instruction_pointer++;
        break;
    case INST_UMULT:
        if (vm->stack_size < 2)
        {
            return TRAP_STACK_UNDERFLOW;
        }

        __uint64_t opernd5 = return_value_unsigned(vm->stack[vm->stack_size - 2]);
        __uint64_t opernd6 = return_value_unsigned(vm->stack[vm->stack_size - 1]);
        __uint64_t ns3 = opernd5 * opernd6;

        double ns_3 = 0;
        set_unsigned_64int(&ns_3, ns3);
        vm->stack[vm->stack_size - 2] = ns_3;

        vm->stack_size -= 1;
        vm->instruction_pointer++;
        break;
    case INST_SDIV:
        if (vm->stack_size < 2)
        {
            return TRAP_STACK_UNDERFLOW;
        }

        __int64_t opernd1 = return_value_signed(vm->stack[vm->stack_size - 1]);
        if (!opernd1)
        {
            return TRAP_DIV_BY_ZERO;
        }

        __int64_t operand7 = return_value_signed(vm->stack[vm->stack_size - 2]);
        __int64_t operand8 = return_value_signed(vm->stack[vm->stack_size - 1]);
        __int64_t ans4 = operand7 / operand8;

        double ans_4 = 0;
        set_signed_64int(&ans_4, ans4);
        vm->stack[vm->stack_size - 2] = ans_4;

        vm->stack_size -= 1;
        vm->instruction_pointer++;
        break;
    case INST_UDIV:
        if (vm->stack_size < 2)
        {
            return TRAP_STACK_UNDERFLOW;
        }

        __uint64_t opernd2 = return_value_unsigned(vm->stack[vm->stack_size - 1]);
        if (!opernd2)
        {
            return TRAP_DIV_BY_ZERO;
        }

        __uint64_t opernd7 = return_value_unsigned(vm->stack[vm->stack_size - 2]);
        __uint64_t opernd8 = return_value_unsigned(vm->stack[vm->stack_size - 1]);
        __uint64_t ns4 = opernd7 / opernd8;

        double ns_4 = 0;
        set_unsigned_64int(&ns_4, ns4);
        vm->stack[vm->stack_size - 2] = ns_4;

        vm->stack_size -= 1;
        vm->instruction_pointer++;
        break;
    case INST_FDIV:
        if (vm->stack_size < 2)
        {
            return TRAP_STACK_UNDERFLOW;
        }

        if (vm->stack[vm->stack_size - 1] == 0.0)
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
        __uint64_t opernd = return_value_unsigned(inst.operand);
        if (opernd >= vm->program_size)
        {
            return TRAP_ILLEGAL_JMP;
        }
        vm->instruction_pointer = opernd;
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
        __uint64_t opernd111 = return_value_unsigned(inst.operand);
        if (vm->stack_size < 1)
        {
            return TRAP_STACK_UNDERFLOW;
        }
        if ((opernd111 >= vm->program_size))
        {
            return TRAP_ILLEGAL_JMP;
        }
        if (vm->stack[vm->stack_size - 1])
        {
            vm->stack_size--;
            vm->instruction_pointer = opernd111;
        }
        else
        {
            vm->instruction_pointer++;
        }
        break;
    case INST_DUP:
        __uint64_t opernd222 = return_value_unsigned(inst.operand);
        // printf("%ld\n", opernd2);
        if (vm->stack_size >= vm_stack_capacity)
        {
            return TRAP_STACK_OVERFLOW;
        }
        if ((__int64_t)vm->stack_size - 1 - (__int64_t)(opernd222) < 0)
        {
            return TRAP_STACK_UNDERFLOW;
        }
        vm->stack_size++;
        vm->stack[vm->stack_size - 1] = vm->stack[vm->stack_size - 2 - opernd222];
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

void label_init()
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
}

void vm_init(VirtualMachine *vm)
{
    vm->stack = malloc(sizeof(double) * vm_stack_capacity);
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

    if (sv_eq(inst_name, cstr_as_sv("fpush")))
    {
        if (!has_operand)
        {
            fprintf(stderr, "push requires an operand\n");
            exit(EXIT_FAILURE);
        }
        // printf("%.*s%d\n", (int)line.count, line.data, (int)line.count);
        double operand = sv_to_value(&line);
        // printf("%lf\n", operand);
        if (str_errno == FAILURE)
        {
            fprintf(stderr, "ERROR: %.*s is not a valid value\n", (int)line.count, line.data);
            exit(EXIT_FAILURE);
        }
        else if (str_errno == OPERAND_OVERFLOW)
        {
            fprintf(stderr, "ERROR: %.*s overflows a 64 bit signed value\n", (int)line.count, line.data);
            exit(EXIT_FAILURE);
        }

        return (Inst){.type = INST_FPUSH, .operand = operand};
    }
    if (sv_eq(inst_name, cstr_as_sv("spush")))
    {
        if (!has_operand)
        {
            fprintf(stderr, "push requires an operand\n");
            exit(EXIT_FAILURE);
        }
        // printf("%.*s%d\n", (int)line.count, line.data, (int)line.count);
        double operand = sv_to_value(&line);
        // printf("%lf\n", operand);
        if (str_errno == FAILURE)
        {
            fprintf(stderr, "ERROR: %.*s is not a valid value\n", (int)line.count, line.data);
            exit(EXIT_FAILURE);
        }
        else if (str_errno == OPERAND_OVERFLOW)
        {
            fprintf(stderr, "ERROR: %.*s overflows a 64 bit signed value\n", (int)line.count, line.data);
            exit(EXIT_FAILURE);
        }

        __int64_t int1 = (__int64_t)operand;
        // printf("%ld %lf\n", int1, operand);
        set_signed_64int(&operand, int1);
        // printf("%ld %lf\n", int1, operand);
        // printf("%ld %ld\n", return_value_signed(operand), int1);

        return (Inst){.type = INST_SPUSH, .operand = operand};
    }
    if (sv_eq(inst_name, cstr_as_sv("upush")))
    {
        if (!has_operand)
        {
            fprintf(stderr, "push requires an operand\n");
            exit(EXIT_FAILURE);
        }
        // printf("%.*s%d\n", (int)line.count, line.data, (int)line.count);
        double operand = sv_to_value(&line);
        // printf("%lf\n", operand);
        if (str_errno == FAILURE)
        {
            fprintf(stderr, "ERROR: %.*s is not a valid value\n", (int)line.count, line.data);
            exit(EXIT_FAILURE);
        }
        else if (str_errno == OPERAND_OVERFLOW)
        {
            fprintf(stderr, "ERROR: %.*s overflows a 64 bit signed value\n", (int)line.count, line.data);
            exit(EXIT_FAILURE);
        }

        __uint64_t int1 = (__uint64_t)operand;
        // printf("%ld %lf\n", int1, operand);
        set_unsigned_64int(&operand, int1);
        // printf("%ld %lf\n", int1, operand);
        // printf("%ld %ld\n", return_value_signed(operand), int1);

        return (Inst){.type = INST_UPUSH, .operand = operand};
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
        double operand = sv_to_value(&line);
        if (str_errno == FAILURE)
        {
            fprintf(stderr, "ERROR: %.*s is not a valid value\n", (int)line.count, line.data);
            exit(EXIT_FAILURE);
        }
        else if (str_errno == OPERAND_OVERFLOW)
        {
            fprintf(stderr, "ERROR: %.*s overflows a 64 bit unsigned value\n", (int)line.count, line.data);
            exit(EXIT_FAILURE);
        }

        if (is_fraction)
        {
            fprintf(stderr, "ERROR: illegal operand value for dup instruction: %.*s\nMust be an unsigned value\n", (int)line.count, line.data);
        }
        else if (is_negative)
        {
            fprintf(stderr, "ERROR: illegal operand value for dup instruction: %.*s\nMust be an unsigned value\n", (int)line.count, line.data);
        }
        else
        {
            set_unsigned_64int(&operand, (__int64_t)operand);
            return (Inst){.type = INST_DUP, .operand = operand};
        }
    }
    else if (sv_eq(inst_name, cstr_as_sv("jmp")))
    {
        if (!has_operand)
        {
            fprintf(stderr, "jmp requires an operand\n");
            exit(EXIT_FAILURE);
        }
        double operand = sv_to_value(&line);
        if (str_errno == FAILURE)
        {
            push_to_not_resolved_yet(line, current_program_counter);
            return (Inst){.type = INST_JMP, .operand = 0};
        }
        else if (str_errno == OPERAND_OVERFLOW)
        {
            fprintf(stderr, "ERROR: %.*s overflows a 64 bit unsigned value\n", (int)line.count, line.data);
            exit(EXIT_FAILURE);
        }

        if (is_fraction)
        {
            fprintf(stderr, "ERROR: illegal operand value for jmp instruction: %.*s\nMust be an unsigned value\n", (int)line.count, line.data);
        }
        else if (is_negative)
        {
            fprintf(stderr, "ERROR: illegal operand value for jmp instruction: %.*s\nMust be an unsigned value\n", (int)line.count, line.data);
        }
        else
        {
            set_unsigned_64int(&operand, (__uint64_t)operand);
            return (Inst){.type = INST_JMP, .operand = operand};
        }
    }
    else if (sv_eq(inst_name, cstr_as_sv("jmp_if")))
    {
        if (!has_operand)
        {
            fprintf(stderr, "jmp_if requires an operand\n");
            exit(EXIT_FAILURE);
        }
        double operand = sv_to_value(&line);
        if (str_errno == FAILURE)
        {
            push_to_not_resolved_yet(line, current_program_counter);
            return (Inst){.type = INST_JMP_IF, .operand = 0};
        }
        else if (str_errno == OPERAND_OVERFLOW)
        {
            fprintf(stderr, "ERROR: %.*s overflows a 64 bit unsigned value\n", (int)line.count, line.data);
            exit(EXIT_FAILURE);
        }

        if (is_fraction)
        {
            fprintf(stderr, "ERROR: illegal operand value for jmp_if instruction: %.*s\nMust be an unsigned value\n", (int)line.count, line.data);
        }
        else if (is_negative)
        {
            fprintf(stderr, "ERROR: illegal operand value for jmp_if instruction: %.*s\nMust be an unsigned value\n", (int)line.count, line.data);
        }
        else
        {
            set_unsigned_64int(&operand, (__uint64_t)operand);
            return (Inst){.type = INST_JMP_IF, .operand = operand};
        }
    }
    else if (sv_eq(inst_name, cstr_as_sv("splus")))
    {
        if (has_operand)
        {
            fprintf(stderr, "splus doesn't require an operand\n");
            exit(EXIT_FAILURE);
        }

        // printf("yes\n");
        return (Inst){.type = INST_SPLUS};
    }
    else if (sv_eq(inst_name, cstr_as_sv("uplus")))
    {
        if (has_operand)
        {
            fprintf(stderr, "uplus doesn't require an operand\n");
            exit(EXIT_FAILURE);
        }

        // printf("yes\n");
        return (Inst){.type = INST_UPLUS};
    }
    else if (sv_eq(inst_name, cstr_as_sv("fplus")))
    {
        if (has_operand)
        {
            fprintf(stderr, "fplus doesn't require an operand\n");
            exit(EXIT_FAILURE);
        }

        // printf("yes\n");
        return (Inst){.type = INST_FPLUS};
    }
    else if (sv_eq(inst_name, cstr_as_sv("sminus")))
    {
        if (has_operand)
        {
            fprintf(stderr, "sminus doesn't require an operand\n");
            exit(EXIT_FAILURE);
        }

        // printf("yes\n");
        return (Inst){.type = INST_SMINUS};
    }
    else if (sv_eq(inst_name, cstr_as_sv("uminus")))
    {
        if (has_operand)
        {
            fprintf(stderr, "uminus doesn't require an operand\n");
            exit(EXIT_FAILURE);
        }

        // printf("yes\n");
        return (Inst){.type = INST_UMINUS};
    }
    else if (sv_eq(inst_name, cstr_as_sv("fminus")))
    {
        if (has_operand)
        {
            fprintf(stderr, "fminus doesn't require an operand\n");
            exit(EXIT_FAILURE);
        }

        // printf("yes\n");
        return (Inst){.type = INST_FMINUS};
    }
    else if (sv_eq(inst_name, cstr_as_sv("smult")))
    {
        if (has_operand)
        {
            fprintf(stderr, "smult doesn't require an operand\n");
            exit(EXIT_FAILURE);
        }

        // printf("yes\n");
        return (Inst){.type = INST_SMULT};
    }
    else if (sv_eq(inst_name, cstr_as_sv("umult")))
    {
        if (has_operand)
        {
            fprintf(stderr, "umult doesn't require an operand\n");
            exit(EXIT_FAILURE);
        }

        // printf("yes\n");
        return (Inst){.type = INST_UMULT};
    }
    else if (sv_eq(inst_name, cstr_as_sv("fmult")))
    {
        if (has_operand)
        {
            fprintf(stderr, "fmult doesn't require an operand\n");
            exit(EXIT_FAILURE);
        }

        // printf("yes\n");
        return (Inst){.type = INST_FMULT};
    }
    else if (sv_eq(inst_name, cstr_as_sv("sdiv")))
    {
        if (has_operand)
        {
            fprintf(stderr, "sdiv doesn't require an operand\n");
            exit(EXIT_FAILURE);
        }

        // printf("yes\n");
        return (Inst){.type = INST_SDIV};
    }
    else if (sv_eq(inst_name, cstr_as_sv("udiv")))
    {
        if (has_operand)
        {
            fprintf(stderr, "udiv doesn't require an operand\n");
            exit(EXIT_FAILURE);
        }

        // printf("yes\n");
        return (Inst){.type = INST_UDIV};
    }
    else if (sv_eq(inst_name, cstr_as_sv("fdiv")))
    {
        if (has_operand)
        {
            fprintf(stderr, "fdiv doesn't require an operand\n");
            exit(EXIT_FAILURE);
        }

        // printf("yes\n");
        return (Inst){.type = INST_FDIV};
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

    return (Inst){0};
}

size_t vm_translate_source(String_View source, Inst *program, size_t program_capacity)
{
    size_t program_size = 0;

    while (source.count > 0) // Process each line
    {

        if (program_size >= program_capacity)
        {
            fprintf(stderr, "Program Too Big\n");
            exit(EXIT_FAILURE);
        }

        String_View line = sv_chop_by_delim(&source, '\n');

        line = sv_chop_by_delim(&line, '#'); // Remove comments

        sv_trim_left(&line);
        sv_trim_right(&line);

        if (line.count == 0) // Ignore empty lines
        {
            continue;
        }

        // Check for a label
        String_View label = sv_chop_by_delim(&line, ':');
        if (*(line.data - 1) == ':') // If there's a label
        {
            if (label_array_counter >= label_capacity)
            {
                fprintf(stderr, "ERROR: label capacity exceeded at label: %.*s\n", (int)label.count, label.data);
            }
            double operand = 0;
            set_unsigned_64int(&operand, (__uint64_t)program_size);
            push_to_label_array(label, operand);

            if (line.count > 0) // instruction remaining after the label
            {
                sv_trim_left(&line);
                program[program_size] = vm_translate_line(line, program_size);
                program_size++;
            }
            continue;
        }

        line = label;
        program[program_size] = vm_translate_line(line, program_size);
        program_size++;
    }

    // Resolve labels
    for (size_t i = 0; i < not_resolved_yet_counter; i++)
    {
        for (size_t j = 0; j < label_array_counter; j++)
        {
            if (sv_eq(not_resolved_yet[i].label, label_array[j].label))
            {
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