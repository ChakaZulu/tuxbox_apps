#include "setupvideo.h"

#include <core/base/i18n.h>

#include <core/driver/eavswitch.h>
#include <core/driver/rc.h>
#include <core/driver/streamwd.h>
#include <core/gui/elabel.h>
#include <core/gui/ebutton.h>
#include <core/gui/eskin.h>
#include <core/system/econfig.h>


eZapVideoSetup::eZapVideoSetup(): eWindow(0)
{
/*	eSkin *skin=eSkin::getActive();
	if (skin->build(this, "setup.video"))
		qFatal("skin load of \"setup.video\" failed");*/

	if (eConfig::getInstance()->getKey("/elitedvb/video/colorformat", v_colorformat))
		v_colorformat = 1;

	if (eConfig::getInstance()->getKey("/elitedvb/video/pin8", v_pin8))
		v_pin8 = 0;

	eDebug("v_pin8 = %i, colorformat = %i", v_pin8, v_colorformat );

	int fd=eSkin::getActive()->queryValue("fontsize", 20);

	setText(_("Video Setup"));
	move(ePoint(150, 136));
	resize(eSize(350, 250));

	eLabel *l=new eLabel(this);
	l->setText("Colorformat:");
	l->move(ePoint(10, 20));
	l->resize(eSize(200, fd+4));

	colorformat=new eListBox<eListBoxEntryText>(this);
	colorformat->setFlags(eListBox<eListBoxEntryText>::flagNoUpDownMovement  | eListBox<eListBoxEntryText>::flagLoadDeco);
	colorformat->move(ePoint(160, 20));
	colorformat->resize(eSize(120, 35));
	eListBoxEntryText* entrys[3];
	entrys[0]=new eListBoxEntryText(colorformat, _("FBAS"), (void*)1);
	entrys[1]=new eListBoxEntryText(colorformat, _("RGB"), (void*)2);
	entrys[2]=new eListBoxEntryText(colorformat, _("SVideo"), (void*)3);
	colorformat->setCurrent(entrys[v_colorformat-1]);

  l=new eLabel(this);
	l->setText("Aspect Ratio:");
	l->move(ePoint(10, 65));
	l->resize(eSize(150, fd+4));
	
	pin8=new eListBox<eListBoxEntryText>(this);
	pin8->setFlags(eListBox<eListBoxEntryText>::flagNoUpDownMovement | eListBox<eListBoxEntryText>::flagLoadDeco);
	pin8->move(ePoint(160, 65));
	pin8->resize(eSize(170, 35));

	entrys[0]=new eListBoxEntryText(pin8, _("4:3 letterbox"), (void*)0);
	entrys[1]=new eListBoxEntryText(pin8, _("4:3 panscan"), (void*)1);
	entrys[2]=new eListBoxEntryText(pin8, _("16:9 (PIN8)"), (void*)2);
	pin8->setCurrent(entrys[v_pin8]);

	ok=new eButton(this);
	ok->setText(_("save"));
	ok->move(ePoint(20, 135));
	ok->resize(eSize(90, fd+4));

	CONNECT(ok->selected, eZapVideoSetup::okPressed);		

	abort=new eButton(this);
	abort->setText(_("abort"));
	abort->move(ePoint(140, 135));
	abort->resize(eSize(100, fd+4));

	CONNECT(abort->selected, eZapVideoSetup::abortPressed);
}

eZapVideoSetup::~eZapVideoSetup()
{
}

void eZapVideoSetup::okPressed()
{
	v_colorformat = (int) colorformat->getCurrent()->getKey();
	eDebug("v_colorformat = %i", v_colorformat);
	v_pin8 = (int) pin8->getCurrent()->getKey();
	eDebug("v_pin8 = %i", v_pin8 );
	if ( eConfig::getInstance()->setKey("/elitedvb/video/colorformat", v_colorformat ))
	{
		eConfig::getInstance()->delKey("/elitedvb/video/colorformat");
		eDebug("Write v_colorformat with error %i", eConfig::getInstance()->setKey("/elitedvb/video/colorformat", v_colorformat ) );
	}
	if ( eConfig::getInstance()->setKey("/elitedvb/video/pin8", v_pin8 ))
	{
		eConfig::getInstance()->delKey("/elitedvb/video/pin8");
		eDebug("Write v_pin8 with error %i", eConfig::getInstance()->setKey("/elitedvb/video/pin8", v_pin8 ));
	}
	eAVSwitch::getInstance()->reloadSettings();
	eStreamWatchdog::getInstance()->reloadSettings();
	close(1);
}

void eZapVideoSetup::abortPressed()
{
	close(0);
}


