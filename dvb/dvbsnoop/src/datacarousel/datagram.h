/*
$Id: datagram.h,v 1.3 2003/11/26 19:55:31 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 mainly for me to learn about dvb streams, mpeg2, mhp, dsm-cc, ...

 http://dvbsnoop.sourceforge.net/

 (c) 2001-2003   Rainer.Scherg@gmx.de


*/


#ifndef _DATAGRAM_H
#define _DATAGRAM_H 1


void decode_DSMCC_DATAGRAM (u_char *b, int len);


#endif


