#define _VM_IMPLEMENTATION
#include "virt_mach.h"

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "ERROR: no input is provided\n");
        fprintf(stderr, "Usage: ./devasm <input.vm>\n");
        exit(EXIT_FAILURE);
    }

    const char *input = argv[1];

    Inst program[vm_program_capacity];
    size_t program_size = vm_load_program_from_file(program, input);

    for (size_t i = 0; i < program_size; i++)
    {
        Inst_Type type = program[i].type;
        switch (type)
        {
        case INST_NOP:
            // No operation
            printf("%s\n", inst_type_as_asm_str(type));
            break;

        case INST_UPUSH:
            // Push instruction, prints operand
            printf("%s %ld\n", inst_type_as_asm_str(type), return_value_signed(program[i].operand));
            break;

        case INST_SPUSH:

            printf("%s %ld\n", inst_type_as_asm_str(type), return_value_unsigned(program[i].operand));
            break;

        case INST_FPUSH:

            printf("%s %lf\n", inst_type_as_asm_str(type), program[i].operand);
            break;

        case INST_DUP:
            // Duplicate, prints operand
            printf("%s %ld\n", inst_type_as_asm_str(type), return_value_unsigned(program[i].operand));

            break;

        case INST_UPLUS:
            // Push instruction, prints operand
            printf("%s\n", inst_type_as_asm_str(type));
            break;

        case INST_SPLUS:

            printf("%s\n", inst_type_as_asm_str(type));
            break;

        case INST_FPLUS:

            printf("%s\n", inst_type_as_asm_str(type));
            break;

        case INST_UMINUS:
            // Push instruction, prints operand
            printf("%s\n", inst_type_as_asm_str(type));
            break;

        case INST_SMINUS:

            printf("%s\n", inst_type_as_asm_str(type));
            break;

        case INST_FMINUS:

            printf("%s\n", inst_type_as_asm_str(type));
            break;

        case INST_UMULT:
            // Push instruction, prints operand
            printf("%s\n", inst_type_as_asm_str(type));
            break;

        case INST_SMULT:

            printf("%s\n", inst_type_as_asm_str(type));
            break;

        case INST_FMULT:

            printf("%s\n", inst_type_as_asm_str(type));
            break;

        case INST_SDIV:
            // Push instruction, prints operand
            printf("%s\n", inst_type_as_asm_str(type));
            break;

        case INST_UDIV:

            printf("%s\n", inst_type_as_asm_str(type));
            break;

        case INST_FDIV:

            printf("%s\n", inst_type_as_asm_str(type));
            break;

        case INST_JMP:
            // Unconditional jump, prints operand
            printf("%s %lu\n", inst_type_as_asm_str(type), return_value_unsigned(program[i].operand));
            break;

        case INST_HALT:
            // Halt instruction, no operand
            printf("%s\n", inst_type_as_asm_str(type));
            break;

        case INST_JMP_IF:
            // Conditional jump, prints operand
            printf("%s %lu\n", inst_type_as_asm_str(type), return_value_unsigned(program[i].operand));
            break;

        case INST_EQ:
            // Equality check, no operand
            printf("%s\n", inst_type_as_asm_str(type));
            break;

        case INST_ASR:
            printf("%s %lu\n", inst_type_as_asm_str(type), return_value_unsigned(program[i].operand));
            break;

        case INST_LSR:
            printf("%s %lu\n", inst_type_as_asm_str(type), return_value_unsigned(program[i].operand));
            break;

        case INST_SL:
            printf("%s %lu\n", inst_type_as_asm_str(type), return_value_unsigned(program[i].operand));
            break;

        default:
            fprintf(stderr, "file '%s' corrupted\n", argv[1]);
            break;
        }
    }
    return EXIT_SUCCESS;
}
