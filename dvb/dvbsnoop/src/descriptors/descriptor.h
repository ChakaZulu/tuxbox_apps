/*
$Id: descriptor.h,v 1.9 2004/02/07 01:28:01 rasc Exp $ 


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de (rasc)


 -- Descriptors Section




$Log: descriptor.h,v $
Revision 1.9  2004/02/07 01:28:01  rasc
MHP Application  Information Table
some AIT descriptors

Revision 1.8  2004/01/11 21:01:31  rasc
PES stream directory, PES restructured

Revision 1.7  2004/01/02 22:25:35  rasc
DSM-CC  MODULEs descriptors complete

Revision 1.6  2004/01/01 20:09:19  rasc
DSM-CC INT/UNT descriptors
PES-sync changed, TS sync changed,
descriptor scope
other changes

Revision 1.5  2003/11/26 19:55:32  rasc
no message

Revision 1.4  2003/07/08 19:59:50  rasc
restructuring... some new, some fixes,
trying to include DSM-CC, Well someone a ISO13818-6 and latest version of ISO 18313-1 to spare?


*/


#ifndef __DESCRIPTOR_H
#define __DESCRIPTOR_H 1


// Descriptor tag space/scope...
typedef enum {
	MPEG, DVB_SI,
	DSMCC_STREAM, DSMCC_CAROUSEL, DSMCC_INT_UNT, MHP_AIT
} DTAG_SCOPE;


int   descriptor (u_char *b, DTAG_SCOPE s);
void  descriptor_any (u_char *b);


#endif


