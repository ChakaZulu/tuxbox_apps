/*
$Id: print_header.h,v 1.2 2004/01/01 20:09:26 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de (rasc)



*/

#ifndef __PRINT_HEADER_H
#define __PRINT_HEADER_H 1

#include "misc/cmdline.h"


void  print_packet_header (OPTION *opt, char *packetTyp, int pid, int count, int length, int skipped_bytes);


#endif


