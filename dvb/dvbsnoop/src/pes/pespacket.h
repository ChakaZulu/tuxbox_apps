/*

   
 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 mainly for me to learn about dvb streams, mpeg2, mhp, dsm-cc, ...

 http://dvbsnoop.sourceforge.net/

 (c) 2001-2003   Rainer.Scherg@gmx.de



*/

#ifndef __PESPACKET_H
#define __PESPACKET_H 1

void decodePES_buf (u_char *b, u_int len, int pid);

#endif

