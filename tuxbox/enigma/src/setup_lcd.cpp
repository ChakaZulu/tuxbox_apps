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
	eConfig::getInstance()->getKey("/ezap/lcd/brightness", lcdbrightness);
	eConfig::getInstance()->getKey("/ezap/lcd/contrast", lcdcontrast);
	eConfig::getInstance()->getKey("/ezap/lcd/standby", lcdstandby );


	p_brightness=CreateSkinnedSlider("brightness", "lbrightness", 0, LCD_BRIGHTNESS_MAX );
	CONNECT(p_brightness->changed, eZapLCDSetup::brightnessChanged );

	p_contrast=CreateSkinnedSlider("contrast","lcontrast", 0, LCD_CONTRAST_MAX );
	CONNECT(p_contrast->changed, eZapLCDSetup::contrastChanged );

	p_standby=CreateSkinnedSlider("standby","lstandby", 0, LCD_BRIGHTNESS_MAX );
	CONNECT(p_standby->changed, eZapLCDSetup::standbyChanged );

	inverted=CreateSkinnedCheckbox("inverted",0,"/ezap/lcd/inverted");
	CONNECT(inverted->checked, eZapLCDSetup::invertedChanged );

	CONNECT(CreateSkinnedButton("ok")->selected, eZapLCDSetup::okPressed);

	BuildSkin("eZapLCDSetup");

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
