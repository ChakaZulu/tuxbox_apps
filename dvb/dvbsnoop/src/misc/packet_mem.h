/*
$Id: packet_mem.h,v 1.1 2004/04/15 03:40:39 rasc Exp $


   
 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de


*/


#ifndef __PACKET_MEM_H
#define __PACKET_MEM_H


int  packetMem_acquire (u_long  requested_length);
void packetMem_free (int handle);
void packetMem_clear (int handle);
u_long packetMem_length (int handle);
u_char *packetMem_buffer_start (int handle);
int packetMem_add_data (int handle, u_char *buf, u_long len);


#endif


