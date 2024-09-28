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

#ifdef _WIN32
    Inst program[VM_PROGRAM_CAPACITY];
#else
    Inst program[vm_program_capacity];
#endif
    vm_header_ header = vm_load_program_from_file(program, input);
    size_t program_size = header.code_section_size;

    for (size_t i = 0; i < program_size; i++)
    {
        printf("%s ", get_inst_name(program[i].type));
        if (has_operand_function(program[i].type))
        {
            uint8_t op_type = get_operand_type(program[i].type);
            if (op_type == TYPE_DOUBLE)
            {
                printf("%lf", program[i].operand);
            }
            else if (op_type == TYPE_SIGNED_64INT)
            {
                printf("%lld", return_value_signed(program[i].operand));
            }
            else if (op_type == TYPE_UNSIGNED_64INT)
            {
                printf("%llu", return_value_unsigned(program[i].operand));
            }
        }
        printf("\n");
    }
    return EXIT_SUCCESS;
}
