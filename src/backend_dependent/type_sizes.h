#pragma once

/* The width of the compiler's data types. Not the width of the data
 * types in this actual codebase */

/* Widths are measured in bytes */

#define m_TypeSize_char 1
#define m_TypeSize_int 4
#define m_TypeSize_ptr 4

/* in bytes */
#define m_TypeSize_stack_min_alignment 4

/* how many bytes it takes to make a new stack frame */
#define m_TypeSize_stack_frame_size 4

/* not counting the base and stack pointers */
#define m_TypeSize_callee_saved_regs_stack_size (4*3)

#define m_TypeSize_return_address_size 4
