#define FBV_SUPPORT_PNG
#define FBV_SUPPORT_BMP
#define FBV_SUPPORT_JPEG
//#define FBV_SUPPORT_GIF


#include <stdio.h>    /* printf       */
#include <sys/time.h> /* gettimeofday */


#define dbout(fmt, args...) {struct timeval tv; gettimeofday(&tv,NULL); \
        printf( "PV[%ld|%02ld] " fmt, (long)tv.tv_sec, (long)tv.tv_usec/10000, ## args);}
