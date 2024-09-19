#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

#define SUCCESS 1
#define VM_STACK_CAPACITY 1024
#define VM_PROGRAM_CAPACITY 1024
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define MAKE_INST_PUSH(value) {.type = INST_PUSH, .operand = (value)}
#define MAKE_INST_PLUS {.type = INST_PLUS}
#define MAKE_INST_MINUS {.type = INST_MINUS}
#define MAKE_INST_MULT {.type = INST_MULT}
#define MAKE_INST_DIV {.type = INST_DIV}
#define MAKE_INST_JMP(addr) {.type = INST_JMP, .operand = addr}
#define MAKE_INST_HALT {.type = INST_HALT}

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
} Trap; // exceptions that stop the execution on the virtual machine

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
    default:
        assert(00 && "trap_as_cstr: Unreachnable");
    }
}

typedef __int64_t word;

typedef enum
{
    INST_PUSH,  // push a word to the stack top; we assume that our stack grows downwards
    INST_PLUS,  // add the last element on stack onto the second last element, and remove the last element from the stack
    INST_MINUS, // subtract the last element on the stack from the second last element, and remove the last element from the stack
    INST_MULT,  // multiply the last element on the stack to the second last element, and remove the last element from the stack
    INST_DIV,   // integer divide the second last element on the stack by the last element and store the result in the second last element, and then remove the last element from the stack
    INST_JMP,   // unconditional jump
    INST_HALT,  // halt the machine
} Inst_Type;    // enum for the instruction types

typedef struct
{
    Inst_Type type; // type of the instruction
    word operand;   // operand of the instruction
} Inst;             // structure for the actual instruction

typedef struct // structure defining the actual virtual machine
{
    word stack[VM_STACK_CAPACITY]; // the stack of the virtual machine; the stack top is the end of the array
    size_t stack_size;             // current stack size
    word instruction_pointer;      // the address of the next instruction to be executed
    int halt;
    Inst program[VM_PROGRAM_CAPACITY];
    size_t program_size;
} VirtualMachine;

const char *inst_type_as_cstr(Inst_Type type)
{
    switch (type)
    {
    case INST_PUSH:
        return "INST_PUSH";
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
    default:
        assert(0 && "inst_type_as_cstr: unreachable");
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
    case INST_PUSH:
        if (vm->stack_size >= VM_STACK_CAPACITY)
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
        if (vm->stack[vm->stack_size - 1] == 0)
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
        if (inst.operand < 0 || inst.operand >= vm->program_size)
        {
            return TRAP_ILLEGAL_JMP;
        }
        vm->instruction_pointer = inst.operand;
        break;
    default:
        return TRAP_ILLEGAL_INSTRUCTION;
        break;
    }
    return TRAP_OK;
}

void vm_init(VirtualMachine *vm, Inst *program, size_t program_size)
{
    vm->stack_size = 0;
    vm->program_size = program_size;
    memcpy(vm->program, program, program_size * sizeof(Inst));
    vm->instruction_pointer = 0;
    vm->halt = 0;
}

int vm_exec_program(VirtualMachine *vm)
{
    int ret;
    if (vm->program[vm->program_size - 1].type != INST_HALT)
    {
        return TRAP_NO_HALT_FOUND;
    }
    while (!vm->halt)
    {
        fprintf(stdout, "%s\n", inst_type_as_cstr(vm->program[vm->instruction_pointer].type));
        ret = vm_execute_at_inst_pointer(vm);
        vm_dump_stack(stdout, vm);
        if (ret != TRAP_OK)
        {
            fprintf(stderr, "Trap activated: %s\n", trap_as_cstr(ret));
            return ret;
            //vm_dump_stack(stderr, &vm);
        }
    }
    return SUCCESS;
}

void vm_push_inst(VirtualMachine *vm, Inst *inst)
{
    if (vm->program_size == VM_PROGRAM_CAPACITY)
    {
        fprintf(stderr, "Program Size Limit Exceeded");
        exit(EXIT_FAILURE);
    }

    vm->program[vm->program_size++ - 1] = *inst;
}

// VirtualMachine vm = {0}; // zeroes everything in vm (the stack is zeroed to)

static Inst program[] = {
    MAKE_INST_PUSH(69),
    MAKE_INST_PUSH(420),
    MAKE_INST_MULT,
    MAKE_INST_PUSH(10),
    MAKE_INST_JMP(89),
    MAKE_INST_MINUS,
    MAKE_INST_PUSH(0),
    MAKE_INST_DIV,
    //MAKE_INST_HALT,
}; // can't call functions in const arrays; so we made the instructions as macros

int main(/* int argc, char **argv */)
{
    VirtualMachine vm;
    vm_init(&vm, program, ARRAY_SIZE(program));
    int ret = vm_exec_program(&vm);
    if (ret == TRAP_NO_HALT_FOUND)
    {
        fprintf(stderr, "%s\n", trap_as_cstr(TRAP_NO_HALT_FOUND));
    }
    return EXIT_SUCCESS;
}