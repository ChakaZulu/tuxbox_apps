/*
$Id: dsmcc_int_unt_descriptor.h,v 1.3 2003/12/27 18:17:17 rasc Exp $ 


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2003   Rainer.Scherg@gmx.de


 -- Private TAG Space  DSM-CC   INT, UNT, ...
 -- DSM-CC Descriptors  ISO 13818-6  // TR 102 006



$Log: dsmcc_int_unt_descriptor.h,v $
Revision 1.3  2003/12/27 18:17:17  rasc
dsmcc PES dsmcc_program_stream_descriptorlist

Revision 1.2  2003/11/26 19:55:32  rasc
no message

Revision 1.1  2003/10/29 20:56:18  rasc
more PES stuff, DSM descriptors, testdata



*/


#ifndef __DSM_INT_UNT_DESCRIPTOR_H
#define __DSM_INT_UNT_DESCRIPTOR_H 1


int   descriptorDSMCC_INT_UNT_Private (u_char *b);
void  descriptorDSMCC_INT_UNT_any (u_char *b);



#endif


