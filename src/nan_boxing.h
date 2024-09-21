#ifndef NAN_BOXING_H
#define NAN_BOXING_H

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#define VALUE_IN_BITS(value)                       \
    {                                              \
        printf("%lf = \n      ", (double)(value)); \
        print_bits((double)(value));               \
    }
#define TYPE_DOUBLE (__uint8_t)0
#define TYPE_UNSIGNED_64INT (__uint8_t)1
#define TYPE_SIGNED_64INT (__uint8_t)2
#define TYPE_POINTER (__uint8_t)3

static inline void print_bits(double value);
static inline bool is_nan(double value);
static inline void set_signed_64int(double *value_nan, __int64_t actual_value);
static inline void set_unsigned_64int(double *value_nan, __uint64_t actual_value);
static inline void set_pointer(double *value_nan, __uint64_t actual_value);
static inline __uint8_t return_type(double value);
static inline __int64_t return_value_signed(double value);
static inline __uint64_t return_value_unsigned(double value);
static inline __uint64_t return_value_pointer(double value);

#ifndef NAN_IMPLEMENTATION
#define NAN_IMPLEMENTATION

static inline void print_bits(double value)
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

static inline bool is_nan(double value)
{
    // this next line is actually really clever holy fuck
    __int64_t bits = (*(__int64_t *)&value); // store the raw bytes of double without typecasting; __uint64_t bits = value would just typecast

    // Extract the exponent and mantissa using bit masking
    __int64_t exponent = (bits >> 52) & 0x7FF;   // 11 bits for the exponent
    __int64_t mantissa = bits & 0xFFFFFFFFFFFFF; // 52 bits for the mantissa

    // Check if the exponent is all 1s and mantissa is not 0
    return (exponent == 0x7FF) && (mantissa != 0);
}

static inline void set_signed_64int(double *value_nan, __int64_t actual_value)
{
    // Check for value range
    if (actual_value > (1L << 47) || actual_value < -(1L << 47))
    {
        fprintf(stderr, "ERROR: value %ld is out of range\n", actual_value);
        exit(EXIT_FAILURE);
    }

    /* __int64_t bits = 0;
    bits |= (((__int64_t)TYPE_SIGNED_64INT) << 48); // the typecast won't do a sign extension, so lite
    bits |= 0x7FF0000000000000L;
    actual_value &= 0x00007FFFFFFFFFFFL;
    bits |= actual_value;

    *value_nan = (*(double *)&bits); */

   __int64_t bits = 0;
   bits |= ((__int64_t)TYPE_SIGNED_64INT) << 48L;
   bits |= 0xFFF0000000000000;
   actual_value &= 0x0000FFFFFFFFFFFF;

   bits != actual_value;

   *value_nan = (*(double *)&bits);
}

static inline void set_unsigned_64int(double *value_nan, __uint64_t actual_value)
{
    if (actual_value > (1UL << 47))
    {
        fprintf(stderr, "ERROR: value %ld is out of range\n", actual_value);
        exit(EXIT_FAILURE);
    }

    __uint64_t bits = 0;
    bits |= (((__uint64_t)TYPE_UNSIGNED_64INT) << 48);
    bits |= 0xFFF0000000000000UL;
    actual_value &= 0x000FFFFFFFFFFFFF;
    bits |= actual_value;

    *value_nan = (*(double *)&bits);
}

static inline void set_pointer(double *value_nan, __uint64_t actual_value)
{
    if (actual_value > (1UL << 47)) // check signed against signed; unsigned against unsigned; dont mix
    {
        fprintf(stderr, "ERROR: value %ld is out of range\n", actual_value);
        exit(EXIT_FAILURE);
    }

    __uint64_t bits = 0;
    bits |= (((__uint64_t)TYPE_POINTER) << 48);
    bits |= 0xFFF0000000000000UL;
    actual_value &= 0x000FFFFFFFFFFFFF;
    bits |= actual_value;

    *value_nan = (*(double *)&bits);
}

static inline __uint8_t return_type(double value)
{
    __uint64_t bits = (*(__uint64_t *)&value);
    __uint8_t result = (__uint8_t)((bits >> 48) & 0x000000000000000FUL);
    /* if (~result && ~is_nan(value))
    {
        return TYPE_DOUBLE;
    } */
    return result;
}

static inline __int64_t return_value_signed(double value)
{
    __int64_t bits = (*(__int64_t *)&value);
    __int64_t val = 0x0000FFFFFFFFFFFF & bits;
    __int64_t signed_bit = ((val & (1UL << 47)) >> 47);
    signed_bit && (val |= 0xFFFF000000000000);
    return (__int64_t)val;
}

static inline __uint64_t return_value_unsigned(double value)
{
    __uint64_t bits = (*(__uint64_t *)&value);
    __uint64_t val = 0x0000FFFFFFFFFFFFUL & bits;
    return (__uint64_t)val;
}

static inline __uint64_t return_value_pointer(double value)
{
    __uint64_t bits = (*(__uint64_t *)&value);
    __uint64_t val = 0x0000FFFFFFFFFFFFUL & bits;
    return (__uint64_t)val;
}

// IEEE 754 NaNs are encoded with the exponent field filled with ones (like infinity values)
// , and some non-zero number in the significand field (to make them distinct from infinity values);
// this allows the definition of multiple distinct NaN values, depending on which bits are set in the significand field
// , but also on the value of the leading sign bit.

#endif // NAN_IMPLEMENTATION

#endif // NAN_BOXING_H