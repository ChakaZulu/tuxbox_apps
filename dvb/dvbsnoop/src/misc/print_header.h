/*
$Id: print_header.h,v 1.1 2003/12/28 14:00:27 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2003   Rainer.Scherg@gmx.de

*/

#ifndef __PRINT_HEADER_H
#define __PRINT_HEADER_H 1

#include "misc/cmdline.h"


void  print_packet_header (OPTION *opt, char *packetTyp, int pid, int count, int length, int skipped_bytes);


#endif


