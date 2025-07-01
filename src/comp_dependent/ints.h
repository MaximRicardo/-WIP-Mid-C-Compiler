#pragma once

#include <limits.h>

/*
 * WIDTHS SHOULD BE VERIFIED TO BE VALID AT THE START OF THE PROGRAM.
 * SOME COMPILERS MIGHT USE DIFFERENT SIZES FOR THESE VARIABLES
 */

typedef signed i32;
typedef unsigned u32;
typedef signed short i16;
typedef unsigned short u16;
typedef signed char i8;
typedef unsigned char u8;

#define m_i32_max INT_MAX
#define m_i32_min INT_MIN
#define m_u32_max UINT_MAX

/* ints will always be able to represent atleast this much so we can just hard
 * code the value */
#define m_i16_max 32767
#define m_i16_min -32768
#define m_u16_max 65535
#define m_i8_max 127
#define m_i8_min -128
#define m_u8_max 255
