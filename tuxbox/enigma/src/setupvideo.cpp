#include <setupvideo.h>

#include <lib/base/i18n.h>

#include <lib/driver/eavswitch.h>
#include <lib/driver/rc.h>
#include <lib/driver/streamwd.h>
#include <lib/gui/elabel.h>
#include <lib/gui/ebutton.h>
#include <lib/gui/eskin.h>
#include <lib/system/econfig.h>


eZapVideoSetup::eZapVideoSetup(): eWindow(0)
{
/*	eSkin *skin=eSkin::getActive();
	if (skin->build(this, "setup.video"))
		qFatal("skin load of \"setup.video\" failed");*/

/*	cresize( eSize(height(), width()) );
	cmove( ePoint(0,0) );*/

	if (eConfig::getInstance()->getKey("/elitedvb/video/colorformat", v_colorformat))
		v_colorformat = 1;

	if (eConfig::getInstance()->getKey("/elitedvb/video/pin8", v_pin8))
		v_pin8 = 0;

	int fd=eSkin::getActive()->queryValue("fontsize", 20);

	setText(_("Video Setup"));
	move(ePoint(150, 136));
	cresize(eSize(390, 200));

	eLabel *l=new eLabel(this);
	l->setText("Colorformat:");
	l->move(ePoint(10, 20));
	l->resize(eSize(200, fd+4));

	colorformat=new eListBox<eListBoxEntryText>(this, l);
	colorformat->loadDeco();
	colorformat->setFlags(eListBox<eListBoxEntryText>::flagNoUpDownMovement);
	colorformat->move(ePoint(160, 20));
	colorformat->resize(eSize(120, 35));
	eListBoxEntryText* entrys[3];
	entrys[0]=new eListBoxEntryText(colorformat, _("FBAS"), (void*)1);
	entrys[1]=new eListBoxEntryText(colorformat, _("RGB"), (void*)2);
	entrys[2]=new eListBoxEntryText(colorformat, _("SVideo"), (void*)3);
	colorformat->setCurrent(entrys[v_colorformat-1]);
	colorformat->setHelpText(_("choose colour format ( left, right )"));

  l=new eLabel(this);
	l->setText("Aspect Ratio:");
	l->move(ePoint(10, 65));
	l->resize(eSize(150, fd+4));
	
	pin8=new eListBox<eListBoxEntryText>(this, l);
	pin8->loadDeco();	
	pin8->setFlags(eListBox<eListBoxEntryText>::flagNoUpDownMovement);
	
	pin8->move(ePoint(160, 65));
	pin8->resize(eSize(170, 35));
	pin8->setHelpText(_("choose aspect ratio ( left, right )"));
	entrys[0]=new eListBoxEntryText(pin8, _("4:3 letterbox"), (void*)0);
	entrys[1]=new eListBoxEntryText(pin8, _("4:3 panscan"), (void*)1);
	entrys[2]=new eListBoxEntryText(pin8, _("16:9 (PIN8)"), (void*)2);
	pin8->setCurrent(entrys[v_pin8]);

	ok=new eButton(this);
	ok->setText(_("save"));
	ok->setShortcut("green");
	ok->setShortcutPixmap("green");
	ok->move(ePoint(20, 120));
	ok->resize(eSize(170, 40));
	ok->setHelpText(_("save changes and return"));
	ok->loadDeco();

	CONNECT(ok->selected, eZapVideoSetup::okPressed);		

	abort=new eButton(this);
	abort->setText(_("abort"));
	abort->move(ePoint(210, 120));
	abort->resize(eSize(170, 40));
	abort->setHelpText(_("ignore changes and return"));
	abort->loadDeco();

	CONNECT(abort->selected, eZapVideoSetup::abortPressed);

	status = new eStatusBar(this);	
	status->move( ePoint(0, clientrect.height()-30) );
	status->resize( eSize( clientrect.width(), 30) );
	status->loadDeco();
}

eZapVideoSetup::~eZapVideoSetup()
{
	if (status)
		delete status;
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
	eConfig::getInstance()->flush();
	eAVSwitch::getInstance()->reloadSettings();
	eStreamWatchdog::getInstance()->reloadSettings();
	close(1);
}

void eZapVideoSetup::abortPressed()
{
	close(0);
}


