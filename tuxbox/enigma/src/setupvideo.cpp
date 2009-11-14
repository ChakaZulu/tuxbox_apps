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
#include <lib/gui/testpicture.h>
#include <lib/system/econfig.h>
#include <lib/system/info.h>
#include <lib/system/init.h>
#include <lib/system/init_num.h>

#include <src/enigma_lcd.h>
                                               
eZapVideoSetup::eZapVideoSetup()
	:eWindow(0)
{
	init_eZapVideoSetup();
}

void eZapVideoSetup::init_eZapVideoSetup()
{

	if (eConfig::getInstance()->getKey("/elitedvb/video/colorformat", v_colorformat))
		v_colorformat = 1;

	if (eConfig::getInstance()->getKey("/elitedvb/video/pin8", v_pin8))
		v_pin8 = 0;

	if (eConfig::getInstance()->getKey("/elitedvb/video/disableWSS", v_disableWSS ))
		v_disableWSS = 0;

	if (eConfig::getInstance()->getKey("/elitedvb/video/tvsystem", v_tvsystem ))
		v_tvsystem = 1;
		
	if (!v_tvsystem)
		v_tvsystem = 1;

	if (eConfig::getInstance()->getKey("/elitedvb/video/vcr_switching", v_VCRSwitching ))
		v_VCRSwitching=1;


	colorformat=new eListBox<eListBoxEntryText>(this, CreateSkinnedLabel("lcolorformat"));
	colorformat->setName("colorformat");
	colorformat->setFlags(eListBox<eListBoxEntryText>::flagNoUpDownMovement);
	eListBoxEntryText* entrys[4];
	entrys[0]=new eListBoxEntryText(colorformat, _("CVBS"), (void*)1);
	entrys[1]=new eListBoxEntryText(colorformat, _("RGB"), (void*)2);
	entrys[2]=new eListBoxEntryText(colorformat, _("SVideo"), (void*)3);
	entrys[3]=new eListBoxEntryText(colorformat, _("YPbPr"), (void*)4);

	eListBoxEntryText* selcolorformat = entrys[v_colorformat-1];
	CONNECT( colorformat->selchanged, eZapVideoSetup::CFormatChanged );

	
	pin8=new eListBox<eListBoxEntryText>(this, CreateSkinnedLabel("lpin8"));
	pin8->setName("pin8");
	pin8->setFlags(eListBox<eListBoxEntryText>::flagNoUpDownMovement);
	entrys[0]=new eListBoxEntryText(pin8, _("4:3 letterbox"), (void*)0);
	entrys[1]=new eListBoxEntryText(pin8, _("4:3 panscan"), (void*)1);
	entrys[2]=new eListBoxEntryText(pin8, _("16:9"), (void*)2);
	/* dbox, dm700, dm7020 can do black bars left and right of 4:3 video */
	if ( eSystemInfo::getInstance()->getHwType() <= eSystemInfo::DM7020 )
		entrys[3]=new eListBoxEntryText(pin8, _("always 16:9"), (void*)3);
	eListBoxEntryText* selpin8 = entrys[v_pin8];
	CONNECT( pin8->selchanged, eZapVideoSetup::VPin8Changed );

	tvsystem=new eListBox<eListBoxEntryText>(this, CreateSkinnedLabel("ltvsystem"));
	tvsystem->setName("tvsystem");
	tvsystem->setFlags(eListBox<eListBoxEntryText>::flagNoUpDownMovement);
	
	// our bitmask is:
	
	// have pal     1
	// have ntsc    2
	// have pal60   4  (aka. PAL-M bis wir PAL60 supporten)
	
	// allowed bitmasks:
	
	//  1    pal only, no ntsc
	//  2    ntsc only, no pal
	//  3    multinorm
	//  5    pal, pal60
	
	entrys[0]=new eListBoxEntryText(tvsystem, "PAL", (void*)1);
	entrys[1]=new eListBoxEntryText(tvsystem, "PAL + PAL60", (void*)5);
	entrys[2]=new eListBoxEntryText(tvsystem, "Multinorm", (void*)3);
	entrys[3]=new eListBoxEntryText(tvsystem, "NTSC", (void*)2);
	
	int i = 0;
	switch (v_tvsystem)
	{
	case 1: i = 0; break;
	case 5: i = 1; break;
	case 3: i = 2; break;
	case 2: i = 3; break;
	}
	eListBoxEntryText* seltvsystem = entrys[i];
	CONNECT( tvsystem->selchanged, eZapVideoSetup::TVSystemChanged );

	CONNECT( CreateSkinnedCheckbox("c_disableWSS",v_disableWSS)->checked, eZapVideoSetup::DisableWSSChanged );

	int sac3default = 0;
	sac3default=eAudio::getInstance()->getAC3default();

	CONNECT( CreateSkinnedCheckbox("ac3default",sac3default)->checked, eZapVideoSetup::ac3defaultChanged );

	if ( eSystemInfo::getInstance()->hasScartSwitch() )
	{
		CONNECT( CreateSkinnedCheckbox("VCRSwitching",v_VCRSwitching)->checked, eZapVideoSetup::VCRChanged );
	}


	CONNECT(CreateSkinnedButton("ok")->selected, eZapVideoSetup::okPressed);

	CONNECT(CreateSkinnedButton("testpicture")->selected, eZapVideoSetup::showTestpicture);

	BuildSkin("eZapVideoSetup");

	if (!eSystemInfo::getInstance()->hasScartSwitch() )
	{
		CreateSkinnedCheckbox("VCRSwitching",v_VCRSwitching)->hide();
	}
	tvsystem->setCurrent(seltvsystem);
	colorformat->setCurrent(selcolorformat);
	pin8->setCurrent(selpin8);


	setHelpID(86);
}

eZapVideoSetup::~eZapVideoSetup()
{
}

void eZapVideoSetup::showTestpicture()
{
	hide();
	
	int mode = 1;
	while ((mode > 0) && (mode < 9))
		mode = eTestPicture::display(mode-1);
	
	show();
}

void eZapVideoSetup::okPressed()
{
	eConfig::getInstance()->setKey("/elitedvb/video/vcr_switching", v_VCRSwitching );
	eConfig::getInstance()->setKey("/elitedvb/video/colorformat", v_colorformat );
	eConfig::getInstance()->setKey("/elitedvb/video/pin8", v_pin8 );
	eConfig::getInstance()->setKey("/elitedvb/video/disableWSS", v_disableWSS );
	eConfig::getInstance()->setKey("/elitedvb/video/tvsystem", v_tvsystem);
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
	unsigned int old = 1;
	eConfig::getInstance()->getKey("/elitedvb/video/colorformat", old);
	if ( e )
	{
		v_colorformat = (unsigned int) e->getKey();
		eConfig::getInstance()->setKey("/elitedvb/video/colorformat", v_colorformat );
		eAVSwitch::getInstance()->reloadSettings();
		eConfig::getInstance()->setKey("/elitedvb/video/colorformat", old );
	}
}

void eZapVideoSetup::VPin8Changed( eListBoxEntryText * e)
{
	unsigned int old = 0;
	eConfig::getInstance()->getKey("/elitedvb/video/pin8", old);

	if ( e )
	{
		v_pin8 = (unsigned int) e->getKey();
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

void eZapVideoSetup::TVSystemChanged( eListBoxEntryText *e )
{
	unsigned int old = 0;
	eConfig::getInstance()->getKey("/elitedvb/video/tvsystem", old );

	if (e)
	{
		v_tvsystem = (unsigned int) e->getKey();
		eConfig::getInstance()->setKey("/elitedvb/video/tvsystem", v_tvsystem);
		eAVSwitch::getInstance()->reloadSettings();
		eConfig::getInstance()->setKey("/elitedvb/video/tvsystem", old );
	}
}

void eZapVideoSetup::ac3defaultChanged( int i )
{
	eAudio::getInstance()->setAC3default( i );
}

class eWizardTVSystem: public eWindow
{
	eListBox<eListBoxEntryText> *tvsystem;
	unsigned int v_tvsystem;
	void TVSystemChanged( eListBoxEntryText * );
	void okPressed();
	int eventHandler( const eWidgetEvent &e );
public:
	eWizardTVSystem();
	static int run();
};

void eWizardTVSystem::TVSystemChanged( eListBoxEntryText *e )
{
	unsigned int old = 0;
	eConfig::getInstance()->getKey("/elitedvb/video/tvsystem", old );
	if (e)
	{
		v_tvsystem = (unsigned int) e->getKey();
		eConfig::getInstance()->setKey("/elitedvb/video/tvsystem", v_tvsystem);
		eAVSwitch::getInstance()->reloadSettings();
		eConfig::getInstance()->setKey("/elitedvb/video/tvsystem", old );
	}
}

int eWizardTVSystem::eventHandler( const eWidgetEvent &e)
{
	switch(e.type)
	{
		case eWidgetEvent::execDone:
			eAVSwitch::getInstance()->reloadSettings();
			eStreamWatchdog::getInstance()->reloadSettings();
			break;
		case eWidgetEvent::wantClose:
			unsigned int bla;
			if ( eConfig::getInstance()->getKey("/elitedvb/video/tvsystem", bla ) )
			    break;
		default:
			return eWindow::eventHandler( e );
	}
	return 1;
}

eWizardTVSystem::eWizardTVSystem(): eWindow(0)
{
	v_tvsystem=1;
	eConfig::getInstance()->getKey("/elitedvb/video/tvsystem", v_tvsystem );

	tvsystem=new eListBox<eListBoxEntryText>(this, CreateSkinnedLabel("ltvsystem"));
	tvsystem->setName("tvsystem");
	tvsystem->setFlags(eListBox<eListBoxEntryText>::flagNoUpDownMovement);

	// our bitmask is:

	// have pal     1
	// have ntsc    2
	// have pal60   4  (aka. PAL-M bis wir PAL60 supporten)

	// allowed bitmasks:

	//  1    pal only, no ntsc
	//  2    ntsc only, no pal
	//  3    multinorm
	//  5    pal, pal60

	eListBoxEntryText *entrys[4];
	entrys[0]=new eListBoxEntryText(tvsystem, "PAL", (void*)1);
	entrys[1]=new eListBoxEntryText(tvsystem, "PAL + PAL60", (void*)5);
	entrys[2]=new eListBoxEntryText(tvsystem, "Multinorm", (void*)3);
	entrys[3]=new eListBoxEntryText(tvsystem, "NTSC", (void*)2);

	int i = 0;
	switch (v_tvsystem)
	{
	case 1: i = 0; break;
	case 5: i = 1; break;
	case 3: i = 2; break;
	case 2: i = 3; break;
	}
	tvsystem->setCurrent(entrys[i]);
	CONNECT( tvsystem->selchanged, eWizardTVSystem::TVSystemChanged );

	CONNECT(CreateSkinnedButton("ok")->selected, eWizardTVSystem::okPressed);

	BuildSkin("eWizardTVSystem");
}

void eWizardTVSystem::okPressed()
{
	eConfig::getInstance()->setKey("/elitedvb/video/tvsystem", v_tvsystem);
	eAudio::getInstance()->saveSettings();
	eConfig::getInstance()->flush();
	close(1);
}

class eWizardTVSystemInit
{
public:
	eWizardTVSystemInit()
	{
		if ( eSystemInfo::getInstance()->getHwType() >= eSystemInfo::DM7000 )
		{
			// only run wizzard when language not yet setup'ed
			unsigned int tvsystem=0;
			if ( eConfig::getInstance()->getKey("/elitedvb/video/tvsystem", tvsystem) )
			{
				eWizardTVSystem w;
#ifndef DISABLE_LCD
				eZapLCD* pLCD = eZapLCD::getInstance();
				pLCD->lcdMain->hide();
				pLCD->lcdMenu->show();
    			w.setLCD( pLCD->lcdMenu->Title, pLCD->lcdMenu->Element );
#endif
				w.show();
				w.exec();
				w.hide();
#ifndef DISABLE_LCD
				pLCD->lcdMenu->hide();
				pLCD->lcdMain->show();
#endif
			}
			else
				eDebug("tvsystem already selected.. do not start tvsystem wizard");
		}
	}
};

eAutoInitP0<eWizardTVSystemInit> init_eWizardTVSystemInit(eAutoInitNumbers::wizard-1, "wizard: tv system");
