#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <stdbool.h>

#define ANS 1
#define RUN 2
#define ACTION ANS

#define SUCCESS 1
#define FAILURE 0
#define OPERAND_OVERFLOW -1
#define VM_STACK_CAPACITY 128
#define VM_PROGRAM_CAPACITY 1024
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define MAKE_INST_PUSH(value) {.type = INST_PUSH, .operand = (value)}
#define MAKE_INST_DUP(rel_addr) {.type = INST_DUP, .operand = (rel_addr)}
#define MAKE_INST_PLUS {.type = INST_PLUS}
#define MAKE_INST_MINUS {.type = INST_MINUS}
#define MAKE_INST_MULT {.type = INST_MULT}
#define MAKE_INST_DIV {.type = INST_DIV}
#define MAKE_INST_JMP(addr) {.type = INST_JMP, .operand = (addr)}
#define MAKE_INST_HALT {.type = INST_HALT}
#define MAKE_INST_JMP_IF(addr) {.type = INST_JMP_IF, .operand = (addr)}
#define MAKE_INST_EQ {.type = INST_EQ}
#define MAKE_INST_NOP {.type = INST_NOP}

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

typedef __int64_t word;

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
    word stack[VM_STACK_CAPACITY]; // the stack of the virtual machine; the stack top is the end of the array
    size_t stack_size;             // current stack size
    word instruction_pointer;      // the address of the next instruction to be executed
    int halt;
    Inst program[VM_PROGRAM_CAPACITY]; // the actual instruction array
    size_t program_size;               // number of instructions in the program
} VirtualMachine;

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
        if (vm->stack_size >= VM_STACK_CAPACITY)
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

void vm_init(VirtualMachine *vm, Inst *program, size_t program_size)
{
    vm->stack_size = 0;
    vm->program_size = program_size;
    memcpy(vm->program, program, program_size * sizeof(program[0]));
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
            // vm_dump_stack(stderr, &vm);
        }
        // vm_dump_stack(stdout, &vm);
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

void vm_save_program_to_file(Inst *program, size_t program_size, const char *file_path)
{
    FILE *f = fopen(file_path, "wb");
    if (!f)
    {
        fprintf(stderr, "ERROR: Could not open file '%s': %s\n", file_path, strerror(errno));
        exit(EXIT_FAILURE);
    }

    fwrite(program, sizeof(program[0]), program_size, f);
    if (ferror(f)) // did some error occur due to the last stdio function call on f?
    {
        fprintf(stderr, "ERROR: Could not write to file '%s': %s\n", file_path, strerror(errno));
        fclose(f);
        exit(EXIT_FAILURE);
    }

    fclose(f);
}

void vm_load_program_from_file(VirtualMachine *vm, const char *file_path)
{
    long ret;
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

    assert(ret % sizeof(vm->program[0]) == 0);
    assert((size_t)ret <= VM_PROGRAM_CAPACITY * sizeof(vm->program[0]));

    if (fseek(f, 0, SEEK_SET))
    {
        fclose(f);
        fprintf(stderr, "ERROR: Could not read file '%s': %s\n", file_path, strerror(errno));
        exit(EXIT_FAILURE);
    }

    vm->program_size = fread(vm->program, sizeof(vm->program[0]), ret / sizeof(vm->program[0]), f);
    if (ferror(f))
    {
        fclose(f);
        fprintf(stderr, "ERROR: Could not read file '%s': %s\n", file_path, strerror(errno));
        exit(EXIT_FAILURE);
    }

    vm->stack_size = 0;
    vm->instruction_pointer = 0;
    vm->halt = 0;

    fclose(f);
}

// VirtualMachine vm = {0}; // zeroes everything in vm (the stack is zeroed to)

/* static Inst program[] = {
    MAKE_INST_PUSH(0), // 0 1 0
    MAKE_INST_PUSH(1),
    MAKE_INST_DUP(1),
    MAKE_INST_DUP(1),
    MAKE_INST_PLUS,
    MAKE_INST_JMP(2),
    MAKE_INST_HALT,
}; */
// can't call functions in const arrays; so we made the instructions as macros

/* const char *source_code =
    "   push      0\n"
    "push 1\n"
    "dup 1\n"
    "dup 1\n"
    "plus  \n"
    "jmp 2   \n"
    "halt\n"; */ // these individual strings are concatenated into a single string

// equivalent to const char *source_code = "push 0\npush 1\ndup 1\ndup 1\nplus\njmp 2\nhalt\n";

typedef struct
{
    size_t count;
    char *data;
} String_View;

String_View cstr_as_sv(char *cstr)
{
    return (String_View){
        .count = strlen(cstr),
        .data = cstr,
    };
}

String_View sv_chop_by_delim(String_View *sv, const char delim)
{
    String_View chopped = {0}; // Initialize chopped.count to 0
    chopped.data = sv->data;

    // Loop through until the delimiter or end of string is found
    while (sv->count > 0 && *(sv->data) != delim) // if there's no delim at the end; sv.count will be zero and will point to a null byte
    {
        sv->count--;
        chopped.count++;
        sv->data++;
    }

    // If delimiter found and not at the end of the string
    if (sv->count > 0 && *(sv->data) == delim)
    {
        sv->count--; // Skip the delimiter
        sv->data++;  // Move past the delimiter
    }

    return chopped;
}

void sv_trim_left(String_View *line)
{
    while (isspace((int)*(line->data)) && line->count > 0)
    {
        line->data++;
        line->count--;
    }
}

void sv_trim_right(String_View *line)
{
    while (isspace((int)(line->data[line->count - 1])) && line->count > 0)
    {
        line->count--;
    }
}

bool sv_eq(String_View a, String_View b)
{
    if (a.count != b.count)
    {
        return false;
    }

    for (size_t i = 0; i < a.count; i++)
    {
        if (a.data[i] != b.data[i])
        {
            return false;
        }
    }

    return true;
}

int str_errno = SUCCESS;

word sv_to_int(String_View *op)
{
    word num = 0;
    for (size_t i = 0; i < op->count; i++)
    {
        int dig = op->data[i] - '0';
        if (dig > 9 || dig < 0)
        {
            str_errno = FAILURE; // Set error if non-digit is encountered
            return -1;
        }

        // Check for overflow before multiplying and adding
        if (num > (__INT_LEAST64_MAX__ - dig) / 10)
        {
            str_errno = OPERAND_OVERFLOW; // Set error if overflow occurs
            return -1;
        }

        num = num * 10 + dig;
    }
    return num;
}

Inst vm_translate_line(String_View line)
{
    sv_trim_left(&line);
    String_View inst_name = sv_chop_by_delim(&line, ' ');
    // printf("%d\n", line.count);
    sv_trim_left(&line);
    sv_trim_right(&line);
    bool has_operand = line.count > 0;
    // line should point to a null byte now if the instruction doesn't have any operands

    /* for(size_t i = 0; i < inst_name.count; i++)
    {
        printf("%c", inst_name.data[i]);
    } */
    // printf("%.*s\n", (int)inst_name.count, inst_name.data);
    // printf("%.*s\n", (int)line.count, line.data);

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
            fprintf(stderr, "ERROR: %.*s is not a valid integer\n", (int)line.count, line.data);
            exit(EXIT_FAILURE);
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
            fprintf(stderr, "ERROR: %.*s is not a valid integer\n", (int)line.count, line.data);
            exit(EXIT_FAILURE);
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
        fprintf(stderr, "invalid instruction\n");
        exit(EXIT_FAILURE);
    }
}

size_t vm_translate_source(String_View source, Inst *program, size_t program_capacity)
{
    size_t program_size = 0;
    while (source.count > 0) // source.count is zero if we reach the end of source
    {
        if (program_size >= program_capacity)
        {
            fprintf(stderr, "Program Too Big\n");
        }
        String_View line = sv_chop_by_delim(&source, '\n');
        // printf("#%.*s#\n", (int)line.count, line.data);
        program[program_size++] = vm_translate_line(line);
    }
    return program_size;
}

char *trim_left(char *str, size_t str_size)
{
    for (size_t i = 0; i < str_size; i++)
    {
        if (!isspace((int)*str))
        {
            return str;
        }
        str++;
    }
    return str;
} // trims the left of the string to remove all the white spaces; returns pointer to the first non-whitespace character in the string;
// if the string is all whitespaces, returns the pointer to the end of the string, i.e, the null byte pointer

/* Inst vm_translate_line(char *line, size_t line_size)
{
    {
        char *line_start = line;
        char *actual_start = trim_left(line, line_size);
        line_size -= (actual_start - line_start);
    }

    if (!line_size)
    {
        fprintf(stderr, "ERROR: Could not translate empty line to an instruction\n");
        exit(EXIT_FAILURE);
    }
} */

String_View slurp_file(const char *file_path)
{
    int ret = 0;
    FILE *f = fopen(file_path, "r");
    if (!f)
    {
        fprintf(stderr, "ERROR: Couldn't open file: '%s': %s", file_path, strerror(errno));
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

    fread(buffer, 1, ret, f);
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

int main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "Usage: ./virtmach -action <anc|run> [<input.vasm> <output.vm>] or [<file.vm>]\n");
        exit(EXIT_FAILURE);
    }

    // Check if the first argument is -action
    if (strcmp(argv[1], "-action") != 0) {
        fprintf(stderr, "ERROR: Expected -action flag.\n");
        fprintf(stderr, "Usage: ./virtmach -action <anc|run> [<input.vasm> <output.vm>] or [<file.vm>]\n");
        exit(EXIT_FAILURE);
    }

    // Check the action value
    const char *action = argv[2];
    
    if (strcmp(action, "anc") == 0) {
        // ANC action code
        if (argc < 5) {
            fprintf(stderr, "./virtmach -action anc <input.vasm> <output.vm>\n");
            fprintf(stderr, "ERROR: expected input and output\n");
            exit(EXIT_FAILURE);
        }

        char *input = argv[3];
        char *output = argv[4];

        String_View source = slurp_file(input);
        VirtualMachine vm;
        vm.program_size = vm_translate_source(source, &(vm.program[0]), VM_PROGRAM_CAPACITY);
        vm_save_program_to_file(vm.program, vm.program_size, output);

        return EXIT_SUCCESS;

    } else if (strcmp(action, "run") == 0) {
        // RUN action code
        if (argc < 4) {
            fprintf(stderr, "./virtmach -action run <file.vm>\n");
            fprintf(stderr, "ERROR: Expected a .vm file\n");
            exit(EXIT_FAILURE);
        }

        VirtualMachine vm = {0};
        const char *input = argv[3];
        vm_load_program_from_file(&vm, input);
        vm_exec_program(&vm);

        return EXIT_SUCCESS;

    } else {
        // Invalid action
        fprintf(stderr, "ERROR: Unknown action '%s'. Expected 'anc' or 'run'.\n", action);
        fprintf(stderr, "Usage: ./virtmach -action <anc|run> [<input.vasm> <output.vm>] or [<file.vm>]\n");
        exit(EXIT_FAILURE);
    }
}