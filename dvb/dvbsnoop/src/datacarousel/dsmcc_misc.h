/*
$Id: dsmcc_misc.h,v 1.4 2004/02/15 01:01:01 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de (rasc)


*/


#ifndef __DSMCC_MISC_H
#define __DSMCC_MISC_H 



// U-N defines  (server messages only)

#define DownloadInfoIndication		0x1002
#define	DownloadDataBlock		0x1003
#define DownloadCancel			0x1005
#define	DownloadServerInitiate		0x1006




int   dsmcc_pto_descriptor_loop (u_char *name, u_char *b);
int   dsmcc_CompatibilityDescriptor(u_char *b);

int   dsmcc_MessageHeader (int v, u_char *b, int len, int *msg_len,
		int *dsmccType, int *messageId);
int   dsmcc_AdaptationHeader (int v, u_char *b, int len);
int   dsmcc_ConditionalAccess (int v, u_char *b, int len);
int   dsmcc_UserID (int v, u_char *b, int len);

int   dsmcc_print_transactionID_32 (int v, u_char *b);



#endif



