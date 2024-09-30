#ifndef _SV
#define _SV

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

typedef struct
{
    size_t count;
    char *data;
} String_View;

typedef int64_t word;

#define SUCCESS 1
#define FAILURE 0
#define OPERAND_OVERFLOW -1

int str_errno = SUCCESS;

String_View cstr_as_sv(const char *cstr);
String_View sv_chop_by_delim(String_View *sv, const char delim);
void sv_trim_left(String_View *line);
void sv_trim_right(String_View *line);
void sv_trim_side_comments(String_View *line);
bool sv_eq(String_View a, String_View b);
double sv_to_value(String_View *op);

#ifdef _SV_IMPLEMENTATION

String_View cstr_as_sv(const char *cstr)
{
    return (String_View){
        .count = strlen(cstr),
        .data = cstr,
    };
}

String_View sv_chop_by_delim(String_View *sv, const char delim)
{
    String_View chopped = {0}; // Initialize chopped.count to 0
    chopped.data = sv->data;

    // Loop through until the delimiter or end of string is found
    while (sv->count > 0 && *(sv->data) != delim) // if there's no delim at the end; sv.count will be zero and will point to a null byte
    {
        sv->count--;
        chopped.count++;
        sv->data++;
    }

    // If delimiter found and not at the end of the string
    if (sv->count > 0 && *(sv->data) == delim)
    {
        sv->count--; // Skip the delimiter
        sv->data++;  // Move past the delimiter
    }

    return chopped;
}

void sv_trim_left(String_View *line)
{
    while (isspace((int)*(line->data)) && line->count > 0)
    {
        line->data++;
        line->count--;
    }
}

void sv_trim_right(String_View *line)
{
    while (isspace((int)(line->data[line->count - 1])) && line->count > 0)
    {
        line->count--;
    }
}

void sv_trim_side_comments(String_View *line)
{
    for (size_t i = 0; i < line->count; i++)
    {
        if (line->data[i] == '#')
        {
            line->count -= i;
            break;
        }
    }
}

bool sv_eq(String_View a, String_View b)
{
    if (a.count != b.count)
    {
        return false;
    }

    for (size_t i = 0; i < a.count; i++)
    {
        if (a.data[i] != b.data[i])
        {
            return false;
        }
    }

    return true;
}

/* bool is_fraction = false;
bool is_negative = false; */
/*
bool is_integer_register = false;
bool is_floating_point_register = false;
bool invalid_operand = false;
bool integer_register_count_exceed = false;
bool floating_point_register_count_exceed = false;

size_t sv_to_reg(String_View *op)
{
    is_integer_register = false;
    is_floating_point_register = false;
    invalid_operand = false;
    floating_point_register_count_exceed = false;

    if (op->data[0] == 'r')
    {
        is_integer_register = true;
        is_floating_point_register = false;
    }
    else if (op->data[0] == 'x')
    {
        is_integer_register = false;
        is_floating_point_register = true;
    }
    else
    {
        invalid_operand = true;
    }

    String_View register_number = {.count = op->count - 1, .data = op->data + 1};
    size_t num = 0;

    for (size_t i = 0; i < register_number.count; i++)
    {
        char ch = register_number.data[i];

        int dig = ch - '0';
        if (dig < 0 || dig > 9)
        {
            invalid_operand = true;
            return -1;
        }

        num = num * 10 + dig;
    }

    if (is_integer_register && num >= integer_register_no)
    {
        integer_register_count_exceed = true;
    }
    else if (is_floating_point_register && num >= floating_point_register_no)
    {
        floating_point_register_count_exceed = true;
    }

    return num;
} */

/* double sv_to_value(String_View *op)

{
    double num = 0;
    double fraction_part = 0;
    double divisor = 1;
    is_negative = false;
    is_fraction = false;
    str_errno = SUCCESS; // Reset the error flag before processing

    for (size_t i = 0; i < op->count; i++)
    {
        char ch = op->data[i];

        // Check for negative sign
        if (ch == '-')
        {
            if (i > 0) // Negative sign should only be at the beginning
            {
                str_errno = FAILURE;
                return -1;
            }
            is_negative = true;
            continue;
        }

        if (ch == '.')
        {
            if (is_fraction)
            {
                // Multiple dots encountered, return error
                str_errno = FAILURE;
                return -1;
            }
            is_fraction = true;
            continue;
        }

        int dig = ch - '0';
        if (dig < 0 || dig > 9)
        {
            str_errno = FAILURE; // Set error if non-digit is encountered
            return -1;
        }

        if (is_fraction)
        {
            divisor *= 10;
            fraction_part += dig / divisor;
        }
        else
        {
            // Check for overflow before multiplying and adding
            if (num > (INT64_MAX - dig) / 10)
            {
                str_errno = OPERAND_OVERFLOW; // Set error if overflow occurs
                return -1;
            }

            num = num * 10 + dig;
        }
    }

    double result = num + fraction_part;
    return is_negative ? -result : result; // Apply the negative sign if needed
}
 */

bool is_negative(String_View value)
{
    return (value.data[0] == '-');
}

bool is_fraction(String_View value)
{
    for (size_t i = 0; i < value.count; i++)
    {
        if (value.data[i] == '.')
        {
            return true;
        }
    }

    return false;
}

double sv_to_double(String_View *value)
{
    double num = 0;
    double fraction_part = 0;
    bool is_neg = false;
    bool in_fraction = false;
    double divisor = 1;
    str_errno = SUCCESS; // Reset the error flag before processing

    for (size_t i = 0; i < value->count; i++)
    {
        char ch = value->data[i];

        // Check for negative sign
        if (ch == '-')
        {
            if (i > 0) // Negative sign should only be at the beginning
            {
                str_errno = FAILURE;
                return -1;
            }
            is_neg = true;
            continue;
        }

        // Check for positive sign (optional)
        if (ch == '+')
        {
            if (i > 0) // Positive sign should only be at the beginning
            {
                str_errno = FAILURE;
                return -1;
            }
            continue;
        }

        // Check for decimal point
        if (ch == '.')
        {
            if (in_fraction) // Multiple decimal points are not allowed
            {
                str_errno = FAILURE;
                return -1;
            }
            in_fraction = true;
            continue;
        }

        int dig = ch - '0';
        if (dig < 0 || dig > 9)
        {
            str_errno = FAILURE; // Set error if non-digit is encountered
            return -1;
        }

        if (in_fraction)
        {
            divisor *= 10;
            fraction_part += dig / divisor;
        }
        else
        {
            num = num * 10 + dig; // Accumulate integer part
        }
    }

    double result = num + fraction_part;
    return is_neg ? -result : result; // Apply the negative sign if needed
}

uint64_t sv_to_unsigned64(String_View *value)
{
    str_errno = SUCCESS;
    uint64_t num = 0;

    for (size_t i = 0; i < value->count; i++)
    {
        char ch = value->data[i];

        // Check for positive sign (optional)
        if (ch == '+')
        {
            if (i > 0) // Positive sign should only be at the beginning
            {
                str_errno = FAILURE;
                return 0; // Return 0 in case of failure for unsigned
            }
            continue;
        }

        int dig = ch - '0';
        if (dig < 0 || dig > 9)
        {
            str_errno = FAILURE; // Set error if non-digit is encountered
            return 0;            // Return 0 in case of failure for unsigned
        }

        // Check for overflow before performing the multiplication
        if (num > (UINT64_MAX - dig) / 10)
        {
            str_errno = FAILURE; // Overflow occurred
            return 0;
        }

        num = num * 10 + dig;
    }

    return num;
}

int64_t sv_to_signed64(String_View *value)
{
    str_errno = SUCCESS;
    bool is_neg = false;
    int64_t num = 0;

    for (size_t i = 0; i < value->count; i++)
    {
        char ch = value->data[i];

        // Check for positive sign (optional)
        if (ch == '+')
        {
            if (i > 0) // Positive sign should only be at the beginning
            {
                str_errno = FAILURE;
                return 0; // Return 0 in case of failure
            }
            continue;
        }

        // Check for negative sign
        if (ch == '-')
        {
            if (i > 0) // Negative sign should only be at the beginning
            {
                str_errno = FAILURE;
                return 0; // Return 0 in case of failure
            }
            is_neg = true;
            continue;
        }

        int dig = ch - '0';
        if (dig < 0 || dig > 9)
        {
            str_errno = FAILURE; // Set error if non-digit is encountered
            return 0;            // Return 0 in case of failure
        }

        // Check for overflow before performing the multiplication
        if (num > (INT64_MAX - dig) / 10)
        {
            str_errno = FAILURE; // Overflow occurred
            return 0;
        }

        num = num * 10 + dig;
    }

    return is_neg ? -num : num;
}

#endif // _SV_IMPLEMENTATION

#endif // _SV
