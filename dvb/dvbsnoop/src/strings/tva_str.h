/*
$Id: tva_str.h,v 1.1 2004/08/06 22:21:38 rasc Exp $ 


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de


 -- TV-Anytime Strings




$Log: tva_str.h,v $
Revision 1.1  2004/08/06 22:21:38  rasc
New: TV-Anytime (TS 102 323) RNT descriptors 0x40 - 0x42




*/


#ifndef __TVA_STR_H
#define __TVA_STR_H 


char *tvaStrTVA_DescriptorTAG (u_int i);

char *tvastr_CRI_DATA_scheduled_flag (u_int i);




#endif




