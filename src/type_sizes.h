#pragma once

/* The width of the compiler's data types. Not the width of the data
 * types in this actual codebase */

/* Widths are measured in bytes */

#define m_TypeSize_int 4

/* All variables on the stack are assumed to take up a multiple of this many
 * bytes on the stack */
#define m_TypeSize_var_stack_size 8
