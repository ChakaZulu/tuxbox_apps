/*
$Id: file_io.h,v 1.2 2005/10/20 22:25:06 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2005   Rainer.Scherg@gmx.de (rasc)

*/

#ifndef __FILE_IO_H
#define __FILE_IO_H

   
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>


#ifndef O_LARGEFILE
/* dummy Flag, if not supported */
#warning "No large file support..."
#define O_LARGEFILE 0x00
#endif


#ifndef O_BINARY
/* dummy Flag, if not supported - needed for CYGWIN support */
#define O_BINARY 0x00
#endif


#endif


