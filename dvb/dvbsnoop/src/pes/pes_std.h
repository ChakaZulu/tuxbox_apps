/*
$Id: pes_std.h,v 1.2 2004/02/02 23:34:08 rasc Exp $

   
 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de (rasc)



*/

#ifndef __PES_STD_H
#define __PES_STD_H 1

void  PES_decode_std (u_char *b, int len, u_int PES_streamID);

#endif

