#include "setupvideo.h"
#include "elabel.h"
#include "edvb.h"
#include "enumber.h"
#include "ebutton.h"
#include "echeckbox.h"
#include "rc.h"
#include "enigma.h"
#include "eskin.h"
#include "eavswitch.h"
#include "streamwd.h"

#define ASSIGN(v, t, n)	\
	v =(t*)search(n); if (! v ) { qFatal("skin has undefined element: %s", n); }

void eZapVideoSetup::setPin8(int w)
{
	switch (w)
	{
	case 0:
		pin8->setText("4:3 Letterbox");
		break;
	case 1:
		pin8->setText("4:3 Panscan");
		break;
	case 2:
		pin8->setText("16:9 (PIN8)");
		break;
	}
}

void eZapVideoSetup::setColorFormat(eAVColorFormat w)
{
	switch (w)
	{
	case cfCVBS:
		colorformat->setText("FBAS");
		break;
	case cfRGB:
		colorformat->setText("RGB");
		break;
	case cfYC:
		colorformat->setText("SVideo");
		break;
	}
}

eZapVideoSetup::eZapVideoSetup(): eWindow(0)
{
	eSkin *skin=eSkin::getActive();
	if (skin->build(this, "setup.video"))
		qFatal("skin load of \"setup.video\" failed");

	ASSIGN(colorformat, eButton, "colorformat");
	ASSIGN(pin8, eButton, "pin8");
	ASSIGN(ok, eButton, "ok");
	ASSIGN(abort, eButton, "abort");
	
	connect(colorformat, SIGNAL(selected()), SLOT(toggleColorformat()));
	connect(pin8, SIGNAL(selected()), SLOT(togglePin8()));

	connect(ok, SIGNAL(selected()), SLOT(okPressed()));
	connect(abort, SIGNAL(selected()), SLOT(abortPressed()));

	unsigned int temp;
	v_colorformat=cfCVBS;
	v_pin8=0;
	if (eDVB::getInstance()->config.getKey("/elitedvb/video/colorformat", temp))
		temp=cfCVBS;
	v_colorformat=(eAVColorFormat)temp;
	if (v_colorformat==cfNull)
		v_colorformat=cfCVBS;
	eDVB::getInstance()->config.getKey("/elitedvb/video/pin8", v_pin8);
	
	setColorFormat(v_colorformat);
	setPin8(v_pin8);
}

eZapVideoSetup::~eZapVideoSetup()
{
}

void eZapVideoSetup::okPressed()
{
	eDVB::getInstance()->config.setKey("/elitedvb/video/colorformat", (unsigned int)v_colorformat);
	eDVB::getInstance()->config.setKey("/elitedvb/video/pin8", v_pin8);
	eAVSwitch::getInstance()->reloadSettings();
	eStreamWatchdog::getInstance()->reloadSettings();
	close(1);
}

void eZapVideoSetup::abortPressed()
{
	close(0);
}

void eZapVideoSetup::toggleColorformat()
{
	switch (v_colorformat)
	{
	case cfCVBS:
		v_colorformat=cfRGB;
		break;
	case cfRGB:
		v_colorformat=cfYC;
		break;
	case cfYC:
	default:
		v_colorformat=cfCVBS;
		break;
	}
	setColorFormat(v_colorformat);
}

void eZapVideoSetup::togglePin8()
{
	v_pin8++;
	if (v_pin8>2)
		v_pin8=0;
	setPin8(v_pin8);
}

int eZapVideoSetup::eventFilter(const eWidgetEvent &event)
{
	switch (event.type)
	{
	case eWidgetEvent::keyDown:
		switch(event.parameter)
		{
		case eRCInput::RC_RIGHT:
		case eRCInput::RC_DOWN:
			focusNext(0);
			return 1;
			break;
		case eRCInput::RC_LEFT:
		case eRCInput::RC_UP:
			focusNext(1);
			return 1;
			break;
		}
	}
	return 0;
}
