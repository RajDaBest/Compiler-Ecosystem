#ifndef _SV
#define _SV

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>


typedef struct 
{
    size_t count;
    char *data;
} String_View;

typedef __int64_t word;

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
    for(size_t i = 0; i < line->count; i++)
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

bool is_fraction = false;
bool is_negative = false;

double sv_to_value(String_View *op)
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
            if (num > (__INT_LEAST64_MAX__ - dig) / 10)
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

#endif // _SV_IMPLEMENTATION

#endif // _SV