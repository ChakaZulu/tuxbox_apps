#include <setup_osd.h>

#include <lib/gui/echeckbox.h>
#include <lib/gui/eskin.h>
#include <lib/system/econfig.h>
#include <lib/base/i18n.h>
#include <lib/gdi/gfbdc.h>

eZapOsdSetup::eZapOsdSetup(): eWindow(0)
{
	setText("OSD Setup");
	move(ePoint(150, 136));
	cresize(eSize(440, 335));

	int fd=eSkin::getActive()->queryValue("fontsize", 20);

	int state=0;
	eConfig::getInstance()->getKey("/ezap/osd/showOSDOnEITUpdate", state);

	showOSDOnEITUpdate = new eCheckbox(this, state, fd);
	showOSDOnEITUpdate->setText(_("Show OSD on EIT Update"));
	showOSDOnEITUpdate->move(ePoint(20, 25));
	showOSDOnEITUpdate->resize(eSize(fd+4+300, fd+4));
	showOSDOnEITUpdate->setHelpText(_("shows OSD when now/next info is changed"));

	state=0;
	eConfig::getInstance()->getKey("/ezap/osd/showConsoleOnFB", state);
	showConsoleOnFB = new eCheckbox(this, state, fd);
	showConsoleOnFB->setText(_("Show Console on Framebuffer"));
	showConsoleOnFB->move(ePoint(20, 65));
	showConsoleOnFB->resize(eSize(fd+4+300, fd+4));
	showConsoleOnFB->setHelpText(_("shows the linux console on TV"));
	
	showConsoleOnFB->hide();

	alpha = gFBDC::getInstance()->getAlpha();
	eLabel* l = new eLabel(this);
	l->setText(_("Alpha:"));
	l->move(ePoint(20, 105));
	l->resize(eSize(110, fd+4));
	sAlpha = new eSlider( this, l, 0, 512 );
	sAlpha->setIncrement( 10 ); // Percent !
	sAlpha->move( ePoint( 140, 105 ) );
	sAlpha->resize(eSize( 280, fd+4 ) );
	sAlpha->setHelpText(_("change the transparency correction"));
	sAlpha->setValue( alpha);
	CONNECT( sAlpha->changed, eZapOsdSetup::alphaChanged );

	brightness = gFBDC::getInstance()->getBrightness();
	l = new eLabel(this);
	l->setText(_("Brightness:"));
	l->move(ePoint(20, 145));
	l->resize(eSize(110, fd+4));
	sBrightness = new eSlider( this, l, 0, 255 );
	sBrightness->setIncrement( 5 ); // Percent !
	sBrightness->move( ePoint( 140, 145 ) );
	sBrightness->resize(eSize( 280, fd+4 ) );
	sBrightness->setHelpText(_("change the brightness correction"));
	sBrightness->setValue( brightness);
	CONNECT( sBrightness->changed, eZapOsdSetup::brightnessChanged );

	gamma = gFBDC::getInstance()->getGamma();
	l = new eLabel(this);
	l->setText(_("Contrast:"));
	l->move(ePoint(20, 185));
	l->resize(eSize(110, fd+4));
	sGamma = new eSlider( this, l, 0, 255 );
	sGamma->setIncrement( 5 ); // Percent !
	sGamma->move( ePoint( 140, 185 ) );
	sGamma->resize(eSize( 280, fd+4 ) );
	sGamma->setHelpText(_("change the contrast"));
	sGamma->setValue( gamma);
	CONNECT( sGamma->changed, eZapOsdSetup::gammaChanged );

	ok=new eButton(this);
	ok->setText(_("save"));
	ok->move(ePoint(20, 250));
	ok->resize(eSize(170, 40));
	ok->setHelpText(_("close window and save changes"));
	ok->loadDeco();
	
	CONNECT(ok->selected, eZapOsdSetup::okPressed);

	abort=new eButton(this);
	abort->setText(_("abort"));
	abort->move(ePoint(210, 250));
	abort->resize(eSize(170, 40));
	abort->setHelpText(_("close window (no changes are saved)"));
	abort->loadDeco();

	CONNECT(abort->selected, eZapOsdSetup::abortPressed);

	statusbar=new eStatusBar(this);
	statusbar->move( ePoint(0, clientrect.height()-30 ) );
	statusbar->resize( eSize( clientrect.width(), 30) );
	statusbar->loadDeco();
}

eZapOsdSetup::~eZapOsdSetup()
{
}

void eZapOsdSetup::alphaChanged( int i )
{
	alpha = i;
	gFBDC::getInstance()->setAlpha(alpha);
}

void eZapOsdSetup::brightnessChanged( int i )
{
	brightness = i;
	gFBDC::getInstance()->setBrightness(brightness);
}

void eZapOsdSetup::gammaChanged( int i )
{
	gamma = i;
	gFBDC::getInstance()->setGamma(gamma);
}

void eZapOsdSetup::fieldSelected(int *number)
{
	focusNext(eWidget::focusDirNext);
}

void eZapOsdSetup::okPressed()
{
	gFBDC::getInstance()->saveSettings();
	eConfig::getInstance()->setKey("/ezap/osd/showOSDOnEITUpdate", showOSDOnEITUpdate->isChecked());
	eConfig::getInstance()->setKey("/ezap/osd/showConsoleOnFB", showConsoleOnFB->isChecked());

	eConfig::getInstance()->flush();
	close(1);
}

void eZapOsdSetup::abortPressed()
{
	gFBDC::getInstance()->reloadSettings();
	close(0);
}
