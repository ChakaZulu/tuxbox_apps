/*
$Id: ts2secpes.h,v 1.3 2004/04/18 19:30:32 rasc Exp $


   
 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de


*/


#ifndef __TS2SECPES_H
#define __TS2SECPES_H


int  ts2SecPesInit (void);
void ts2SecPesFree (void);
int  ts2SecPes_AddPacketStart (int pid, int cc, u_char *b, u_int len);
int  ts2SecPes_AddPacketContinue (int pid, int cc, u_char *b, u_int len);

void ts2SecPes_subdecode (u_char *b, int len, u_int pid);
int  ts2SecPes_checkAndDo_PacketSubdecode_Output (void);
void ts2SecPes_Output_subdecode (void);




#endif


