#include <setup_audio.h>

#include <netinet/in.h>

#include <lib/base/i18n.h>

#include <lib/dvb/edvb.h>
#include <lib/dvb/eaudio.h>
#include <lib/gui/elabel.h>
#include <lib/gui/ebutton.h>
#include <lib/gui/echeckbox.h>
#include <lib/gui/eskin.h>
#include <lib/driver/rc.h>
#include <lib/system/econfig.h>

eZapAudioSetup::eZapAudioSetup():
	eWindow(0)
{
	setText(_("Audio setup"));
	cmove(ePoint(150, 136));
	cresize(eSize(390, 200));

	int fd=eSkin::getActive()->queryValue("fontsize", 20);
	int sac3default = 0;

	sac3default=eAudio::getInstance()->getAC3default();
// Key("/elitedvb/audio/ac3default", sac3default);

	ac3default=new eCheckbox(this, sac3default, 1);
	ac3default->setText("AC3 default output");
	ac3default->move(ePoint(10, 20));
	ac3default->resize(eSize(fd+4+240, fd+4));
	ac3default->setHelpText(_("enable/disable ac3 default output (ok)"));
	CONNECT( ac3default->checked, eZapAudioSetup::ac3defaultChanged );

	ok=new eButton(this);
	ok->setText(_("save"));
	ok->setShortcut("green");
	ok->setShortcutPixmap("green");
	ok->move(ePoint(20, 120));
	ok->resize(eSize(170, 40));
	ok->setHelpText(_("save changes and return"));
	ok->loadDeco();
	
	CONNECT(ok->selected, eZapAudioSetup::okPressed);

	abort=new eButton(this);
	abort->loadDeco();
	abort->setText(_("abort"));
	abort->move(ePoint(210, 120));
	abort->resize(eSize(170, 40));
	abort->setHelpText(_("ignore changes and return"));

	CONNECT(abort->selected, eZapAudioSetup::abortPressed);

	statusbar=new eStatusBar(this);
	statusbar->move( ePoint(0, clientrect.height()-30 ) );
	statusbar->resize( eSize( clientrect.width(), 30) );
	statusbar->loadDeco();
}

eZapAudioSetup::~eZapAudioSetup()
{
}

void eZapAudioSetup::ac3defaultChanged( int i )
{
	eAudio::getInstance()->setAC3default( i );
}

void eZapAudioSetup::okPressed()
{

	eAudio::getInstance()->saveSettings();

	//int sac3default=ac3default->isChecked();
	
	//eConfig::getInstance()->setKey("/elitedvb/audio/ac3default", sac3default);
	//eConfig::getInstance()->flush();
	
	close(1);
}

void eZapAudioSetup::abortPressed()
{
	eAudio::getInstance()->reloadSettings();

	close(0);
}

