/*
$Id: sig_abort.h,v 1.1 2005/09/06 23:39:04 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2005   Rainer.Scherg@gmx.de (rasc)


 -- Handle Kill/Interrupt Signals from User/OS
 -- (e.g. Kill -1)




$Log: sig_abort.h,v $
Revision 1.1  2005/09/06 23:39:04  rasc
catch OS signals (kill ...) for smooth program termination



*/


#ifndef __SIG_ABORT_H
#define __SIG_ABORT_H


void initOSSigHandler (void);
int  isSigAbort (void);
void restoreOSSigHandler (void);


#endif


