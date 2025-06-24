#pragma once

#define m_max(x, y) ((x) > (y) ? (x) : (y))
#define m_min(x, y) ((x) < (y) ? (x) : (y))

#define m_sign(x) ((x) < 0 ? -1 : (x) > 1 ? 1 : 0)

#define m_arr_size(arr) (sizeof(arr)/sizeof((arr)[0]))
