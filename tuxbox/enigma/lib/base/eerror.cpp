#include "eerror.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define __DEBUG__

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

void eDebug(const char* fmt, ...)
{
#ifdef __DEBUG__
	char buf[1024];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, 1024, fmt, ap);
	va_end(ap);
	fprintf(stderr, "%s\n",buf );
#endif //__DEBUG__
}

