#define _VM_IMPLEMENTATION
#include "virt_mach.h"

void print_usage_and_exit() {
    fprintf(stderr, "Usage: ./virtmach -action <anc|run> [options] [<input.vasm> <output.vm>] or [<file.vm>]\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -stack-size <size_in_bytes>       Set the virtual machine's stack size (non-negative integer)\n");
    fprintf(stderr, "  -program-capacity <size_in_bytes> Set the program capacity in bytes (non-negative integer)\n");
    exit(EXIT_FAILURE);
}

int parse_non_negative_int(const char *arg) {
    int value = atoi(arg);
    if (value < 0) {
        fprintf(stderr, "ERROR: Expected non-negative integer, got '%s'.\n", arg);
        print_usage_and_exit();
    }
    return value;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        print_usage_and_exit();
    }

    const char *action = NULL;
    const char *input = NULL;
    const char *output = NULL;

    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-action") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "ERROR: Missing value for -action.\n");
                print_usage_and_exit();
            }
            action = argv[++i];
        } else if (strcmp(argv[i], "-stack-size") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "ERROR: Missing value for -stack-size.\n");
                print_usage_and_exit();
            }
            vm_stack_capacity = parse_non_negative_int(argv[++i]);
        } else if (strcmp(argv[i], "-program-capacity") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "ERROR: Missing value for -program-capacity.\n");
                print_usage_and_exit();
            }
            vm_program_capacity = parse_non_negative_int(argv[++i]);
        } else if (!input) {
            input = argv[i];  // First non-option argument is input
        } else if (!output) {
            output = argv[i];  // Second non-option argument is output
        } else {
            fprintf(stderr, "ERROR: Too many arguments.\n");
            print_usage_and_exit();
        }
    }

    // Ensure action was provided
    if (!action) {
        fprintf(stderr, "ERROR: Missing -action option.\n");
        print_usage_and_exit();
    }

    // Handle the "anc" action
    if (strcmp(action, "anc") == 0) {
        if (!input || !output) {
            fprintf(stderr, "./virtmach -action anc <input.vasm> <output.vm>\n");
            fprintf(stderr, "ERROR: expected input and output files for the 'anc' action.\n");
            exit(EXIT_FAILURE);
        }

        String_View source = slurp_file(input);
        Inst program[vm_program_capacity];
        size_t program_size = vm_translate_source(source, program, vm_program_capacity);
        vm_save_program_to_file(program, program_size, output);

        return EXIT_SUCCESS;

    } else if (strcmp(action, "run") == 0) {
        if (!input) {
            fprintf(stderr, "./virtmach -action run <file.vm>\n");
            fprintf(stderr, "ERROR: expected a .vm file for the 'run' action.\n");
            exit(EXIT_FAILURE);
        }

        VirtualMachine vm;
        vm_init(&vm);
        vm.program_size = vm_load_program_from_file(&(vm.program[0]), input);
        printf("%d\n", vm_exec_program(&vm) == TRAP_NO_HALT_FOUND);

        return EXIT_SUCCESS;

    } else {
        fprintf(stderr, "ERROR: Unknown action '%s'. Expected 'anc' or 'run'.\n", action);
        print_usage_and_exit();
    }

    return EXIT_FAILURE;
}