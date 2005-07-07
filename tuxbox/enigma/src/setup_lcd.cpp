#ifndef DISABLE_LCD
#include <setup_lcd.h>

#include <lib/gui/slider.h>
#include <lib/gui/ebutton.h>
#include <lib/gui/echeckbox.h>
#include <lib/gui/elabel.h>
#include <lib/gui/enumber.h>
#include <lib/gui/eskin.h>
#include <lib/system/econfig.h>
#include <lib/base/i18n.h>
#include <lib/dvb/dvbwidgets.h>
#include <lib/gdi/lcd.h>

void eZapLCDSetup::brightnessChanged( int i )
{
	eDebug("Brightness changed to %i", i);
	lcdbrightness = i;
	update(lcdbrightness, lcdcontrast);
}

void eZapLCDSetup::contrastChanged( int i )
{
	eDebug("contrast changed to %i", i);
	lcdcontrast = i;
	update(lcdbrightness, lcdcontrast);
}

void eZapLCDSetup::standbyChanged( int i )
{
	eDebug("standby changed to %i", i);
	lcdstandby = i;
	update(lcdstandby, lcdcontrast);
}

void eZapLCDSetup::invertedChanged( int i )
{
	eDebug("invertion changed to %s", (i?"inverted":"not inverted") );
	eDBoxLCD::getInstance()->setInverted( i?255:0 );
}

void eZapLCDSetup::update(int brightness, int contrast)
{
	eDBoxLCD::getInstance()->setLCDParameter(brightness, contrast);
}

eZapLCDSetup::eZapLCDSetup()
	:eWindow(0)
{
	init_eZapLCDSetup();
}

void eZapLCDSetup::init_eZapLCDSetup()
{
	setText(_("LC-Display Setup"));
	move(ePoint(150, 126));
	cresize(eSize(400, 290));

	int fd=eSkin::getActive()->queryValue("fontsize", 20);

	eConfig::getInstance()->getKey("/ezap/lcd/brightness", lcdbrightness);
	eConfig::getInstance()->getKey("/ezap/lcd/contrast", lcdcontrast);
	eConfig::getInstance()->getKey("/ezap/lcd/standby", lcdstandby );
	int tmp;
	eConfig::getInstance()->getKey("/ezap/lcd/inverted", tmp );
	unsigned char lcdinverted = (unsigned char) tmp;

	bbrightness=new eLabel(this);
	bbrightness->setText(_("Brightness:"));
	bbrightness->move(ePoint(20, 20));
	bbrightness->resize(eSize(110, fd+4));

	bcontrast=new eLabel(this);
	bcontrast->setText(_("Contrast:"));
	bcontrast->move(ePoint(20, 60));
	bcontrast->resize(eSize(110, fd+4));

	bstandby=new eLabel(this);
	bstandby->setText(_("Standby:"));
	bstandby->move(ePoint(20, 100));
	bstandby->resize(eSize(110, fd+4));

	p_brightness=new eSlider(this, bbrightness, 0, LCD_BRIGHTNESS_MAX );
	p_brightness->setName("brightness");
	p_brightness->move(ePoint(140, 20));
	p_brightness->resize(eSize(240, fd+4));
	p_brightness->setHelpText(_("set LCD brightness ( left / right )"));
	CONNECT( p_brightness->changed, eZapLCDSetup::brightnessChanged );

	p_contrast=new eSlider(this, bcontrast, 0, LCD_CONTRAST_MAX );
	p_contrast->setName("contrast");
	p_contrast->move(ePoint(140, 60));
	p_contrast->resize(eSize(240, fd+4));
	p_contrast->setHelpText(_("set LCD contrast ( left / right )"));
	CONNECT( p_contrast->changed, eZapLCDSetup::contrastChanged );

	p_standby=new eSlider(this, bstandby, 0, LCD_BRIGHTNESS_MAX );
	p_standby->setName("standby");
	p_standby->move(ePoint(140, 100));
	p_standby->resize(eSize(240, fd+4));
	p_standby->setHelpText(_("set LCD brightness for Standby Mode ( left / right )"));
	CONNECT( p_standby->changed, eZapLCDSetup::standbyChanged );

	inverted=new eCheckbox(this);
	inverted->move(ePoint(20, 140));
	inverted->resize(eSize(150, fd+4));
	inverted->setText(_("Inverted: "));
	inverted->setCheck(lcdinverted);
	inverted->setHelpText(_("enable/disable inverted LCD (ok)"));
	CONNECT( inverted->checked, eZapLCDSetup::invertedChanged );

	ok=new eButton(this);
	ok->setText(_("save"));
	ok->setShortcut("green");
	ok->setShortcutPixmap("green");
	ok->move(ePoint(20, 190));
	ok->resize(eSize(220, 40));
	ok->setHelpText(_("save changes and return"));
	ok->loadDeco();
	CONNECT(ok->selected, eZapLCDSetup::okPressed);

	statusbar=new eStatusBar(this);
	statusbar->move( ePoint(0, clientrect.height()-50 ) );
	statusbar->resize( eSize( clientrect.width(), 50) );
	statusbar->loadDeco();

	p_brightness->setValue(lcdbrightness);
	p_contrast->setValue(lcdcontrast);
	p_standby->setValue(lcdstandby);
	
	setHelpID(84);
}

eZapLCDSetup::~eZapLCDSetup()
{
}

void eZapLCDSetup::okPressed()
{
	eConfig::getInstance()->setKey("/ezap/lcd/brightness", lcdbrightness);
	eConfig::getInstance()->setKey("/ezap/lcd/contrast", lcdcontrast);
	eConfig::getInstance()->setKey("/ezap/lcd/standby", lcdstandby);
	eConfig::getInstance()->setKey("/ezap/lcd/inverted", inverted->isChecked()?255:0 );
	eConfig::getInstance()->flush();
	update(lcdbrightness, lcdcontrast);
	close(1);
}

int eZapLCDSetup::eventHandler( const eWidgetEvent& e)
{
	switch (e.type)
	{
		case eWidgetEvent::execDone:
			eConfig::getInstance()->getKey("/ezap/lcd/brightness", lcdbrightness);
			eConfig::getInstance()->getKey("/ezap/lcd/contrast", lcdcontrast);
			eDBoxLCD::getInstance()->setInverted( inverted->isChecked()?255:0 );
			update(lcdbrightness, lcdcontrast);
			break;
		default:
			return eWindow::eventHandler( e );
	}
	return 1;
}

#endif //DISABLE_LCD
