#pragma once

#if !defined(__has_attribute)
#  define has_no_has_attribute
#  define __has_attribute(x) 0
#endif //!defined(__has_attribute)

#if !defined(Q_WS_WIN) && !defined(Q_WS_WIN32)
#  if !__has_attribute(ms_abi)
#    define ms_abi
#  endif
#  if !__has_attribute(fastcall)
#    define __fastcall
#  endif
#  if 1
#    define __builtin_ms_va_list va_list
#    define __builtin_ms_va_start(list,arg) va_start(list,arg)
#    define __builtin_ms_va_end(list) va_end(list)
#  endif
#endif
