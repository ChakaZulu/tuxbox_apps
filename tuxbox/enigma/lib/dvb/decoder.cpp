#ifdef DBOX
#define VIDEO_DEV	 "/dev/ost/video0"
#define AUDIO_DEV "/dev/ost/audio0"
#define DEMUX_DEV "/dev/ost/demux0"
#else
#define DEMUX_DEV "/dev/demuxapi0"
#define VIDEO_DEV "/dev/vdec_dev"
#define AUDIO_DEV_MPEG "/dev/adec_mpg"
#define AUDIO_DEV_AC3  "/dev/adec_ac3"
#endif


#include <qobject.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <memory.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

typedef unsigned char __u8;

#ifdef DBOX
#include <ost/dmx.h>
#include <ost/video.h>
#include <ost/audio.h>
// #define NEW_CAMD
#else
#include <xp/xp_osd_user.h>
#include <vid/vid_inf.h>
#include <aud/aud_inf.h>
#endif

#include "decoder.h"

decoderParameters Decoder::current;
decoderParameters Decoder::parms;
int Decoder::fd::video;
int Decoder::fd::audio;
int Decoder::fd::demux_video;
int Decoder::fd::demux_audio;
int Decoder::fd::demux_pcr;
#define USE_CAMD

static void SetECM(int vpid, int apid, int ecmpid, int emmpid, int pmtpid, int casystemid, int descriptor_length, __u8 *descriptors)
{
#ifdef DBOX
	static int lastpid=-1;

	if (lastpid!=-1)
	{
		kill(lastpid, SIGKILL);
		waitpid(lastpid, 0, 0);
		lastpid=-1;
	}

	if (!descriptor_length)
		return;

	char buffer[6][5];
	sprintf(buffer[0], "%x", vpid);
	sprintf(buffer[1], "%x", apid);
	sprintf(buffer[2], "%x", ecmpid);
	sprintf(buffer[3], "%x", emmpid);
	sprintf(buffer[4], "%x", pmtpid);
	sprintf(buffer[5], "%x", casystemid);
	
	char descriptor[2048];
	
	for (int i=0; i<descriptor_length; i++)
		sprintf(descriptor+i*2, "%02x", descriptors[i]);
	
	switch (lastpid=fork())
	{
	case -1:
		printf("fork failed!\n");
		return;
	case 0:
	{ 
#if 1
		close(0);
		close(1);
		close(2);
#endif
#ifdef USE_CAMD
		if (execlp("camd", "camd", buffer[0], buffer[1], buffer[4], 
#ifdef NEW_CAMD
			descriptor, 
#endif
			0)<0)
			perror("camd");
#endif

		exit(0);
		break;
	}
	}
#endif
}

int Decoder::Initialize()
{
	parms.vpid=
	parms.apid=
	parms.tpid=
	parms.pcrpid=
	parms.ecmpid=
	parms.emmpid=-1;
	parms.audio_type=0;
	parms.emmpid=-2;
	parms.recordmode=0;
	parms.descriptor_length=0;
	current=parms;
	fd.video=
	fd.audio=
	fd.demux_video=
	fd.demux_audio=
	fd.demux_pcr=-1;
	return 0;
}

void Decoder::Close()
{	
	Flush();
}

void Decoder::Flush()
{
	parms.vpid=
	parms.apid=
	parms.tpid=
	parms.pcrpid=
	parms.ecmpid=-1;
//	parms.emmpid=
	parms.audio_type=0;
	parms.descriptor_length=0;
	parms.emmpid=-2;
	parms.recordmode=0;
	Set();
}

int Decoder::Set()
{
	int changed=0;
#ifdef DBOX
	dmxPesFilterParams pes_filter;
#else
	demux_pes_para pes_filter;
#endif
	 
	if (parms.vpid != current.vpid)
		changed|=1;
	if (parms.apid != current.apid)
		changed|=2;
	if (parms.tpid != current.tpid)
		changed|=4;
	if (parms.pcrpid != current.pcrpid)
		changed|=8;
	if (parms.ecmpid != current.ecmpid)
		changed|=0x10;
	if (parms.emmpid != current.emmpid)
		changed|=0x20;
	if (parms.pmtpid != current.pmtpid)
		changed|=0x40;
	if (parms.casystemid != current.casystemid)
		changed|=0x80;
	if (parms.audio_type != current.audio_type)
		changed|=0x100;
	if (parms.recordmode != current.recordmode)
		changed|=0xF;

	qDebug(" ------------> changed! %x", changed);

	if (changed&9)													// stop decoding
		if (fd.video!=-1)
		{
#ifdef DBOX
			qDebug("VIDEO_STOP");
			if (ioctl(fd.video, VIDEO_STOP, 1)<0)
				perror("VIDEO_STOP");
#else
			if (ioctl(fd.video, MPEG_VID_STOP, 1)<0)
				perror("MPEG_VID_STOP");
#endif
			close(fd.video);
			fd.video=-1;
		}

	if (changed&1)													// stop queues
		if (fd.demux_video>=0)
		{
			close(fd.demux_video);
			fd.demux_video=-1;
		}

	if (changed&2)
		if (fd.demux_audio>=0)
		{
			close(fd.demux_audio);
			fd.demux_audio=-1;
		}

	if (changed&0xF7)
		SetECM(parms.vpid, parms.apid, parms.ecmpid, parms.emmpid, parms.pmtpid, parms.casystemid, parms.descriptor_length, parms.descriptors);

	if (changed & 8)
	{
		if (fd.demux_pcr!=-1)
			close(fd.demux_pcr);
		fd.demux_pcr=open(DEMUX_DEV, O_RDWR);
		if (fd.demux_pcr<0)
			perror(DEMUX_DEV);
		else
		{
			pes_filter.pid=parms.pcrpid;
#ifdef DBOX
			pes_filter.input=DMX_IN_FRONTEND;
			pes_filter.output=DMX_OUT_DECODER;
			pes_filter.pesType=DMX_PES_PCR;
			pes_filter.flags=0;
			if (ioctl(fd.demux_pcr, DMX_SET_PES_FILTER, &pes_filter)<0)
				perror("DMX_SET_PES_FILTER - PCR");
#else
			pes_filter.output=OUT_DECODER;
			pes_filter.pesType=DMX_PES_PCR;
			if (ioctl(fd.demux_pcr, DEMUX_FILTER_PES_SET, &pes_filter)<0)
				perror("DEMUX_FILTER_PES_SET - PCR");
#endif
		}
	}

	if (changed&9)													// open decoder
		if ((parms.vpid!=-1) || (parms.apid!=-1)) 
		{
			fd.video=open(VIDEO_DEV, O_RDWR);
			if (fd.video<0)
				perror(VIDEO_DEV);
		}

	if (changed&9)													// start decoding
		if ((!parms.recordmode) && (fd.video!=-1))
		{
#ifdef DBOX
			videoStatus status;
			qDebug("VIDEO_GET_STATUS, VIDEO_SELECT_SOURCE, VIDEO_PLAY");
			if (ioctl(fd.video, VIDEO_GET_STATUS, &status)<0)
				perror("VIDEO_GET_STATUS");
			if (status.streamSource != (videoStreamSource_t)VIDEO_SOURCE_DEMUX)
				if (ioctl(fd.video, VIDEO_SELECT_SOURCE, (videoStreamSource_t)VIDEO_SOURCE_DEMUX)<0)
					perror("VIDEO_SELECT_SOURCE");
			if (ioctl(fd.video, VIDEO_PLAY, 0)<0)
				perror("VIDEO_PLAY");
#else
			qDebug("VIDEO_SELECT_SOURCE, VIDEO_PLAY");
			if (ioctl(fd.video, MPEG_VID_SELECT_SOURCE, 0)<0)
				perror("MPEG_VID_SELECT_SOURCE");
			if (ioctl(fd.video, MPEG_VID_PLAY, 0)<0)
				perror("MPEG_VID_PLAY");
#endif
		}

#ifdef DBOX
	if (changed&0x100)
	{
		int fd=open(AUDIO_DEV, O_RDWR);
		if (fd<0)
			qDebug("couldn't set audio mode (%s) - maybe old driver?", strerror(errno));
		else
		{
			qDebug(" ----------------------------- setting audiomode to %d", (parms.audio_type==DECODE_AUDIO_MPEG)?1:0);
			ioctl(fd, AUDIO_SET_BYPASS_MODE, (parms.audio_type==DECODE_AUDIO_MPEG)?1:0);
			close(fd);
		}
	}
#else
	if (changed&0x102)
	{	
		printf("closing old (%d)\n", fd.audio);
		if (fd.audio!=-1)
			close(fd.audio);
		if (parms.apid!=-1)
		{
			printf("opening new %s\n", (parms.audio_type==DECODE_AUDIO_MPEG)?AUDIO_DEV_MPEG:AUDIO_DEV_AC3);
			if (parms.audio_type==DECODE_AUDIO_MPEG)
				fd.audio=open(AUDIO_DEV_MPEG, O_RDWR);
			else
				fd.audio=open(AUDIO_DEV_AC3, O_RDWR);
			printf("it's you (%d)\n", fd.audio);
			if (fd.audio<0)
				perror((parms.audio_type==DECODE_AUDIO_MPEG)?AUDIO_DEV_MPEG:AUDIO_DEV_AC3);
			if (ioctl(fd.audio, MPEG_AUD_SELECT_SOURCE, 0)<0)
				perror("MPEG_AUD_SELECT_SOURCE");
			if (ioctl(fd.audio, MPEG_AUD_SET_STREAM_TYPE, AUD_STREAM_TYPE_PES) < 0)
				perror("MPEG_AUD_SET_STREAM_TYPE\n");
			if (ioctl(fd.audio, MPEG_AUD_PLAY, 0)<0)
				perror("MPEG_AUD_PLAY");
		}
	}
#endif

	if (changed&1)													// re-enable queues
		if ((!parms.recordmode) && parms.vpid != -1)
		{
			fd.demux_video=open(DEMUX_DEV, O_RDWR);
			qDebug("open pid %x -> video (%d)", parms.vpid, fd.demux_video);
			pes_filter.pid		 = parms.vpid;
#ifdef DBOX
			pes_filter.input	 = DMX_IN_FRONTEND;
			pes_filter.output	 = DMX_OUT_DECODER;
			pes_filter.pesType = DMX_PES_VIDEO;
			pes_filter.flags	 = 0;
			if (ioctl(fd.demux_video, DMX_SET_PES_FILTER, &pes_filter)<0)
				perror("DMX_SET_PES_FILTER - video");
#else
			pes_filter.output  = OUT_DECODER;
			pes_filter.pesType = DMX_PES_VIDEO;		// ok, es ist *KEIN* zufall :)
			if (ioctl(fd.demux_video, DEMUX_FILTER_PES_SET, &pes_filter)<0)
				perror("DMX_FILTER_PES_SET - video");
#endif
		}

	if (changed&2)
		if ((!parms.recordmode) && parms.apid != -1)
		{
			fd.demux_audio=open(DEMUX_DEV, O_RDWR);
			qDebug("open pid %x -> audio (%d)", parms.apid, fd.demux_audio);
			pes_filter.pid		 = parms.apid;
#ifdef DBOX
			pes_filter.input	 = DMX_IN_FRONTEND;
			pes_filter.output	 = DMX_OUT_DECODER;
			pes_filter.pesType = DMX_PES_AUDIO;
			pes_filter.flags	 = 0;
			if (ioctl(fd.demux_audio, DMX_SET_PES_FILTER, &pes_filter)<0)
				perror("DMX_SET_PES_FILTER - audio");
#else
			pes_filter.output  = OUT_DECODER;
			pes_filter.pesType = DMX_PES_AUDIO;
			if (ioctl(fd.demux_audio, DEMUX_FILTER_PES_SET, &pes_filter)<0)
				perror("DMX_FILTER_PES_SET - audio");
#endif
		}

#ifndef DBOX
	if (changed&3)
	{
		int syncmode=VID_SYNC_NO;
		if ((parms.apid!=-1) && (parms.vpid!=-1))
			syncmode=VID_SYNC_VID;
		ioctl(fd.video, MPEG_VID_SYNC_ON, syncmode);
	}
#endif

  if (changed&8)
  	if ((!parms.recordmode) && (parms.pcrpid!=-1))
  	{
			qDebug("start pcr");
#ifdef DBOX
			if (ioctl(fd.demux_pcr, DMX_START,0)<0)	
				perror("DMX_START");
#else
			if (ioctl(fd.demux_pcr, DEMUX_START,0)<0)
				perror("DEMUX_START");
#endif
		}

  if (changed&1)
  	if ((!parms.recordmode) && (parms.vpid!=-1))
  	{
			qDebug("start video");
#ifdef DBOX
			if (ioctl(fd.demux_video, DMX_START,0)<0)
				perror("DMX_START");
#else
			if (ioctl(fd.demux_video, DEMUX_START,0)<0)
				perror("DEMUX_START");
#endif
		}

  if (changed&2)
  	if ((!parms.recordmode) && (parms.apid!=-1))
  	{
			qDebug("start audio");
#ifdef DBOX
			if (ioctl(fd.demux_audio, DMX_START,0)<0)
				perror("DMX_START");
#else
			if (ioctl(fd.demux_audio, DEMUX_START,0)<0)
				perror("DEMUX_START");
#endif
		}

	current=parms;
	return 0;
}

void Decoder::addCADescriptor(__u8 *descriptor)
{
	memcpy(parms.descriptors+parms.descriptor_length, descriptor, descriptor[1]+2);
	parms.descriptor_length+=descriptor[1]+2;
}
