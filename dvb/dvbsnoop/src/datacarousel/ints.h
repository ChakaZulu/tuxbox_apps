/*
$Id: ints.h,v 1.3 2003/11/26 19:52:12 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 mainly for me to learn about dvb streams, mpeg2, mhp, dsm-cc, ...

 http://dvbsnoop.sourceforge.net/

 (c) 2001-2003   Rainer.Scherg@gmx.de


*/


#ifndef __INTS_H
#define __INTS_H 1


void  decode_INT_DSMCC (u_char *b, int len);
int   pto_descriptor_loop (u_char *name, u_char *b);


#endif



