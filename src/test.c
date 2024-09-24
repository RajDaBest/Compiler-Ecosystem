#include <stdio.h>
#define _SV_IMPLEMENTATION
#include "String_View.h"

int main(int argc, char **argv)
{
    /* const char *a = "mov";
    String_View line = cstr_as_sv(a);
    String_View inst_name = sv_chop_by_delim(&line, ' ');
    sv_trim_left(&line);
    sv_trim_right(&line);
    String_View operand_one = sv_chop_by_delim(&line, ',');
    sv_trim_left(&line);
    sv_trim_right(&line);
    bool has_no_operand = operand_one.count == 0;
    bool has_one_operand = ((operand_one.count) > 0) && (line.count == 0);
    bool has_two_operands = line.count > 0; // line is now the second operand
    printf("%d %d %d\n", has_no_operand, has_one_operand, has_two_operands);
    printf("%.*s %.*s %.*s\n", (int)inst_name.count, inst_name.data, (int)operand_one.count, operand_one.data, (int)line.count, line.data); */

    const char *a = "r143";
    String_View op = cstr_as_sv(a);

    printf("%zu %d %d\n", sv_to_reg(&op), is_floating_point_register, is_integer_register);
}