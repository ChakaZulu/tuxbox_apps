/*
$Id: mpe_fec.h,v 1.1 2004/07/24 11:47:08 rasc Exp $

 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de (rasc)



*/


#ifndef __MPE_FEC_H
#define __MPE_FEC_H 


void decode_MPE_FEC (u_char *b, int len);
int  real_time_parameters (u_char *b);


#endif

