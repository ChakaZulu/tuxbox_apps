/*
$Id: ebu_misc.h,v 1.2 2004/02/05 10:30:56 rasc Exp $

   
 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de (rasc)



*/

#ifndef __EBU_MISC_H
#define __EBU_MISC_H

u_char unhamB84 (u_char c);
u_char unhamW84  (u_char lsb, u_char msb);
u_long unhamT24_18 (u_char lsb, u_char msb1, u_char msb2);


u_char invertChar (u_char *b);
void   invertBuffer (u_char *b, int len);


void ebu_rfl_out (int v, u_char *b);


#endif

