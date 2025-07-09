#include "math_funcs.h"
#include <assert.h>

u32 log2_u32(u32 x)
{
    u32 i = 0;

    assert(x > 0);

    /* the logarithm of any unsigned integer, is the index of it's most
     * significant active bit, minus one.
     * for example: log2(10):
     *    8 in binary is 1000
     *    the most significant active bit is the 3rd one (starting from 0).
     *    and you get log2(8) = 3.
     *  to get the index of that bit, you just need to count how many times
     *  you can right shift the number before it becomes 0, and then decrement
     *  the result by 1. */
    while (x != 0) {
        x >>= 1;
        ++i;
    }
    --i;

    return i;
}

u32 log2_u32_up(u32 x)
{
    u32 i = 0;
    u32 og_x = x;

    assert(x > 0);

    /* see log2_u32 */
    while (x != 0) {
        x >>= 1;
        ++i;
    }
    --i;

    /* rounds the result up */
    if ((1 << i) != og_x)
        ++i;

    return i;
}

unsigned long u_round_up(unsigned long num, unsigned long multiple)
{
    unsigned long remainder;

    assert(multiple != 0);

    remainder = num % multiple;
    if (remainder == 0)
        return num;

    return num + multiple - remainder;
}

unsigned long u_round_down(unsigned long num, unsigned long multiple)
{
    return num / multiple * multiple;
}

long s_round_up(long num, long multiple)
{
    long remainder;

    assert(multiple != 0);

    remainder = num % multiple;
    if (remainder == 0)
        return num;

    return num + multiple - remainder;
}

long s_round_down(long num, long multiple)
{
    return num / multiple * multiple;
}
