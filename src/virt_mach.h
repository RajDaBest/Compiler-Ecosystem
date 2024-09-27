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

// long is 32 bit on Windows and 64 bit on Linux; long long is 64 bit on both; so we use that to make it cross platform

#define EPSILON 1e-9
#define VM_STACK_CAPACITY 1024
#define VM_PROGRAM_CAPACITY 1024
#define VM_LABEL_CAPACITY 128
#define VM_EQU_CAPACITY 128
#define VM_NATIVE_CAPACITY 128
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
size_t equ_label_capacity = VM_EQU_CAPACITY;
size_t natives_capacity = VM_NATIVE_CAPACITY;
size_t line_no = 0;
bool compilation_successful = true;

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
    INST_NOP = 1, // does nothing but increment the instruction pointer; if the program array is just zero, it will all be no-ops;
    INST_SPUSH,   // push a word to the stack top; we assume that our stack grows downwards
    INST_UPUSH,
    INST_FPUSH,
    INST_RDUP, // duplicates the element at the position stack_top - addr at the top of the stack; stack_top = stack_size - 1
    INST_ADUP,
    INST_SPLUS, // add the last element on stack onto the second last element, and remove the last element from the stack
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
    INST_JMP,     // unconditional jump
    INST_HALT,    // halt the machine
    INST_UJMP_IF, // jump to an address if the last element on the stack is non-zero; do not jump otherwise
    INST_FJMP_IF,
    INST_EQ,  // checks if the second last stack element is equal to the last stack element; sets the second last element to one if true, and 0 otherwise; removes the last element from the stack
    INST_LSR, // logical shift right; for unsigned
    INST_ASR, // arithmetic shift right;  for signed
    INST_SL,  // shift left; for both signed and unsigned
    INST_AND,
    INST_OR,
    INST_NOT,
    INST_EMPTY,
    INST_POP_AT,
    INST_POP,
    INST_RSWAP,
    INST_ASWAP,
    INST_RET,
    INST_CALL,
    INST_NATIVE,
    INST_COUNT,
} Inst_Type; // enum for the instruction types

typedef struct
{
    Inst_Type type; // type of the instruction
    double operand; // operand of the instruction
} Inst;             // structure for the actual instruction

struct VirtualMachine;

typedef Trap (*native)(struct VirtualMachine *); // you can define functions that match this signature and assign their addresses to variables of type native

typedef struct VirtualMachine // structure defining the actual virtual machine
{
    double *stack;     // the stack of the virtual machine; the stack top is the end of the array
    size_t stack_size; // current stack size

    Inst *program;            // the actual instruction array
    size_t program_size;      // number of instructions in the program
    word instruction_pointer; // the address of the next instruction to be executed

    native *natives;
    size_t natives_size;

    bool has_start;
    size_t start_label_index;

    int halt;
} VirtualMachine;

typedef struct
{
    bool has_start;
    size_t code_section_offset;
    int64_t start_location;
    size_t code_section_size;
    size_t data_section_offset; // not using currently
} vm_header_;

void push_to_not_resolved_yet(String_View label, size_t inst_location);
void push_to_label_array(String_View label, double pointing_location);
const char *trap_as_cstr(Trap trap);
const char *inst_type_as_asm_str(Inst_Type type);
const char *inst_type_as_cstr(Inst_Type type);
void vm_native_push(VirtualMachine *vm, native native_func);
void vm_dump_stack(FILE *stream, const VirtualMachine *vm);
static int handle_shift(VirtualMachine *vm, Inst inst, bool is_arithmetic);
static int handle_push(VirtualMachine *vm, Inst inst);
static int handle_arithmetic(VirtualMachine *vm, Inst inst);
const char *get_inst_name(Inst_Type inst);
bool has_operand_function(Inst_Type inst);
uint8_t get_operand_type(Inst_Type inst);
int vm_execute_at_inst_pointer(VirtualMachine *vm); // executes the instruction inst on vm
int vm_load_program_from_memory(VirtualMachine *vm, Inst *program, size_t program_size);
void label_init();
void label_free();
void vm_init(VirtualMachine *vm, char *source_code);
void vm_internal_free(VirtualMachine *vm);
int vm_exec_program(VirtualMachine *vm, int64_t limit, bool debug);
void vm_push_inst(VirtualMachine *vm, Inst *inst);
void vm_save_program_to_file(Inst *program, vm_header_ header, const char *file_path);
vm_header_ vm_load_program_from_file(Inst *program, const char *file_path);
Inst vm_translate_line(String_View line, size_t current_program_counter);
static void process_label(String_View label, size_t program_size);
static void resolve_labels(Inst *program);
static void check_unresolved_labels();
vm_header_ vm_translate_source(String_View source, Inst *program, size_t program_capacity);
String_View slurp_file(const char *file_path);

#ifdef _VM_IMPLEMENTATION

const char *get_inst_name(Inst_Type inst)
{
    switch (inst)
    {
    case INST_NOP:
        return "nop";
    case INST_SPUSH:
        return "spush";
    case INST_UPUSH:
        return "upush";
    case INST_FPUSH:
        return "fpush";
    case INST_RDUP:
        return "rdup";
    case INST_ADUP:
        return "adup";
    case INST_SPLUS:
        return "splus";
    case INST_UPLUS:
        return "uplus";
    case INST_FPLUS:
        return "fplus";
    case INST_SMINUS:
        return "sminus";
    case INST_UMINUS:
        return "uminus";
    case INST_FMINUS:
        return "fminus";
    case INST_SMULT:
        return "smult";
    case INST_UMULT:
        return "umult";
    case INST_FMULT:
        return "fmult";
    case INST_SDIV:
        return "sdiv";
    case INST_UDIV:
        return "udiv";
    case INST_FDIV:
        return "fdiv";
    case INST_JMP:
        return "jmp";
    case INST_HALT:
        return "halt";
    case INST_UJMP_IF:
        return "ujmp_if";
    case INST_FJMP_IF:
        return "fjmp_if";
    case INST_EQ:
        return "eq";
    case INST_LSR:
        return "lsr";
    case INST_ASR:
        return "asr";
    case INST_SL:
        return "sl";
    case INST_AND:
        return "and";
    case INST_OR:
        return "or";
    case INST_NOT:
        return "not";
    case INST_EMPTY:
        return "empty";
    case INST_POP_AT:
        return "pop_at";
    case INST_POP:
        return "pop";
    case INST_RET:
        return "ret";
    case INST_CALL:
        return "call";
    case INST_ASWAP:
        return "aswap";
    case INST_RSWAP:
        return "rswap";
    case INST_NATIVE:
        return "native";
    default:
        return NULL; // Invalid instruction
    }
}

bool has_operand_function(Inst_Type inst)
{
    switch (inst)
    {
    case INST_NOP:
        return 0;
    case INST_SPUSH:
        return 1;
    case INST_UPUSH:
        return 1;
    case INST_FPUSH:
        return 1;
    case INST_ADUP:
        return 1;
    case INST_RDUP:
        return 1;
    case INST_SPLUS:
        return 0;
    case INST_UPLUS:
        return 0;
    case INST_FPLUS:
        return 0;
    case INST_SMINUS:
        return 0;
    case INST_UMINUS:
        return 0;
    case INST_FMINUS:
        return 0;
    case INST_SMULT:
        return 0;
    case INST_UMULT:
        return 0;
    case INST_FMULT:
        return 0;
    case INST_SDIV:
        return 0;
    case INST_UDIV:
        return 0;
    case INST_FDIV:
        return 0;
    case INST_JMP:
        return 1;
    case INST_HALT:
        return 0;
    case INST_FJMP_IF:
        return 1;
    case INST_UJMP_IF:
        return 1;
    case INST_EQ:
        return 0;
    case INST_LSR:
        return 1;
    case INST_ASR:
        return 1;
    case INST_SL:
        return 1;
    case INST_AND:
        return 0;
    case INST_OR:
        return 0;
    case INST_NOT:
        return 0;
    case INST_EMPTY:
        return 0;
    case INST_POP_AT:
        return 1;
    case INST_POP:
        return 0;
    case INST_RET:
        return 0;
    case INST_CALL:
        return 1;
    case INST_RSWAP:
        return 1;
    case INST_ASWAP:
        return 1;
    case INST_NATIVE:
        return 1;
    default:
        return -1; // Invalid instruction
    }
}

uint8_t get_operand_type(Inst_Type inst)
{
    switch (inst)
    {
    // Signed arithmetic instructions
    case INST_SPUSH:
        return TYPE_SIGNED_64INT;

    // Unsigned arithmetic instructions
    case INST_UPUSH:
        return TYPE_UNSIGNED_64INT;

    // Floating-point arithmetic instructions
    case INST_FPUSH:
        return TYPE_DOUBLE;

    // Shift operations, which often use unsigned integers
    case INST_LSR: // Logical Shift Right
    case INST_ASR: // Arithmetic Shift Right
    case INST_SL:  // Shift Left
        return TYPE_UNSIGNED_64INT;

    // Control flow instructions
    case INST_JMP:
    case INST_UJMP_IF:
    case INST_FJMP_IF:
        return TYPE_UNSIGNED_64INT;

    // Stack manipulation
    case INST_POP_AT:
        return TYPE_UNSIGNED_64INT;

    case INST_ADUP:
    case INST_RDUP:
        return TYPE_UNSIGNED_64INT;

    case INST_ASWAP:
    case INST_RSWAP:
        return TYPE_UNSIGNED_64INT;

    case INST_CALL:
        return TYPE_UNSIGNED_64INT;

    case INST_NATIVE:
        return TYPE_UNSIGNED_64INT;
    default:
        return 0; // No specific operand type or invalid instruction
    }
}

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

/* const char *inst_type_as_cstr(Inst_Type type)
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
    case INST_ASR:
        return "INST_ASR";
    case INST_LSR:
        return "INST_LSR";
    case INST_SL:
        return "INST_SL";
    case INST_AND:
        return "INST_AND";
    case INST_OR:
        return "INST_OR";
    case INST_NOT:
        return "INST_NOT";
    default:
        assert(0 && "inst_type_as_cstr: unreachable");
        break;
    }
} */

/* const char *inst_type_as_asm_str(Inst_Type type)
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
    case INST_ASR:
        return "asr";
    case INST_LSR:
        return "lsr";
    case INST_SL:
        return "sl";
    case INST_NOT:
        return "not";
    case INST_AND:
        return "and";
    case INST_OR:
        return "or";
    default:
        assert(0 && "inst_type_as_asm_str: unreachable");
        break;
    }
} */

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

void vm_native_push(VirtualMachine *vm, native native_func)
{
    assert(vm->natives_size < natives_capacity);
    vm->natives[vm->natives_size++] = native_func;
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

                int64_t element = return_value_signed(vm->stack[i]);
                fprintf(stream, "%lld\n", element);
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

static int handle_swap(VirtualMachine *vm, Inst inst)
{
    uint64_t operand = return_value_unsigned(inst.operand);
    if (inst.type == INST_ASWAP)
    {
        if (operand >= vm->stack_size)
        {
            return TRAP_STACK_OVERFLOW;
        }

        double temp = vm->stack[vm->stack_size - 1];
        vm->stack[vm->stack_size - 1] = vm->stack[operand];
        vm->stack[operand] = temp;
    }
    else
    {
        if (vm->stack_size - operand < 0)
        {
            return TRAP_STACK_UNDERFLOW;
        }

        double temp = vm->stack[vm->stack_size - 1];
        vm->stack[vm->stack_size - 1] = vm->stack[vm->stack_size - 1 - operand];
        vm->stack[vm->stack_size - 1 - operand] = temp;
    }

    vm->instruction_pointer++;
    return TRAP_OK;
}

static int handle_native(VirtualMachine *vm, Inst inst)
{
    uint64_t index = return_value_unsigned(inst.operand);
    if (index > vm->natives_size)
    {
        return TRAP_ILLEGAL_OPERAND;
    }

    Trap ret_val = (vm->natives[index])(vm);

    vm->instruction_pointer++;
    return ret_val;
}

static int handle_shift(VirtualMachine *vm, Inst inst, bool is_arithmetic)
{
    if (vm->stack_size < 1)
        return TRAP_STACK_UNDERFLOW;

    uint64_t shift = return_value_unsigned(inst.operand);
    uint64_t value = return_value_unsigned(vm->stack[vm->stack_size - 1]);

    if (is_arithmetic && (inst.type == INST_ASR))
    {
        int64_t signed_value = (int64_t)value;
        signed_value >>= shift;
        set_signed_64int(&vm->stack[vm->stack_size - 1], signed_value);
    }
    else
    {
        value = (inst.type == INST_SL) ? (value << shift) : (value >> shift);
        set_unsigned_64int(&vm->stack[vm->stack_size - 1], value);
    }

    vm->instruction_pointer++;
    return TRAP_OK;
}

static int handle_push(VirtualMachine *vm, Inst inst)
{
    if (vm->stack_size >= vm_stack_capacity)
        return TRAP_STACK_OVERFLOW;

    // Push the operand onto the stack
    vm->stack[vm->stack_size++] = inst.operand;

    // Increment the instruction pointer to move to the next instruction
    vm->instruction_pointer++;

    return TRAP_OK;
}

static int handle_arithmetic(VirtualMachine *vm, Inst inst)
{
    if (vm->stack_size < 2)
    {
        return TRAP_STACK_UNDERFLOW;
    }

    // Second operand is the top of the stack
    double result = 0.0;
    int64_t a_signed = 0, b_signed = 0;
    uint64_t a_unsigned = 0, b_unsigned = 0;
    double a_float = 0.0, b_float = 0.0;

    // Retrieve the operands based on the instruction type
    switch (inst.type)
    {
    case INST_FPLUS:
    case INST_FMINUS:
    case INST_FMULT:
    case INST_FDIV:
        a_float = vm->stack[vm->stack_size - 2]; // First operand from the given index
        b_float = vm->stack[vm->stack_size - 1]; // Second operand is the top of the stack
        break;
    case INST_SPLUS:
    case INST_SMINUS:
    case INST_SMULT:
    case INST_SDIV:
        a_signed = return_value_signed(vm->stack[vm->stack_size - 2]);
        b_signed = return_value_signed(vm->stack[vm->stack_size - 1]);
        break;
    case INST_UPLUS:
    case INST_UMINUS:
    case INST_UMULT:
    case INST_UDIV:
        a_unsigned = return_value_unsigned(vm->stack[vm->stack_size - 2]);
        b_unsigned = return_value_unsigned(vm->stack[vm->stack_size - 1]);
        break;
    default:
        return TRAP_ILLEGAL_INSTRUCTION;
    }

    // Perform the arithmetic operation
    switch (inst.type)
    {
    case INST_FPLUS:
        result = a_float + b_float;
        break;
    case INST_SPLUS:
        result = (a_signed + b_signed);
        break;
    case INST_UPLUS:
        result = (double)(a_unsigned + b_unsigned);
        break;
    case INST_FMINUS:
        result = a_float - b_float;
        break;
    case INST_SMINUS:
        result = (double)(a_signed - b_signed);
        break;
    case INST_UMINUS:
        result = (double)(a_unsigned - b_unsigned);
        break;
    case INST_FMULT:
        result = a_float * b_float;
        break;
    case INST_SMULT:
        result = (double)(a_signed * b_signed);
        break;
    case INST_UMULT:
        result = (double)(a_unsigned * b_unsigned);
        break;
    case INST_FDIV:
        if (b_float == 0.0)
            return TRAP_DIV_BY_ZERO;
        result = a_float / b_float;
        break;
    case INST_SDIV:
        if (b_signed == 0)
            return TRAP_DIV_BY_ZERO;
        result = (a_signed / b_signed);
        break;
    case INST_UDIV:
        if (b_unsigned == 0)
            return TRAP_DIV_BY_ZERO;
        result = (a_unsigned / b_unsigned);
        break;
    default:
        return TRAP_ILLEGAL_INSTRUCTION;
    }

    // Store the result in the location of the first operand
    if (inst.type == INST_SPLUS || inst.type == INST_SMINUS || inst.type == INST_SMULT || inst.type == INST_SDIV)
    {
        set_signed_64int(&vm->stack[vm->stack_size - 2], (int64_t)result);
    }
    else if (inst.type == INST_UPLUS || inst.type == INST_UMINUS || inst.type == INST_UMULT || inst.type == INST_UDIV)
    {
        set_unsigned_64int(&vm->stack[vm->stack_size - 2], (uint64_t)result);
    }
    else
    {
        vm->stack[vm->stack_size - 2] = result;
    }

    // Reduce the stack size by one, popping the top operand
    vm->stack_size--;

    // Move the instruction pointer to the next instruction
    vm->instruction_pointer++;

    return TRAP_OK;
}

static int handle_functions(VirtualMachine *vm, Inst inst)
{
    if (inst.type == INST_CALL)
    {
        uint64_t addr = return_value_unsigned(inst.operand);
        if (vm->stack[vm->stack_size] >= vm_stack_capacity)
        {
            return TRAP_STACK_OVERFLOW;
        }

        set_unsigned_64int(&vm->stack[vm->stack_size++], (vm->instruction_pointer + 1));
        vm->instruction_pointer = addr;
    }
    else if (inst.type == INST_RET)
    {
        vm->instruction_pointer = return_value_unsigned(vm->stack[vm->stack_size - 1]);
        vm->stack_size--;
    }

    return TRAP_OK;
}

static int handle_jump(VirtualMachine *vm, Inst inst)
{
    uint64_t jump_addr = return_value_unsigned(inst.operand);

    if (jump_addr >= vm->program_size)
        return TRAP_ILLEGAL_JMP;

    if (inst.type == INST_UJMP_IF)
    {
        if (vm->stack_size < 1)
            return TRAP_STACK_UNDERFLOW;
        if (return_value_unsigned(vm->stack[vm->stack_size - 1]))
        {
            vm->stack_size--;
            vm->instruction_pointer = jump_addr;
        }
        else
        {
            vm->instruction_pointer++;
        }
    }
    else if (inst.type == INST_FJMP_IF)
    {
        if (vm->stack_size < 1)
            return TRAP_STACK_UNDERFLOW;
        if (!((vm->stack[vm->stack_size - 1]) < EPSILON))
        {
            vm->stack_size--;
            vm->instruction_pointer = jump_addr;
        }
        else
        {
            vm->instruction_pointer++;
        }
    }
    else
    {
        vm->instruction_pointer = jump_addr;
    }

    return TRAP_OK;
}

static int handle_comparison(VirtualMachine *vm)
{
    if (vm->stack_size < 2)
        return TRAP_STACK_UNDERFLOW;

    vm->stack[vm->stack_size - 2] = (vm->stack[vm->stack_size - 1] == vm->stack[vm->stack_size - 2]);
    vm->stack_size--;
    vm->instruction_pointer++;

    return TRAP_OK;
}

static int handle_adup(VirtualMachine *vm, Inst inst)
{
    uint64_t index = return_value_unsigned(inst.operand);

    if (vm->stack_size >= vm_stack_capacity || index > vm->stack_size)
        return TRAP_STACK_OVERFLOW;

    vm->stack[vm->stack_size] = vm->stack[index];
    vm->stack_size++;
    vm->instruction_pointer++;

    return TRAP_OK;
}

static int handle_rdup(VirtualMachine *vm, Inst inst)
{
    uint64_t index = return_value_unsigned(inst.operand);

    if (vm->stack_size >= vm_stack_capacity)
        return TRAP_STACK_OVERFLOW;
    if ((int64_t)vm->stack_size - 1 - (int64_t)index < 0)
        return TRAP_STACK_UNDERFLOW;

    vm->stack[vm->stack_size] = vm->stack[vm->stack_size - 1 - index];
    vm->stack_size++;
    vm->instruction_pointer++;

    return TRAP_OK;
}

static int handle_bitwise(VirtualMachine *vm, Inst inst)
{
    if (vm->stack_size < (inst.type == INST_NOT ? 1 : 2))
        return TRAP_STACK_UNDERFLOW;

    uint64_t a = return_value_unsigned(vm->stack[vm->stack_size - 1]);
    uint64_t b = (inst.type != INST_NOT) ? return_value_unsigned(vm->stack[vm->stack_size - 2]) : 0;
    uint64_t result;

    bool both_nan = is_nan(vm->stack[vm->stack_size - 1]) &&
                    (inst.type == INST_NOT || is_nan(vm->stack[vm->stack_size - 2]));

    if (both_nan)
    {
        switch (inst.type)
        {
        case INST_AND:
            result = (a & b) | 0xFFF1000000000000;
            break;
        case INST_OR:
            result = (a | b) | 0xFFF1000000000000;
            break;
        case INST_NOT:
            result = (~a | 0xFFF1000000000000);
            break;
        default:
            return TRAP_ILLEGAL_INSTRUCTION;
        }
    }
    else if (!is_nan(vm->stack[vm->stack_size - 1]) && (inst.type == INST_NOT || !is_nan(vm->stack[vm->stack_size - 2])))
    {
        switch (inst.type)
        {
        case INST_AND:
            result = a & b;
            break;
        case INST_OR:
            result = a | b;
            break;
        case INST_NOT:
            result = ~a;
            break;
        default:
            return TRAP_ILLEGAL_INSTRUCTION;
        }
    }
    else
    {
        return TRAP_ILLEGAL_OPERATION;
    }

    set_unsigned_64int(&vm->stack[vm->stack_size - (inst.type == INST_NOT ? 1 : 2)], result);
    if (inst.type != INST_NOT)
        vm->stack_size--;
    vm->instruction_pointer++;
    return TRAP_OK;
}

static int handle_empty(VirtualMachine *vm)
{
    if (vm->stack_size >= 1)
    {
        vm->stack_size++;
        set_unsigned_64int(&vm->stack[vm->stack_size - 1], 0);
    }
    else
    {
        vm->stack_size++;
        set_unsigned_64int(&vm->stack[vm->stack_size - 1], 1);
    }

    vm->instruction_pointer++;
    return TRAP_OK;
}

static int handle_pop(VirtualMachine *vm, Inst inst)
{
    if (inst.type == INST_POP_AT)
    {
        size_t index_to_pop = return_value_unsigned(inst.operand);
        
        if (index_to_pop >= vm->stack_size)
        {
            return TRAP_STACK_OVERFLOW;
        }

        for (size_t i = index_to_pop; i < vm->stack_size - 1; i++)
        {
            vm->stack[i] = vm->stack[i + 1];
        }
    }

    vm->stack_size--;
    vm->instruction_pointer++;
    return TRAP_OK;
}

// Main execution function
int vm_execute_at_inst_pointer(VirtualMachine *vm)
{
    Inst inst = vm->program[vm->instruction_pointer];

    switch (inst.type)
    {
    case INST_ASR:
    case INST_LSR:
    case INST_SL:
        return handle_shift(vm, inst, inst.type == INST_ASR);

    case INST_NOP:
        vm->instruction_pointer++;
        return TRAP_OK;

    case INST_FPUSH:
    case INST_SPUSH:
    case INST_UPUSH:
        return handle_push(vm, inst);

    case INST_FPLUS:
    case INST_SPLUS:
    case INST_UPLUS:
    case INST_FMINUS:
    case INST_SMINUS:
    case INST_UMINUS:
    case INST_FMULT:
    case INST_SMULT:
    case INST_UMULT:
    case INST_SDIV:
    case INST_UDIV:
    case INST_FDIV:
        return handle_arithmetic(vm, inst);

    case INST_HALT:
        vm->halt = 1;
        return TRAP_OK;

    case INST_JMP:
    case INST_FJMP_IF:
    case INST_UJMP_IF:
        return handle_jump(vm, inst);

    case INST_EQ:
        return handle_comparison(vm);

    case INST_ADUP:
        return handle_adup(vm, inst);
    case INST_RDUP:
        return handle_rdup(vm, inst);

    case INST_AND:
    case INST_OR:
    case INST_NOT:
        return handle_bitwise(vm, inst);
    case INST_EMPTY:
        return handle_empty(vm);
    case INST_POP:
    case INST_POP_AT:
        return handle_pop(vm, inst);

    case INST_ASWAP:
    case INST_RSWAP:
        return handle_swap(vm, inst);

    case INST_CALL:
    case INST_RET:
        return handle_functions(vm, inst);

    case INST_NATIVE:
        return handle_native(vm, inst);

    default:
        return TRAP_ILLEGAL_INSTRUCTION;
    }
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

void label_free()
{
    free((void *)not_resolved_yet);
    free((void *)label_array);
}

void vm_init(VirtualMachine *vm, char *source_code)
{
    vm->stack = malloc(sizeof(double) * vm_stack_capacity);
    if (!vm->stack)
    {
        fprintf(stderr, "ERROR: stack allocation failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    vm->program = malloc(sizeof(Inst) * vm_program_capacity);
    if (!vm->program)
    {
        fprintf(stderr, "ERROR: code section allocation failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    vm->natives = malloc(sizeof(native) * natives_capacity);
    if (!vm->natives)
    {
        fprintf(stderr, "ERROR: native function pointer array allocation failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    vm->natives_size = 0;

    vm_header_ header = vm_load_program_from_file(vm->program, source_code);

    vm->program_size = header.code_section_size;
    vm->has_start = header.has_start;
    if (vm->has_start)
    {
        vm->instruction_pointer = header.start_location;
    }
    else
    {
        fprintf(stderr, "ERROR: 'start' not found in %s\n", source_code);
        exit(EXIT_FAILURE);
    }
    vm->stack_size = 0;
    vm->halt = 0;
}

void vm_internal_free(VirtualMachine *vm)
{
    free((void *)vm->program);
    free((void *)vm->natives);
    free((void *)vm->stack);
}

int vm_exec_program(VirtualMachine *vm, int64_t limit, bool debug)
{
    int ret;
    if (vm->program[vm->program_size - 1].type != INST_HALT)
    {
        return TRAP_NO_HALT_FOUND;
    }
    while (!vm->halt && limit != 0)
    {
        if (debug)
        {
            getchar();
            fprintf(stdout, "%s\n", get_inst_name(vm->program[vm->instruction_pointer].type));
        }
        ret = vm_execute_at_inst_pointer(vm);
        if (debug)
        {
            vm_dump_stack(stdout, vm);
        }
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
    // vm_dump_stack(stdout, vm);
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

void vm_save_program_to_file(Inst *program, vm_header_ header, const char *file_path)
{
    FILE *f = fopen(file_path, "wb");
    if (!f)
    {
        fprintf(stderr, "ERROR: Could not open file '%s': %s\n", file_path, strerror(errno));
        exit(EXIT_FAILURE);
    }

    fwrite(&header, sizeof(header), 1, f);
    if (ferror(f)) // did some error occur due to the last stdio function call on f?
    {
        fprintf(stderr, "ERROR: Could not write to file '%s': %s\n", file_path, strerror(errno));
        fclose(f);
        exit(EXIT_FAILURE);
    }

    fwrite(program, sizeof(Inst), header.code_section_size, f);
    if (ferror(f)) // did some error occur due to the last stdio function call on f?
    {
        fprintf(stderr, "ERROR: Could not write to file '%s': %s\n", file_path, strerror(errno));
        fclose(f);
        exit(EXIT_FAILURE);
    }

    fclose(f);
}

vm_header_ vm_load_program_from_file(Inst *program, const char *file_path)
{
    FILE *f = fopen(file_path, "rb");
    if (!f)
    {
        fprintf(stderr, "ERROR: Could not open file '%s': %s\n", file_path, strerror(errno));
        exit(EXIT_FAILURE);
    }

    vm_header_ header = {0};
    int ret_val = fread(&header, sizeof(vm_header_), 1, f);
    if (ferror(f))
    {
        fclose(f);
        fprintf(stderr, "ERROR: Could not read file '%s': %s\n", file_path, strerror(errno));
        exit(EXIT_FAILURE);
    }

    assert(header.code_section_size <= vm_program_capacity * sizeof(Inst));

    if (fseek(f, header.code_section_offset, SEEK_SET))
    {
        fclose(f);
        fprintf(stderr, "ERROR: Could not read file '%s': %s\n", file_path, strerror(errno));
        exit(EXIT_FAILURE);
    }

    ret_val = fread(program, sizeof(Inst), header.code_section_size, f);
    if (ferror(f))
    {
        fclose(f);
        fprintf(stderr, "ERROR: Could not read file '%s': %s\n", file_path, strerror(errno));
        exit(EXIT_FAILURE);
    }

    assert(header.code_section_size <= vm_program_capacity * sizeof(Inst));
    fclose(f);

    return header;
}

Inst vm_translate_line(String_View line, size_t current_program_counter)
{
    String_View inst_name = sv_chop_by_delim(&line, ' ');

    sv_trim_left(&line);
    sv_trim_right(&line);

    bool has_operand_value = line.count > 0;

    for (size_t i = 1; i < (size_t)INST_COUNT; i++) // run uptil the second last instruction in the enum since it's actually the last valid defined instruction
    {
        if (sv_eq(inst_name, cstr_as_sv(get_inst_name(i))))
        {
            if (has_operand_function(i) != has_operand_value)
            {
                fprintf(stderr, "Line Number %zu -> ERROR: %s %s an operand\n",
                        line_no, get_inst_name(i), has_operand_function(i) ? "requires" : "doesn't require");
                compilation_successful = false;
                return (Inst){0};
            }

            if (!has_operand_function(i))
            {
                return (Inst){.type = i};
            }

            double operand = sv_to_value(&line);
            if (str_errno == FAILURE)
            {
                if (i == INST_JMP || i == INST_UJMP_IF || i == INST_FJMP_IF || i == INST_CALL)
                {
                    push_to_not_resolved_yet(line, current_program_counter);
                    return (Inst){.type = i, .operand = 0};
                }

                fprintf(stderr, "Line Number %zu -> ERROR: %.*s is not a valid value\n",
                        line_no, (int)line.count, line.data);
                compilation_successful = false;
                return (Inst){0};
            }
            else if (str_errno == OPERAND_OVERFLOW)
            {
                fprintf(stderr, "Line Number %zu -> ERROR: %.*s overflows a 64 bit %s value\n",
                        line_no, (int)line.count, line.data,
                        get_operand_type(i) == TYPE_SIGNED_64INT ? "signed" : "unsigned");
                compilation_successful = false;
                return (Inst){0};
            }

            if (get_operand_type(i) == TYPE_SIGNED_64INT)
            {
                set_signed_64int(&operand, (int64_t)operand);
            }
            else if (get_operand_type(i) == TYPE_UNSIGNED_64INT)
            {
                if (is_fraction || is_negative)
                {
                    fprintf(stderr, "Line Number %zu -> ERROR: illegal operand value for %s instruction: %.*s\n"
                                    "Must be an unsigned integral value\n",
                            line_no, get_inst_name(i), (int)line.count, line.data);
                    compilation_successful = false;
                    return (Inst){0};
                }
                set_unsigned_64int(&operand, (uint64_t)operand);
            }

            return (Inst){.type = i, .operand = operand};
        }
    }

    fprintf(stderr, "Line Number %zu -> ERROR: invalid instruction %.*s\n",
            line_no, (int)inst_name.count, inst_name.data);
    compilation_successful = false;
    return (Inst){0};
}

static void process_label(String_View label, size_t program_size)
{
    if (label_array_counter >= label_capacity)
    {
        fprintf(stderr, "Line Number %zu -> ERROR: label capacity exceeded at label: %.*s\n",
                line_no, (int)label.count, label.data);
        compilation_successful = false;
        return;
    }
    double operand = 0;
    set_unsigned_64int(&operand, (uint64_t)program_size);
    push_to_label_array(label, operand);
}

static void resolve_labels(Inst *program)
{
    for (size_t i = 0; i < not_resolved_yet_counter; i++)
    {
        for (size_t j = 0; j < label_array_counter; j++)
        {
            if (sv_eq(not_resolved_yet[i].label, label_array[j].label))
            {
                program[not_resolved_yet[i].inst_location].operand = label_array[j].label_pointing_location;
                not_resolved_yet[i].resolved = true;
                break;
            }
        }
    }
}

static void check_unresolved_labels()
{
    bool has_unresolved = false;
    for (size_t i = 0; i < not_resolved_yet_counter; i++)
    {
        if (!not_resolved_yet[i].resolved)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: cannot resolve label: %.*s\n",
                    line_no, (int)not_resolved_yet[i].label.count, not_resolved_yet[i].label.data);
            has_unresolved = true;
        }
    }
    if (has_unresolved)
    {
        compilation_successful = false;
    }
}

int64_t check_start()
{
    String_View start_ = cstr_as_sv("start");
    for (size_t i = 0; i < label_array_counter; i++)
    {
        if (sv_eq(label_array[i].label, start_))
        {
            return return_value_unsigned(label_array[i].label_pointing_location);
        }
    }

    return -1;
}

vm_header_ vm_translate_source(String_View source, Inst *program, size_t program_capacity)
{
    size_t program_size = 0;

    while (source.count > 0)
    {
        if (program_size >= program_capacity)
        {
            fprintf(stderr, "Program Too Big\n");
            compilation_successful = false;
            break;
        }

        line_no++;
        String_View line = sv_chop_by_delim(&source, '\n');
        line = sv_chop_by_delim(&line, ';'); // Remove comments
        sv_trim_left(&line);
        sv_trim_right(&line);

        if (line.count == 0)
            continue; // Ignore empty lines

        // Check for a label
        String_View label = sv_chop_by_delim(&line, ':');
        if (*(line.data - 1) == ':')
        { // If there's a label
            process_label(label, program_size);
            if (line.count > 0)
            { // instruction remaining after the label
                sv_trim_left(&line);
                program[program_size] = vm_translate_line(line, program_size);
                program_size++;
            }
        }
        else
        {
            program[program_size] = vm_translate_line(label, program_size);
            program_size++;
            continue;
        }
    }

    if (program_size > 0 && program[program_size - 1].type != INST_HALT)
    {
        fprintf(stderr, "ERROR: halt required to mark the code end\n");
        compilation_successful = false;
    }

    if (compilation_successful)
    {
        resolve_labels(program);
        check_unresolved_labels();
    }

    if (!compilation_successful)
    {
        fprintf(stderr, "Compilation Failed\n");
    }

    bool has_start = true;

    int64_t ret_val = check_start();
    if (ret_val == -1)
    {
        has_start = false;
    }
    return (vm_header_){.code_section_size = program_size, .has_start = has_start, .data_section_offset = 0, .code_section_offset = sizeof(vm_header_), .start_location = ret_val};
}

String_View slurp_file(const char *file_path)
{
    int ret = 0;
    FILE *f = fopen(file_path, "r");
    if (!f)
    {
        fprintf(stderr, "ERROR: Couldn't open file: '%s': %s\n", file_path, strerror(errno));
        compilation_successful = false;
    }

    if (fseek(f, 0, SEEK_END))
    {
        fclose(f);
        fprintf(stderr, "ERROR: Could not read file '%s': %s\n", file_path, strerror(errno));
        compilation_successful = false;
    }

    ret = ftell(f);
    if (ret == -1)
    {
        fclose(f);
        fprintf(stderr, "ERROR: Could not read file '%s': %s\n", file_path, strerror(errno));
        compilation_successful = false;
    }

    // ret now contains the number of bytes in the file
    if (fseek(f, 0, SEEK_SET))
    {
        fclose(f);
        fprintf(stderr, "ERROR: Could not read file '%s': %s\n", file_path, strerror(errno));
        compilation_successful = false;
    }

    char *buffer = malloc(ret);
    if (!buffer)
    {
        fprintf(stderr, "ERROR: Couldn't allocate memory for file '%s': %s\n", file_path, strerror(errno));
        compilation_successful = false;
    }

    int ret_val = fread(buffer, 1, ret, f);
    if (ferror(f))
    {
        fclose(f);
        fprintf(stderr, "Could not read the file '%s': %s\n", file_path, strerror(errno));
        compilation_successful = false;
    }

    fclose(f);

    return (String_View){
        .count = ret,
        .data = buffer,
    };
}

#endif // _VM_IMPLEMENTATION
#endif // _VM

/* size_t vm_translate_source(String_View source, Inst *program, size_t program_capacity)
{
    size_t program_size = 0;

    while (source.count > 0) // Process each line
    {
        if (program_size >= program_capacity)
        {
            fprintf(stderr, "Program Too Big\n");
            compilation_successful = false;
        }

        line_no++;
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
                fprintf(stderr, "Line Number %zu -> ERROR: label capacity exceeded at label: %.*s\n", line_no, (int)label.count, label.data);
            }
            double operand = 0;
            set_unsigned_64int(&operand, (uint64_t)program_size);
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

    if (program[program_size - 1].type != INST_HALT)
    {
        fprintf(stderr, "ERROR: halt required to mark the code end\n");
        compilation_successful = false;
    }

    if (!compilation_successful)
    {
        fprintf(stderr, "Compilation Failed\n");
        compilation_successful = false;
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
            fprintf(stderr, "Line Number %zu -> ERROR: cannot resolve label: %.*s\n", line_no, (int)not_resolved_yet[i].label.count, not_resolved_yet[i].label.data);
            flag = true;
        }
    }

    if (flag)
    {
        compilation_successful = false;
    }

    return program_size;
} */

/* Inst vm_translate_line(String_View line, size_t current_program_counter)
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
            fprintf(stderr, "Line Number %zu -> fpush requires an operand\n", line_no);
            compilation_successful = false;
        }
        // printf("%.*s%d\n", (int)line.count, line.data, (int)line.count);
        double operand = sv_to_value(&line);
        // printf("%lf\n", operand);
        if (str_errno == FAILURE)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: %.*s is not a valid value\n", line_no, (int)line.count, line.data);
            compilation_successful = false;
        }
        else if (str_errno == OPERAND_OVERFLOW)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: %.*s overflows a 64 bit signed value\n", line_no, (int)line.count, line.data);
            compilation_successful = false;
        }

        return (Inst){.type = INST_FPUSH, .operand = operand};
    }
    if (sv_eq(inst_name, cstr_as_sv("spush")))
    {
        if (!has_operand)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: spush requires an operand\n", line_no);
            compilation_successful = false;
        }
        // printf("%.*s%d\n", (int)line.count, line.data, (int)line.count);
        double operand = sv_to_value(&line);
        // printf("%lf\n", operand);
        if (str_errno == FAILURE)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: %.*s is not a valid value\n", line_no, (int)line.count, line.data);
            compilation_successful = false;
        }
        else if (str_errno == OPERAND_OVERFLOW)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: %.*s overflows a 64 bit signed value\n", line_no, (int)line.count, line.data);
            compilation_successful = false;
        }

        int64_t int1 = (int64_t)operand;
        // printf("%lld %lf\n", int1, operand);
        set_signed_64int(&operand, int1);
        // printf("%lld %lf\n", int1, operand);
        // printf("%lld %lld\n", return_value_signed(operand), int1);

        return (Inst){.type = INST_SPUSH, .operand = operand};
    }
    if (sv_eq(inst_name, cstr_as_sv("upush")))
    {
        if (!has_operand)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: upush requires an operand\n", line_no);
            compilation_successful = false;
        }
        // printf("%.*s%d\n", (int)line.count, line.data, (int)line.count);
        double operand = sv_to_value(&line);
        // printf("%lf\n", operand);
        if (str_errno == FAILURE)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: %.*s is not a valid value\n", line_no, (int)line.count, line.data);
            compilation_successful = false;
        }
        else if (str_errno == OPERAND_OVERFLOW)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: %.*s overflows a 64 bit signed value\n", line_no, (int)line.count, line.data);
        }

        uint64_t int1 = (uint64_t)operand;
        // printf("%lld %lf\n", int1, operand);
        set_unsigned_64int(&operand, int1);
        // printf("%lld %lf\n", int1, operand);
        // printf("%lld %lld\n", return_value_signed(operand), int1);

        return (Inst){.type = INST_UPUSH, .operand = operand};
    }
    else if (sv_eq(inst_name, cstr_as_sv("halt")))
    {
        if (has_operand)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: halt doesn't require an operand\n", line_no);
            compilation_successful = false;
        }

        // printf("yes\n");
        return (Inst){.type = INST_HALT};
    }
    else if (sv_eq(inst_name, cstr_as_sv("dup")))
    {
        if (!has_operand)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: dup requires an operand\n", line_no);
            compilation_successful = false;
        }
        double operand = sv_to_value(&line);
        if (str_errno == FAILURE)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: %.*s is not a valid value\n", line_no, (int)line.count, line.data);
            compilation_successful = false;
        }
        else if (str_errno == OPERAND_OVERFLOW)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: %.*s overflows a 64 bit unsigned value\n", line_no, (int)line.count, line.data);
            compilation_successful = false;
        }

        if (is_fraction)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: illegal operand value for dup instruction: %.*s\nMust be an unsigned value\n", line_no, (int)line.count, line.data);
            compilation_successful = false;
        }
        else if (is_negative)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: illegal operand value for dup instruction: %.*s\nMust be an unsigned value\n", line_no, (int)line.count, line.data);
            compilation_successful = false;
        }
        else
        {
            set_unsigned_64int(&operand, (uint64_t)operand);
            return (Inst){.type = INST_DUP, .operand = operand};
        }
    }
    else if (sv_eq(inst_name, cstr_as_sv("jmp")))
    {
        if (!has_operand)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: jmp requires an operand\n", line_no);
            compilation_successful = false;
        }
        double operand = sv_to_value(&line);
        if (str_errno == FAILURE)
        {
            push_to_not_resolved_yet(line, current_program_counter);
            return (Inst){.type = INST_JMP, .operand = 0};
        }
        else if (str_errno == OPERAND_OVERFLOW)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: %.*s overflows a 64 bit unsigned value\n", line_no, (int)line.count, line.data);
            compilation_successful = false;
        }

        if (is_fraction)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: illegal operand value for jmp instruction: %.*s\nMust be an unsigned integral value\n", line_no, (int)line.count, line.data);
            compilation_successful = false;
        }
        else if (is_negative)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: illegal operand value for jmp instruction: %.*s\nMust be an unsigned value\n", line_no, (int)line.count, line.data);
            compilation_successful = false;
        }
        else
        {
            set_unsigned_64int(&operand, (uint64_t)operand);
            return (Inst){.type = INST_JMP, .operand = operand};
        }
    }
    else if (sv_eq(inst_name, cstr_as_sv("jmp_if")))
    {
        if (!has_operand)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: jmp_if requires an operand\n", line_no);
            compilation_successful = false;
        }
        double operand = sv_to_value(&line);
        if (str_errno == FAILURE)
        {
            push_to_not_resolved_yet(line, current_program_counter);
            return (Inst){.type = INST_JMP_IF, .operand = 0};
        }
        else if (str_errno == OPERAND_OVERFLOW)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: %.*s overflows a 64 bit unsigned value\n", line_no, (int)line.count, line.data);
            compilation_successful = false;
        }

        if (is_fraction)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: illegal operand value for jmp_if instruction: %.*s\nMust be an unsigned integral value\n", line_no, (int)line.count, line.data);
            compilation_successful = false;
        }
        else if (is_negative)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: illegal operand value for jmp_if instruction: %.*s\nMust be an unsigned value\n", line_no, (int)line.count, line.data);
            compilation_successful = false;
        }
        else
        {
            set_unsigned_64int(&operand, (uint64_t)operand);
            return (Inst){.type = INST_JMP_IF, .operand = operand};
        }
    }
    else if (sv_eq(inst_name, cstr_as_sv("splus")))
    {
        if (has_operand)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: splus doesn't require an operand\n", line_no);
            compilation_successful = false;
        }

        // printf("yes\n");
        return (Inst){.type = INST_SPLUS};
    }
    else if (sv_eq(inst_name, cstr_as_sv("uplus")))
    {
        if (has_operand)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: uplus doesn't require an operand\n", line_no);
            compilation_successful = false;
        }

        // printf("yes\n");
        return (Inst){.type = INST_UPLUS};
    }
    else if (sv_eq(inst_name, cstr_as_sv("fplus")))
    {
        if (has_operand)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: fplus doesn't require an operand\n", line_no);
            compilation_successful = false;
        }

        // printf("yes\n");
        return (Inst){.type = INST_FPLUS};
    }
    else if (sv_eq(inst_name, cstr_as_sv("sminus")))
    {
        if (has_operand)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: sminus doesn't require an operand\n", line_no);
            compilation_successful = false;
        }

        // printf("yes\n");
        return (Inst){.type = INST_SMINUS};
    }
    else if (sv_eq(inst_name, cstr_as_sv("uminus")))
    {
        if (has_operand)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: uminus doesn't require an operand\n", line_no);
            compilation_successful = false;
        }

        // printf("yes\n");
        return (Inst){.type = INST_UMINUS};
    }
    else if (sv_eq(inst_name, cstr_as_sv("fminus")))
    {
        if (has_operand)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: fminus doesn't require an operand\n", line_no);
            compilation_successful = false;
        }

        // printf("yes\n");
        return (Inst){.type = INST_FMINUS};
    }
    else if (sv_eq(inst_name, cstr_as_sv("smult")))
    {
        if (has_operand)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: smult doesn't require an operand\n", line_no);
            compilation_successful = false;
        }

        // printf("yes\n");
        return (Inst){.type = INST_SMULT};
    }
    else if (sv_eq(inst_name, cstr_as_sv("umult")))
    {
        if (has_operand)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: umult doesn't require an operand\n", line_no);
            compilation_successful = false;
        }

        // printf("yes\n");
        return (Inst){.type = INST_UMULT};
    }
    else if (sv_eq(inst_name, cstr_as_sv("fmult")))
    {
        if (has_operand)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: fmult doesn't require an operand\n", line_no);
            compilation_successful = false;
        }

        // printf("yes\n");
        return (Inst){.type = INST_FMULT};
    }
    else if (sv_eq(inst_name, cstr_as_sv("sdiv")))
    {
        if (has_operand)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: sdiv doesn't require an operand\n", line_no);
            compilation_successful = false;
        }

        // printf("yes\n");
        return (Inst){.type = INST_SDIV};
    }
    else if (sv_eq(inst_name, cstr_as_sv("udiv")))
    {
        if (has_operand)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: udiv doesn't require an operand\n", line_no);
            compilation_successful = false;
        }

        // printf("yes\n");
        return (Inst){.type = INST_UDIV};
    }
    else if (sv_eq(inst_name, cstr_as_sv("fdiv")))
    {
        if (has_operand)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: fdiv doesn't require an operand\n", line_no);
            compilation_successful = false;
        }

        // printf("yes\n");
        return (Inst){.type = INST_FDIV};
    }
    else if (sv_eq(inst_name, cstr_as_sv("eq")))
    {
        if (has_operand)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: eq doesn't require an operand\n", line_no);
            compilation_successful = false;
        }

        // printf("yes\n");
        return (Inst){.type = INST_EQ};
    }
    else if (sv_eq(inst_name, cstr_as_sv("nop")))
    {
        if (has_operand)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: nop doesn't require an operand\n", line_no);
            compilation_successful = false;
        }

        // printf("yes\n");
        return (Inst){.type = INST_NOP};
    }
    else if (sv_eq(inst_name, cstr_as_sv("asr")))
    {
        if (!has_operand)
        {
            fprintf(stderr, "Line Number %zu -> asr requires an operand\n", line_no);
            compilation_successful = false;
        }
        double operand = sv_to_value(&line);
        if (str_errno == FAILURE)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: %.*s is not a valid value\n", line_no, (int)line.count, line.data);
            compilation_successful = false;
        }
        else if (str_errno == OPERAND_OVERFLOW)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: %.*s overflows a 64 bit unsigned value\n", line_no, (int)line.count, line.data);
            compilation_successful = false;
        }

        if (is_fraction)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: illegal operand value for asr instruction: %.*s\nMust be an unsigned integral value\n", line_no, (int)line.count, line.data);
            compilation_successful = false;
        }
        else if (is_negative)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: illegal operand value for asr instruction: %.*s\nMust be an unsigned value\n", line_no, (int)line.count, line.data);
            compilation_successful = false;
        }
        else
        {
            set_unsigned_64int(&operand, (uint64_t)operand);
            return (Inst){.type = INST_ASR, .operand = operand};
        }
    }
    else if (sv_eq(inst_name, cstr_as_sv("lsr")))
    {
        if (!has_operand)
        {
            fprintf(stderr, "Line Number %zu -> lsr requires an operand\n", line_no);
            compilation_successful = false;
        }
        double operand = sv_to_value(&line);
        if (str_errno == FAILURE)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: %.*s is not a valid value\n", line_no, (int)line.count, line.data);
            compilation_successful = false;
        }
        else if (str_errno == OPERAND_OVERFLOW)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: %.*s overflows a 64 bit unsigned value\n", line_no, (int)line.count, line.data);
            compilation_successful = false;
        }

        if (is_fraction)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: illegal operand value for lsr instruction: %.*s\nMust be an unsigned integral value\n", line_no, (int)line.count, line.data);
            compilation_successful = false;
        }
        else if (is_negative)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: illegal operand value for lsr instruction: %.*s\nMust be an unsigned value\n", line_no, (int)line.count, line.data);
            compilation_successful = false;
        }
        else
        {
            set_unsigned_64int(&operand, (uint64_t)operand);
            return (Inst){.type = INST_LSR, .operand = operand};
        }
    }
    else if (sv_eq(inst_name, cstr_as_sv("sl")))
    {
        if (!has_operand)
        {
            fprintf(stderr, "Line Number %zu -> sl requires an operand\n", line_no);
            compilation_successful = false;
        }
        double operand = sv_to_value(&line);
        if (str_errno == FAILURE)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: %.*s is not a valid value\n", line_no, (int)line.count, line.data);
            compilation_successful = false;
        }
        else if (str_errno == OPERAND_OVERFLOW)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: %.*s overflows a 64 bit unsigned value\n", line_no, (int)line.count, line.data);
            compilation_successful = false;
        }

        if (is_fraction)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: illegal operand value for sl instruction: %.*s\nMust be an unsigned integral value\n", line_no, (int)line.count, line.data);
            compilation_successful = false;
        }
        else if (is_negative)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: illegal operand value for sl instruction: %.*s\nMust be an unsigned value\n", line_no, (int)line.count, line.data);
            compilation_successful = false;
        }
        else
        {
            set_unsigned_64int(&operand, (uint64_t)operand);
            return (Inst){.type = INST_SL, .operand = operand};
        }
    }
    else if (sv_eq(inst_name, cstr_as_sv("and")))
    {
        if (has_operand)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: and doesn't require an operand\n", line_no);
            compilation_successful = false;
        }

        // printf("yes\n");
        return (Inst){.type = INST_AND};
    }
    else if (sv_eq(inst_name, cstr_as_sv("or")))
    {
        if (has_operand)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: or doesn't require an operand\n", line_no);
            compilation_successful = false;
        }

        // printf("yes\n");
        return (Inst){.type = INST_OR};
    }
    else if (sv_eq(inst_name, cstr_as_sv("not")))
    {
        if (has_operand)
        {
            fprintf(stderr, "Line Number %zu -> ERROR: not doesn't require an operand\n", line_no);
            compilation_successful = false;
        }

        // printf("yes\n");
        return (Inst){.type = INST_NOT};
    }
    else
    {
        fprintf(stderr, "Line Number %zu -> ERROR: invalid instruction %.*s\n", line_no, (int)inst_name.count, inst_name.data);
        compilation_successful = false;
    }

    return (Inst){0};
} */

/* int vm_execute_at_inst_pointer(VirtualMachine *vm) // executes the instruction inst on vm
{
    Inst inst = vm->program[vm->instruction_pointer];
    switch (inst.type)
    {
    case INST_ASR:
        if (vm->stack_size < 1)
        {
            return TRAP_STACK_UNDERFLOW;
        }
        uint64_t b1 = return_value_unsigned(inst.operand);
        int64_t a1 = return_value_signed(vm->stack[vm->stack_size - 1]);
        a1 >>= b1; // C right shift operator does arithmetic shift for signed operands and logical shift for unsigned ones
        double c1 = 0;
        set_signed_64int(&c1, a1);
        vm->stack[vm->stack_size - 1] = c1;

        vm->instruction_pointer++;
        break;
    case INST_LSR:
        if (vm->stack_size < 1)
        {
            return TRAP_STACK_UNDERFLOW;
        }
        uint64_t b2 = return_value_unsigned(inst.operand);
        uint64_t a2 = return_value_unsigned(vm->stack[vm->stack_size - 1]);
        a2 >>= b2; // C right shift operator does arithmetic shift for signed operands and logical shift for unsigned ones
        double c2 = 0;
        set_signed_64int(&c2, a2);
        vm->stack[vm->stack_size - 1] = c2;

        vm->instruction_pointer++;
        break;
    case INST_SL:
        if (vm->stack_size < 1)
        {
            return TRAP_STACK_UNDERFLOW;
        }
        uint64_t b3 = return_value_unsigned(inst.operand);
        uint64_t a3 = return_value_unsigned(vm->stack[vm->stack_size - 1]);
        a3 <<= b3; // C right shift operator does arithmetic shift for signed operands and logical shift for unsigned ones
        double c3 = 0;
        set_signed_64int(&c3, a3);
        vm->stack[vm->stack_size - 1] = c3;

        vm->instruction_pointer++;
        break;
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

        int64_t operand1 = return_value_signed(vm->stack[vm->stack_size - 2]);
        int64_t operand2 = return_value_signed(vm->stack[vm->stack_size - 1]);
        int64_t ans = operand1 + operand2;

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

        uint64_t opernd11 = return_value_unsigned(vm->stack[vm->stack_size - 2]);
        uint64_t opernd12 = return_value_unsigned(vm->stack[vm->stack_size - 1]);
        uint64_t ns = opernd11 + opernd12;

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

        int64_t operand3 = return_value_signed(vm->stack[vm->stack_size - 2]);
        int64_t operand4 = return_value_signed(vm->stack[vm->stack_size - 1]);
        int64_t ans2 = operand3 - operand4;

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

        uint64_t opernd3 = return_value_unsigned(vm->stack[vm->stack_size - 2]);
        uint64_t opernd4 = return_value_unsigned(vm->stack[vm->stack_size - 1]);
        uint64_t ns2 = opernd3 - opernd4;

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

        int64_t operand5 = return_value_signed(vm->stack[vm->stack_size - 2]);
        int64_t operand6 = return_value_signed(vm->stack[vm->stack_size - 1]);
        int64_t ans3 = operand5 * operand6;

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

        uint64_t opernd5 = return_value_unsigned(vm->stack[vm->stack_size - 2]);
        uint64_t opernd6 = return_value_unsigned(vm->stack[vm->stack_size - 1]);
        uint64_t ns3 = opernd5 * opernd6;

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

        int64_t opernd1 = return_value_signed(vm->stack[vm->stack_size - 1]);
        if (!opernd1)
        {
            return TRAP_DIV_BY_ZERO;
        }

        int64_t operand7 = return_value_signed(vm->stack[vm->stack_size - 2]);
        int64_t operand8 = return_value_signed(vm->stack[vm->stack_size - 1]);
        int64_t ans4 = operand7 / operand8;

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

        uint64_t opernd2 = return_value_unsigned(vm->stack[vm->stack_size - 1]);
        if (!opernd2)
        {
            return TRAP_DIV_BY_ZERO;
        }

        uint64_t opernd7 = return_value_unsigned(vm->stack[vm->stack_size - 2]);
        uint64_t opernd8 = return_value_unsigned(vm->stack[vm->stack_size - 1]);
        uint64_t ns4 = opernd7 / opernd8;

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
        uint64_t opernd = return_value_unsigned(inst.operand);
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
        uint64_t opernd111 = return_value_unsigned(inst.operand);
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
        uint64_t opernd222 = return_value_unsigned(inst.operand);
        // printf("%lld\n", opernd2);
        if (vm->stack_size >= vm_stack_capacity)
        {
            return TRAP_STACK_OVERFLOW;
        }
        if ((int64_t)vm->stack_size - 1 - (int64_t)(opernd222) < 0)
        {
            return TRAP_STACK_UNDERFLOW;
        }
        vm->stack_size++;
        vm->stack[vm->stack_size - 1] = vm->stack[vm->stack_size - 2 - opernd222];
        vm->instruction_pointer++;
        break;
    case INST_AND:
        if (vm->stack_size < 2)
        {
            return TRAP_STACK_UNDERFLOW;
        }

        if (is_nan(vm->stack[vm->stack_size - 2]) && is_nan(vm->stack[vm->stack_size - 1]))
        {
            uint64_t raw_1 = (*(uint64_t *)&vm->stack[vm->stack_size - 2]);
            uint64_t raw_2 = (*(uint64_t *)&vm->stack[vm->stack_size - 1]);
            uint64_t raw_3 = ((raw_1 & raw_2) | 0xFFF1000000000000);
            vm->stack[vm->stack_size - 2] = *(double *)&raw_3;
        }
        else if (!is_nan(vm->stack[vm->stack_size - 2]) && !is_nan(vm->stack[vm->stack_size - 1])) // for some reason i provide functionality of bitwise operations between any two floating point numbers!
        {
            uint64_t raw_1 = (*(uint64_t *)&vm->stack[vm->stack_size - 2]);
            uint64_t raw_2 = (*(uint64_t *)&vm->stack[vm->stack_size - 1]);
            uint64_t raw_3 = ((raw_1 & raw_2));
            vm->stack[vm->stack_size - 2] = *(double *)&raw_3;
        }
        else
        {
            return TRAP_ILLEGAL_OPERATION;
        }

        vm->stack_size--;
        vm->instruction_pointer++;
        break;

    case INST_OR:
        if (vm->stack_size < 2)
        {
            return TRAP_STACK_UNDERFLOW;
        }

        if (is_nan(vm->stack[vm->stack_size - 2]) && is_nan(vm->stack[vm->stack_size - 1]))
        {
            uint64_t raw_4 = (*(uint64_t *)&vm->stack[vm->stack_size - 2]);
            uint64_t raw_5 = (*(uint64_t *)&vm->stack[vm->stack_size - 1]);
            uint64_t raw_6 = ((raw_4 | raw_5) | 0xFFF1000000000000);
            vm->stack[vm->stack_size - 2] = *(double *)&raw_6;
        }
        else if (!is_nan(vm->stack[vm->stack_size - 2]) && !is_nan(vm->stack[vm->stack_size - 1])) // for some reason i provide functionality of bitwise operations between any two floating point numbers!
        {
            uint64_t raw_4 = (*(uint64_t *)&vm->stack[vm->stack_size - 2]);
            uint64_t raw_5 = (*(uint64_t *)&vm->stack[vm->stack_size - 1]);
            uint64_t raw_6 = ((raw_4 | raw_5));
            vm->stack[vm->stack_size - 2] = *(double *)&raw_6;
        }
        else
        {
            return TRAP_ILLEGAL_OPERATION;
        }

        vm->stack_size--;
        vm->instruction_pointer++;
        break;

    case INST_NOT:
        if (vm->stack_size < 1)
        {
            return TRAP_STACK_UNDERFLOW;
        }

        if (is_nan(vm->stack[vm->stack_size - 1]))
        {
            uint64_t raw_7 = (*(uint64_t *)&vm->stack[vm->stack_size - 1]);
            uint64_t raw_8 = (~raw_7 | 0xFFF1000000000000);
            vm->stack[vm->stack_size - 1] = *(double *)&raw_8;
        }
        else if (!is_nan(vm->stack[vm->stack_size - 2]) && !is_nan(vm->stack[vm->stack_size - 1])) // for some reason i provide functionality of bitwise operations between any two floating point numbers!
        {
            uint64_t raw_7 = (*(uint64_t *)&vm->stack[vm->stack_size - 1]);
            uint64_t raw_8 = (~raw_7);
            vm->stack[vm->stack_size - 2] = *(double *)&raw_8;
        }
        else
        {
            return TRAP_ILLEGAL_OPERATION;
        }

        vm->instruction_pointer++;
        break;

    default:
        return TRAP_ILLEGAL_INSTRUCTION;
        break;
    }
    return TRAP_OK;
} */