#ifndef __E_ERROR__
#define __E_ERROR__

#include "config.h"

void eFatal(const char* fmt, ...);

#ifdef ASSERT
#undef ASSERT
#endif

#ifdef __DEBUG__
    void eDebug(const char* fmt, ...);
    void eDebugNoNewLine(const char* fmt, ...);
    void eWarning(const char* fmt, ...);
		#define ASSERT(x) { if (!(x)) eFatal("%s:%d ASSERTION %s FAILED!", __FILE__, __LINE__, #x); }
#else
#error NO DEBUG !!
    inline void eDebug(const char* fmt, ...)
    {
    }

    inline void eDebugNoNewLine(const char* fmt, ...)
    {
    }

    inline void eWarning(const char* fmt, ...)
    {
    }
		#define ASSERT(x) do { } while (0)
#endif //__DEBUG__

#endif // __E_ERROR__
