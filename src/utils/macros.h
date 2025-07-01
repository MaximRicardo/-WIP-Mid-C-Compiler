#pragma once

#define m_max(x, y) ((x) > (y) ? (x) : (y))
#define m_min(x, y) ((x) < (y) ? (x) : (y))

#define m_sign(x) ((x) < 0 ? -1 : (x) > 1 ? 1 : 0)

#define m_arr_size(arr) (sizeof(arr)/sizeof((arr)[0]))

#define m_emit_compiler_warning_stringify0(x) #x
#define m_emit_compiler_warning_stringify1(x) m_emit_compiler_warning_stringify0(x)
#ifdef __GNUC__
    #define m_emit_compiler_warning_compose(x) GCC warning x
#else
    #define m_emit_compiler_message_preface(type) \
    __FILE__ "(" m_emit_compiler_warning_stringify1(__LINE__) "): " type ": "
    #define m_emit_compiler_warning_compose(x) message(m_emit_compiler_message_preface("warning C0000") x)
#endif
#define m_warning(x) _Pragma(m_emit_compiler_warning_stringify1(m_emit_compiler_warning_compose(x)))
