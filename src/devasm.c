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

        case INST_PUSH:
            // Push instruction, prints operand
            if (is_nan(program[i].operand))
            {
                printf("%s %ld\n", inst_type_as_asm_str(type), return_value_signed(program[i].operand));
            }
            else
            {
                printf("%s %lf\n", inst_type_as_asm_str(type), program[i].operand);
            }
            break;

        case INST_DUP:
            // Duplicate, prints operand
            if (is_nan(program[i].operand))
            {
                printf("%s %ld\n", inst_type_as_asm_str(type), return_value_signed(program[i].operand));
            }
            else
            {
                printf("%s %lf\n", inst_type_as_asm_str(type), program[i].operand);
            }
            break;

        case INST_IPLUS:
            // Plus instruction, no operand
            printf("%s\n", inst_type_as_asm_str(type));
            break;

        case INST_IMINUS:
            // Minus instruction, no operand
            printf("%s\n", inst_type_as_asm_str(type));
            break;

        case INST_IMULT:
            // Multiply instruction, no operand
            printf("%s\n", inst_type_as_asm_str(type));
            break;

        case INST_IDIV:
            // Divide instruction, no operand
            printf("%s\n", inst_type_as_asm_str(type));
            break;

        case INST_FPLUS:
            // Plus instruction, no operand
            printf("%s\n", inst_type_as_asm_str(type));
            break;

        case INST_FMINUS:
            // Minus instruction, no operand
            printf("%s\n", inst_type_as_asm_str(type));
            break;

        case INST_FMULT:
            // Multiply instruction, no operand
            printf("%s\n", inst_type_as_asm_str(type));
            break;

        case INST_FDIV:
            // Divide instruction, no operand
            printf("%s\n", inst_type_as_asm_str(type));
            break;

        case INST_JMP:
            // Unconditional jump, prints operand
            if (is_nan(program[i].operand))
            {
                printf("%s %ld\n", inst_type_as_asm_str(type), return_value_signed(program[i].operand));
            }
            else
            {
                printf("%s %lf\n", inst_type_as_asm_str(type), program[i].operand);
            }
            break;

        case INST_HALT:
            // Halt instruction, no operand
            printf("%s\n", inst_type_as_asm_str(type));
            break;

        case INST_JMP_IF:
            // Conditional jump, prints operand
            if (is_nan(program[i].operand))
            {
                printf("%s %ld\n", inst_type_as_asm_str(type), return_value_signed(program[i].operand));
            }
            else
            {
                printf("%s %lf\n", inst_type_as_asm_str(type), program[i].operand);
            }
            break;

        case INST_EQ:
            // Equality check, no operand
            printf("%s\n", inst_type_as_asm_str(type));
            break;

        default:
            fprintf(stderr, "file '%s' corrupted\n", argv[1]);
            break;
        }
    }
    return EXIT_SUCCESS;
}
