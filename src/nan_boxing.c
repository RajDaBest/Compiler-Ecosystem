#include "virt_mach.h"

#define VALUE_IN_BITS(value)                       \
    {                                              \
        printf("%lf = \n      ", (double)(value)); \
        print_bits((double)(value));               \
    }

#define TYPE_UNSIGNED_64INT (__uint8_t)1
#define TYPE_SIGNED_64INT (__uint8_t)2
#define TYPE_POINTER (__uint8_t)3

void print_bits(double value)
{
    __int8_t *byte_arr = (__int8_t *)&value; // x86-64 is a little endian machine

    for (int i = 7; i >= 0; i--)
    {
        __int8_t byte = byte_arr[i];
        for (int j = 7; j >= 0; j--)
        {
            printf("%d", 1 & (byte >> j));
        }
        printf(" ");
    }
    printf("\n");
}

bool is_nan(double value)
{
    // this next line is actually really clever holy fuck
    __int64_t bits = (*(__int64_t *)&value); // store the raw bytes of double without typecasting; __uint64_t bits = value would just typecast

    // Extract the exponent and mantissa using bit masking
    __int64_t exponent = (bits >> 52) & 0x7FF;   // 11 bits for the exponent
    __int64_t mantissa = bits & 0xFFFFFFFFFFFFF; // 52 bits for the mantissa

    // Check if the exponent is all 1s and mantissa is not 0
    return (exponent == 0x7FF) && (mantissa != 0);
}

void set_signed_64int(double *value_nan, __int64_t actual_value)
{
    // Check for value range
    /* if (actual_value > (1ULL << 47) || actual_value < -(1ULL << 47))
    {
        fprintf(stderr, "ERROR: value %ld is out of range\n", actual_value);
        exit(EXIT_FAILURE);
    } */

    __int64_t bits = 0;
    bits |= (((__int64_t)TYPE_SIGNED_64INT) << 48); // the typecast won't do a sign extension, so lite
    bits |= 0xFFF0000000000000L;
    actual_value &= 0x000FFFFFFFFFFFFFL;
    bits |= actual_value;

    *value_nan = (*(double *)&bits);
}

void set_unsigned_64int(double *value_nan, __uint64_t actual_value)
{
    if (actual_value > (1UL << 47))
    {
        fprintf(stderr, "ERROR: value %ld is too big\n", actual_value);
        exit(EXIT_FAILURE);
    }

    __uint64_t bits = 0;
    bits |= (((__uint64_t)TYPE_UNSIGNED_64INT) << 48);
    bits |= 0xFFF0000000000000UL;
    actual_value &= 0x000FFFFFFFFFFFFF;
    bits |= actual_value;

    *value_nan = (*(double *)&bits);
}

void set_pointer(double *value_nan, __uint64_t actual_value)
{
    if (actual_value > (1UL << 47))
    {
        fprintf(stderr, "ERROR: value %ld is too big\n", actual_value);
        exit(EXIT_FAILURE);
    }

    __uint64_t bits = 0;
    bits |= (((__uint64_t)TYPE_POINTER) << 48);
    bits |= 0xFFF0000000000000UL;
    actual_value &= 0x000FFFFFFFFFFFFF;
    bits |= actual_value;

    *value_nan = (*(double *)&bits);
}

__uint8_t return_type(double value)
{
    __uint64_t bits = (*(__uint64_t *)&value);
    return (__uint8_t)((bits >> 48) & 0x000000000000000FUL);
}

__int64_t return_value_signed(double value)
{
    __int64_t bits = (*(__int64_t *)&value);
    __int64_t val = 0x0000FFFFFFFFFFFF & bits;
    __int64_t signed_bit = ((val & (1UL << 47)) >> 47);
    signed_bit && (val |= 0xFFFF000000000000);
    return (__int64_t)val;
}

__uint64_t return_value_unsigned(double value)
{
    __uint64_t bits = (*(__uint64_t *)&value);
    __uint64_t val = 0x0000FFFFFFFFFFFFUL & bits;
    return (__uint64_t)val;
}

__uint64_t return_value_pointer(double value)
{
    __uint64_t bits = (*(__uint64_t *)&value);
    __uint64_t val = 0x0000FFFFFFFFFFFFUL & bits;
    return (__uint64_t)val;
}

int main()
{
    // Test values
    double value;

    // Testing 0.0
    VALUE_IN_BITS(0.0);
    printf("Is NaN: %d\n", is_nan(0.0));

    // Testing a normal floating-point value
    VALUE_IN_BITS(3.141593);
    printf("Is NaN: %d\n", is_nan(3.141593));

    // Testing positive and negative infinity
    VALUE_IN_BITS(1.0 / 0.0); // Positive Infinity
    printf("Is NaN: %d\n", is_nan(1.0 / 0.0));

    VALUE_IN_BITS(-1.0 / 0.0); // Negative Infinity
    printf("Is NaN: %d\n", is_nan(-1.0 / 0.0));

    /* // Testing NaN
    VALUE_IN_BITS(NAN);
    printf("Is NaN: %d\n", is_nan(NAN)); */

    // Testing signed 64-bit integer encoding
    set_signed_64int(&value, -123456789);
    VALUE_IN_BITS(value);
    printf("Returned signed value: %ld\n", return_value_signed(value));

    // Testing unsigned 64-bit integer encoding
    set_unsigned_64int(&value, 123456789);
    VALUE_IN_BITS(value);
    printf("Returned unsigned value: %lu\n", return_value_unsigned(value));

    // Testing pointer encoding (using a mock address)
    set_pointer(&value, 0xDEADBEEF);
    VALUE_IN_BITS(value);
    printf("Returned pointer value: %lu\n", return_value_pointer(value));

    // Edge case for signed 64-bit integer
    printf("Testing out-of-range signed value...\n");
    set_signed_64int(&value, (1ULL << 47) + 1); // Should trigger an error

    return 0;
}

// IEEE 754 NaNs are encoded with the exponent field filled with ones (like infinity values)
// , and some non-zero number in the significand field (to make them distinct from infinity values);
// this allows the definition of multiple distinct NaN values, depending on which bits are set in the significand field
// , but also on the value of the leading sign bit.