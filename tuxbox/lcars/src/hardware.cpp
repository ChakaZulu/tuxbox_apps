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
$Log: hardware.cpp,v $
Revision 1.6  2002/06/02 12:18:47  TheDOC
source reformatted, linkage-pids correct, xmlrpc removed, all debug-messages removed - 110k smaller lcars with -Os :)

Revision 1.5  2002/05/18 02:55:24  TheDOC
LCARS 0.21TP7

Revision 1.4  2002/03/03 22:56:27  TheDOC
lcars 0.20

Revision 1.4  2001/12/18 02:03:29  tux
VCR-Switch-Eventkram implementiert

Revision 1.3  2001/12/17 16:54:47  tux
Settings halb komplett

Revision 1.2  2001/12/16 22:36:05  tux
IP Eingaben erweitert

Revision 1.3  2001/12/11 13:38:44  TheDOC
new cdk-path-variables, about 10 new features and stuff

Revision 1.2  2001/11/15 00:43:45  TheDOC
 added

*/
#include "hardware.h"

hardware::hardware(settings *s, variables *v)
{
	setting = s;
	vars = v;
	vcr_on = false;
	old_DD_state = true;
	old_fblk = OUTPUT_RGB;
}

void hardware::fnc(int i)
{
	avs = open("/dev/dbox/avs0",O_RDWR);
	ioctl(avs, AVSIOSFNC, &i);

	close(avs);
}

int hardware::getAvsStatus()
{
	avs = open("/dev/dbox/avs0",O_RDWR);
	int val;
	ioctl(avs, AVSIOGSTATUS, &val);

	close(avs);
	return val;
}

int hardware::getVCRStatus()
{
	int val;
	int fp = open("/dev/dbox/fp0",O_RDWR);

	ioctl(fp, FP_IOCTL_GET_VCR, &val);

	close(fp);
	//std::cout << "VCR-Val: " << val << std::endl;
	return val;
}

int hardware::getARatio()
{
	int check = 0;
	char buffer[100];
	FILE* fp = fopen("/proc/bus/bitstream", "r");
	while (!feof(fp))
	{
		fgets(buffer, 100, fp);
		sscanf(buffer, "A_RATIO: %d", &check);
	}
	fclose(fp);
	return check;
}

bool hardware::switch_vcr()
{
	int i = 0, j = 0, nothing, nothinga, fblk = 2;

	//printf("Getbox: %d\n", setting->getBox());
	if (!vcr_on)
	{
		//printf("on\n");
		old_fblk = getfblk();
		avs = open("/dev/dbox/avs0",O_RDWR);
		if (setting->getBox() == SAGEM) // Sagem
		{
			i = 2;
			j = 1;
			nothing = 7;
			ioctl(avs,AVSIOSFBLK,&fblk);
			ioctl(avs,AVSIOSVSW2,&nothing);
			ioctl(avs,AVSIOSVSW1,&i);
			ioctl(avs,AVSIOSASW1,&j);
			ioctl(avs,AVSIOSASW2,&nothinga);
		}
		else if (setting->getBox() == NOKIA) // Nokia
		{
			i = 3;
			j = 2;
			nothing = 1;
			nothinga = 2;
			ioctl(avs,AVSIOSFBLK,&fblk);
			ioctl(avs,AVSIOSVSW2,&nothing);
			ioctl(avs,AVSIOSVSW1,&i);
			ioctl(avs,AVSIOSASW1,&j);
			ioctl(avs,AVSIOSASW2,&nothinga);
		}
		else if (setting->getBox() == PHILIPS) // Philips
		{
			nothing = 3;

			i = 2;
			j = 2;
			ioctl(avs,AVSIOSFBLK,&fblk);
			ioctl(avs,AVSIOSVSW3,&nothing);
			ioctl(avs,AVSIOSVSW2,&i);
			ioctl(avs,AVSIOSASW2,&j);
		}
	}
	else
	{
		setOutputMode(old_fblk);
		avs = open("/dev/dbox/avs0",O_RDWR);
		if (setting->getBox() == SAGEM)
		{
			i = 0;
			j = 0;
			nothing = 0;

			//ioctl(avs,AVSIOSFBLK,&fblk);
			ioctl(avs,AVSIOSVSW2,&nothing);
			ioctl(avs,AVSIOSVSW1,&i);
			ioctl(avs,AVSIOSASW1,&j);

		}
		else if (setting->getBox() == NOKIA)
		{
			i = 5;
			j = 1;
			nothing = 1;
			nothinga = 2;
			//ioctl(avs,AVSIOSFBLK,&fblk);
			ioctl(avs,AVSIOSVSW2,&nothing);
			ioctl(avs,AVSIOSVSW1,&i);
			ioctl(avs,AVSIOSASW1,&j);
			ioctl(avs,AVSIOSASW2,&nothinga);
		}
		else if (setting->getBox() == PHILIPS)
		{
			i = 1;
			j = 1;
			nothing = 1;

			//ioctl(avs,AVSIOSFBLK,&fblk);
			ioctl(avs,AVSIOSVSW3,&nothing);
			ioctl(avs,AVSIOSVSW2,&i);
			ioctl(avs,AVSIOSASW2,&j);
		}
	}
	//printf ("i: %d - j: %d\n", i, j);


	vcr_on = !vcr_on;
	close(avs);
	return vcr_on;

}

void hardware::switch_mute()
{
	int i;

	if (muted)
	{
		i = AVS_UNMUTE;
		vars->setvalue("%IS_MUTED", "false");
	}
	else
	{
		i = AVS_MUTE;
		vars->setvalue("%IS_MUTED", "true");
	}
	avs = open("/dev/dbox/avs0",O_RDWR);
	ioctl(avs, AVSIOSMUTE, &i);
	close(avs);
	muted = !muted;


}

int hardware::vol_minus(int value)
{
	int i;
	avs = open("/dev/dbox/avs0",O_RDWR);
	ioctl(avs, AVSIOGVOL, &i);
	if (i < 63)
		i += value;
	if (i > 63)
		i = 63;
	ioctl(avs, AVSIOSVOL, &i);
	close(avs);
	vars->setvalue("%VOLUME", 63 - i);
	return i;
}

int hardware::vol_plus(int value)
{
	int i;
	avs = open("/dev/dbox/avs0",O_RDWR);
	ioctl(avs, AVSIOGVOL, &i);
	if (i > 0)
		i -= value;
	if (i < 0)
		i = 0;
	ioctl(avs, AVSIOSVOL, &i);
	close(avs);
	vars->setvalue("%VOLUME", 63 - i);
	return i;
}

void hardware::setOutputMode(int i)
{
	int setmode = 0;

	if (setting->getBox() == NOKIA)
	{
		if (i == OUTPUT_FBAS)
			setmode = 0;
		else
			setmode = 3;
	}
	else if (setting->getBox() == PHILIPS)
	{
		if (i == OUTPUT_FBAS)
			setmode = 0;
		else
			setmode = 1;
	}
	else if (setting->getBox() == SAGEM)
	{
	}

	setfblk(setmode);
}

void hardware::setfblk(int i)
{
	avs = open("/dev/dbox/avs0",O_RDWR);
	fblk = i;
	ioctl(avs,AVSIOSFBLK,&fblk);
	close(avs);
}

int hardware::getfblk()
{
	avs = open("/dev/dbox/avs0",O_RDWR);
	if (ioctl(avs, AVSIOGFBLK, &fblk)< 0) {
		perror("AVSIOGFBLK:");
		exit(0);
	}
	close(avs);

	int outputtype = 0;
	if (setting->getBox() == NOKIA)
	{
		if (fblk == 3)
			outputtype = OUTPUT_RGB;
		else if (fblk == 0)
			outputtype = OUTPUT_FBAS;
	}
	else if (setting->getBox() == PHILIPS)
	{
		if (fblk == 1)
			outputtype = OUTPUT_RGB;
		else if (fblk == 0)
			outputtype = OUTPUT_FBAS;

	}
	else if (setting->getBox() == SAGEM)
	{

	}
	return outputtype;
}


void hardware::shutdown()
{
	system("/sbin/halt &");
}

void hardware::reboot()
{
	system("/sbin/reboot &");
}

void hardware::useDD(bool use)
{
	if (old_DD_state == use)
		return;
	int fd = open("/dev/ost/audio0", O_RDWR);
	if (use)
	{
		ioctl(fd, AUDIO_SET_BYPASS_MODE, 0);
	}
	else
	{
		ioctl(fd, AUDIO_SET_BYPASS_MODE, 1);
	}
	close(fd);
	old_DD_state = use;
}


