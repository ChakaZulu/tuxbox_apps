/*
$Id: pes_std.h,v 1.3 2005/11/10 00:05:45 rasc Exp $

   
 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2005   Rainer.Scherg@gmx.de (rasc)



*/

#ifndef __PES_STD_H__
#define __PES_STD_H__ 

void  PES_decode_std (u_char *b, int len, u_int PES_streamID);

#endif

