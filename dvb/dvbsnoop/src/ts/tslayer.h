/*
$Id: tslayer.h,v 1.6 2004/04/05 17:32:14 rasc Exp $


   
 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de


*/


#ifndef __TSLAYER_H
#define __TS_LAYER_H 1


void decodeTS_buf (u_char *b, int len, int pid);
int  ts_adaptation_field (u_char *b);
int  ts_adaptation_field_extension (u_char *b);


#endif


