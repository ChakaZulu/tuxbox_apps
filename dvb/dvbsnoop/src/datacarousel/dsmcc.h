/*
$Id: dsmcc.h,v 1.1 2003/12/27 00:21:16 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2003   Rainer.Scherg@gmx.de


*/


#ifndef __DSMCC_H
#define __DSMCC_H 1


void  decode_DSMCC_section (u_char *b, int len);


#endif



