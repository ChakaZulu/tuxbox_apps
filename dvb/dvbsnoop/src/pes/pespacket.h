/*
$Id: pespacket.h,v 1.9 2005/10/20 22:25:07 rasc Exp $

   
 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2005   Rainer.Scherg@gmx.de (rasc)



*/

#ifndef __PESPACKET_H
#define __PESPACKET_H 

void processPS_PES_packet (u_int pid, long pkt_nr, u_char *buf, int len);
void decodePS_PES_packet (u_char *b, u_int len, int pid);


#endif

