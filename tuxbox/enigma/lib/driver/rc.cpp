#include <sys/ioctl.h>
#include <asm/types.h>
#include <dbox/fp.h>
#include <qdatetime.h>
#include "rc.h"

eRCInput *eRCInput::instance;

eRCInput::eRCInput(): timeout (this)
{
	rc.setName("/dev/dbox/rc0");
	if (!rc.open(IO_ReadOnly))
		qDebug("failed to open /dev/dbox/rc0!");
	ioctl(rc.handle(), RC_IOCTL_BCODES, 1);
	sn=new QSocketNotifier(rc.handle(), QSocketNotifier::Read, this);
	connect(sn, SIGNAL(activated(int)), SLOT(keyPressed(int)));
	connect(&timeout, SIGNAL(timeout()), SLOT(timeOut()));
	connect(&repeattimer, SIGNAL(timeout()), SLOT(repeat()));
	ccode=-1;
	locked=0;
	instance=this;
}

eRCInput::~eRCInput()
{
	instance=0;
	delete sn;
	rc.close();
}

int eRCInput::translate(int code)
{
	if ((code&0xFF00)==0x5C00)
	{
		switch (code&0xFF)
		{
		case 0x0C: return RC_STANDBY;
		case 0x20: return RC_HOME;
		case 0x27: return RC_DBOX;
		case 0x00: return RC_0;
		case 0x01: return RC_1;
		case 0x02: return RC_2;
		case 0x03: return RC_3;
		case 0x04: return RC_4;
		case 0x05: return RC_5;
		case 0x06: return RC_6;
		case 0x07: return RC_7;
		case 0x08: return RC_8;
		case 0x09: return RC_9;
		case 0x3B: return RC_BLUE;
		case 0x52: return RC_YELLOW;
		case 0x55: return RC_GREEN;
		case 0x2D: return RC_RED;
		case 0x54: return RC_UP_LEFT;
		case 0x53: return RC_DOWN_LEFT;
		case 0x0E: return RC_UP;
 		case 0x0F: return RC_DOWN;
		case 0x2F: return RC_LEFT;
 		case 0x2E: return RC_RIGHT;
		case 0x30: return RC_OK;
 		case 0x16: return RC_PLUS;
 		case 0x17: return RC_MINUS;
 		case 0x28: return RC_MUTE;
 		case 0x82: return RC_HELP;
		default:
			qDebug("unknown old rc code %x", code);
			return -1;
		}
	} else if (!(code&0x00))
		return code&0x3F;
	else
		qDebug("unknown rc code %x", code);
	return -1;
}

int eRCInput::keyPressed(int)
{
	__u16 rccode;
	rc.readBlock((char*)&rccode, 2);
	if (rccode==0x5CFE)		// old break code
	{
		timeout.stop();
		repeattimer.stop();
		if (ccode!=-1)
		{
			int old=ccode;
			ccode=-1;
			emit keyUp(translate(old));
		}
	} else // if (rccode!=ccode)
	{
		timeout.start(300, 1);
		int old=ccode;
		ccode=rccode;
		if ((old!=-1) && (old!=rccode))
			emit keyUp(translate(old));
		if (old != rccode)
		{
			repeattimer.start(rdelay, 1);
			emit keyDown(translate(rccode));
		}
	}
	return 0;
}

int eRCInput::timeOut()
{
	int oldcc=ccode;
	ccode=-1;
	repeattimer.stop();
	if (oldcc!=-1)
		emit keyUp(translate(oldcc));
	return 0;
}

int eRCInput::repeat()
{
	if (ccode!=-1)
		emit keyDown(translate(ccode));
	repeattimer.start(rrate, 1);
	return 0;
}

int eRCInput::lock()
{
	if (locked)	
		return -1;
	locked=1;
	return rc.handle();
}

void eRCInput::unlock()
{
	if (locked)
		locked=0;
}
