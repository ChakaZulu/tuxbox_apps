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
Revision 1.11  2002/05/18 02:55:24  TheDOC
LCARS 0.21TP7

Revision 1.10  2002/03/03 22:56:27  TheDOC
lcars 0.20

Revision 1.9  2001/12/20 19:20:49  obi
defined OLD_TUNER_API in Makefile.am instead of tuner.cpp

Revision 1.8  2001/12/17 01:30:02  obi
use /dev/ost/frontend0 for new tuner api.
code for the new tuner api is still disabled by default.

Revision 1.7  2001/12/16 16:09:16  rasc
new dvb tuner API  FE_SET_FRONTEND.
Not tested (drivers have to be changed first), only compile test!

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

#include "tuner.h"

tuner::tuner(settings *s)
{
	setting = s;
}

CodeRate tuner::getFEC(int fec)
{
	switch (fec)
	{
	case -1:
	case 15:
		return FEC_NONE;
	case 0:
		return FEC_AUTO;
	case 1:
		return FEC_1_2;
	case 2:
		return FEC_2_3;
	case 3:
		return FEC_3_4;
	case 4:
		return FEC_5_6;
	case 5:
		return FEC_7_8;
	default:
		return FEC_AUTO;
	}
}

// -- New Tuning API
// -- 2001-12-16 rasc
// polarization = 0 -> H, polarization = 1 -> V
int tuner::tune(int frequ, int symbol, int polarization, int fec, int dis)
{
	int device;
	int frontend;
	struct secCmdSequence seq;
	struct secCommand cmd;
	FrontendParameters frontp;
	int i, status;
	long state1,state2;

	if (setting->boxIsSat())
	{

// $$$ rasc
// Das Verhalten von Sectone (22KHz) sollte konfigurierbar sein.
// Ebenso die ZF fuer die LNBs (1 + 2) fuer jeweils Hi und Lo - Band
// die Werte hier sind Standard fuer das Ku-Band, allerdings waere es
// interessant auch andere Werte zu haben (z.B. 10 GHz ZF, oder 4 GHz ZF)
// Dies ist sinnvoll, wenn man die dbox fuer den Sat-DX Empfang, oder
// aeltere LNBs (naja) nutzen moechte.

		if (frequ > 11700)
		{
			frontp.Frequency = (frequ * 1000)-10600000;
			seq.continuousTone = SEC_TONE_ON;
		}
		else
		{
			frontp.Frequency = (frequ * 1000)-9750000;
			seq.continuousTone = SEC_TONE_OFF;
		}

		if (polarization == 0)
		{
			seq.voltage=SEC_VOLTAGE_18;
		}
		else
		{
			seq.voltage=SEC_VOLTAGE_13;	
		}

		frontp.u.qpsk.FEC_inner = getFEC(fec);

		// -- symbol rate (SAT)
		frontp.u.qpsk.SymbolRate = symbol * 1000;

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
			perror("OPEN SEC DEVICE");
			exit(1);
		}

		if (ioctl(device,SEC_SEND_SEQUENCE,&seq) < 0)
		{
			perror("SEC_SEND_SEQUENCE");
		}

		close(device);
	}

	if (setting->boxIsCable())
	{
		frontp.Frequency = frequ * 100;
		frontp.u.qam.SymbolRate = symbol * 1000;
		frontp.u.qam.FEC_inner = getFEC(fec);
		// frontp.u.qam.FEC_outer = FEC_AUTO;
		frontp.u.qam.QAM = QAM_64;
	}

	// -- Spektrum Inversion
	// -- should be configurable, fixed for now (rasc)
	if (setting->getInversion() == INVERSION_ON)
	{
		frontp.Inversion = INVERSION_ON;
	}
	else if (setting->getInversion() == INVERSION_OFF)
	{
		frontp.Inversion = INVERSION_OFF;
	}
	else if (setting->getInversion() == INVERSION_AUTO)
	{
		frontp.Inversion = INVERSION_AUTO;
	}
	if ((frontend = open("/dev/ost/frontend0", O_RDWR)) < 0)
	{
		perror("OPEN FRONTEND DEVICE");
		exit(1);
	}
	
	if (ioctl(frontend, FE_SET_FRONTEND, &frontp) < 0)
	{
		perror("FE_SET_FRONTEND");
	}

	if (ioctl(frontend, FE_READ_STATUS, &status) < 0)
	{
		perror("FE_READ_STATUS");
	}

#ifdef DEBUG
	printf (" Frequ: %ld   ifreq: %ld  Pol: %d  FEC: %d  Sym: %ld  dis: %d  (param: 0x%02x)\n",
		(long)frequ,(long)frontp.Frequency,(int)polarization ,(int)fec,
		(long)symbol, (int)dis,(int)cmd.u.diseqc.params[0]);

	printf ("... Tuner-Lock Status: %ld\n",status);

	if (ioctl(frontend, FE_READ_SNR, &state1) < 0)
	{
		perror("FE_READ_SNR");
	}

	if (ioctl(frontend, FE_READ_SIGNAL_STRENGTH, &state2) < 0)
	{
		perror("FE_READ_SIGNAL_STRENGTH");
	}

	printf ("... S/N: %ld  SigStrength: %ld \n",state1,state2);
#endif

	close(frontend);

	return (status & FE_HAS_SIGNAL);
}

