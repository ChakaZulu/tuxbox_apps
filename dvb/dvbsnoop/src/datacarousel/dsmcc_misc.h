/*
$Id: dsmcc_misc.h,v 1.6 2004/02/17 23:54:12 rasc Exp $


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


// data from dsmcc_message_header
typedef struct _DSMCC_MSG_HEADER {
	int	msg_len;
	int	dsmccType;
	int	messageId;
	u_long	transaction_id;		// also download_id
} DSMCC_MSG_HD;




int   dsmcc_pto_descriptor_loop (u_char *name, u_char *b);
int   dsmcc_CompatibilityDescriptor(u_char *b);

int   dsmcc_MessageHeader (int v, u_char *b, int len, DSMCC_MSG_HD *dmh);
int   dsmcc_AdaptationHeader (int v, u_char *b, int len);
int   dsmcc_ConditionalAccess (int v, u_char *b, int len);
int   dsmcc_UserID (int v, u_char *b, int len);

u_long dsmcc_print_transactionID_32 (int v, u_char *b);

int   dsmcc_carousel_NSAP_address_B20 (int v, const char *s, u_char *b);
int   dsmcc_CarouselDescriptor_Loop (const char *s, u_char *b, int len);


#endif



