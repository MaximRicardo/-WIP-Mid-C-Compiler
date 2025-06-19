#pragma once

/* keep compiler independency */
#ifdef __GNUC__
#define ATTRIBUTE(x) __attribute__ (x)
#else
#define ATTRIBUTE(x)
#endif
