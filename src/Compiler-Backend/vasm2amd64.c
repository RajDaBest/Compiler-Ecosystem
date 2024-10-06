#define _VM_IMPLEMENTATION
#define _SV_IMPLEMENATION
#include "../non_nanboxed/virt_mach.h"
#include "../non_nanboxed/String_View.h"

#include <stdlib.h>
#include <stdio.h>

#define TYPE_INVALID ((uint8_t)10)

typedef struct label_hashnode_
{
    String_View label;
    struct label_hashnode_ *next;
} label_hashnode;

typedef struct
{
    String_View label;
    size_t line_no;
} unresolved_label_;

label_hashnode *label_table[MAX_HASHTABLE_SIZE];

size_t unresolved_labels_counter = 0;
size_t l_num = 0;

label_hashnode *search_in_label_table(String_View label)
{
    uint32_t key = hash_sv(label) % MAX_HASHTABLE_SIZE;

    label_hashnode *current_node = label_table[key];

    while (current_node != NULL)
    {
        if (sv_eq(current_node->label, label))
        {
            return current_node;
        }

        current_node = current_node->next;
    }

    return NULL;
}

void push_to_label_table(String_View label)
{

    if (search_in_label_table(label))
    {
        fprintf(stderr, "Line Number %zu -> ERROR: Label '%.*s' redefinition\n", line_no, (int)label.count, label.data);
        compilation_successful = false;
        return;
    }

    uint32_t key = hash_sv(label) % MAX_HASHTABLE_SIZE;

    label_hashnode *node = (label_hashnode *)malloc(sizeof(label_hashnode));
    if (!node)
    {
        fprintf(stderr, "ERROR: memory allocation for the hash node of the label %.*s failed\n", (int)label.count, label.data);
        exit(EXIT_FAILURE);
    }
    node->label = label;

    if (!label_table[key]) // its null
    {
        label_table[key] = node;
        node->next = NULL;
    }
    else
    {
        node->next = label_table[key];
        label_table[key] = node;
    }
}

/*

If there's no sign and no decimal point, we return TYPE_UNSIGNED64.
If there's a leading + sign, the type should still be TYPE_UNSIGNED64 since it's non-negative.
If there's a leading -, we return TYPE_SIGNED64.
If there's a decimal point, return TYPE_DOUBLE, regardless of whether there's a leading sign.

*/

uint8_t check_operand_type(String_View *operand)
{
    bool point_found = false;
    bool is_negative = false;

    for (size_t i = 0; i < operand->count; i++)
    {
        char data = operand->data[i];

        // Check for leading sign (only valid at the first character)
        if (i == 0 && (data == '-' || data == '+'))
        {
            if (data == '-')
            {
                is_negative = true;
            }
            continue;
        }

        // Check for decimal point (indicates a double)
        if (data == '.' && !point_found)
        {
            point_found = true;
            continue;
        }

        // Check if character is a valid digit
        char digit = data - '0';
        if (digit < 0 || digit > 9)
        {
            return TYPE_INVALID; // Invalid operand type if non-numeric character found
        }
    }

    // Determine type based on findings
    if (point_found)
    {
        return TYPE_DOUBLE;
    }
    else if (is_negative)
    {
        return TYPE_SIGNED_64INT; // If negative sign is found, it must be signed
    }
    else
    {
        return TYPE_UNSIGNED_64INT; // Default to unsigned if no negative sign and no decimal point
    }
}

void handle_instruction(FILE *asm_file_program, FILE *asm_file_data, size_t inst_number, String_View *operand)
{
    switch (inst_number)
    {
    case INST_UPUSH:
    case INST_SPUSH:
        fprintf(asm_file_program, "mov rax, %.*s\npush rax\n", (int)operand->count, operand->data);
        break;
    case INST_FPUSH:
        fprintf(asm_file_data, "L%zu: dq %.*s\n", l_num, (int)operand->count, operand->data);
        fprintf(asm_file_program, "movsd xmm0, [L%zu]\nsub rsp, 8\nmovsd [rsp], xmm0", l_num);
        l_num++;
    case INST_HALT:
        fprintf(asm_file_program, "mov rax, 60\nmov rdi, 0\nsyscall\n");
        break;
    }
}

void handle_code_line(FILE *asm_file_program, FILE *asm_file_data, String_View *line, size_t program_counter, unresolved_label_ *unresolved_labels)
{
    String_View inst_name = sv_chop_by_delim(line, ' ');
    sv_trim_left(line);
    bool has_operand_value = line->count > 0;

    for (size_t i = 1; i < (size_t)INST_COUNT; i++)
    {
        if (sv_eq(inst_name, cstr_as_sv(get_inst_name(i))))
        {
            if (has_operand_function(i) != has_operand_value)
            {
                fprintf(stderr, "Line Number %zu -> ERROR: %s %s an operand\n",
                        line_no, get_inst_name(i), has_operand_function(i) ? "requires" : "doesn't require");
                compilation_successful = false;
                return;
            }

            if (!has_operand_function(i))
            {
                handle_instruction(asm_file_program, asm_file_data, i, &(String_View){0}); // the constant value will go into the static memory
            }

            // we dont currently handle operand overflow at the compiler level; maybe it's already handled by the assembler

            uint8_t operand_type = check_operand_type(line);

            switch (get_operand_type(i))
            {
            case TYPE_SIGNED_64INT:
            {
                if (operand_type == TYPE_UNSIGNED_64INT || operand_type == TYPE_SIGNED_64INT)
                {
                    handle_instruction(asm_file_program, asm_file_data, i, line);
                    return;
                }
                else if (operand_type == TYPE_DOUBLE)
                {
                    fprintf(stderr, "Line Number %zu -> ERROR: illegal operand value for %s instruction: %.*s\n"
                                    "Must be an signed integral value\n",
                            line_no, get_inst_name(i), (int)line->count, line->data);
                    compilation_successful = false;
                    return;
                }
                else if (operand_type == TYPE_INVALID)
                {
                    unresolved_labels[unresolved_labels_counter].label = *line;
                    unresolved_labels[unresolved_labels_counter].line_no = line_no;
                    unresolved_labels_counter++;
                    return;
                }
            }
            case TYPE_UNSIGNED_64INT:
            {
                if (operand_type == TYPE_UNSIGNED_64INT)
                {
                    handle_instruction(asm_file_program, asm_file_data, i, line);
                    return;
                }
                else if (operand_type == TYPE_DOUBLE || operand_type == TYPE_SIGNED_64INT)
                {
                    fprintf(stderr, "Line Number %zu -> ERROR: illegal operand value for %s instruction: %.*s\n"
                                    "Must be an unsigned integral value\n",
                            line_no, get_inst_name(i), (int)line->count, line->data);
                    compilation_successful = false;
                    return;
                }
                else if (operand_type == TYPE_INVALID)
                {
                    unresolved_labels[unresolved_labels_counter].label = *line;
                    unresolved_labels[unresolved_labels_counter].line_no = line_no;
                    unresolved_labels_counter++;
                    return;
                }
            }

            case TYPE_DOUBLE:
            {
                if (operand_type == TYPE_INVALID)
                {
                    unresolved_labels[unresolved_labels_counter].label = *line;
                    unresolved_labels[unresolved_labels_counter].line_no = line_no;
                    unresolved_labels_counter++;
                    return;
                }
                else
                {
                    handle_instruction(asm_file_program, asm_file_data, i, line);
                    return;
                }
            }
            default:
                fprintf(stderr, "Line Number %zu -> ERROR: unknown operand type for instruction %s\n",
                        line_no, get_inst_name(i));
                compilation_successful = false;
                return;
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

    unresolved_label_ unresolved_labels[VM_LABEL_CAPACITY];

#else

    unresolved_label_ unresolved_labels[label_capacity];

#endif

    String_View source = slurp_file(input); // have to free the source buffer

    assert(vm_program_capacity <= UINT64_MAX); // asssert some shit
    assert(vm_memory_capacity <= UINT64_MAX);
    assert(vm_stack_capacity <= UINT64_MAX);
    assert(vm_default_memory_size < vm_memory_capacity);

    FILE *asm_file = fopen(ouput, "w+");
    fprintf(asm_file, "section .data\n");

    FILE *asm_file_program = fopen("temp.asm", "w+");
    fprintf(asm_file_program, "section .text\nglobal _start\n");

    size_t code_section_offset = 0;
    size_t data_section_offset = 0;

    bool is_code = true;
    bool is_data = false;

    while (source.count > 0)
    {
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
        // printf("%.*s\n", (int)line.count, line.data);

        if (sv_eq(directive, cstr_as_sv(".text")))
        {
            is_code = true;
            is_data = false;
        }
        else if (sv_eq(directive, cstr_as_sv(".data")))
        {
            is_code = false;
            is_data = true;
        }
        else if (is_code)
        {
            String_View label = sv_chop_by_delim(&line, ':');
            if (*(line.data - 1) == ':') // if there's a label
            {
                fprintf(asm_file_program, "%.*s:\n", (int)label.count, label.data);
                push_to_label_table(label);
                sv_trim_left(&line);

                if (line.count > 0)
                { // instruction remaining after the label
                    handle_code_line(asm_file_program, asm_file, &line, code_section_offset, unresolved_labels);
                }
            }
            else
            {
                handle_code_line(asm_file_program, asm_file, &label, code_section_offset, unresolved_labels);
            }
            code_section_offset++;
        }
        else if (is_data)
        {
            String_View label = sv_chop_by_delim(&line, ':');
            if (*(line.data - 1) == ':') // if there's a label
            {
                fprintf(asm_file, "%.*s: ", (int)label.count, label.data);
                push_to_label_table(label);
            }
            else
            {
                line.count = label.count; // Reset line if no label found
                line.data = label.data;
            }

            sv_trim_left(&line);
            String_View data_type = sv_chop_by_delim(&line, ' ');
            sv_trim_left(&line);

            if (sv_eq(data_type, cstr_as_sv(".byte")))
            {
                if (data_section_offset >= vm_default_memory_size - sizeof(int8_t))
                {
                    fprintf(stderr, "Line Number: %zu -> ERROR: Not enough default static memory for .byte %.*s\n", line_no, (int)line.count, line.data);
                    exit(EXIT_FAILURE);
                }

                fprintf(asm_file, "db %.*s\n", (int)line.count, line.data);

                data_section_offset += sizeof(int8_t);
            }
            else if (sv_eq(data_type, cstr_as_sv(".word")))
            {
                if (data_section_offset >= vm_default_memory_size - sizeof(int16_t))
                {
                    fprintf(stderr, "Line Number: %zu -> ERROR: Not enough default static memory for .word %.*s\n", line_no, (int)line.count, line.data);
                    exit(EXIT_FAILURE);
                }

                fprintf(asm_file, "dw %.*s\n", (int)line.count, line.data);

                data_section_offset += sizeof(int16_t);
            }
            else if (sv_eq(data_type, cstr_as_sv(".doubleword")))
            {
                if (data_section_offset >= vm_default_memory_size - sizeof(int32_t))
                {
                    fprintf(stderr, "Line Number: %zu -> ERROR: Not enough default static memory for .doubleword %.*s\n", line_no, (int)line.count, line.data);
                    exit(EXIT_FAILURE);
                }

                fprintf(asm_file, "dd %.*s\n", (int)line.count, line.data);

                data_section_offset += sizeof(int32_t);
            }
            else if (sv_eq(data_type, cstr_as_sv(".quadword")))
            {
                if (data_section_offset >= vm_default_memory_size - sizeof(int64_t))
                {
                    fprintf(stderr, "Line Number: %zu -> ERROR: Not enough default static memory for .quadword %.*s\n", line_no, (int)line.count, line.data);
                    exit(EXIT_FAILURE);
                }

                fprintf(asm_file, "dq %.*s\n", (int)line.count, line.data);

                data_section_offset += sizeof(int64_t);
            }
            else if (sv_eq(data_type, cstr_as_sv(".double")))
            {
                if (data_section_offset >= vm_default_memory_size - sizeof(double))
                {
                    fprintf(stderr, "Line Number: %zu -> ERROR: Not enough default static memory for .quadword %.*s\n", line_no, (int)line.count, line.data);
                    exit(EXIT_FAILURE);
                }

                fprintf(asm_file, "dq %.*s\n", (int)line.count, line.data);

                data_section_offset += sizeof(double);
            }
            else if (sv_eq(data_type, cstr_as_sv(".string")))
            {
                if (line.count >= 2 && line.data[0] == '"' && line.data[line.count - 1] == '"')
                {
                    line.data++;
                    line.count -= 2;
                }
                else
                {
                    fprintf(stderr, "Line Number: %zu -> ERROR: %.*s not a valid vasm string\n", line_no, (int)line.count, line.data);
                    compilation_successful = false;
                }

                if (data_section_offset > vm_default_memory_size - line.count - 1)
                {
                    fprintf(stderr, "Line Number: %zu -> ERROR: Not enough default static memory for .string %.*s\n", line_no, (int)line.count, line.data);
                    exit(EXIT_FAILURE);
                }

                fprintf(asm_file, "db '%.*s'\n", (int)line.count, line.data);

                data_section_offset += line.count;

                /* data_section[*data_section_offset] = '\0';
                 *data_section_offset += 1; */
            }
            else
            {
                fprintf(stderr, "Line Number: %zu -> ERROR: invalid data type '%.*s'\n", line_no, (int)data_type.count, data_type.data);
                compilation_successful = false;
            }
        }
    }

    /* if (code_section_offset > 0 && program[code_section_offset - 1].type != INST_HALT)
    {
        fprintf(stderr, "ERROR: halt required to mark the code end\n");
        return false;
    }
    return true; */

    if (compilation_successful)
    {
        for (size_t i = 0; i < unresolved_labels_counter; i++)
        {
            if (!search_in_label_table(unresolved_labels[i].label))
            {
                printf("Line No: %zu -> ERROR: Undefined label: '%.*s'\n", unresolved_labels[i].line_no, (int)unresolved_labels[i].label.count, unresolved_labels[i].label.data);
                compilation_successful = false;
            }
        }
    }

    if (!compilation_successful)
    {
        printf("Compilation Unsuccessful!\n");
    }
    else
    {
        size_t program_file_size = ftell(asm_file_program);
        char program_file[program_file_size];

        fseek(asm_file_program, 0, SEEK_SET);
        fread(program_file, sizeof(uint8_t), program_file_size, asm_file_program);
        fprintf(asm_file, "\n");
        fwrite(program_file, sizeof(uint8_t), program_file_size, asm_file);
        system("rm -r temp.asm");
    }

    // free((void *)source.data);
    return EXIT_SUCCESS;
}