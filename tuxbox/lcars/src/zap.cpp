/***************************************************************************
    copyright            : (C) 2001 by TheDOC
    email                : thedoc@chatville.de
	homepage			 : www.chatville.de
	modified by			 : -
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/*
$Log: zap.cpp,v $
Revision 1.6  2002/06/08 20:21:09  TheDOC
adding the cam-sources with slight interface-changes

Revision 1.5  2002/06/02 12:18:47  TheDOC
source reformatted, linkage-pids correct, xmlrpc removed, all debug-messages removed - 110k smaller lcars with -Os :)

Revision 1.4  2002/05/18 02:55:24  TheDOC
LCARS 0.21TP7

Revision 1.3  2002/03/03 22:56:27  TheDOC
lcars 0.20

Revision 1.2  2001/12/16 22:36:05  tux
IP Eingaben erweitert

Revision 1.2  2001/11/15 00:43:45  TheDOC
 added

*/
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <memory.h>
#include <ost/dmx.h>
#include <ost/video.h>
#include <ost/frontend.h>
#include <ost/audio.h>
#include <ost/sec.h>
#include <ost/sec.h>
#include <ost/ca.h>
#include <dbox/avs_core.h>
#include <dbox/fp.h>

#include <set>
#include <algorithm>
#include <string>

#include "zap.h"
#include "help.h"
#include "settings.h"
#include "nit.h"
#include "sdt.h"
#include "osd.h"
#include "tuner.h"
#include "pat.h"
#include "pmt.h"

#define BSIZE 10000

zap::zap(settings &set, osd &o, tuner &t, cam &c) : setting(set), osdd(o), tune(t), ca(c)
{
	//printf("Initializing zapper...\n");

	vid = open("/dev/ost/video0", O_RDWR);
	if((video = open("/dev/ost/demux0", O_RDWR)) < 0) {
		//printf("Cannot open demux device \n");
		exit(1);
	}

	if((audio = open("/dev/ost/demux0", O_RDWR)) < 0) {
		//printf("Cannot open demux device\n");
		exit(1);
	}
	aud = open("/dev/ost/audio0", O_RDWR);
	ioctl(vid, VIDEO_SELECT_SOURCE, (videoStreamSource_t)VIDEO_SOURCE_DEMUX);
	old_frequ = 0;
	old_TS = -1;
}

zap::~zap()
{
	close(vid);
	close(video);
	close(audio);
	close(frontend);
}


void zap::close_dev()
{
	close(vid);
	close(video);
	close(audio);

}

void zap::zap_allstop()
{
	ioctl(video,DMX_STOP, true);
	ioctl(audio,DMX_STOP, true);
	ioctl(pcr,DMX_STOP, true);
	ioctl(vid, VIDEO_STOP, true);
	ioctl(aud, AUDIO_STOP, true);
}

void zap::zap_to(pmt_data pmt, int VPID, int APID, int PCR, int ECM, int SID, int ONID, int TS, int PID1, int PID2)
{
	zap_allstop();

	close(vid);
	close(aud);
	close(video);
	close(audio);
	close(pcr);

	vid = open("/dev/ost/video0", O_RDWR);
	if (vid < 0)
		perror("/dev/ost/video0");

	if((video = open("/dev/ost/demux0", O_RDWR)) < 0) {
		perror("/dev/ost/demux0");
		exit(1);
	}

	if((pcr = open("/dev/ost/demux0", O_RDWR)) < 0) {
		perror("/dev/ost/demux0");
		exit(1);
	}

	aud = open("/dev/ost/audio0", O_RDWR);
	if (aud < 0)
		perror("/dev/ost/audio0");

	if((audio = open("/dev/ost/demux0", O_RDWR)) < 0) {
		perror("/dev/ost/demux0");
		exit(1);
	}
	struct dmxPesFilterParams pes_filter;

	if (VPID == 0)
		VPID = 0x1fff;

	//printf("Zappe auf\nSID: %04x\nVPID: %04x\nAPID: %04x\nECM: %04x\nONID: %04x\n\n", SID, VPID, APID, ECM, ONID);

	bool usevideo = false, useaudio = false, usepcr = false;
	//ioctl(audio,AUDIO_SET_BYPASS_MODE, 0);
	if ((VPID >= 0x20) && (VPID <= 0x1FFB))
	{
		/* vpid */
		pes_filter.pid     = VPID;
		pes_filter.input   = DMX_IN_FRONTEND;
		pes_filter.output  = DMX_OUT_DECODER;
		pes_filter.pesType = DMX_PES_VIDEO;
		pes_filter.flags   = 0;
		ioctl(video,DMX_SET_PES_FILTER,&pes_filter);
		usevideo = true;
	}

	/* apid */
	if ((APID >= 0x20) && (APID <= 0x1FFB))
	{
		pes_filter.pid     = APID;
		pes_filter.input   = DMX_IN_FRONTEND;
		pes_filter.output  = DMX_OUT_DECODER;
		pes_filter.pesType = DMX_PES_AUDIO;
		pes_filter.flags   = 0;
		ioctl(audio,DMX_SET_PES_FILTER,&pes_filter);
		useaudio = true;
	}

	/* apid */
	if ((PCR >= 0x20) && (PCR <= 0x1FFB))
	{
		pes_filter.pid     = PCR;
		pes_filter.input   = DMX_IN_FRONTEND;
		pes_filter.output  = DMX_OUT_DECODER;
		pes_filter.pesType = DMX_PES_PCR;
		pes_filter.flags   = 0;
		ioctl(pcr,DMX_SET_PES_FILTER,&pes_filter);
		usepcr = true;
	}

	if (usevideo)
		ioctl(vid, VIDEO_PLAY);

	if (useaudio)
		ioctl(aud, AUDIO_PLAY);

	if (usepcr)
		ioctl(pcr, DMX_START);

	if (useaudio)
		ioctl(audio,DMX_START);
	if (usevideo)
		ioctl(video,DMX_START);


	//ioctl(audio,AUDIO_SET_BYPASS_MODE, 1);

	//printf("Zapping...\n");
	if (ECM != 0)
	{
		ca.initialize();
		if (VPID != 0x1fff)
		{
			ca.addPID(VPID);
		}
		ca.addPID(APID);
		ca.setECM(ECM);
		ca.setSID(SID);
		ca.setONID(ONID);
		ca.setPMTentry(pmt);
		if (PID1 != -1)
		{
			ca.addPID(PID1);
		}
		if (PID2 != -1)
			ca.addPID(PID2);

		if (old_TS != TS)
		{
			ca.reset();
			ca.setEMM(setting.getEMMpid());
			ca.startEMM();
		}
		ca.descramble();
	}

	old_TS = TS;

}

void zap::zap_audio(int VPID, int APID, int ECM, int SID, int ONID)
{
	struct dmxPesFilterParams pes_filter;

	int i = AVS_MUTE;
	int avs = open("/dev/dbox/avs0",O_RDWR);
	ioctl(avs,AVSIOSMUTE,&i);
	close(avs);
	ioctl(aud, AUDIO_STOP, true);
	ioctl(audio, DMX_STOP, true);

	//printf("Zappe auf\nSID: %04x\nVPID: %04x\nAPID: %04x\nECM: %04x\nONID: %04x\n\n", SID, VPID, APID, ECM, ONID);

	/* apid */
	pes_filter.pid     = APID;
	pes_filter.input   = DMX_IN_FRONTEND;
	pes_filter.output  = DMX_OUT_DECODER;
	pes_filter.pesType = DMX_PES_AUDIO;
	pes_filter.flags   = 0;
	ioctl(audio, DMX_SET_PES_FILTER, &pes_filter);

	ioctl(audio, DMX_START, true);
	ioctl(aud, AUDIO_PLAY, true);
	usleep(300000);
	i = AVS_UNMUTE;
	avs = open("/dev/dbox/avs0",O_RDWR);
	ioctl(avs,AVSIOSMUTE,&i);
	close(avs);
}


void zap::dmx_start()
{
}

void zap::dmx_stop()
{
	ioctl(video,DMX_STOP,0);
	ioctl(audio,DMX_STOP,0);
	ioctl(vid, VIDEO_STOP);
}
