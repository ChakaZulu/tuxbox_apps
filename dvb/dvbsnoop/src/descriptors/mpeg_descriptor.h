/*
$Id: mpeg_descriptor.h,v 1.1 2003/07/08 19:59:50 rasc Exp $ 

  dvbsnoop
  (c) Rainer Scherg 2001-2003

  MPEG Descriptors  ISO/IEC 13818-2


$Log: mpeg_descriptor.h,v $
Revision 1.1  2003/07/08 19:59:50  rasc
restructuring... some new, some fixes,
trying to include DSM-CC, Well someone a ISO13818-6 and latest version of ISO 18313-1 to spare?


*/

int   descriptorMPEG (u_char *b);
void  descriptorMPEG_any (u_char *b);

void  descriptorMPEG_VideoStream (u_char *b);
void  descriptorMPEG_AudioStream (u_char *b);
void  descriptorMPEG_Hierarchy (u_char *b);
void  descriptorMPEG_Registration (u_char *b);
void  descriptorMPEG_DataStreamAlignment (u_char *b);
void  descriptorMPEG_TargetBackgroundGrid (u_char *b);
void  descriptorMPEG_VideoWindow (u_char *b);
void  descriptorMPEG_CA (u_char *b);
void  descriptorMPEG_ISO639_Lang (u_char *b);
void  descriptorMPEG_SystemClock (u_char *b);
void  descriptorMPEG_MultiplexBufUtil (u_char *b);
void  descriptorMPEG_Copyright (u_char *b);
void  descriptorMPEG_MaxBitrate (u_char *b);
void  descriptorMPEG_PrivateDataIndicator (u_char *b);
void  descriptorMPEG_SmoothingBuf (u_char *b);
void  descriptorMPEG_STD (u_char *b);
void  descriptorMPEG_IBP (u_char *b);

