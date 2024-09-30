#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define SYSTEM_COMMAND(command) system(command)
#else
#define SYSTEM_COMMAND(command) system(command)
#endif

#define _VM_IMPLEMENTATION
#include "./virt_mach.h"

static Trap vm_alloc(VirtualMachine *vm)
{
    if (vm->stack_size < 1)
    {
        return TRAP_STACK_UNDERFLOW;
    }

    uint64_t size = vm->stack[vm->stack_size - 1]._as_u64;

    if (size >= vm_memory_capacity) // vm_memory_capacity <= UINT64_MAX
    {
        return TRAP_ILLEGAL_MEMORY_ACCESS;
    }

    if (size >= vm_memory_capacity - vm->static_break)
    {
        return TRAP_ILLEGAL_MEMORY_ACCESS;
    }

    vm->static_break += size;
    vm->stack_size--;

    return TRAP_OK;
}

static Trap vm_free(VirtualMachine *vm)
{
    if (vm->stack_size < 1)
    {
        return TRAP_STACK_UNDERFLOW;
    }

    uint64_t size = vm->stack[vm->stack_size - 1]._as_u64;

    if (size >= vm->static_break) // vm->static_break < vm_memory_capacity
    {
        return TRAP_ILLEGAL_MEMORY_ACCESS;
    }

    vm->static_break -= size;

    vm->stack_size--;
    return TRAP_OK;
}

static Trap vm_print_f64(VirtualMachine *vm)
{
    if (vm->stack_size < 1)
    {
        return TRAP_STACK_UNDERFLOW;
    }

    double value = vm->stack[vm->stack_size - 1]._as_f64;
    fprintf(stdout, "%lf\n", value);

    return TRAP_OK;
}

static Trap vm_print_u64(VirtualMachine *vm)
{
    if (vm->stack_size < 1)
    {
        return TRAP_STACK_UNDERFLOW;
    }

    uint64_t value = vm->stack[vm->stack_size - 1]._as_u64;
    fprintf(stdout, "%llu\n", value);

    return TRAP_OK;
}

static Trap vm_print_s64(VirtualMachine *vm)
{
    if (vm->stack_size < 1)
    {
        return TRAP_STACK_UNDERFLOW;
    }

    int64_t value = vm->stack[vm->stack_size - 1]._as_s64;
    fprintf(stdout, "%lld\n", value);

    return TRAP_OK;
}

static Trap vm_dump_static(VirtualMachine *vm)
{
    if (vm->stack_size < 2)
    {
        return TRAP_STACK_UNDERFLOW;
    }

    uint64_t addr = vm->stack[vm->stack_size - 2]._as_u64;
    uint64_t count = vm->stack[vm->stack_size - 1]._as_u64;

    if (addr >= vm_memory_capacity) // vm_memory_capacity less than UINT64_MAX
    {
        return TRAP_ILLEGAL_MEMORY_ACCESS;
    }

    if (count > vm_memory_capacity - addr)
    {
        return TRAP_ILLEGAL_MEMORY_ACCESS;
    }

    for (size_t i = 0; i < count; i++)
    {
        printf("signed: %d, unsigned: %u\n", (unsigned int)vm->static_memory[addr + i], (int)vm->static_memory[addr + i]);
    }

    return TRAP_OK;
}
void print_usage_and_exit()
{
    fprintf(stderr, "Usage: ./virtmach --action <asm|run|pp> [--lib <library-path>] [--stack-size <size>] [--program-capacity <size>] [--static-size <size>] [--limit <n>] [--save-vpp [filename]] [--debug] [--vpp] <input> [output]\n");
    exit(EXIT_FAILURE);
}

int64_t parse_non_negative_int(const char *str)
{
    int64_t value = atoll(str);
    if (value < 0)
    {
        fprintf(stderr, "ERROR: Value must be non-negative.\n");
        print_usage_and_exit();
    }
    return value;
}

int main(int argc, char **argv)
{
    int64_t limit = -1; // Default instruction limit: unlimited (-1)
    int debug = 0;      // Default debug mode: disabled
    int save_vpp = 0;   // Flag to check if --save-vpp is provided
    int use_vpp = 0;    // Flag to check if --vpp is provided
    const char *vpp_filename = NULL;
    const char *lib_path = NULL; // Library path for --lib

    if (argc < 3)
    {
        print_usage_and_exit();
    }

    const char *action = NULL;
    const char *input = NULL;
    const char *output = NULL;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--action") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "ERROR: Missing value for --action.\n");
                print_usage_and_exit();
            }
            action = argv[++i];
        }
        else if (strcmp(argv[i], "--lib") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "ERROR: Missing value for --lib.\n");
                print_usage_and_exit();
            }
            lib_path = argv[++i]; // Capture the library path
        }
        else if (strcmp(argv[i], "--stack-size") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "ERROR: Missing value for --stack-size.\n");
                print_usage_and_exit();
            }
            vm_stack_capacity = parse_non_negative_int(argv[++i]);
        }
        else if (strcmp(argv[i], "--program-capacity") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "ERROR: Missing value for --program-capacity.\n");
                print_usage_and_exit();
            }
            vm_program_capacity = parse_non_negative_int(argv[++i]);
        }
        else if (strcmp(argv[i], "--limit") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "ERROR: Missing value for --limit.\n");
                print_usage_and_exit();
            }
            limit = atoll(argv[++i]);
        }
        else if (strcmp(argv[i], "--static-size") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "ERROR: Missing value for --static-size.\n");
                print_usage_and_exit();
            }
            vm_default_memory_size = parse_non_negative_int(argv[++i]);
        }
        else if (strcmp(argv[i], "--save-vpp") == 0)
        {
            save_vpp = 1;
            if (i + 1 < argc && argv[i + 1][0] != '-') // Check if there's a filename provided
            {
                vpp_filename = argv[++i];
            }
        }
        else if (strcmp(argv[i], "--debug") == 0)
        {
            debug = 1; // Enable debug mode if --debug is present
        }
        else if (strcmp(argv[i], "--vpp") == 0)
        {
            use_vpp = 1; // Enable vpp preprocessor if --vpp is present
        }
        else if (!input)
        {
            input = argv[i];
        }
        else if (!output)
        {
            output = argv[i];
        }
        else
        {
            fprintf(stderr, "ERROR: Too many arguments.\n");
            print_usage_and_exit();
        }
    }

    if (!action)
    {
        fprintf(stderr, "ERROR: Missing --action option.\n");
        print_usage_and_exit();
    }

    // Check if lib_path is provided or get it from the VLIB environment variable
    if (!lib_path)
    {
        lib_path = getenv("VLIB");
    }

    // If no lib_path is provided or set, produce an error
    if (!lib_path)
    {
        fprintf(stderr, "ERROR: No library path provided. Use --lib or set the VLIB environment variable.\n");
        print_usage_and_exit();
    }

    // Default vpp file name if not provided, without .vasm extension
    char default_vpp_file[100];
    if (!vpp_filename)
    {
        size_t i = 0;
        for (; i < strlen(input); i++)
        {
            if (input[i + 1] == '.')
            {
                break;
            }
        }
        memcpy(default_vpp_file, input, i + 1);
        default_vpp_file[i + 1] = '\0';
        sprintf(default_vpp_file, "%s.vpp", default_vpp_file);
        vpp_filename = default_vpp_file;
    }

    if (strcmp(action, "asm") == 0 || strcmp(action, "pp") == 0)
    {
        if (!input)
        {
            fprintf(stderr, "ERROR: Expected input file for the '%s' action.\n", action);
            print_usage_and_exit();
        }

        // Preprocess the input file
        char pre_process[400];
        if (use_vpp)
        {
            // Use lib_path for preprocessing
            sprintf(pre_process, "vpp --lib %s %s", lib_path, input);
        }
        else
        {
#ifdef _WIN32
            sprintf(pre_process, "cl /EP %s > %s", input, vpp_filename); // MSVC preprocessor
#else
            sprintf(pre_process, "cpp -P %s %s", input, vpp_filename); // GCC preprocessor for non-Windows
#endif
        }

        int result = SYSTEM_COMMAND(pre_process);
        if (result != 0)
        {
            fprintf(stderr, "ERROR: Preprocessing failed with error code: %d\n", result);
            exit(EXIT_FAILURE);
        }

        if (strcmp(action, "pp") == 0)
        {
            return EXIT_SUCCESS;
        }

        // Continue with assembly if action is "asm"
        String_View source = slurp_file(vpp_filename);

        assert(vm_program_capacity <= UINT64_MAX);
        assert(vm_memory_capacity <= UINT64_MAX);
        assert(vm_stack_capacity <= UINT64_MAX);
        assert(vm_default_memory_size < vm_memory_capacity);

        label_init();

#ifdef _WIN32
        Inst program[VM_PROGRAM_CAPACITY]; // clang does not fucking support run-time allocation of data segment memory
        uint8_t data_section[VM_DEFAULT_MEMORY_SIZE]
#else
        Inst program[vm_program_capacity];
        uint8_t data_section[vm_default_memory_size];
#endif

            vm_header_ header = vm_translate_source(source, program, data_section);
        label_free();
        free((void *)source.data);
        vm_save_program_to_file(program, data_section, header, output);
        /* for (size_t i = 0; i < header.data_section_size; i++)
        {
            printf("%d\n", data_section[i]);
        } */
        if (!save_vpp)
        {
            char rm_file[200];
#ifdef _WIN32
            sprintf(rm_file, "del %s", vpp_filename); // Use 'del' for Windows
#else
            sprintf(rm_file, "rm %s", vpp_filename); // Use 'rm' for Linux
#endif
            SYSTEM_COMMAND(rm_file);
        }

        return EXIT_SUCCESS;
    }
    else if (strcmp(action, "run") == 0)
    {
        if (!input)
        {
            fprintf(stderr, "ERROR: Expected a .vm file for the 'run' action.\n");
            print_usage_and_exit();
        }

        VirtualMachine vm;
        vm_init(&vm, input);
        vm_native_push(&vm, vm_alloc);
        vm_native_push(&vm, vm_free);
        vm_native_push(&vm, vm_print_f64);
        vm_native_push(&vm, vm_print_s64);
        vm_native_push(&vm, vm_print_u64);
        vm_native_push(&vm, vm_dump_static);
        vm_exec_program(&vm, limit, debug);
        vm_internal_free(&vm);

        return EXIT_SUCCESS;
    }
    else
    {
        fprintf(stderr, "ERROR: Unknown action '%s'. Expected 'asm', 'run', or 'pp'.\n", action);
        print_usage_and_exit();
    }

    return EXIT_FAILURE;
}