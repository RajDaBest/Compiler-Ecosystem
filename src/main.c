#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define SYSTEM_COMMAND(command) system(command)
#else
#define SYSTEM_COMMAND(command) system(command)
#endif

#define _VM_IMPLEMENTATION
#include "virt_mach.h"

static Trap vm_alloc(VirtualMachine *vm)
{
    if (vm->stack_size < 1)
    {
        return TRAP_STACK_UNDERFLOW;
    }

    uint64_t actual_value = (uint64_t)malloc(return_value_unsigned(vm->stack[vm->stack_size - 1]));
    set_unsigned_64int(&vm->stack[vm->stack_size - 1], actual_value);

    return TRAP_OK;
}

static Trap vm_free(VirtualMachine *vm)
{
    if (vm->stack_size < 1)
    {
        return TRAP_STACK_UNDERFLOW;
    }

    uint64_t ptr = return_value_unsigned(vm->stack[vm->stack_size - 1]);
    vm->stack_size--;
    free((void *)ptr);

    return TRAP_OK;
}

static Trap vm_print_f64(VirtualMachine *vm)
{
    if (vm->stack_size < 1)
    {
        return TRAP_STACK_UNDERFLOW;
    }

    double value = vm->stack[vm->stack_size - 1];
    fprintf(stdout, "%lf\n", value);

    return TRAP_OK;
}

static Trap vm_print_u64(VirtualMachine *vm)
{
    if (vm->stack_size < 1)
    {
        return TRAP_STACK_UNDERFLOW;
    }

    uint64_t value = return_value_unsigned(vm->stack[vm->stack_size - 1]);
    fprintf(stdout, "%llu\n", value);

    return TRAP_OK;
}

static Trap vm_print_s64(VirtualMachine *vm)
{
    if (vm->stack_size < 1)
    {
        return TRAP_STACK_UNDERFLOW;
    }

    uint64_t value = return_value_signed(vm->stack[vm->stack_size - 1]);
    fprintf(stdout, "%lld\n", value);

    return TRAP_OK;
}

void print_usage_and_exit()
{
    fprintf(stderr, "Usage: ./virtmach --action <asm|run|pp> [--stack-size <size>] [--program-capacity <size>] [--limit <n>] [--save-vpp [filename]] [--debug] <input> [output]\n");
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
    int debug = 0;        // Default debug mode: disabled
    int save_vpp = 0;     // Flag to check if --save-vpp is provided
    const char *vpp_filename = NULL;

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

    if (strcmp(action, "asm") == 0)
    {
        if (!input || !output)
        {
            fprintf(stderr, "ERROR: Expected input and output files for the 'asm' action.\n");
            print_usage_and_exit();
        }

        // Preprocess the input file
        char pre_process[300];
#ifdef _WIN32
        sprintf(pre_process, "cl /EP %s > %s", input, vpp_filename); // MSVC preprocessor
#else
        sprintf(pre_process, "cpp -P %s %s", input, vpp_filename);   // GCC preprocessor for non-Windows
#endif
        int result = SYSTEM_COMMAND(pre_process);
        if (result != 0)
        {
            fprintf(stderr, "ERROR: Preprocessing failed with error code: %d\n", result);
            exit(EXIT_FAILURE);
        }

        printf("Processing file: %s\n", vpp_filename);
        String_View source = slurp_file(vpp_filename);

        label_init();
        Inst program[vm_program_capacity];
        vm_header_ header = vm_translate_source(source, program, vm_program_capacity);
        label_free();
        free((void *)source.data);
        vm_save_program_to_file(program, header, output);

        if (!save_vpp)
        {
            char rm_file[200];
#ifdef _WIN32
            sprintf(rm_file, "del %s", vpp_filename); // Use 'del' for Windows
#else
            sprintf(rm_file, "rm %s", vpp_filename);  // Use 'rm' for Linux
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
        vm_exec_program(&vm, limit, debug);
        vm_internal_free(&vm);

        return EXIT_SUCCESS;
    }
    else if (strcmp(action, "pp") == 0)
    {
        if (!input)
        {
            fprintf(stderr, "ERROR: Expected input file for the 'pp' action.\n");
            print_usage_and_exit();
        }

        // Preprocess the input file
        char pre_process[300];
#ifdef _WIN32
        sprintf(pre_process, "cl /EP %s > %s", input, vpp_filename); // MSVC preprocessor
#else
        sprintf(pre_process, "cpp -P %s %s", input, vpp_filename);   // GCC preprocessor for non-Windows
#endif
        int result = SYSTEM_COMMAND(pre_process);
        if (result != 0)
        {
            fprintf(stderr, "ERROR: Preprocessing failed with error code: %d\n", result);
            exit(EXIT_FAILURE);
        }

        printf("Preprocessed file saved as: %s\n", vpp_filename);
        return EXIT_SUCCESS;
    }
    else
    {
        fprintf(stderr, "ERROR: Unknown action '%s'. Expected 'asm', 'run', or 'pp'.\n", action);
        print_usage_and_exit();
    }

    return EXIT_FAILURE;
}
