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

$Log: tuner.cpp,v $
Revision 1.6  2001/12/11 13:38:44  TheDOC
new cdk-path-variables, about 10 new features and stuff

Revision 1.5  2001/12/07 19:36:10  rasc
fix to diseqc Step 2.

Revision 1.4  2001/12/07 14:12:51  rasc
minor fix.

Revision 1.3  2001/12/07 14:10:33  rasc
Fixes for SAT tuning and Diseqc. Diseqc doesn't work properly for me (diseqc 2.0 switch).
Someone should check this please..

Revision 1.2  2001/11/15 00:43:45  TheDOC
 added

*/
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
int tuner::tune(int frequ, int symbol, int polarization = -1, int fec = -1, int dis = 0)
{
	int device;
	int frontend;
	struct secCmdSequence seq;
	struct secCommand cmd;
	struct qpskParameters front;


	if (setting.boxIsSat())
	{

// $$$ rasc
// Das Verhalten von Sectone (22KHz) sollte konfigurierbar sein.
// Ebenso die ZF fuer die LNBs (1 + 2) fuer jeweils Hi und Lo - Band
// die Werte hier sind Standard fuer das Ku-Band, allerdings waere es
// interessant auch andere Werte zu haben (z.B. 10 GHz ZF, oder 4 GHz ZF)
// Dies ist sinnvoll, wenn man die dbox fuer den Sat-DX Empfang, oder
// aeltere LNBs (naja) nutzen moechte.  

		if (frequ > 11700) {
			front.iFrequency = (frequ * 1000)-10600000;
			seq.continuousTone = SEC_TONE_ON;
		} else {
			front.iFrequency = (frequ * 1000)-9750000;
			seq.continuousTone = SEC_TONE_OFF;
		}

		if (polarization == 0) {
			seq.voltage=SEC_VOLTAGE_18;
		} else {
			seq.voltage=SEC_VOLTAGE_13;	
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


		// diseqc Group Byte  (dis: 0..3)
		// 2001-12-06 rasc

		cmd.type=SEC_CMDTYPE_DISEQC;
		cmd.u.diseqc.addr=0x10;
		cmd.u.diseqc.cmd=0x38;
		cmd.u.diseqc.numParams=1;
		cmd.u.diseqc.params[0]=0xF0 
				| ((dis*4) & 0x0F) 
				| ((seq.voltage == SEC_VOLTAGE_18)     ? 2 : 0)
				| ((seq.continuousTone == SEC_TONE_ON) ? 1 : 0);

		seq.miniCommand=SEC_MINI_NONE;
		seq.numCommands=1;
		seq.commands=&cmd;



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
		// normaly  FE_HAS_LOCK|FE_HAS_SIGNAL    (rasc)
		if (status & (FE_HAS_SIGNAL))
			break;
		usleep(100);
	}

// $$$ rasc: Debug
	printf (" Frequ: %ld   ifreq: %ld  Pol: %d  FEC: %d  Sym: %ld  dis: %d  (param: 0x%02x)\n",
	(long)frequ,(long)front.iFrequency,(int)polarization ,(int)fec,
	(long)symbol, (int)dis,(int)cmd.u.diseqc.params[0]);


	printf ("... Tuner-Lock Status: %ld\n",status);
	long state1,state2;
	ioctl(frontend, FE_READ_SNR, &state1); 
	ioctl(frontend, FE_READ_SIGNAL_STRENGTH, &state2);    
	printf ("... S/N: %ld  SigStrength: %ld \n",state1,state2);
// $$$

	close(frontend);
	return (status & (FE_HAS_SIGNAL));
}
