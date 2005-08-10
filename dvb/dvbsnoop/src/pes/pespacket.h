/*
$Id: pespacket.h,v 1.8 2005/08/10 21:28:18 rasc Exp $

   
 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de (rasc)



*/

#ifndef __PESPACKET_H
#define __PESPACKET_H 1

void decodePS_buf (u_char *b, u_int len, int pid);
void decodePES_buf (u_char *b, u_int len, int pid);

#endif

