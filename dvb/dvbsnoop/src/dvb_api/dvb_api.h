/*
$Id: dvb_api.h,v 1.2 2004/01/01 20:09:23 rasc Exp $


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

#include <linux/dvb/dmx.h>
#define DEMUX_DEVICE "/dev/dvb/adapter0/demux0"
#define DVR_DEVICE   "/dev/dvb/adapter0/dvr0"

#elif defined(HAVE_OST_DMX_H)

#include <ost/dmx.h>
#define DEMUX_DEVICE "/dev/dvb/card0/demux0"
#define DVR_DEVICE   "/dev/dvb/card0/dvr0"
#define dmx_pes_filter_params dmxPesFilterParams
#define dmx_sct_filter_params dmxSctFilterParams
#define pes_type pesType

#endif


#endif


