#define _VM_IMPLEMENTATION
#include "./virt_mach.h"

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
    uint8_t data_section[VM_DEFAULT_MEMORY_SIZE];
#else
    Inst program[vm_program_capacity];
    uint8_t data_section[vm_default_memory_size];
#endif
    vm_header_ header = vm_load_program_from_file(program, data_section, input);
    size_t program_size = header.code_section_size;

     for (size_t i = 0; i < header.data_section_size; i++)
    {
        printf("%d\n", data_section[i]);
    } 

    for (size_t i = 0; i < program_size; i++)
    {
        printf("%s ", get_inst_name(program[i].type));
        if (has_operand_function(program[i].type))
        {
            uint8_t op_type = get_operand_type(program[i].type);
            if (op_type == TYPE_DOUBLE)
            {
                printf("%lf", program[i].operand._as_f64);
            }
            else if (op_type == TYPE_SIGNED_64INT)
            {
                printf("%lld", program[i].operand._as_s64);
            }
            else if (op_type == TYPE_UNSIGNED_64INT)
            {
                printf("%llu", program[i].operand._as_u64);
            }
        }
        printf("\n");
    }
    return EXIT_SUCCESS;
}
