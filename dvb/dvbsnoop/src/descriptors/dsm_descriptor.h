/*
$Id: dsm_descriptor.h,v 1.1 2003/07/08 19:59:50 rasc Exp $ 


  dvbsnoop
  (c) Rainer Scherg 2001-2003

  DSM-CC Descriptors  ISO 13818-6


$Log: dsm_descriptor.h,v $
Revision 1.1  2003/07/08 19:59:50  rasc
restructuring... some new, some fixes,
trying to include DSM-CC, Well someone a ISO13818-6 and latest version of ISO 18313-1 to spare?


*/


int   descriptorDSMCC (u_char *b);
void  descriptorDSMCC_any (u_char *b);



