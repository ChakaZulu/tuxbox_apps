/*
$Id: pkt_time.h,v 1.2 2001/10/02 21:52:44 rasc Exp $

 -- Print Packet receive time
 -- (c) 2001 rasc


$Log: pkt_time.h,v $
Revision 1.2  2001/10/02 21:52:44  rasc
- init der time_delta
- PES erweitert, PES arbeitet im read() noch nicht richtig!!
- muss tmbinc fragem, ob ich Mist baue, oder der Treiber (??)

Revision 1.1  2001/09/30 13:05:20  rasc
dvbsnoop v0.7  -- Commit to CVS


*/


void  out_receive_time (int verbose, OPTION *opt);
void  init_receive_time (void);
