#pragma once

#include "../comp_dependent/ints.h"

/* rounds the result down */
u32 log2_u32(u32 x);
/* rounds the result up */
u32 log2_u32_up(u32 x);
unsigned long u_round_up(unsigned long num, unsigned long multiple);
unsigned long u_round_down(unsigned long num, unsigned long multiple);
long s_round_up(long num, long multiple);
long s_round_down(long num, long multiple);
