#include "eavswitch.h"
#include <unistd.h>
#include <fcntl.h>
#include <dbox/avs_core.h>
#include <sys/ioctl.h>
#include "qglobal.h"
#include "edvb.h"

/* sucks */

#define SAAIOGREG               1 /* read registers                             */
#define SAAIOSINP               2 /* input control                              */
#define SAAIOSOUT               3 /* output control                     */
#define SAAIOSENC               4 /* set encoder (pal/ntsc)             */
#define SAAIOSMODE              5 /* set mode (rgb/fbas/svideo) */

#define SAA_MODE_RGB    0
#define SAA_MODE_FBAS   1
#define SAA_MODE_SVIDEO 2

#define SAA_NTSC                0
#define SAA_PAL                 1

#define SAA_INP_MP1             1
#define SAA_INP_MP2             2
#define SAA_INP_CSYNC   4
#define SAA_INP_DEMOFF  6
#define SAA_INP_SYMP    8
#define SAA_INP_CBENB   128

eAVSwitch *eAVSwitch::instance=0;

eAVSwitch::eAVSwitch()
{
	active=0;
	if (!instance)
		instance=this;
	fd=open("/dev/dbox/avs0", O_RDWR);
	saafd=open("/dev/dbox/saa0", O_RDWR);
	reloadSettings();
}

eAVSwitch *eAVSwitch::getInstance()
{
	return instance;
}

eAVSwitch::~eAVSwitch()
{
	if (instance==this)
		instance=0;

	if (fd>=0)
		close(fd);
	if (saafd>=0)
		close(saafd);
}


void eAVSwitch::reloadSettings()
{
	unsigned int colorformat;
	eDVB::getInstance()->config.getKey("/elitedvb/video/colorformat", colorformat);
	setColorFormat((eAVColorFormat)colorformat);
}

int eAVSwitch::setVolume(int vol)
{
	vol=63-vol/(65536/64);
	if (vol<0)
		vol=0;
	if (vol>63)
		vol=63;

	return ioctl(fd, AVSIOSVOL, &vol);
}

int eAVSwitch::setTVPin8(int vol)
{
	int fnc;
	switch (vol)
	{
	case 0:
		fnc=0;
		break;
	case 6:
		fnc=1;
		break;
	case 12:
		fnc=(Type==PHILIPS?3:2);
		break;
	}
	return ioctl(fd, AVSIOSFNC, &fnc);
}

int eAVSwitch::setColorFormat(eAVColorFormat c)
{
	colorformat=c;
	int arg=0;
	switch (c)
	{
	case cfNull:
		return -1;
	case cfCVBS:
		arg=SAA_MODE_FBAS;
		break;
	case cfRGB:
		arg=SAA_MODE_RGB;
		break;
	case cfYC:
		arg=SAA_MODE_SVIDEO;
		break;
	}
	int fblk = (c == cfRGB)?1:0;
	ioctl(saafd, SAAIOSMODE, &arg);
	ioctl(fd, AVSIOSFBLK, &fblk);
}

int eAVSwitch::setInput(int v)
{	
	qDebug("setInput %d, fd=%d", v, fd);
	switch (v)
	{
	case 0:
		v = (Type == SAGEM)?0:((colorformat == cfRGB)?1:2);
		ioctl(fd, AVSIOSFBLK, &v);
		ioctl(fd, AVSIOSVSW1, dvb);
		ioctl(fd, AVSIOSASW1, dvb+1);
		ioctl(fd, AVSIOSVSW2, dvb+2);
		ioctl(fd, AVSIOSASW2, dvb+3);
		ioctl(fd, AVSIOSVSW3, dvb+4);
		ioctl(fd, AVSIOSASW3, dvb+5);
		if (!eDVB::getInstance()->mute)
			ioctl(fd, AVSIOSVOL, &eDVB::getInstance()->volume);
		break;
	case 1:
		v = (colorformat == cfRGB)?1:0;
		ioctl(fd, AVSIOSFBLK, &v);
		ioctl(fd, AVSIOSVSW1, scart);
		ioctl(fd, AVSIOSASW1, scart+1);
		ioctl(fd, AVSIOSVSW2, scart+2);
		ioctl(fd, AVSIOSASW2, scart+3);
		ioctl(fd, AVSIOSVSW3, scart+4);
		ioctl(fd, AVSIOSASW3, scart+5);
		v = 0;  // full Volume
		ioctl(fd, AVSIOSVOL, &v);
		break;
	}
	return 0;
}

int eAVSwitch::setAspectRatio(eAVAspectRatio as)
{
	aspect=as;
	return setTVPin8(active?((aspect==r169)?6:12):0);
}

int eAVSwitch::isVCRActive()
{
	return 0;
}

int eAVSwitch::setActive(int a)
{
	active=a;
	return setTVPin8(active?((aspect==r169)?6:12):0);
}
