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

/*#define ASSIGN(v, t, n)	\
	v =(t*)search(n); if (! v ) { qFatal("skin has undefined element: %s", n); }*/

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
/*	eSkin *skin=eSkin::getActive();
	if (skin->build(this, "setup.video"))
		qFatal("skin load of \"setup.video\" failed");

	ASSIGN(colorformat, eButton, "colorformat");
	ASSIGN(pin8, eButton, "pin8");
	ASSIGN(colorformat, eButton, "colorformat");
	ASSIGN(abort, eButton, "abort");*/

	int fd=eSkin::getActive()->queryValue("fontsize", 20);

	setText("Video Setup");
	move(ePoint(150, 136));
	resize(eSize(350, 250));

	eLabel *l=new eLabel(this);
	l->setText("Colorformat:");
	l->move(ePoint(10, 20));
	l->resize(eSize(150, fd+4));
	
	colorformat=new eButton(this, l);
	colorformat->setText("[color]");
	colorformat->move(ePoint(160, 20));
	colorformat->resize(eSize(85, fd+4));
//	connect(colorformat, SIGNAL(selected()), SLOT(toggleColorformat()));
	CONNECT(colorformat->selected, eZapVideoSetup::toggleColorformat);

  l=new eLabel(this);
	l->setText("Aspect Ratio:");
	l->move(ePoint(10, 55));
	l->resize(eSize(170, fd+4));

	pin8=new eButton(this, l);
	pin8->setText("[Pin8]");
	pin8->move(ePoint(160, 55));
	pin8->resize(eSize(140, fd+4));
//	connect(pin8, SIGNAL(selected()), SLOT(togglePin8()));
	CONNECT(pin8->selected, eZapVideoSetup::togglePin8);

	ok=new eButton(this);
	ok->setText("[OK]");
	ok->move(ePoint(10, 150));
	ok->resize(eSize(50, fd+4));
//  connect(ok, SIGNAL(selected()), SLOT(okPressed()));
	CONNECT(ok->selected, eZapVideoSetup::okPressed);	

	abort=new eButton(this);
	abort->setText("[ABORT]");
	abort->move(ePoint(80, 150));
	abort->resize(eSize(100, fd+4));
//	connect(abort, SIGNAL(selected()), SLOT(abortPressed()));
	CONNECT(abort->selected, eZapVideoSetup::abortPressed);


	unsigned int temp;
	v_colorformat=cfCVBS;
	v_pin8=0;
	if (eDVB::getInstance()->config.getKey("/elitedvb/video/colorformat", temp))
		temp=cfCVBS;
	qDebug("colorformat: %i\n", temp);
	v_colorformat=(eAVColorFormat)temp;
	if (v_colorformat==cfNull)
		v_colorformat=cfCVBS;

	eDVB::getInstance()->config.getKey("/elitedvb/video/pin8", v_pin8);
	qDebug("Pin8: %i\n", v_pin8);	
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
