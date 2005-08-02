#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <memory.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

#include <lib/gdi/gfbdc.h>
#include <lib/dvb/decoder.h>
#include <lib/gui/eskin.h>

#include <config.h>
#if HAVE_DVB_API_VERSION < 3
#include <ost/dmx.h>
#include <ost/video.h>
#include <ost/audio.h>
#define VIDEO_DEV "/dev/dvb/card0/video0"
#define AUDIO_DEV "/dev/dvb/card0/audio0"
#define DEMUX_DEV "/dev/dvb/card0/demux0"
#else
#include <linux/dvb/dmx.h>
#include <linux/dvb/video.h>
#include <linux/dvb/audio.h>
#define VIDEO_DEV "/dev/dvb/adapter0/video0"
#define AUDIO_DEV "/dev/dvb/adapter0/audio0"
#define DEMUX_DEV "/dev/dvb/adapter0/demux0"
#define audioStatus audio_status
#define videoStatus video_status
#define pesType pes_type
#define playState play_state
#define audioStreamSource_t audio_stream_source_t
#define videoStreamSource_t video_stream_source_t
#define streamSource stream_source
#define dmxPesFilterParams dmx_pes_filter_params
#endif

#include <lib/dvb/subtitling.h>

eSubtitleWidget *eSubtitleWidget::instance;

static int extractPTS(unsigned long long &pts, unsigned char *pkt)
{
	pkt += 7;
	int flags = *pkt++;
	
	pkt++; // header length
	
	if (flags & 0x80) /* PTS present? */
	{
			/* damn gcc bug */
		pts  = ((unsigned long long)(((pkt[0] >> 1) & 7))) << 30;
		pts |=   pkt[1] << 22;
		pts |=  (pkt[2]>>1) << 15;
		pts |=   pkt[3] << 7;
		pts |=  (pkt[5]>>1);
		
		return 0;
	} else
		return -1;
}

void eSubtitleWidget::processPESPacket(unsigned char *pkt, int len)
{
	unsigned long long current = 0;
	if (Decoder::getSTC(current))
		eDebug("bloed, going unsyced");
	eDebug("DEMUX STC: %08llx\n", current);
	
	unsigned long long pts = 0;
	
	int enqueue = 0;
	
	if (!queue.empty())
		enqueue = 1;
	else if (!extractPTS(pts, pkt))
	{
		eDebug("PES   STC: %08llx\n", pts);
		signed long long int diff = pts - current;
		eDebug("      STC: %lld\n", diff);
		if (diff > 900000) // 10s
		{
			eDebug("rediculous high delay! showing now");
		} else if (diff > 2000)
			enqueue = 1;
		else
			eDebug("showing instantly, diff small enough!");
	}

	if (enqueue)
	{
		int wasempty = queue.empty();
		struct pes_packet_s pes;
		pes.pts = pts;
		pes.pkt = new unsigned char[len];
		memcpy(pes.pkt, pkt, len);
		pes.len = len;
		queue.push(pes);
		if (wasempty)
		{
			eDebug("setting timer to %d ms!\n", (pes.pts - current) / 90);
			timer.start((pes.pts - current) / 90, 1);
		}
		return;
	}
	subtitle_process_pes(subtitle, pesbuffer, peslen);
}

void eSubtitleWidget::displaying_timeout()
{
	eDebug("displaying timeout reached... hide visible subtitles");
	subtitle_reset(subtitle);
	if ( isVisible() )
		subtitle_clear_screen(subtitle);
}

void eSubtitleWidget::processNext()
{
	if (queue.empty())
	{
		eWarning("Subtitle queue is empty, but timer was called!");
		return;
	}

	unsigned long long fpts=0;
	int first = 1;
	while (!queue.empty())
	{
		pes_packet_s pes = queue.front();
		if (pes.pts && !first)
			break;
		if (first)
			fpts = pes.pts;
		first = 0;
		queue.pop();

		subtitle_process_pes(subtitle, pes.pkt, pes.len);

		delete [] pes.pkt;
	}

	unsigned long long current = 0;
	
	if (Decoder::getSTC(current))
	{
		eWarning("getSTC failed, dropping all Subtitle packets!");
		while (!queue.empty())
		{
			pes_packet_s pkt = queue.front();
			queue.pop();
			delete [] pkt.pkt;
		}
		return;
	}
	
	eDebug("by the way, actual delay was %lld(%lld msek)\n", current - fpts, (current-fpts)/90 );

	if (queue.empty())
		return;
	
	signed long long int diff = queue.front().pts - current;
	timer.start(diff / 90, 1);
}

void eSubtitleWidget::gotData(int what)
{
	while (1)
	{
		unsigned char packet[1024];
		int l;
		l=::read(fd, packet, 1024);
		if (l <= 0)
			break;
		
		unsigned char *p = packet;
		
		while (l)
		{
			if (pos >= 6) // length ok?
			{
				int max = peslen - pos;
				if (max > l)
					max = l;
				memcpy(pesbuffer + pos, p, max);
				pos += max;
				p += max;
				
				l -= max;
				
				if (pos == peslen)
				{
					processPESPacket(pesbuffer, pos);
					pos = 0;
				}
			} else
			{
				if (pos < 4)
					if (*p != "\x00\x00\x01\xbd"[pos])
					{
						pos = 0;
						p++;
						l--;
						continue;
					}
				pesbuffer[pos++] = *p++; l--;
				if (pos == 6)
				{
					peslen = ((pesbuffer[4] << 8) | pesbuffer[5]) + 6;
				}
			}
		}
	}
}

int eSubtitleWidget::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
	case eWidgetEvent::willShow:
//		eDebug("willShow!!!");
		isvisible = 1;
		subtitle_screen_enable(subtitle, 1);
		break;
	case eWidgetEvent::willHide:
//		eDebug("willHide!!!");
		isvisible = 0;
		subtitle_screen_enable(subtitle, 0);
		//restore old palette
		eSkin::getActive()->setPalette(gFBDC::getInstance());
		break;
	default:
		return eWidget::eventHandler(event);;
	}
	return 0;
}

void eSubtitleWidget::start(int pid, const std::set<int> &ppageids)
{
	pageids = ppageids;
	stop();
	if (isvisible)
		subtitle_screen_enable(subtitle, 1);
	fd = open(DEMUX_DEV, O_RDWR|O_NONBLOCK);
	if (fd == -1)
	{
		eWarning("failed to open " DEMUX_DEV ": %m");
		return;
	}
	
	sn = new eSocketNotifier(eApp, fd, eSocketNotifier::Read);
	CONNECT(sn->activated, eSubtitleWidget::gotData);

	struct dmxPesFilterParams f;
	this->pid = f.pid = pid;
	f.input = DMX_IN_FRONTEND;
	f.output = DMX_OUT_TAP;
	f.pesType = DMX_PES_OTHER;
	f.flags = DMX_IMMEDIATE_START;
	if (::ioctl(fd, DMX_SET_PES_FILTER, &f) == -1)
		eWarning("DMX_SET_PES_FILTER: %m (subtitling)");
	else
		eDebug("started subtitling filter..");
		
	pos = 0;
}

static void subtitle_set_palette(struct subtitle_clut *pal)
{
	static gRGB def_palette[16];
	static bool def_palette_initialized;

	gPainter p(*gFBDC::getInstance());
	if ( !pal )// use default pallette
	{
		if ( !def_palette_initialized )  // fill default palette
		{
			for (int i=0; i < 16; ++i)
			{
				if (!i)
					def_palette[i].a = 0xFF;
				else if (i&8)
				{
					if (i & 1)
						def_palette[i].r = 0x80;
					if (i & 2)
						def_palette[i].g = 0x80;
					if (i & 4)
						def_palette[i].b = 0x80;
				}
				else
				{
					if (i & 1)
						def_palette[i].r = 0xFF;
					if (i & 2)
						def_palette[i].g = 0xFF;
					if (i & 4)
						def_palette[i].b = 0xFF;
				}
//				eDebug("%d %02x%02x%02x%02x",
//					i, def_palette[i].r, def_palette[i].g, def_palette[i].b, def_palette[i].a);
			}
			def_palette_initialized=1;
		}
		p.setPalette(def_palette, 240, 16);
	}
	else
	{
	//	eDebug("updating palette!");
		gRGB palette[pal->size];

		for (int i=0; i<pal->size; ++i)
		{
			int y = pal->entries[i].Y, cr = pal->entries[i].Cr, cb = pal->entries[i].Cb;
		
			if (y > 0)
			{
				y -= 16;
				cr -= 128;
				cb -= 128;
#if 1
//				let's try a bit different conversion method
				palette[i].r = MAX(MIN(((298 * y            + 460 * cr) / 256), 255), 0);
				palette[i].g = MAX(MIN(((298 * y -  55 * cb - 137 * cr) / 256), 255), 0);
				palette[i].b = MAX(MIN(((298 * y + 543 * cb           ) / 256), 255), 0);
#else
				palette[i].r = ((1164 * y + 1596 * cr) + 500) / 1000;
				palette[i].g = ((1164 * y - 813 * cr - 392 * cb) + 500) / 1000;
				palette[i].b = ((1164 * y + 2017 * cb) + 500) / 1000;
#endif
				palette[i].a = (pal->entries[i].T) & 0xFF;
			} else
			{
				palette[i].r = 0;
				palette[i].g = 0;
				palette[i].b = 0;
				palette[i].a = 0xFF;
			}
//		eDebug("%d: %d %d %d %d", i, palette[i].r, palette[i].g, palette[i].b, palette[i].a);
		}
		p.setPalette(palette, 240, pal->size);
	}
//	eDebug("palette changed");
}

eSubtitleWidget::eSubtitleWidget()
	:timer(eApp), timeout(eApp)
{
	instance = this;
	fd = -1;
	sn = 0;
	subtitle = new subtitle_ctx;
	subtitle->pages = 0;
	subtitle->bbox_left = 0;
	subtitle->bbox_right = 0;
	subtitle->bbox_top = 0;
	subtitle->bbox_bottom = 0;
	subtitle->screen_enabled = 0;
	subtitle->timeout_timer = &timeout;

	gFBDC *fbdc = gFBDC::getInstance();
	gPixmap *pixmap = &fbdc->getPixmap();

	subtitle->screen_width = pixmap->x;
	subtitle->screen_height = pixmap->y;
	subtitle->screen_buffer = (__u8*)pixmap->data;
	subtitle->set_palette = subtitle_set_palette;
	
	CONNECT(timer.timeout, eSubtitleWidget::processNext);
	CONNECT(timeout.timeout, eSubtitleWidget::displaying_timeout);
	CONNECT(eWidget::globalFocusChanged, eSubtitleWidget::globalFocusHasChanged);
}

eSubtitleWidget::~eSubtitleWidget()
{
	while (!queue.empty())
	{
		pes_packet_s pkt = queue.front();
		queue.front();
		delete [] pkt.pkt;
	}
	delete subtitle;
}

int eSubtitleWidget::getCurPid()
{
	return pid;
}

void eSubtitleWidget::stop()
{
	if ( sn )
	{
		pid=-1;
		delete sn;
		sn = 0;
		subtitle_screen_enable(subtitle, 0);
		subtitle_reset(subtitle);
		if (fd != -1)
		{
			::close(fd);
			fd = -1;
		}
		eSkin::getActive()->setPalette(gFBDC::getInstance());
	}
}

void eSubtitleWidget::globalFocusHasChanged(const eWidget* newFocus)
{
	if ( !sn ) // not running
		return; 
	if ( newFocus )
		hide();
	else
		show();
}
