/*
$Id: dvb_descriptor_premiere.h,v 1.1 2004/11/03 21:01:02 rasc Exp $ 


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de (rasc)


 -- private DVB Descriptors  Premiere.de



$Log: dvb_descriptor_premiere.h,v $
Revision 1.1  2004/11/03 21:01:02  rasc
 - New: "premiere.de" private tables and descriptors (tnx to Peter.Pavlov, Premiere)
 - New: cmd option "-privateprovider <provider name>"
 - New: Private provider sections and descriptors decoding
 - Changed: complete restructuring of private descriptors and sections


*/


#ifndef _PREMIERE_DVB_DESCRIPTOR_H
#define _PREMIERE_DVB_DESCRIPTOR_H 


void descriptor_PRIVATE_PremiereDE_ContentOrder (u_char *b);
void descriptor_PRIVATE_PremiereDE_ParentalInformation (u_char *b);
void descriptor_PRIVATE_PremiereDE_ContentTransmition (u_char *b);


#endif

