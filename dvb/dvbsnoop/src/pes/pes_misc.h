/*
$Id: pes_misc.h,v 1.2 2004/01/22 22:26:35 rasc Exp $

   
 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de (rasc)



*/

#ifndef __PES_MISC_H
#define __PES_MISC_H 1


void print_xTS_field (int v, const char *str, u_char *b, int bit_offset);
void pack_header (int v, u_char *b, int len);
void system_header (int v, u_char *b, int len);


#endif

