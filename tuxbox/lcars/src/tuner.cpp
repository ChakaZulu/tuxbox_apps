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

#include <iostream.h>
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

#include "settings.h"
#include "tuner.h"

tuner::tuner(settings &s) : setting(s)
{
}

// polarization = 0 -> H, polarization = 1 -> V
int tuner::tune(int frequ, int symbol, int polarization = -1, int fec = -1, int dis = 1)
{
	int device;
	int frontend;
	struct secCmdSequence seq;
	struct secCommand cmd;
	struct secDiseqcCmd diseqc;
	struct qpskParameters front;
	unsigned char bits;

	if (setting.boxIsSat())
	{
		bits |= 0;
		diseqc.addr=0x10;
		diseqc.cmd=0x38;
		diseqc.numParams=1;
		if (dis == 1)
			diseqc.params[0]=0xF7;
		else if (dis == 2)
			diseqc.params[0]=0xF1;

		cmd.type=SEC_CMDTYPE_DISEQC;
		seq.continuousTone = SEC_TONE_ON;
		if (polarization == 0)
		{
			seq.voltage=SEC_VOLTAGE_18;
			bits|=2;
		}
		else
		{
			seq.voltage=SEC_VOLTAGE_13;	
			bits|=0;
		}
		switch (fec)
		{
			case 1:
				front.FEC_inner = FEC_1_2;
				break;
			case 2:
				front.FEC_inner = FEC_2_3;
				break;
			case 3:
				front.FEC_inner = FEC_3_4;
				break;
			case 4:
				front.FEC_inner = FEC_5_6;
				break;
			case 5:
				front.FEC_inner = FEC_7_8;
				break;
			case -1:
				front.FEC_inner = FEC_AUTO;
				break;
		}
		cmd.u.diseqc=diseqc;
		seq.miniCommand=SEC_MINI_NONE;
		seq.numCommands=1;
		seq.commands=&cmd;
		front.iFrequency = (frequ * 1000)-10600000;
		seq.continuousTone = SEC_TONE_ON;
		bits|=1;
	    
		if((device = open("/dev/ost/sec0", O_RDWR)) < 0)
		{
			exit(1);
		}
		ioctl(device,SEC_SEND_SEQUENCE,&seq);
	
		close(device);

	}
	
	if (setting.boxIsCable())
	{	
		front.iFrequency = frequ * 100;
		front.FEC_inner = 0;
	}
	
	front.SymbolRate = symbol * 1000;
	
	frontend = open("/dev/ost/qpskfe0", O_RDWR);
	ioctl(frontend, QPSK_TUNE, &front);

	int i, status;

	for (i = 0; i < 200; i++)
	{
		
		ioctl(frontend, FE_READ_STATUS, &status);
		if (status & 2)
			break;
		usleep(100);
	}
	close(frontend);
	return (i != 200);
}
