/*
$Id: hexprint.h,v 1.8 2004/03/09 20:59:23 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de (rasc)




*/

#ifndef __HEXPRINT_H
#define __HEXPRINT_H 

void setHexPrintMode (int i);
void printhex_buf (int verbose, u_char *buf, int len);
void printhexline_buf (int verbose, u_char *buf, int len);
void printasciiline_buf (int verbose, u_char *buf, int n);

#endif

