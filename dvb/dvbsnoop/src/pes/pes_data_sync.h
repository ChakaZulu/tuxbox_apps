/*
$Id: pes_data_sync.h,v 1.2 2005/11/10 00:05:45 rasc Exp $

   
 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2005   Rainer.Scherg@gmx.de (rasc)



*/

#ifndef __PES_DATA_SYNC_H
#define __PES_DATA_SYNC_H 


void PES_decodeDATA_SYNC (u_char *b, int len);


#endif

