/*
$Id: ait.h,v 1.1 2004/02/07 01:28:00 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de (rasc)


*/


#ifndef __AIT_H
#define __AIT_H 



void  decode_MHP_AIT (u_char *b, int len);
int  mhp_application_identifier (int v, u_char *b);



#endif



