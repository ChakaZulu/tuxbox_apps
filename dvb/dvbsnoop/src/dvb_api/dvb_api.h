/*
$Id: dvb_api.h,v 1.4 2004/01/06 14:06:08 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de (rasc)


*/

#ifndef __DVB_API_H
#define __DVB_API_H 1


#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>



#if defined(HAVE_LINUX_DVB_DMX_H)

// API 3
#define DVB_API_VERSION 3

#include <linux/dvb/dmx.h>
#define DEMUX_DEVICE    "/dev/dvb/adapter0/demux0"
#define DVR_DEVICE      "/dev/dvb/adapter0/dvr0"
#include <linux/dvb/frontend.h> 
#define FRONTEND_DEVICE "/dev/dvb/adapter0/frontend0"

#elif defined(HAVE_OST_DMX_H)

// API 1
#define DVB_API_VERSION 1

#include <ost/dmx.h>
#define DEMUX_DEVICE   "/dev/dvb/card0/demux0"
#define DVR_DEVICE     "/dev/dvb/card0/dvr0"
#define dmx_pes_filter_params dmxPesFilterParams
#define dmx_sct_filter_params dmxSctFilterParams
#define pes_type pesType
#include <ost/frontend.h> 
#define FRONTEND_DEVICE "/dev/dvb/card0/frontend0"
#define fe_status_t FrontendStatus

#endif


#endif

