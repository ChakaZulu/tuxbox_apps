/*
$Id: dsmcc_misc.h,v 1.1 2003/12/26 23:27:39 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2003   Rainer.Scherg@gmx.de


*/


#ifndef __DSMCC_MISC_H
#define __DSMCC_MISC_H 1


int   dsmcc_pto_descriptor_loop (u_char *name, u_char *b);
int   dsmcc_CompatibilityDescriptor(u_char *b);

                                           


#endif



