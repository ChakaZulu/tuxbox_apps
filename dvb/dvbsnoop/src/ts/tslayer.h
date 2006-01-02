/*
$Id: tslayer.h,v 1.10 2006/01/02 18:24:34 rasc Exp $


   
 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2006   Rainer.Scherg@gmx.de


*/


#ifndef __TSLAYER_H
#define __TSLAYER_H 


void processTS_packet (u_int pid, long packet_nr, u_char *b, int len);
void decodeTS_packet (u_char *b, int len, u_int pid);
int  ts_adaptation_field (u_char *b);
int  ts_adaptation_field_extension (u_char *b);
int  print_PCR_field (int v, const char *str, u_char *b, int bit_offset);


#endif


