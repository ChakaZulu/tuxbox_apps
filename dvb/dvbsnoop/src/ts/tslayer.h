/*

 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 mainly for me to learn about dvb streams, mpeg2, mhp, dsm-cc, ...

 http://dvbsnoop.sourceforge.net/

 (c) 2001-2003   Rainer.Scherg@gmx.de


*/


#ifndef __TSLAYER_H
#define __TS_LAYER_H 1


void decodeTS_buf (u_char *b, int len, int pid);
int  ts_adaption_field (u_char *b);
int  ts_adaption_field_extension (u_char *b);


#endif


