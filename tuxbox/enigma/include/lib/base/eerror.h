#ifndef __E_ERROR__
#define __E_ERROR__

void eFatal(const char* fmt, ...);

#define __DEBUG__

#ifdef __DEBUG__
    void eDebug(const char* fmt, ...);
    void eDebugNoNewLine(const char* fmt, ...);
    void eWarning(const char* fmt, ...);
#else
    inline void eDebug(const char* fmt, ...)
    {
    }

    inline void eDebugNoNewLine(const char* fmt, ...)
    {
    }

    inline void eWarning(const char* fmt, ...)
    {
    }
#endif //__DEBUG__

#endif // __E_ERROR__

