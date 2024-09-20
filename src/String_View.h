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

String_View cstr_as_sv(char *cstr);
String_View sv_chop_by_delim(String_View *sv, const char delim);
void sv_trim_left(String_View *line);
void sv_trim_right(String_View *line);
void sv_trim_side_comments(String_View *line);
bool sv_eq(String_View a, String_View b);
word sv_to_int(String_View *op);

#ifdef _SV_IMPLEMENTATION

String_View cstr_as_sv(char *cstr)
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

word sv_to_int(String_View *op)
{
    word num = 0;
    for (size_t i = 0; i < op->count; i++)
    {
        int dig = op->data[i] - '0';
        if (dig > 9 || dig < 0)
        {
            str_errno = FAILURE; // Set error if non-digit is encountered
            return -1;
        }

        // Check for overflow before multiplying and adding
        if (num > (__INT_LEAST64_MAX__ - dig) / 10)
        {
            str_errno = OPERAND_OVERFLOW; // Set error if overflow occurs
            return -1;
        }

        num = num * 10 + dig;
    }
    return num;
}


#endif // _SV_IMPLEMENTATION

#endif // _SV