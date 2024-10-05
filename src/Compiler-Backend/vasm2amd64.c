#define VM_IMPLEMENTATION
#include "../non_nanboxed/virt_mach.h";

#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv)
{

    char *input = argv[1];

#ifdef _WIN32

    Inst program[VM_PROGRAM_CAPACITY * sizeof(Inst)];
    uint8_t data_section[VM_DEFAULT_MEMORY_SIZE];

#else

    Inst program[vm_program_capacity * sizeof(Inst)];
    uint8_t data_section[vm_default_memory_size];

#endif

    vm_header_ header = vm_load_program_from_file(program, data_section, input);

    return EXIT_SUCCESS;
}