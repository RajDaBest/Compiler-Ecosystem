#define _VM_IMPLEMENTATION
#define _SV_IMPLEMENTATION

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include "./virt_mach.h"
#include "./String_View.h"

#define VM_DEFINE_LIMIT 128
#define MAX_PATH_LENGTH 1024
#define MAX_INCLUDE_FILE_LENGTH 4096

typedef struct
{
    String_View label_name;
    String_View label_value;
} define_labels;

define_labels *define_labels_array;
size_t define_labels_array_counter = 0;
bool preprocessing_failed = false;

void push_to_define_array(String_View label_name, String_View label_value)
{
    if (define_labels_array_counter >= VM_DEFINE_LIMIT)
    {
        fprintf(stderr, "ERROR: define labels array overflow.\n");
        exit(EXIT_FAILURE);
    }

    define_labels_array[define_labels_array_counter++] = (define_labels){.label_name = label_name, .label_value = label_value};
}

void print_usage_and_exit()
{
    fprintf(stderr, "Usage: ./program --lib <library_path> <input_file> [output_file]\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        print_usage_and_exit();
    }

    char *lib_path = NULL;
    char *input = NULL;
    char *output = NULL;

    // Argument parsing
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--lib") == 0)
        {
            if (i + 1 < argc)
            {
                lib_path = argv[i + 1];
                i++; // Skip the next argument as it's part of --lib
            }
            else
            {
                fprintf(stderr, "ERROR: Missing value for --lib argument.\n");
                print_usage_and_exit();
            }
        }
        else if (!input)
        {
            input = argv[i];
        }
        else if (!output)
        {
            output = argv[i];
        }
    }

    // Check the VLIB environment variable if --lib is not provided
    if (!lib_path)
    {
        lib_path = getenv("VLIB");
        if (!lib_path)
        {
            fprintf(stderr, "ERROR: No --lib argument provided, and VLIB environment variable is not set.\n");
            print_usage_and_exit();
        }
    }

    if (!input)
    {
        fprintf(stderr, "ERROR: Missing required input file.\n");
        print_usage_and_exit();
    }

    // If no output file provided, create one from input filename
    if (!output)
    {
        output = malloc(MAX_PATH_LENGTH);
        snprintf(output, MAX_PATH_LENGTH, "%.*s.vpp", (int)(strrchr(input, '.') - input), input);
    }

    define_labels_array = (define_labels *)malloc(VM_DEFINE_LIMIT * sizeof(define_labels));
    if (!define_labels_array)
    {
        fprintf(stderr, "ERROR: define labels array allocation failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    while (true)
    {

        String_View include_processing = slurp_file(input);
        String_View copy_one = include_processing;
        size_t line_no = 0;

        if (include_processing.data == NULL)
        {
            fprintf(stderr, "ERROR: Failed to read input file: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        FILE *temp = fopen("temp", "w");
        if (!temp)
        {
            fprintf(stderr, "ERROR: Failed to create temp file: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        size_t include_no = 0;
        while (include_processing.count > 0)
        {
            String_View line = sv_chop_by_delim(&include_processing, '\n');
            /* line = sv_chop_by_delim(&line, ';'); // Remove comments
            sv_trim_left(&line);
            sv_trim_right(&line);

            if (line.count == 0)
                continue; // Ignore empty lines */

            if (*line.data == '%')
            {
                String_View directive = sv_chop_by_delim(&line, ' ');
                sv_trim_left(&line);

                if (sv_eq(directive, cstr_as_sv("%include")))
                {
                    if (line.count == 0)
                    {
                        fprintf(stderr, "Line Number %zu -> ERROR: %%include directive has no file\n", line_no);
                        preprocessing_failed = true;
                        continue;
                    }

                    String_View include_label = sv_chop_by_delim(&line, '>');
                    sv_chop_by_delim(&include_label, '<');
                    if (!include_label.count)
                    {
                        fprintf(stderr, "Line number %zu -> ERROR: Invalid %%include usage\n", line_no);
                        preprocessing_failed = true;
                        continue;
                    }

                    char file_name[MAX_PATH_LENGTH];
                    snprintf(file_name, sizeof(file_name), "%s/%.*s", lib_path, (int)include_label.count, include_label.data);

                    FILE *file = fopen(file_name, "r");
                    if (!file)
                    {
                        fprintf(stderr, "ERROR: Failed to open file %s: %s\n", file_name, strerror(errno));
                        exit(EXIT_FAILURE);
                    }

                    fseek(file, 0, SEEK_END);
                    size_t file_size = ftell(file);
                    fseek(file, 0, SEEK_SET);

                    if (file_size > MAX_INCLUDE_FILE_LENGTH)
                    {
                        fprintf(stderr, "Line Number %zu -> ERROR: %%include <%s> exceeded MAX_INCLUDE_FILE_LENGTH\n", line_no, file_name);
                        fclose(file);
                        exit(EXIT_FAILURE);
                    }

#ifdef _WIN32
                    char include_file_array[MAX_INCLUDE_FILE_LENGTH]; // clang does not fucking support variable length arrays
#else
                    char include_file_array[file_size];
#endif
                    fread(include_file_array, file_size, 1, file);
                    fwrite(include_file_array, file_size, 1, temp);
                    fprintf(temp, "\n");

                    fclose(file);
                    include_no++;
                }
                else
                {
                    fprintf(temp, "%.*s %.*s\n", (int)directive.count, directive.data, (int)line.count, line.data);
                }
            }
            else
            {
                fprintf(temp, "%.*s\n", (int)line.count, line.data);
            }
            fflush(temp);
            line_no++;
        }

        free((void *)copy_one.data);
        fclose(temp);
        if (!include_no)
        {
            break;
        }

        input = "temp";
    }
    // Preprocessing phase: Handle include directives

    // Open the VPP output file
    FILE *vpp = fopen(output, "w");
    if (!vpp)
    {
        fprintf(stderr, "ERROR: Failed to open file %s: %s\n", output, strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Processing the temp file with defines
    String_View unprocessed_file = slurp_file("temp");
    String_View copy_two = unprocessed_file;

    size_t line_no = 0;
    while (unprocessed_file.count > 0)
    {
        String_View line = sv_chop_by_delim(&unprocessed_file, '\n');
        sv_trim_left(&line);
        sv_trim_right(&line);

        if (*line.data == '%')
        {
            String_View directive = sv_chop_by_delim(&line, ' ');
            sv_trim_left(&line);

            if (sv_eq(directive, cstr_as_sv("%define")))
            {
                if (line.count == 0)
                {
                    fprintf(stderr, "Line Number %zu -> ERROR: %%define directive has no label\n", line_no);
                    preprocessing_failed = true;
                    continue;
                }

                String_View define_label = sv_chop_by_delim(&line, ' ');
                String_View label_value = sv_chop_by_delim(&line, ' ');

                if (!label_value.count)
                {
                    push_to_define_array(define_label, (String_View){0});
                }
                else
                {
                    push_to_define_array(define_label, label_value);
                }
            }
        }
        else
        {
            // Replace tokens with defined values
            while (line.count > 0)
            {
                String_View token = sv_chop_by_delim(&line, ' ');
                bool found = false;

                for (size_t i = 0; i < define_labels_array_counter; i++)
                {
                    if (sv_eq(define_labels_array[i].label_name, token))
                    {
                        if (define_labels_array[i].label_value.count > 0)
                        {
                            fprintf(vpp, "%.*s ", (int)define_labels_array[i].label_value.count, define_labels_array[i].label_value.data);
                        }
                        found = true;
                        break;
                    }
                }

                if (!found)
                {
                    fprintf(vpp, "%.*s ", (int)token.count, token.data);
                }
            }
            fprintf(vpp, "\n");
        }
        fflush(vpp);
        line_no++;
    }

    // Cleanup
    free(define_labels_array);
    free((void *)copy_two.data);
    fclose(vpp);
    remove("./temp");

    return preprocessing_failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
