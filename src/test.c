#include <stdio.h>
#define _SV_IMPLEMENTATION
#include "String_View.h"
#include <stdbool.h>


int main(void)
{
    String_View a = {.count = 5, .data = "12342"};
    double d = sv_to_value(&a);
    printf("%lf %d %d %d\n", d, is_negative, is_fraction, str_errno);
}