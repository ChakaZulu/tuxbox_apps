/*
$Id: pes_data_sync.h,v 1.1 2004/02/02 23:41:23 rasc Exp $

   
 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de (rasc)



*/

#ifndef __PES_DATA_SYNC_H
#define __PES_DATA_SYNC_H 1


void PES_decodeDATA_SYNC (u_char *b, int len);


#endif

