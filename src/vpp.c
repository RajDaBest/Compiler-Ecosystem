#define _VM_IMPLEMENTATION
#define _SV_IMPLEMENTATION

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <libgen.h>
#include "virt_mach.h"
#include "String_View.h"

#define VM_DEFINE_LIMIT 128
#define MAX_PATH_LENGTH 1024

typedef struct
{
    String_View label_name;
    String_View label_value;
} define_labels;

define_labels *define_labels_array;
size_t define_labels_array_counter = 0;
bool preprocessing_failed = false;

char* get_filename_without_extension(const char* path) {
    char* filename = basename(strdup(path));
    char* dot = strrchr(filename, '.');
    if (dot != NULL) {
        *dot = '\0';
    }
    return filename;
}

void push_to_define_array(String_View label_name, String_View label_value)
{
    fprintf(stderr, "LOG: Pushing define: label_name = %.*s, label_value = %.*s\n", 
            (int)label_name.count, label_name.data, 
            (int)label_value.count, label_value.data);

    if (define_labels_array_counter >= VM_DEFINE_LIMIT)
    {
        fprintf(stderr, "ERROR: define labels array overflow.\n");
        exit(EXIT_FAILURE);
    }
    define_labels_array[define_labels_array_counter++] = (define_labels){.label_name = label_name, .label_value = label_value};
}

int main(int argc, char **argv)
{
    fprintf(stderr, "LOG: Starting program...\n");

    if (argc < 2)
    {
        fprintf(stderr, "ERROR: No input file provided.\n");
        exit(EXIT_FAILURE);
    }

    char *input = argv[1];
    fprintf(stderr, "LOG: Input file is '%s'\n", input);

    define_labels_array = (define_labels *)malloc(VM_DEFINE_LIMIT * sizeof(define_labels));
    if (!define_labels_array)
    {
        fprintf(stderr, "ERROR: %%define labels array allocation failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    fprintf(stderr, "LOG: define_labels_array allocated with VM_DEFINE_LIMIT = %d\n", VM_DEFINE_LIMIT);

    // Build the .vpp filename
    char input_copy[MAX_PATH_LENGTH];
    strncpy(input_copy, input, MAX_PATH_LENGTH - 1);
    input_copy[MAX_PATH_LENGTH - 1] = '\0';  // Ensure null-termination

    char* dir = dirname(strdup(input_copy));
    char* filename = get_filename_without_extension(input);

    char vpp_filename[MAX_PATH_LENGTH];
    snprintf(vpp_filename, sizeof(vpp_filename), "%s/%s.vpp", dir, filename);

    fprintf(stderr, "LOG: Output vpp filename is '%s'\n", vpp_filename);

    FILE *vpp = fopen(vpp_filename, "w");  // Use write mode instead of append
    if (!vpp)
    {
        fprintf(stderr, "ERROR: Failed to open file %s: %s\n", vpp_filename, strerror(errno));
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "LOG: Successfully opened '%s' for writing.\n", vpp_filename);

    String_View unprocessed_file = slurp_file(input);
    String_View Copy = unprocessed_file;
    if (unprocessed_file.data == NULL) {
        fprintf(stderr, "ERROR: Failed to read input file: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    fprintf(stderr, "LOG: Successfully slurped file '%s', size = %zu bytes.\n", input, unprocessed_file.count);

    size_t line_no = 0;

    while (unprocessed_file.count > 0)
    {
        String_View line = sv_chop_by_delim(&unprocessed_file, '\n');
        line = sv_chop_by_delim(&line, ';'); // Remove comments
        sv_trim_left(&line);
        sv_trim_right(&line);

        fprintf(stderr, "LOG: Processing line %zu, content: '%.*s'\n", line_no, (int)line.count, line.data);

        if (line.count == 0)
            continue; // Ignore empty lines

        if (*line.data == '%')
        {
            String_View directive = sv_chop_by_delim(&line, ' ');
            fprintf(stderr, "LOG: Found directive '%.*s'\n", (int)directive.count, directive.data);

            if (sv_eq(directive, cstr_as_sv("%define")))
            {
                sv_trim_left(&line);
                if (!line.count)
                {
                    fprintf(stderr, "Line Number %zu -> ERROR: %%define directive has no label\n", line_no);
                    preprocessing_failed = true;
                    continue;  // Skip to next line
                }

                String_View define_label = sv_chop_by_delim(&line, ' ');
                sv_trim_left(&line);
                String_View label_value = sv_chop_by_delim(&line, ' ');

                fprintf(stderr, "LOG: Parsed %%define: label_name = '%.*s', label_value = '%.*s'\n", 
                        (int)define_label.count, define_label.data, 
                        (int)label_value.count, label_value.data);

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
            while (line.count > 0)
            {
                bool found = false;
                String_View token = sv_chop_by_delim(&line, ' ');
                sv_trim_left(&line);

                fprintf(stderr, "LOG: Processing token '%.*s'\n", (int)token.count, token.data);

                for (size_t i = 0; i < define_labels_array_counter; i++)
                {
                    if (sv_eq(define_labels_array[i].label_name, token))
                    {
                        fprintf(stderr, "LOG: Replacing token '%.*s' with value '%.*s'\n", 
                                (int)token.count, token.data, 
                                (int)define_labels_array[i].label_value.count, 
                                define_labels_array[i].label_value.data);

                        if (define_labels_array[i].label_value.count > 0) {
                            if (fprintf(vpp, "%.*s ", (int)define_labels_array[i].label_value.count, define_labels_array[i].label_value.data) < 0) {
                                fprintf(stderr, "ERROR: Failed to write to output file: %s\n", strerror(errno));
                                exit(EXIT_FAILURE);
                            }
                        }
                        found = true;
                        break;
                    }
                }

                if (!found)
                {
                    fprintf(stderr, "LOG: Token '%.*s' not found in defines, writing as is.\n", (int)token.count, token.data);
                    if (fprintf(vpp, "%.*s ", (int)token.count, token.data) < 0) {
                        fprintf(stderr, "ERROR: Failed to write to output file: %s\n", strerror(errno));
                        exit(EXIT_FAILURE);
                    }
                }
            }
            if (fprintf(vpp, "\n") < 0) {
                fprintf(stderr, "ERROR: Failed to write newline to output file: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            fflush(vpp);
        }

        line_no++;
    }

    free((void *)Copy.data);  // Free slurped file data
    free(define_labels_array);  // Free define_labels_array
    fclose(vpp);

    fprintf(stderr, "LOG: Finished processing. Preprocessing failed = %s\n", preprocessing_failed ? "true" : "false");

    return preprocessing_failed ? EXIT_FAILURE : EXIT_SUCCESS;
}