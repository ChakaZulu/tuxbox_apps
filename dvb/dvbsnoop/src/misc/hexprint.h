/*
$Id: hexprint.h,v 1.6 2004/01/01 20:09:26 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de (rasc)




*/

#ifndef __HEXPRINT_H
#define __HEXPRINT_H 1

void setHexPrintMode (int i);
void printhex_buf (int verbose, u_char *buf, int len);

void printhexdump_buf (int verbose, u_char *buf, int len);
void printhexdump2_buf (int verbose, u_char *buf, int len);
void printhexline_buf (int verbose, u_char *buf, int len);
void printasciiline_buf (int verbose, u_char *buf, int len);

#endif

