/*
$Id: file_io.h,v 1.1 2004/12/07 21:01:41 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de (rasc)

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


#endif


