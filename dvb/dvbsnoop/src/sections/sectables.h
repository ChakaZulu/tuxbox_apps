/*
$Id: sectables.h,v 1.3 2003/11/26 19:55:34 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 mainly for me to learn about dvb streams, mpeg2, mhp, dsm-cc, ...

 http://dvbsnoop.sourceforge.net/

 (c) 2001-2003   Rainer.Scherg@gmx.de

*/



#ifndef __SECTABLES_H
#define __SECTABLES_H 1


void decodeSections_buf (u_char *buf, int len, u_int pid);


#endif





