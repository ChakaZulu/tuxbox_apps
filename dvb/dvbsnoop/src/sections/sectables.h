/*
$Id: sectables.h,v 1.4 2004/01/01 20:09:31 rasc Exp $

 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de (rasc)



*/




#ifndef __SECTABLES_H
#define __SECTABLES_H 1


void decodeSections_buf (u_char *buf, int len, u_int pid);


#endif





