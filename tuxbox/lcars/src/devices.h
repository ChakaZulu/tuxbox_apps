#ifndef DEVICES_H
#define DEVICES_H

#include "config.h"

#ifdef HAVE_LINUX_DVB_VERSION_H

#include <linux/dvb/dmx.h>
#include <linux/dvb/video.h>
#include <linux/dvb/frontend.h>
#include <linux/dvb/audio.h>
#include <linux/dvb/ca.h>
#define VIDEO_DEV "/dev/dvb/adapter0/video0"
#define AUDIO_DEV "/dev/dvb/adapter0/audio0"
#define DEMUX_DEV "/dev/dvb/adapter0/demux0"
#define FRONTEND_DEV "/dev/dvb/adapter0/frontend0"
#define PIG_DEV "/dev/v4l/video0"

#elif HAVE_OST_DMX_H

#include <ost/dmx.h>
#include <ost/video.h>
#include <ost/frontend.h>
#include <ost/audio.h>
#include <ost/sec.h>
#include <ost/ca.h>

#include <dbox/avia_gt_pig.h>

#define VIDEO_DEV "/dev/dvb/card0/video0"
#define AUDIO_DEV "/dev/dvb/card0/audio0"
#define DEMUX_DEV "/dev/dvb/card0/demux0"
#define FRONTEND_DEV "/dev/dvb/card0/frontend0"
#define PIG_DEV "/dev/dbox/pig0"

#define fe_code_rate_t CodeRate
#define audio_status audioStatus
#define video_status videoStatus
#define dmx_pes_filter_params dmxPesFilterParams
#define dmx_sct_filter_params dmxSctFilterParams
#define dmx_filter dmxFilter
#define video_stream_source_t videoStreamSource_t
#define audio_stream_source_t audioStreamSource_t

#endif

#include <dbox/avs_core.h>
#include <dbox/fp.h>
#include <dbox/info.h>
#include <dbox/event.h>

#define AVS_DEV "/dev/dbox/avs0"

#endif

