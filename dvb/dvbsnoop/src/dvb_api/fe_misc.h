/*
$Id: fe_misc.h,v 1.2 2004/03/21 18:02:45 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de (rasc)

*/

#ifndef __FE_MISC_H
#define __FE_MISC_H

#include "dvb_api.h"



typedef struct _fe_signal {
	int16_t   strength;
	int16_t   snr;
	uint32_t  ber;
	uint32_t  ublocks;
	fe_status_t status;
} FE_SIGNAL;

typedef struct _fe_signal_cap {
	int  ber;
	int  snr;
	int  strength;
	int  status;
	int  ublocks;
} FE_SIG_CAP;



int capability_Check (int f, int cap);
int read_Signal(int f, FE_SIGNAL *s, FE_SIG_CAP *cap);
void out_status_detail (int v, fe_status_t s);



#if DVB_API_VERSION != 1

// -- only API3

int read_FEInfo(int f, struct dvb_frontend_info *fi);
int read_FEParam(int f, struct dvb_frontend_parameters *p);

#endif








#endif


