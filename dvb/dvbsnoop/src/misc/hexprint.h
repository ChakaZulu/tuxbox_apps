/*

 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 mainly for me to learn about dvb streams, mpeg2, mhp, dsm-cc, ...

 http://dvbsnoop.sourceforge.net/

 (c) 2001-2003   Rainer.Scherg@gmx.de


*/

#ifndef __HEXPRINT_H
#define __HEXPRINT_H 1

void setHexPrintMode (int i);
void printhex_buf (int verbose, u_char *buf, int len);
void printhexdump_buf (int verbose, u_char *buf, int len);
void printhexline_buf (int verbose, u_char *buf, int len);
void printasciiline_buf (int verbose, u_char *buf, int len);

#endif

