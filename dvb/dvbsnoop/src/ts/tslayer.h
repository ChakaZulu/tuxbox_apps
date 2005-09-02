/*
$Id: tslayer.h,v 1.8 2005/09/02 14:11:36 rasc Exp $


   
 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2005   Rainer.Scherg@gmx.de


*/


#ifndef __TSLAYER_H
#define __TS_LAYER_H 


void decodeTS_buf (u_char *b, int len, u_int pid);
int  ts_adaptation_field (u_char *b);
int  ts_adaptation_field_extension (u_char *b);
int  print_PCR_field (int v, const char *str, u_char *b, int bit_offset);


#endif


