#include <setupvideo.h>

#include <lib/base/i18n.h>

#include <lib/driver/eavswitch.h>
#include <lib/dvb/edvb.h>
#include <lib/dvb/eaudio.h>
#include <lib/driver/rc.h>
#include <lib/driver/streamwd.h>
#include <lib/gui/elabel.h>
#include <lib/gui/ebutton.h>
#include <lib/gui/eskin.h>
#include <lib/gui/echeckbox.h>
#include <lib/system/econfig.h>
#include <lib/system/info.h>
                                               
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

	if (eConfig::getInstance()->getKey("/elitedvb/video/disableWSS", v_disableWSS ))
		v_disableWSS = 0;

	if (eConfig::getInstance()->getKey("/elitedvb/video/palM", v_palM ))
		v_palM=0;

	if (eConfig::getInstance()->getKey("/elitedvb/video/vcr_switching", v_VCRSwitching ))
		v_VCRSwitching=1;

	int fd=eSkin::getActive()->queryValue("fontsize", 20);

	setText(_("A/V Settings"));
	move(ePoint(160, 90));
	cresize(eSize(390, 350));

	eLabel *l=new eLabel(this);
	l->setText(_("Color Format:"));
	l->move(ePoint(20, 20));
	l->resize(eSize(150, fd+4));

	colorformat=new eListBox<eListBoxEntryText>(this, l);
	colorformat->loadDeco();
	colorformat->setFlags(eListBox<eListBoxEntryText>::flagNoUpDownMovement);
	colorformat->move(ePoint(180, 20));
	colorformat->resize(eSize(120, 35));
	eListBoxEntryText* entrys[4];
	entrys[0]=new eListBoxEntryText(colorformat, _("CVBS"), (void*)1);
	entrys[1]=new eListBoxEntryText(colorformat, _("RGB"), (void*)2);
	entrys[2]=new eListBoxEntryText(colorformat, _("SVideo"), (void*)3);

	if( eSystemInfo::getInstance()->getHwType() > eSystemInfo::dbox2Philips  )
		entrys[3]=new eListBoxEntryText(colorformat, _("YPbPr"), (void*)4);

	colorformat->setCurrent(entrys[v_colorformat-1]);
	colorformat->setHelpText(_("choose color format ( left, right )"));
	CONNECT( colorformat->selchanged, eZapVideoSetup::CFormatChanged );

  l=new eLabel(this);
	l->setText(_("Aspect Ratio:"));
	l->move(ePoint(20, 60));
	l->resize(eSize(150, fd+4));
	
	pin8=new eListBox<eListBoxEntryText>(this, l);
	pin8->loadDeco();	
	pin8->setFlags(eListBox<eListBoxEntryText>::flagNoUpDownMovement);
	
	pin8->move(ePoint(180, 60));
	pin8->resize(eSize(170, 35));
	pin8->setHelpText(_("choose aspect ratio ( left, right )"));
	entrys[0]=new eListBoxEntryText(pin8, _("4:3 letterbox"), (void*)0);
	entrys[1]=new eListBoxEntryText(pin8, _("4:3 panscan"), (void*)1);
	entrys[2]=new eListBoxEntryText(pin8, _("16:9"), (void*)2);
	entrys[3]=new eListBoxEntryText(pin8, _("always 16:9"), (void*)3);
	pin8->setCurrent(entrys[v_pin8]);
	CONNECT( pin8->selchanged, eZapVideoSetup::VPin8Changed );

	c_disableWSS = new eCheckbox(this, v_disableWSS, 1);
	c_disableWSS->move(ePoint(20,100));
	c_disableWSS->resize(eSize(350,30));
	c_disableWSS->setText(_("Disable WSS on 4:3"));
	c_disableWSS->setHelpText(_("don't send WSS signal when A-ratio is 4:3"));
	CONNECT( c_disableWSS->checked, eZapVideoSetup::DisableWSSChanged );

	int sac3default = 0;
	sac3default=eAudio::getInstance()->getAC3default();

	ac3default=new eCheckbox(this, sac3default, 1);
	ac3default->setText(_("AC3 default output"));
	ac3default->move(ePoint(20, 135));
	ac3default->resize(eSize(350, 30));
	ac3default->setHelpText(_("enable/disable ac3 default output (ok)"));
	CONNECT( ac3default->checked, eZapVideoSetup::ac3defaultChanged );

	palM=new eCheckbox(this, v_palM, 1);
	palM->setText(_("use PAL-M for NTSC"));
	palM->move(ePoint(20, 170));
	palM->resize(eSize(350, 30));
	palM->setHelpText(_("use PAL-M instead of real NTSC"));
	CONNECT( palM->checked, eZapVideoSetup::palMChanged );

	VCRSwitching=new eCheckbox(this, v_VCRSwitching, 1);
	VCRSwitching->setText(_("Auto VCR switching"));
	VCRSwitching->move(ePoint(20, 205));
	VCRSwitching->resize(eSize(350, 30));
	VCRSwitching->setHelpText(_("auto switch to VCR connector"));
	CONNECT( VCRSwitching->checked, eZapVideoSetup::VCRChanged );

	ok=new eButton(this);
	ok->setText(_("save"));
	ok->setShortcut("green");
	ok->setShortcutPixmap("green");
	ok->move(ePoint(20, 250));
	ok->resize(eSize(220, 40));
	ok->setHelpText(_("save changes and return"));
	ok->loadDeco();

	CONNECT(ok->selected, eZapVideoSetup::okPressed);		

	status = new eStatusBar(this);	
	status->move( ePoint(0, clientrect.height()-50) );
	status->resize( eSize( clientrect.width(), 50) );
	status->loadDeco();
	setHelpID(86);
}

eZapVideoSetup::~eZapVideoSetup()
{
	if (status)
		delete status;
}

void eZapVideoSetup::okPressed()
{
	eConfig::getInstance()->setKey("/elitedvb/video/vcr_switching", v_VCRSwitching );
	eConfig::getInstance()->setKey("/elitedvb/video/colorformat", v_colorformat );
	eConfig::getInstance()->setKey("/elitedvb/video/pin8", v_pin8 );
	eConfig::getInstance()->setKey("/elitedvb/video/disableWSS", v_disableWSS );
	eConfig::getInstance()->setKey("/elitedvb/video/palM", v_palM);
	eAudio::getInstance()->saveSettings();
	eConfig::getInstance()->flush();
	close(1);
}

int eZapVideoSetup::eventHandler( const eWidgetEvent &e)
{
	switch(e.type)
	{
		case eWidgetEvent::execDone:
			eAVSwitch::getInstance()->reloadSettings();
			eStreamWatchdog::getInstance()->reloadSettings();
			eAudio::getInstance()->reloadSettings();
			break;
		default:
			return eWindow::eventHandler( e );
	}
	return 1;
}

void eZapVideoSetup::CFormatChanged( eListBoxEntryText * e )
{
	int old = 1;
	eConfig::getInstance()->getKey("/elitedvb/video/colorformat", old);
	if ( e )
	{
		v_colorformat = (int) e->getKey();
		eConfig::getInstance()->setKey("/elitedvb/video/colorformat", v_colorformat );
		eAVSwitch::getInstance()->reloadSettings();
		eConfig::getInstance()->setKey("/elitedvb/video/colorformat", old );
	}
}

void eZapVideoSetup::VPin8Changed( eListBoxEntryText * e)
{
	int old = 0;
	eConfig::getInstance()->getKey("/elitedvb/video/pin8", old);

	if ( e )
	{
		v_pin8 = (int) e->getKey();
		eConfig::getInstance()->setKey("/elitedvb/video/pin8", v_pin8 );
		eStreamWatchdog::getInstance()->reloadSettings();
		eConfig::getInstance()->setKey("/elitedvb/video/pin8", old );
	}
}

void eZapVideoSetup::DisableWSSChanged( int i )
{
	unsigned int old = 0;
	eConfig::getInstance()->getKey("/elitedvb/video/disableWSS", old );

	v_disableWSS = (unsigned int) i;
	eConfig::getInstance()->setKey("/elitedvb/video/disableWSS", v_disableWSS );
	eStreamWatchdog::getInstance()->reloadSettings();
	eConfig::getInstance()->setKey("/elitedvb/video/disableWSS", old );
}

void eZapVideoSetup::VCRChanged( int i )
{
	unsigned int old = 0;
	eConfig::getInstance()->getKey("/elitedvb/video/vcr_switching", old );

	v_VCRSwitching = (unsigned int) i;
	eConfig::getInstance()->setKey("/elitedvb/video/vcr_switching", v_VCRSwitching );
	eStreamWatchdog::getInstance()->reloadSettings();
	eConfig::getInstance()->setKey("/elitedvb/video/vcr_switching", old );
}

void eZapVideoSetup::palMChanged( int i )
{
	unsigned int old = 0;
	eConfig::getInstance()->getKey("/elitedvb/video/palM", old );

	v_palM = (unsigned int) i;
	eConfig::getInstance()->setKey("/elitedvb/video/palM", v_palM);
	eAVSwitch::getInstance()->reloadSettings();
	eConfig::getInstance()->setKey("/elitedvb/video/palM", old );
}

void eZapVideoSetup::ac3defaultChanged( int i )
{
	eAudio::getInstance()->setAC3default( i );
}
