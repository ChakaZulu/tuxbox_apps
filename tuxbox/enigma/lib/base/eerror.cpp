#include "eerror.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void eFatal(const char* fmt, ...)
{
	char buf[1024];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, 1024, fmt, ap);
	va_end(ap);
	fprintf(stderr, "%s\n",buf );
	exit(0);
}

#ifdef __DEBUG__
void eDebug(const char* fmt, ...)
{
	char buf[1024];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, 1024, fmt, ap);
	va_end(ap);
	fprintf(stderr, "%s\n",buf );
}

void eDebugNoNewLine(const char* fmt, ...)
{
	char buf[1024];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, 1024, fmt, ap);
	va_end(ap);
	fprintf(stderr, "%s" ,buf );
}

void eWarning(const char* fmt, ...)
{
	char buf[1024];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, 1024, fmt, ap);
	va_end(ap);
	fprintf(stderr, "%s\n",buf );
}
#endif // __DEBUG__
