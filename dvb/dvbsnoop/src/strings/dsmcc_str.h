/*
$Id: dsmcc_str.h,v 1.14 2004/01/02 16:40:43 rasc Exp $ 


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de


 -- dsmcc decoder helper functions




$Log: dsmcc_str.h,v $
Revision 1.14  2004/01/02 16:40:43  rasc
DSM-CC  INT/UNT descriptors complete
minor changes and fixes

Revision 1.13  2004/01/02 02:18:34  rasc
more DSM-CC  INT/UNT descriptors

Revision 1.12  2004/01/01 20:09:40  rasc
DSM-CC INT/UNT descriptors
PES-sync changed, TS sync changed,
descriptor scope
other changes

Revision 1.11  2003/12/27 18:17:18  rasc
dsmcc PES dsmcc_program_stream_descriptorlist

Revision 1.10  2003/12/26 23:27:40  rasc
DSM-CC  UNT section

Revision 1.9  2003/12/17 23:15:05  rasc
PES DSM-CC  ack and control commands  according ITU H.222.0 Annex B

Revision 1.8  2003/11/29 23:11:43  rasc
no message

Revision 1.7  2003/11/26 23:54:49  rasc
-- bugfixes on Linkage descriptor

Revision 1.6  2003/11/26 19:55:34  rasc
no message

Revision 1.5  2003/11/01 21:40:27  rasc
some broadcast/linkage descriptor stuff

Revision 1.4  2003/10/29 20:54:57  rasc
more PES stuff, DSM descriptors, testdata

Revision 1.3  2003/10/26 21:36:20  rasc
private DSM-CC descriptor Tags started,
INT-Section completed..

Revision 1.2  2003/10/25 19:11:50  rasc
no message

Revision 1.1  2003/10/16 19:02:28  rasc
some updates to dvbsnoop...
- small bugfixes
- tables updates from ETR 162



*/


#ifndef __DSMCC_H
#define __DSMCC_H 1



char *dsmccStrDSMCC_DescriptorTAG (u_int i);
char *dsmccStrDSMCC_INT_UNT_DescriptorTAG (u_int i);

char *dsmccStrMHPOrg (u_int id);
char *dsmccStrAction_Type (u_int id);
char *dsmccStrProcessing_order (u_int id);
char *dsmccStrPayload_scrambling_control (u_int id);
char *dsmccStrAddress_scrambling_control (u_int id);
char *dsmccStrLinkage0CTable_TYPE (u_int i);
char *dsmccStrMultiProtEncapsMACAddrRangeField (u_int i);
char *dsmccStrPlatform_ID (u_int id);
char *dsmccStrCarouselType_ID (u_int id);
char *dsmccStrHigherProtocol_ID (u_int id);
char *dsmccStrUpdateType_ID (u_int id);
char *dsmccStrOUI  (u_int id);

char *dsmccStr_Command_ID  (u_int id);
char *dsmccStr_SelectMode_ID  (u_int id);
char *dsmccStr_DirectionIndicator (u_int id);

char *dsmccStr_DescriptorType (u_int id);
char *dsmccStr_SpecifierType (u_int id);
char *dsmccStr_AccessMode (u_int id);

char *dsmccStr_UpdateFlag (u_int id);
char *dsmccStr_UpdateMethod (u_int id);
char *dsmccStr_TimeUnits (u_int id);




#endif




