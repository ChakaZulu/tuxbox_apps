#define FRONT_DEV "/dev/ost/qpskfe0"
#define DEMUX_DEV "/dev/ost/demux0"
#define SEC_DEV	 "/dev/ost/sec0"
#define VIDEO_DEV	 "/dev/ost/video0"
#define AUDIO_DEV "/dev/ost/audio0"

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

#include <ost/dmx.h>
#include <ost/frontend.h>
#include <ost/sec.h>
#include <ost/video.h>
#include <ost/ca.h>
#include <ost/audio.h>

#include "decoder.h"

decoderParameters Decoder::current;
decoderParameters Decoder::parms;
int Decoder::fd::video;
int Decoder::fd::demux_video;
int Decoder::fd::demux_audio;
#define USE_CAMD

static void SetECM(int vpid, int apid, int ecmpid, int emmpid, int pmtpid, int casystemid, int descriptor_length, __u8 *descriptors)
{
	static int lastpid=-1;

	if (lastpid!=-1)
	{
		kill(lastpid, SIGKILL);
		waitpid(lastpid, 0, 0);
		lastpid=-1;
	}

	if (emmpid==-2)		// big evil hack
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
#if 0
		close(0);
		close(1);
		close(2);
#endif
#ifdef USE_CAMD
		printf("%s\n", descriptor);
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
}

int Decoder::Initialize()
{
	parms.vpid=
	parms.apid=
	parms.tpid=
	parms.pcrpid=
	parms.ecmpid=
	parms.emmpid=
	parms.audio_type=-1;
	parms.emmpid=-2;
	parms.recordmode=0;
	parms.descriptor_length=0;
	current=parms;
	fd.video=
	fd.demux_video=
	fd.demux_audio=-1;
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
	parms.ecmpid=
//	parms.emmpid=
	parms.audio_type=-1;
	parms.descriptor_length=0;
	parms.emmpid=-2;
	parms.recordmode=0;
	Set(0);
}

int Decoder::Set(int useAC3)
{
	int changed=0;
	struct dmxPesFilterParams pes_filter;
	 
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
			qDebug("VIDEO_STOP");
			if (ioctl(fd.video, VIDEO_STOP, 1)<0)
				perror("VIDEO_STOP");
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
			qDebug("VIDEO_SELECT_SOURCE, VIDEO_PLAY");
			if (ioctl(fd.video, VIDEO_SELECT_SOURCE, (videoStreamSource_t)VIDEO_SOURCE_DEMUX)<0)
				perror("VIDEO_SELECT_SOURCE");

			if (ioctl(fd.video, VIDEO_PLAY, 0)<0)
				perror("VIDEO_PLAY");
		}

	if (changed&0x100 && useAC3)
	{
		int fd=open(AUDIO_DEV, O_RDWR);
		if (fd<0)
			qDebug("couldn't set audio mode (%s) - maybe old driver?", strerror(errno));
		else
		{
			qDebug("setting audiomode to %d", (parms.audio_type==DECODE_AUDIO_MPEG)?1:0);
			ioctl(fd, AUDIO_SET_BYPASS_MODE, (parms.audio_type==DECODE_AUDIO_MPEG)?1:0);
			close(fd);
		}
	}

	if (changed&1)													// re-enable queues
		if ((!parms.recordmode) && parms.vpid != -1)
		{
			fd.demux_video=open(DEMUX_DEV, O_RDWR);
			qDebug("open pid %x -> video (%d)", parms.vpid, fd.demux_video);
			pes_filter.pid		 = parms.vpid;
			pes_filter.input	 = DMX_IN_FRONTEND;
			pes_filter.output	 = DMX_OUT_DECODER;
			pes_filter.pesType = DMX_PES_VIDEO;
			pes_filter.flags	 = 0;
			if (ioctl(fd.demux_video, DMX_SET_PES_FILTER, &pes_filter)<0)
				perror("DMX_SET_PES_FILTER - video");
		}

	if (changed&2)
		if ((!parms.recordmode) && parms.apid != -1)
		{
			fd.demux_audio=open(DEMUX_DEV, O_RDWR);
			qDebug("open pid %x -> audio (%d)", parms.apid, fd.demux_audio);
			pes_filter.pid		 = parms.apid;
			pes_filter.input	 = DMX_IN_FRONTEND;
			pes_filter.output	 = DMX_OUT_DECODER;
			pes_filter.pesType = DMX_PES_AUDIO;
			pes_filter.flags	 = 0;
			if (ioctl(fd.demux_audio, DMX_SET_PES_FILTER, &pes_filter)<0)
				perror("DMX_SET_PES_FILTER - audio");
		}

  if (changed&1)
  	if ((!parms.recordmode) && (parms.vpid!=-1))
  	{
			qDebug("start video");
			if (ioctl(fd.demux_video, DMX_START,0)<0)	
				perror("DMX_START");
		}

  if (changed&2)
  	if ((!parms.recordmode) && (parms.apid!=-1))
  	{
			qDebug("start audio");
			if (ioctl(fd.demux_audio, DMX_START,0)<0)
				perror("DMX_START");
		}

	current=parms;
	return 0;
}

void Decoder::addCADescriptor(__u8 *descriptor)
{
	printf("adding ca descriptor, length now %d\n", parms.descriptor_length);
	memcpy(parms.descriptors+parms.descriptor_length, descriptor, descriptor[1]+2);
	parms.descriptor_length+=descriptor[1]+2;
}
