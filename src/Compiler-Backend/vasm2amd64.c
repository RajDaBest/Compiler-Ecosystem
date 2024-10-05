#define _VM_IMPLEMENTATION
#define _SV_IMPLEMENATION
#include "../non_nanboxed/virt_mach.h"
#include "../non_nanboxed/String_View.h"

#include <stdlib.h>
#include <stdio.h>

void handle_inst_uplus(FILE *asm_file, String_View *inst_name)
{
    fprintf(asm_file, "mov r ");
}

void handle_line(FILE *asm_file, String_View line, size_t program_counter)
{
    String_View inst_name = sv_chop_by_delim(&line, ' ');
    sv_trim_left(&line);
    bool has_operand_value = line.count > 0;

    for (size_t i = 1; i < (size_t)INST_COUNT; i++)
    {
        if (sv_eq(inst_name, cstr_as_sv(get_inst_name(i))))
        {
            if (has_operand_function(i) != has_operand_value)
            {
                fprintf(stderr, "Line Number %zu -> ERROR: %s %s an operand\n",
                        line_no, get_inst_name(i), has_operand_function(i) ? "requires" : "doesn't require");
                compilation_successful = false;
                return (Inst){0};
            }

            if (!has_operand_function(i))
            {
                fprintf(asm_file, "")
            }

            union
            {
                double f64;
                int64_t s64;
                uint64_t u64;
            } operand = {0};

            bool is_frac = is_fraction(line);
            bool is_neg = is_negative(line);

            if (is_frac)
            {
                operand.f64 = sv_to_double(&line);
            }
            else if (is_neg)
            {
                operand.s64 = sv_to_signed64(&line);
            }
            else
            {
                operand.u64 = sv_to_unsigned64(&line);
            }

            if (str_errno == FAILURE)
            {
                push_to_not_resolved_yet(line, program_counter, line_no);
                return (Inst){.type = i, .operand._as_u64 = 0};
            }
            else if (str_errno == OPERAND_OVERFLOW)
            {
                fprintf(stderr, "Line Number %zu -> ERROR: %.*s overflows a 64 bit %s value\n",
                        line_no, (int)line.count, line.data,
                        get_operand_type(i) == TYPE_SIGNED_64INT ? "signed" : "unsigned");
                compilation_successful = false;
                return (Inst){0};
            }

            switch (get_operand_type(i))
            {
            case TYPE_SIGNED_64INT:
                return (Inst){.type = i, .operand._as_s64 = operand.s64};
            case TYPE_UNSIGNED_64INT:
                if (is_frac || is_neg)
                {
                    fprintf(stderr, "Line Number %zu -> ERROR: illegal operand value for %s instruction: %.*s\n"
                                    "Must be an unsigned integral value\n",
                            line_no, get_inst_name(i), (int)line.count, line.data);
                    compilation_successful = false;
                    return (Inst){0};
                }
                return (Inst){.type = i, .operand._as_u64 = operand.u64};
            case TYPE_DOUBLE:
                return (Inst){.type = i, .operand._as_f64 = operand.f64};
            default:
                fprintf(stderr, "Line Number %zu -> ERROR: unknown operand type for instruction %s\n",
                        line_no, get_inst_name(i));
                compilation_successful = false;
                return (Inst){0};
            }
        }
    }

    fprintf(stderr, "Line Number %zu -> ERROR: invalid instruction %.*s\n",
            line_no, (int)inst_name.count, inst_name.data);
    compilation_successful = false;
}

int main(int argc, char **argv)
{

    char *input = argv[1];
    char *ouput = argv[2];

#ifdef _WIN32

    Inst program[VM_PROGRAM_CAPACITY * sizeof(Inst)];
    uint8_t data_section[VM_DEFAULT_MEMORY_SIZE];

#else

    Inst program[vm_program_capacity * sizeof(Inst)];
    uint8_t data_section[vm_default_memory_size];

#endif

    String_View source = slurp_file(input); // have to free the source buffer

    assert(vm_program_capacity <= UINT64_MAX); // asssert some shit
    assert(vm_memory_capacity <= UINT64_MAX);
    assert(vm_stack_capacity <= UINT64_MAX);
    assert(vm_default_memory_size < vm_memory_capacity);

    FILE *asm_file = fopen(ouput, "w");

    while (source.count > 0)
    {
        size_t code_section_offset = 0;
        size_t data_section_offset = 0;
        bool is_code = true;
        bool is_data = false;

        if (code_section_offset >= vm_program_capacity)
        {
            fprintf(stderr, "Program Too Big\n");
            compilation_successful = false;
            break;
        }

        line_no++;
        String_View line = sv_chop_by_delim(&source, '\n');
        line = sv_chop_by_delim(&line, ';'); // Remove comments
        sv_trim_left(&line);
        sv_trim_right(&line);

        if (line.count == 0)
            continue; // Ignore empty lines

        String_View section = line;
        String_View directive = sv_chop_by_delim(&section, ' ');

        if (sv_eq(directive, cstr_as_sv(".text")))
        {
            is_code = true;
            is_data = false;
            fprintf(asm_file, "section .text\n");
        }
        else if (sv_eq(directive, cstr_as_sv(".data")))
        {
            is_code = false;
            is_data = true;
            fprintf(asm_file, "section .data\n");
        }
        else if (is_code)
        {
            String_View label = sv_chop_by_delim(&line, ':');
            if (*(line.data - 1) == ':') // if there's a label
            {
                fprintf(asm_file, "%.*s:\n", (int)label.count, label.data);
                sv_trim_left(&line);

                if (line.count > 0)
                { // instruction remaining after the label
                    program[code_section_offset] = vm_translate_line(line, code_section_offset);
                    (code_section_offset)++;
                }
            }

            /* for (size_t i = 1; i < INST_COUNT; i++)
            {
                if ()
            } */
        }
        else if (is_data)
        {
        }
    }

    // free((void *)source.data);
    return EXIT_SUCCESS;
}