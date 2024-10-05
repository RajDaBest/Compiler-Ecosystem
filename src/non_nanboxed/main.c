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

static Trap vm_read(VirtualMachine *vm)
{
    if (vm->stack_size < 2)
    {
        return TRAP_STACK_UNDERFLOW;
    }

    uint64_t addr = vm->stack[vm->stack_size - 2]._as_u64;
    size_t len = vm->stack[vm->stack_size - 1]._as_u64;

    if (addr >= vm_memory_capacity)
    {
        return TRAP_ILLEGAL_MEMORY_ACCESS;
    }

    if (len >= vm_memory_capacity)
    {
        return TRAP_ILLEGAL_MEMORY_ACCESS;
    }

    if (len >= vm_memory_capacity - addr)
    {
        return TRAP_ILLEGAL_MEMORY_ACCESS;
    }

    fread(&vm->static_memory[addr], 1, len, stdin);

    vm->stack_size -= 2;
    return TRAP_OK;
}

static Trap vm_write(VirtualMachine *vm)
{
    if (vm->stack_size < 2)
    {
        return TRAP_STACK_UNDERFLOW;
    }

    uint64_t addr = vm->stack[vm->stack_size - 2]._as_u64;
    size_t len = vm->stack[vm->stack_size - 1]._as_u64;

    if (addr >= vm_memory_capacity)
    {
        return TRAP_ILLEGAL_MEMORY_ACCESS;
    }

    if (len >= vm_memory_capacity)
    {
        return TRAP_ILLEGAL_MEMORY_ACCESS;
    }

    if (len >= vm_memory_capacity - addr)
    {
        return TRAP_ILLEGAL_MEMORY_ACCESS;
    }

    fwrite(&vm->static_memory[addr], 1, len, stdout);

    return TRAP_OK;
}

static Trap vm_print_string(VirtualMachine *vm)
{
    if (vm->stack_size < 1)
    {
        return TRAP_STACK_UNDERFLOW;
    }

    uint64_t addr = vm->stack[vm->stack_size - 1]._as_u64;

    for (size_t i = addr; i < vm_memory_capacity; i++)
    {
        if (vm->static_memory[i] == '\0')
        {
            printf("%s\n", &vm->static_memory[addr]);
            return TRAP_OK;
        }
    }

    // printf("%s\n", &vm->static_memory[addr]); // may search beyond the buffer if null termination not found; but since memory is zeroed upon allocation, this wont be a problem
    // until every byte beyond the string is filled with a non zero value

    return TRAP_ILLEGAL_MEMORY_ACCESS;
}

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

#ifdef _WIN32
#define SYSTEM_COMMAND(command) system(command)
#define PATH_SEPARATOR ";"
#else
#define SYSTEM_COMMAND(command) system(command)
#define PATH_SEPARATOR ":"
#endif

#define MAX_LIB_PATHS 32
#define MAX_PATH_LENGTH 1024
#define MAX_COMMAND_LENGTH 4096

typedef struct
{
    char paths[MAX_LIB_PATHS][MAX_PATH_LENGTH];
    size_t count;
} LibPaths;

// ... [keep all the vm_* functions unchanged] ...

void print_usage_and_exit()
{
    fprintf(stderr, "Usage: ./virtmach --action <asm|run|pp> [--lib <library-path>]... [--vlib-ignore] [--stack-size <size>] [--program-capacity <size>] [--static-size <size>] [--limit <n>] [--save-vpp [filename]] [--debug] [--vpp] <input> [output]\n");
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

void split_env_paths(const char *env_value, LibPaths *lib_paths)
{
    if (!env_value)
        return;

    char env_copy[MAX_PATH_LENGTH];
    strncpy(env_copy, env_value, MAX_PATH_LENGTH - 1);
    env_copy[MAX_PATH_LENGTH - 1] = '\0';

    char *token = strtok(env_copy, PATH_SEPARATOR);
    while (token && lib_paths->count < MAX_LIB_PATHS)
    {
        strncpy(lib_paths->paths[lib_paths->count], token, MAX_PATH_LENGTH - 1);
        lib_paths->paths[lib_paths->count][MAX_PATH_LENGTH - 1] = '\0';
        lib_paths->count++;
        token = strtok(NULL, PATH_SEPARATOR);
    }
}

char *build_lib_path_string(const LibPaths *lib_paths)
{
    static char result[MAX_COMMAND_LENGTH] = "";
    result[0] = '\0';

    for (size_t i = 0; i < lib_paths->count; i++)
    {
        if (i > 0)
        {
            strcat(result, " --lib ");
        }
        else
        {
            strcat(result, "--lib ");
        }
        strcat(result, lib_paths->paths[i]);
    }

    return result;
}

int main(int argc, char **argv)
{
    int64_t limit = -1;
    int debug = 0;
    int save_vpp = 0;
    int use_vpp = 0;
    int vlib_ignore = 0;
    const char *vpp_filename = NULL;
    LibPaths lib_paths = {0};

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
            if (lib_paths.count >= MAX_LIB_PATHS)
            {
                fprintf(stderr, "ERROR: Too many library paths.\n");
                print_usage_and_exit();
            }
            strncpy(lib_paths.paths[lib_paths.count], argv[++i], MAX_PATH_LENGTH - 1);
            lib_paths.paths[lib_paths.count][MAX_PATH_LENGTH - 1] = '\0';
            lib_paths.count++;
        }
        else if (strcmp(argv[i], "--vlib-ignore") == 0)
        {
            vlib_ignore = 1;
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
            if (i + 1 < argc && argv[i + 1][0] != '-')
            {
                vpp_filename = argv[++i];
            }
        }
        else if (strcmp(argv[i], "--debug") == 0)
        {
            debug = 1;
        }
        else if (strcmp(argv[i], "--vpp") == 0)
        {
            use_vpp = 1;
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

    // Handle VLIB environment variable if not ignored
    if (!vlib_ignore)
    {
        LibPaths env_paths = {0};
        split_env_paths(getenv("VLIB"), &env_paths);

        // Shift existing paths to make room for env paths at the beginning
        if (env_paths.count > 0)
        {
            size_t total_paths = env_paths.count + lib_paths.count;
            if (total_paths > MAX_LIB_PATHS)
            {
                total_paths = MAX_LIB_PATHS;
            }

            if (lib_paths.count > 0)
            {
                memmove(&lib_paths.paths[env_paths.count],
                        lib_paths.paths,
                        (total_paths - env_paths.count) * MAX_PATH_LENGTH);
            }

            memcpy(lib_paths.paths, env_paths.paths,
                   env_paths.count * MAX_PATH_LENGTH);

            lib_paths.count = total_paths;
        }
    }

    // Check if we have any library paths
    if (lib_paths.count == 0)
    {
        fprintf(stderr, "ERROR: No library paths provided. Use --lib or set the VLIB environment variable.\n");
        print_usage_and_exit();
    }

    // Default vpp file name if not provided
    char default_vpp_file[MAX_PATH_LENGTH];
    if (!vpp_filename)
    {
        const char *dot = strrchr(input, '.');
        if (dot)
        {
            size_t base_len = dot - input;
            snprintf(default_vpp_file, sizeof(default_vpp_file), "%.*s.vpp", (int)base_len, input);
        }
        else
        {
            snprintf(default_vpp_file, sizeof(default_vpp_file), "%s.vpp", input);
        }
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
        char pre_process[MAX_COMMAND_LENGTH];
        if (use_vpp)
        {
            char *lib_path_string = build_lib_path_string(&lib_paths);
            if (vlib_ignore)
            {
                snprintf(pre_process, sizeof(pre_process), "vpp %s --vlib-ignore %s",
                         lib_path_string, input);
            }
            else
            {
                snprintf(pre_process, sizeof(pre_process), "vpp %s %s",
                         lib_path_string, input);
            }
        }
        else
        {
#ifdef _WIN32
            snprintf(pre_process, sizeof(pre_process), "cl /EP %s > %s", input, vpp_filename);
#else
            snprintf(pre_process, sizeof(pre_process), "cpp -P %s %s", input, vpp_filename);
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
        Inst program[VM_PROGRAM_CAPACITY * sizeof(Inst)];
        uint8_t data_section[VM_DEFAULT_MEMORY_SIZE];
#else
        Inst program[vm_program_capacity * sizeof(Inst)];
        uint8_t data_section[vm_default_memory_size];
#endif

        vm_header_ header = vm_translate_source(source, program, data_section);
        label_free();
        free((void *)source.data);
        vm_save_program_to_file(program, data_section, header, output);

        if (!save_vpp)
        {
            char rm_file[MAX_PATH_LENGTH];
#ifdef _WIN32
            snprintf(rm_file, sizeof(rm_file), "del %s", vpp_filename);
#else
            snprintf(rm_file, sizeof(rm_file), "rm %s", vpp_filename);
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
        vm_native_push(&vm, vm_print_string);
        vm_native_push(&vm, vm_read);
        vm_native_push(&vm, vm_write);
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
