/*
$Id: dsmcc_misc.h,v 1.2 2004/01/01 20:09:16 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de (rasc)


*/


#ifndef __DSMCC_MISC_H
#define __DSMCC_MISC_H 1


int   dsmcc_pto_descriptor_loop (u_char *name, u_char *b);
int   dsmcc_CompatibilityDescriptor(u_char *b);

                                           


#endif



