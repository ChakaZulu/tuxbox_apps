/*
$Id: tslayer.h,v 1.5 2004/01/01 20:09:43 rasc Exp $


   
 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de


*/


#ifndef __TSLAYER_H
#define __TS_LAYER_H 1


void decodeTS_buf (u_char *b, int len, int pid);
int  ts_adaption_field (u_char *b);
int  ts_adaption_field_extension (u_char *b);


#endif


