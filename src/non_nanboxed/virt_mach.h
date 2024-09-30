#ifndef _VM
#define _VM
#define _SV_IMPLEMENTATION
#define _NAN_IMPLEMENTATION

#include "./String_View.h"
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>

// long is 32 bit on Windows and 64 bit on Linux; long long is 64 bit on both; so we use that to make it cross platform

#define TYPE_UNSIGNED_64INT 0
#define TYPE_SIGNED_64INT 1
#define TYPE_DOUBLE 2

#define EPSILON 1e-9
#define VM_STACK_CAPACITY 1024
#define VM_MEMORY_CAPACITY 640 * 1024
#define VM_DEFAULT_MEMORY_SIZE 1024
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
size_t vm_memory_capacity = VM_MEMORY_CAPACITY;
size_t label_capacity = VM_LABEL_CAPACITY;
size_t equ_label_capacity = VM_EQU_CAPACITY;
size_t natives_capacity = VM_NATIVE_CAPACITY;
size_t line_no = 0;
size_t vm_default_memory_size = VM_DEFAULT_MEMORY_SIZE;
bool compilation_successful = true;

typedef union
{
    int64_t _as_s64;
    uint64_t _as_u64;
    double _as_f64;
} Value;

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
    TRAP_ILLEGAL_MEMORY_ACCESS,
    TRAP_ILLEGAL_OPERATION, // doing arithmetic on floating points and ints together
} Trap;                     // exceptions that stop the execution on the virtual machine

typedef enum
{
    INST_NOP = 1, // does nothing but increment the instruction pointer; if the program array is just zero, it will all be no-ops;
    INST_SPUSH,   // push a word to the stack top; we assume that our stack grows downwards
    INST_FPUSH,
    INST_UPUSH,
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
    INST_WRITE8, // write the raw bytes on the stack onto the memory locations
    INST_WRITE16,
    INST_WRITE32,
    INST_WRITE64,
    INST_ZEREAD8, // zero extend the memory value into the 64-bit stack
    INST_ZEREAD16,
    INST_ZEREAD32,
    INST_READ64, 
    INST_SEREAD8, // sign extend the memory value into the 64-bit stack
    INST_SEREAD16,
    INST_SEREAD32,
    INST_COUNT,
} Inst_Type; // enum for the instruction types

typedef struct
{
    Inst_Type type; // type of the instruction
    Value operand;  // operand of the instruction
} Inst;             // structure for the actual instruction

struct VirtualMachine;

typedef Trap (*native)(struct VirtualMachine *); // you can define functions that match this signature and assign their addresses to variables of type native

typedef struct VirtualMachine // structure defining the actual virtual machine
{
    Value *stack;      // the stack of the virtual machine; the stack top is the end of the array
    size_t stack_size; // current stack size

    Inst *program;            // the actual instruction array
    size_t program_size;      // number of instructions in the program
    word instruction_pointer; // the address of the next instruction to be executed

    native *natives;
    size_t natives_size;

    bool has_start;
    size_t start_label_index;

    uint8_t *static_memory;
    uint64_t static_break;

    int halt;
} VirtualMachine;

typedef struct
{
    bool has_start;
    size_t code_section_offset;
    int64_t start_location;
    size_t code_section_size;
    size_t data_section_offset; // not using currently
    size_t data_section_size;
} vm_header_;

void push_to_not_resolved_yet(String_View label, size_t inst_location);
void push_to_label_array(String_View label, size_t pointing_location);
const char *trap_as_cstr(Trap trap);
const char *inst_type_as_asm_str(Inst_Type type);
const char *inst_type_as_cstr(Inst_Type type);
void vm_native_push(VirtualMachine *vm, native native_func);
void vm_dump_stack(FILE *stream, const VirtualMachine *vm);
static int handle_static(VirtualMachine *vm, Inst inst);
static int handle_swap(VirtualMachine *vm, Inst inst);
static int handle_native(VirtualMachine *vm, Inst inst);
static int handle_shift(VirtualMachine *vm, Inst inst, bool is_arithmetic);
static int handle_push(VirtualMachine *vm, Inst inst);
static int handle_arithmetic(VirtualMachine *vm, Inst inst);
static int handle_functions(VirtualMachine *vm, Inst inst);
static int handle_jump(VirtualMachine *vm, Inst inst);
static int handle_comparison(VirtualMachine *vm);
static int handle_adup(VirtualMachine *vm, Inst inst);
static int handle_rdup(VirtualMachine *vm, Inst inst);
static int handle_bitwise(VirtualMachine *vm, Inst inst);
static int handle_empty(VirtualMachine *vm);
static int handle_pop(VirtualMachine *vm, Inst inst);
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
void vm_save_program_to_file(Inst *program, uint8_t *data_section, vm_header_ header, const char *file_path);
vm_header_ vm_load_program_from_file(Inst *program, uint8_t *data_section, const char *file_path);
Inst vm_translate_line(String_View line, size_t current_program_counter);
static void process_label(String_View label, size_t program_size);
static void resolve_labels(Inst *program);
static void check_unresolved_labels();
static void process_section_directive(String_View section, bool *is_code);
static void process_code_line(String_View line, Inst *program, size_t *code_section_offset);
static void process_data_line(String_View line, uint8_t *data_section, size_t *data_section_offset);
static bool check_compilation_status(Inst *program, size_t code_section_offset);
static vm_header_ create_vm_header(size_t code_section_offset, size_t data_section_offset);
vm_header_ vm_translate_source(String_View source, Inst *program, uint8_t *data_section);
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
    case INST_FPUSH:
        return "fpush";
    case INST_UPUSH:
        return "upush";
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
    case INST_RSWAP:
        return "rswap";
    case INST_ASWAP:
        return "aswap";
    case INST_RET:
        return "ret";
    case INST_CALL:
        return "call";
    case INST_NATIVE:
        return "native";
    case INST_ZEREAD8:
        return "zeread8";
    case INST_ZEREAD16:
        return "zeread16";
    case INST_ZEREAD32:
        return "zeread32";
    case INST_READ64:
        return "read64";
    case INST_SEREAD8:
        return "seread8";
    case INST_SEREAD16:
        return "seread16";
    case INST_SEREAD32:
        return "seread32";
    case INST_WRITE8:
        return "write8";
    case INST_WRITE16:
        return "write16";
    case INST_WRITE32:
        return "write32";
    case INST_WRITE64:
        return "write64";
    default:
        return NULL; // Invalid instruction
    }
}

bool has_operand_function(Inst_Type inst)
{
    switch (inst)
    {
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
    case INST_JMP:
        return 1;
    case INST_FJMP_IF:
        return 1;
    case INST_UJMP_IF:
        return 1;
    case INST_LSR:
        return 1;
    case INST_ASR:
        return 1;
    case INST_SL:
        return 1;
    case INST_POP_AT:
        return 1;
    case INST_CALL:
        return 1;
    case INST_RSWAP:
        return 1;
    case INST_ASWAP:
        return 1;
    case INST_NATIVE:
        return 1;
    }

    return 0;
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

void push_to_not_resolved_yet(String_View label, size_t inst_location)
{
    not_resolved_yet[not_resolved_yet_counter].inst_location = inst_location;
    not_resolved_yet[not_resolved_yet_counter].label = label;
    not_resolved_yet[not_resolved_yet_counter].resolved = false;
    not_resolved_yet_counter++;
}

void push_to_label_array(String_View label, size_t pointing_location)
{
    for (size_t i = 0; i < label_array_counter; i++)
    {
        if (sv_eq(label_array[i].label, label))
        {
            fprintf(stderr, "Line Number %zu -> ERROR: Label '%.*s' redefinition\n", line_no, (int)label.count, label.data);
            compilation_successful = false;
            return;
        }
    }
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
            fprintf(stream, "u64: %llu, s64: %lld, f64: %lf\n", vm->stack[i]._as_u64, vm->stack[i]._as_s64, vm->stack[i]._as_f64);
        }
    }
    else
    {
        fprintf(stream, " [empty]\n");
    }
}

static int handle_static(VirtualMachine *vm, Inst inst)
{
    switch (inst.type)
    {
    case INST_ZEREAD8:
    {
        if (vm->stack_size < 1)
        {
            return TRAP_STACK_UNDERFLOW;
        }

        uint64_t addr = vm->stack[vm->stack_size - 1]._as_u64;
        if (addr >= vm_memory_capacity - 1)
        {
            return TRAP_ILLEGAL_MEMORY_ACCESS;
        }

        vm->stack[vm->stack_size - 1]._as_u64 = *(uint8_t *)&vm->static_memory[addr];
        break;
    }

    case INST_ZEREAD16:
    {
        if (vm->stack_size < 1)
        {
            return TRAP_STACK_UNDERFLOW;
        }

        uint64_t addr = vm->stack[vm->stack_size - 1]._as_u64;
        if (addr >= vm_memory_capacity - 2)
        {
            return TRAP_ILLEGAL_MEMORY_ACCESS;
        }

        vm->stack[vm->stack_size - 1]._as_u64 = *(uint16_t *)&vm->static_memory[addr];
        break;
    }

    case INST_ZEREAD32:
    {
        if (vm->stack_size < 1)
        {
            return TRAP_STACK_UNDERFLOW;
        }

        uint64_t addr = vm->stack[vm->stack_size - 1]._as_u64;
        if (addr >= vm_memory_capacity - 4)
        {
            return TRAP_ILLEGAL_MEMORY_ACCESS;
        }

        vm->stack[vm->stack_size - 1]._as_u64 = *(uint32_t *)&vm->static_memory[addr];
        break;
    }

    case INST_READ64:
    {
        if (vm->stack_size < 1)
        {
            return TRAP_STACK_UNDERFLOW;
        }

        uint64_t addr = vm->stack[vm->stack_size - 1]._as_u64;
        if (addr >= vm_memory_capacity - 8)
        {
            return TRAP_ILLEGAL_MEMORY_ACCESS;
        }

        vm->stack[vm->stack_size - 1]._as_u64 = *(uint64_t *)&vm->static_memory[addr];
        break;
    }

    case INST_SEREAD8:
    {
        if (vm->stack_size < 1)
        {
            return TRAP_STACK_UNDERFLOW;
        }

        uint64_t addr = vm->stack[vm->stack_size - 1]._as_u64;
        if (addr >= vm_memory_capacity - 1)
        {
            return TRAP_ILLEGAL_MEMORY_ACCESS;
        }

        vm->stack[vm->stack_size - 1]._as_s64 = *(int8_t *)&vm->static_memory[addr];
        break;
    }

    case INST_SEREAD16:
    {
        if (vm->stack_size < 1)
        {
            return TRAP_STACK_UNDERFLOW;
        }

        uint64_t addr = vm->stack[vm->stack_size - 1]._as_u64;
        if (addr >= vm_memory_capacity - 2)
        {
            return TRAP_ILLEGAL_MEMORY_ACCESS;
        }

        vm->stack[vm->stack_size - 1]._as_s64 = *(int16_t *)&vm->static_memory[addr];
        break;
    }

    case INST_SEREAD32:
    {
        if (vm->stack_size < 1)
        {
            return TRAP_STACK_UNDERFLOW;
        }

        uint64_t addr = vm->stack[vm->stack_size - 1]._as_u64;
        if (addr >= vm_memory_capacity - 4)
        {
            return TRAP_ILLEGAL_MEMORY_ACCESS;
        }

        vm->stack[vm->stack_size - 1]._as_s64 = *(int32_t *)&vm->static_memory[addr];
        break;
    }

    case INST_WRITE8:
    {
        if (vm->stack_size < 2)
        {
            return TRAP_STACK_UNDERFLOW;
        }

        uint64_t addr = vm->stack[vm->stack_size - 1]._as_u64;
        if (addr >= vm_memory_capacity - 1)
        {
            return TRAP_ILLEGAL_MEMORY_ACCESS;
        }

        uint8_t value = vm->stack[vm->stack_size - 2]._as_u64;
        *(uint8_t *)&vm->static_memory[addr] = value;
        vm->stack_size -= 2;
        break;
    }

    case INST_WRITE16:
    {
        if (vm->stack_size < 2)
        {
            return TRAP_STACK_UNDERFLOW;
        }

        uint64_t addr = vm->stack[vm->stack_size - 1]._as_u64;
        if (addr >= vm_memory_capacity - 2)
        {
            return TRAP_ILLEGAL_MEMORY_ACCESS;
        }

        uint16_t value = vm->stack[vm->stack_size - 2]._as_u64;
        *(uint16_t *)&vm->static_memory[addr] = value;
        vm->stack_size -= 2;
        break;
    }

    case INST_WRITE32:
    {
        if (vm->stack_size < 2)
        {
            return TRAP_STACK_UNDERFLOW;
        }

        uint64_t addr = vm->stack[vm->stack_size - 1]._as_u64;
        if (addr >= vm_memory_capacity - 4)
        {
            return TRAP_ILLEGAL_MEMORY_ACCESS;
        }

        uint32_t value = vm->stack[vm->stack_size - 2]._as_u64;
        *(uint32_t *)&vm->static_memory[addr] = value;
        vm->stack_size -= 2;
        break;
    }

    case INST_WRITE64:
    {
        if (vm->stack_size < 2)
        {
            return TRAP_STACK_UNDERFLOW;
        }

        uint64_t addr = vm->stack[vm->stack_size - 1]._as_u64;
        if (addr >= vm_memory_capacity - 8)
        {
            return TRAP_ILLEGAL_MEMORY_ACCESS;
        }

        uint64_t value = vm->stack[vm->stack_size - 2]._as_u64;
        *(uint64_t *)&vm->static_memory[addr] = value;
        vm->stack_size -= 2;
        break;
    }
    }

    vm->instruction_pointer++;
    return TRAP_OK;
}

static int handle_swap(VirtualMachine *vm, Inst inst)
{
    uint64_t operand = inst.operand._as_u64;
    if (inst.type == INST_ASWAP)
    {
        if (operand >= vm->stack_size)
        {
            return TRAP_STACK_OVERFLOW;
        }

        Value temp = vm->stack[vm->stack_size - 1];
        vm->stack[vm->stack_size - 1] = vm->stack[operand];
        vm->stack[operand] = temp;
    }
    else
    {
        if (vm->stack_size - operand < 0)
        {
            return TRAP_STACK_UNDERFLOW;
        }

        Value temp = vm->stack[vm->stack_size - 1];
        vm->stack[vm->stack_size - 1] = vm->stack[vm->stack_size - 1 - operand];
        vm->stack[vm->stack_size - 1 - operand] = temp;
    }

    vm->instruction_pointer++;
    return TRAP_OK;
}

static int handle_native(VirtualMachine *vm, Inst inst)
{
    uint64_t index = inst.operand._as_u64;
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

    uint64_t shift = inst.operand._as_u64;

    if (is_arithmetic && (inst.type == INST_ASR))
    {
        int64_t signed_value = vm->stack[vm->stack_size - 1]._as_s64;
        signed_value >>= shift;
        vm->stack[vm->stack_size - 1]._as_s64 = signed_value;
    }
    else
    {
        uint64_t value = vm->stack[vm->stack_size - 1]._as_u64;
        value = (inst.type == INST_SL) ? (value << shift) : (value >> shift);
        vm->stack[vm->stack_size - 1]._as_u64 = value;
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
        a_float = vm->stack[vm->stack_size - 2]._as_f64; // First operand from the given index
        b_float = vm->stack[vm->stack_size - 1]._as_f64; // Second operand is the top of the stack
        break;
    case INST_SPLUS:
    case INST_SMINUS:
    case INST_SMULT:
    case INST_SDIV:
        a_signed = vm->stack[vm->stack_size - 2]._as_s64;
        b_signed = vm->stack[vm->stack_size - 1]._as_s64;
        break;
    case INST_UPLUS:
    case INST_UMINUS:
    case INST_UMULT:
    case INST_UDIV:
        a_unsigned = vm->stack[vm->stack_size - 2]._as_u64;
        b_unsigned = vm->stack[vm->stack_size - 1]._as_u64;
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
        vm->stack[vm->stack_size - 2]._as_s64 = (int64_t)result;
    }
    else if (inst.type == INST_UPLUS || inst.type == INST_UMINUS || inst.type == INST_UMULT || inst.type == INST_UDIV)
    {
        vm->stack[vm->stack_size - 2]._as_u64 = (uint64_t)result;
    }
    else
    {
        vm->stack[vm->stack_size - 2]._as_f64 = result;
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
        uint64_t addr = inst.operand._as_u64;
        if (vm->stack_size >= vm_stack_capacity)
        {
            return TRAP_STACK_OVERFLOW;
        }

        vm->stack[vm->stack_size++]._as_u64 = (vm->instruction_pointer + 1);
        vm->instruction_pointer = addr;
    }
    else if (inst.type == INST_RET)
    {
        vm->instruction_pointer = vm->stack[vm->stack_size - 1]._as_u64;
        vm->stack_size--;
    }

    return TRAP_OK;
}

static int handle_jump(VirtualMachine *vm, Inst inst)
{
    uint64_t jump_addr = inst.operand._as_u64;

    if (jump_addr >= vm->program_size)
        return TRAP_ILLEGAL_JMP;

    if (inst.type == INST_UJMP_IF)
    {
        if (vm->stack_size < 1)
            return TRAP_STACK_UNDERFLOW;
        if (vm->stack[vm->stack_size - 1]._as_u64)
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
        if (!((vm->stack[vm->stack_size - 1]._as_f64) < EPSILON))
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

/* static int handle_comparison(VirtualMachine *vm)
{
    if (vm->stack_size < 2)
        return TRAP_STACK_UNDERFLOW;

    vm->stack[vm->stack_size - 2]._as_u64 = (vm->stack[vm->stack_size - 1] == vm->stack[vm->stack_size - 2]);
    vm->stack_size--;
    vm->instruction_pointer++;

    return TRAP_OK;
}
 */

static int handle_adup(VirtualMachine *vm, Inst inst)
{
    uint64_t index = inst.operand._as_u64;

    if (vm->stack_size >= vm_stack_capacity || index > vm->stack_size)
        return TRAP_STACK_OVERFLOW;

    vm->stack[vm->stack_size] = vm->stack[index];
    vm->stack_size++;
    vm->instruction_pointer++;

    return TRAP_OK;
}

static int handle_rdup(VirtualMachine *vm, Inst inst)
{
    uint64_t index = inst.operand._as_u64;

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

    uint64_t a = vm->stack[vm->stack_size - 1]._as_u64;
    uint64_t b = (inst.type != INST_NOT) ? vm->stack[vm->stack_size - 2]._as_u64 : 0;
    uint64_t result;

    /*     bool both_nan = is_nan(vm->stack[vm->stack_size - 1]) &&
                        (inst.type == INST_NOT || is_nan(vm->stack[vm->stack_size - 2])); */

    /* if (both_nan)
    { */
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
    /*    }
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
   } */
    /* else
    {
        return TRAP_ILLEGAL_OPERATION;
    } */

    vm->stack[vm->stack_size - (inst.type == INST_NOT ? 1 : 2)]._as_u64 = result;
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
        vm->stack[vm->stack_size - 1]._as_u64 = 0;
    }
    else
    {
        vm->stack_size++;
        vm->stack[vm->stack_size - 1]._as_s64 = 1;
    }

    vm->instruction_pointer++;
    return TRAP_OK;
}

static int handle_pop(VirtualMachine *vm, Inst inst)
{
    if (inst.type == INST_POP_AT)
    {
        size_t index_to_pop = inst.operand._as_u64;

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

        /*  case INST_EQ:
             return handle_comparison(vm); */

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

        /* case INST_READ:
        case INST_WRITE:
            return handle_static(vm, inst); */

    case INST_WRITE8:
    case INST_WRITE16:
    case INST_WRITE32:
    case INST_WRITE64:
    case INST_SEREAD8:
    case INST_SEREAD16:
    case INST_SEREAD32:
    case INST_READ64:
    case INST_ZEREAD8:
    case INST_ZEREAD16:
    case INST_ZEREAD32:
        return handle_static(vm, inst);

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
    vm->stack = malloc(sizeof(Value) * vm_stack_capacity);
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

    vm->static_memory = malloc(sizeof(uint8_t) * vm_memory_capacity);
    if (!vm->static_memory)
    {
        fprintf(stderr, "ERROR: static memory allocation failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    vm->static_break = vm_default_memory_size;

    vm_header_ header = vm_load_program_from_file(vm->program, vm->static_memory, source_code);

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
    free((void *)vm->static_memory);
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

void vm_save_program_to_file(Inst *program, uint8_t *data_section, vm_header_ header, const char *file_path)
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

    fwrite(data_section, sizeof(uint8_t), header.data_section_size, f);
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

vm_header_ vm_load_program_from_file(Inst *program, uint8_t *data_section, const char *file_path)
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

    assert(header.data_section_size <= vm_default_memory_size * sizeof(uint8_t));

    if (fseek(f, header.data_section_offset, SEEK_SET))
    {
        fclose(f);
        fprintf(stderr, "ERROR: Could not read file '%s': %s\n", file_path, strerror(errno));
        exit(EXIT_FAILURE);
    }

    ret_val = fread(data_section, sizeof(uint8_t), header.data_section_size, f);
    if (ferror(f))
    {
        fclose(f);
        fprintf(stderr, "ERROR: Could not read file '%s': %s\n", file_path, strerror(errno));
        exit(EXIT_FAILURE);
    }

    /*     for (size_t i = 0; i < header.data_section_size; i++)
        {
            printf("%d\n", data_section[i]);
        } */

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

    fclose(f);
    return header;
}

Inst vm_translate_line(String_View line, size_t current_program_counter)
{
    String_View inst_name = sv_chop_by_delim(&line, ' ');
    sv_trim_left(&line);
    bool has_operand_value = line.count > 0;

    for (size_t i = 1; i < (size_t)INST_COUNT; i++)
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

            union
            {
                double f64;
                int64_t s64;
                uint64_t u64;
            } operand = {0};

            bool is_frac = is_fraction(line);
            bool is_neg = is_negative(line);

            if (is_frac)
            {
                operand.f64 = sv_to_double(&line);
            }
            else if (is_neg)
            {
                operand.s64 = sv_to_signed64(&line);
            }
            else
            {
                operand.u64 = sv_to_unsigned64(&line);
            }

            if (str_errno == FAILURE)
            {
                push_to_not_resolved_yet(line, current_program_counter);
                return (Inst){.type = i, .operand._as_u64 = 0};
            }
            else if (str_errno == OPERAND_OVERFLOW)
            {
                fprintf(stderr, "Line Number %zu -> ERROR: %.*s overflows a 64 bit %s value\n",
                        line_no, (int)line.count, line.data,
                        get_operand_type(i) == TYPE_SIGNED_64INT ? "signed" : "unsigned");
                compilation_successful = false;
                return (Inst){0};
            }

            switch (get_operand_type(i))
            {
            case TYPE_SIGNED_64INT:
                return (Inst){.type = i, .operand._as_s64 = operand.s64};
            case TYPE_UNSIGNED_64INT:
                if (is_frac || is_neg)
                {
                    fprintf(stderr, "Line Number %zu -> ERROR: illegal operand value for %s instruction: %.*s\n"
                                    "Must be an unsigned integral value\n",
                            line_no, get_inst_name(i), (int)line.count, line.data);
                    compilation_successful = false;
                    return (Inst){0};
                }
                return (Inst){.type = i, .operand._as_u64 = operand.u64};
            case TYPE_DOUBLE:
                return (Inst){.type = i, .operand._as_f64 = operand.f64};
            default:
                fprintf(stderr, "Line Number %zu -> ERROR: unknown operand type for instruction %s\n",
                        line_no, get_inst_name(i));
                compilation_successful = false;
                return (Inst){0};
            }
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

    push_to_label_array(label, (uint64_t)program_size);
}

static void resolve_labels(Inst *program)
{
    for (size_t i = 0; i < not_resolved_yet_counter; i++)
    {
        for (size_t j = 0; j < label_array_counter; j++)
        {
            if (sv_eq(not_resolved_yet[i].label, label_array[j].label))
            {
                program[not_resolved_yet[i].inst_location].operand._as_u64 = label_array[j].label_pointing_location;
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
            return (int64_t)label_array[i].label_pointing_location;
        }
    }

    return -1;
}

vm_header_ vm_translate_source(String_View source, Inst *program, uint8_t *data_section)
{
    size_t code_section_offset = 0;
    size_t data_section_offset = 0;
    bool is_code = true;
    size_t line_no = 0;

    while (source.count > 0)
    {
        if (code_section_offset >= vm_program_capacity)
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

        if (*line.data == '.')
        {
            process_section_directive(line, &is_code);
        }
        else if (is_code)
        {
            process_code_line(line, program, &code_section_offset);
        }
        else
        {
            process_data_line(line, data_section, &data_section_offset);
        }
    }

    compilation_successful = check_compilation_status(program, code_section_offset);

    if (compilation_successful)
    {
        resolve_labels(program);
        check_unresolved_labels();
    }
    else
    {
        fprintf(stderr, "Compilation Failed\n");
    }

    return create_vm_header(code_section_offset, data_section_offset);
}

static void process_section_directive(String_View section, bool *is_code)
{
    String_View directive = sv_chop_by_delim(&section, ' ');
    if (sv_eq(directive, cstr_as_sv(".text")))
    {
        *is_code = true;
    }
    else if (sv_eq(directive, cstr_as_sv(".data")))
    {
        *is_code = false;
    }
    else
    {
        fprintf(stderr, "ERROR: invalid section type '%.*s'\n", (int)directive.count, directive.data);
        compilation_successful = false;
    }
}

static void process_code_line(String_View line, Inst *program, size_t *code_section_offset)
{
    String_View label = sv_chop_by_delim(&line, ':');
    if (*(line.data - 1) == ':')
    { // If there's a label
        process_label(label, *code_section_offset);
        sv_trim_left(&line);
        if (line.count > 0)
        { // instruction remaining after the label
            program[*code_section_offset] = vm_translate_line(line, *code_section_offset);
            (*code_section_offset)++;
        }
    }
    else
    {
        program[*code_section_offset] = vm_translate_line(label, *code_section_offset);
        (*code_section_offset)++;
    }
}

static void process_data_line(String_View line, uint8_t *data_section, size_t *data_section_offset)
{
    String_View label = sv_chop_by_delim(&line, ':');
    if (*(line.data - 1) == ':')
    {
        process_label(label, *data_section_offset);
        sv_trim_left(&line);
    }
    else
    {
        line.count = label.count; // Reset line if no label found
        line.data = label.data;
    }

    sv_trim_left(&line);
    String_View data_type = sv_chop_by_delim(&line, ' ');
    sv_trim_left(&line);

    bool is_neg = is_negative(line);

    if (sv_eq(data_type, cstr_as_sv(".byte")))
    {
        if (is_neg)
        {
            int8_t value = (int8_t)sv_to_signed64(&line);
            *(int8_t *)&data_section[*data_section_offset] = value;
        }
        else
        {
            uint8_t value = (uint8_t)sv_to_unsigned64(&line);
            *(uint8_t *)&data_section[*data_section_offset] = value;
        }
        *data_section_offset += sizeof(int8_t);
    }
    else if (sv_eq(data_type, cstr_as_sv(".word")))
    {
        if (is_neg)
        {
            int16_t value = (int16_t)sv_to_signed64(&line);
            *(int16_t *)&data_section[*data_section_offset] = value;
        }
        else
        {
            uint16_t value = (uint16_t)sv_to_unsigned64(&line);
            *(uint16_t *)&data_section[*data_section_offset] = value;
        }
        *data_section_offset += sizeof(int16_t);
    }
    else if (sv_eq(data_type, cstr_as_sv(".doubleword")))
    {
        if (is_neg)
        {
            int32_t value = (int32_t)sv_to_signed64(&line);
            *(int32_t *)&data_section[*data_section_offset] = value;
        }
        else
        {
            uint32_t value = (uint32_t)sv_to_unsigned64(&line);
            *(uint32_t *)&data_section[*data_section_offset] = value;
        }
        *data_section_offset += sizeof(int32_t);
    }
    else if (sv_eq(data_type, cstr_as_sv(".quadword")))
    {
        if (is_neg)
        {
            int64_t value = sv_to_signed64(&line);
            *(int64_t *)&data_section[*data_section_offset] = value;
        }
        else
        {
            uint64_t value = sv_to_unsigned64(&line);
            *(uint64_t *)&data_section[*data_section_offset] = value;
        }
        *data_section_offset += sizeof(int64_t);
    }
    else if (sv_eq(data_type, cstr_as_sv(".double")))
    {
        double value = sv_to_double(&line);
        *(double *)&data_section[*data_section_offset] = value;
        *data_section_offset += sizeof(double);
    }
    else
    {
        fprintf(stderr, "Line %zu -> ERROR: invalid data type '%.*s'\n", line_no, (int)data_type.count, data_type.data);
        compilation_successful = false;
        return;
    }

    if (str_errno != SUCCESS)
    {
        fprintf(stderr, "Line %zu -> ERROR: failed to parse value for %.*s\n", line_no, (int)data_type.count, data_type.data);
        compilation_successful = false;
    }
}

static bool check_compilation_status(Inst *program, size_t code_section_offset)
{
    if (code_section_offset > 0 && program[code_section_offset - 1].type != INST_HALT)
    {
        fprintf(stderr, "ERROR: halt required to mark the code end\n");
        return false;
    }
    return true;
}

static vm_header_ create_vm_header(size_t code_section_offset, size_t data_section_offset)
{
    bool has_start = true;
    int64_t start_location = check_start();
    if (start_location == -1)
    {
        has_start = false;
    }

    return (vm_header_){
        .code_section_size = code_section_offset,
        .has_start = has_start,
        .data_section_offset = sizeof(vm_header_),
        .code_section_offset = sizeof(vm_header_) + sizeof(uint8_t) * data_section_offset,
        .start_location = start_location,
        .data_section_size = data_section_offset};
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