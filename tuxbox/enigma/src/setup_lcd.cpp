#include "setup_lcd.h"

#include <core/gui/eprogress.h>
#include <core/gui/ebutton.h>
#include <core/gui/elabel.h>
#include <core/gui/enumber.h>
#include <core/gui/eskin.h>
#include <core/system/econfig.h>
#include <core/base/i18n.h>
#include <core/dvb/dvbwidgets.h>
#include <core/gdi/lcd.h>

void eZapLCDSetup::update()
{
	p_brightness->setPerc(lcdbrightness*100/LCD_BRIGHTNESS_MAX);
	p_contrast->setPerc(lcdcontrast*100/LCD_CONTRAST_MAX);
	eDBoxLCD::getInstance()->setLCDParameter(lcdbrightness, lcdcontrast);
}

int eZapLCDSetup::eventHandler(const eWidgetEvent &event)
{
	switch(event.type)
	{
	case eWidgetEvent::evtAction:
		if(event.action == &i_cursorActions->right)
		{
			if(this->focus==bbrightness)
			{
				lcdbrightness+=4;
				if(lcdbrightness>LCD_BRIGHTNESS_MAX)
					lcdbrightness=LCD_BRIGHTNESS_MAX;
			} else
			if(this->focus==bcontrast)
			{
				lcdcontrast+=2;
				if(lcdcontrast>LCD_CONTRAST_MAX)
					lcdcontrast=LCD_CONTRAST_MAX;
			} else
			break;
		} else
		if(event.action == &i_cursorActions->left)
		{
			if(this->focus==bbrightness)
			{
				lcdbrightness-=4;
				if(lcdbrightness<(LCD_BRIGHTNESS_MIN+1))
					lcdbrightness=LCD_BRIGHTNESS_MIN+1;
			} else
			if(this->focus==bcontrast)
			{
				lcdcontrast-=2;
				if(lcdcontrast<(LCD_CONTRAST_MIN+1))
					lcdcontrast=LCD_CONTRAST_MIN+1;
			} else
			break;
		} else
			break;
		update();
		return(1);
	default:break;
	}

	return eWindow::eventHandler(event);
}

eZapLCDSetup::eZapLCDSetup(): eWindow(0)
{
	setText("LCD Setup");
	move(ePoint(150, 136));
	resize(eSize(400, 250));

	int fd=eSkin::getActive()->queryValue("fontsize", 20);
	
	eConfig::getInstance()->getKey("/ezap/lcd/brightness", lcdbrightness);
	eConfig::getInstance()->getKey("/ezap/lcd/contrast", lcdcontrast);

	bbrightness=new eButton(this);
	bbrightness->setText(_("Brightness:"));
	bbrightness->move(ePoint(20, 40));
	bbrightness->resize(eSize(100, fd+4));

	bcontrast=new eButton(this);
	bcontrast->setText(_("Contrast:"));
	bcontrast->move(ePoint(20, 80));
	bcontrast->resize(eSize(100, fd+4));

	p_brightness=new eProgress(this);
	p_brightness->setName("brightness");
	p_brightness->move(ePoint(130, 40));
	p_brightness->resize(eSize(240, fd+4));

	p_contrast=new eProgress(this);
	p_contrast->setName("contrast");
	p_contrast->move(ePoint(130, 80));
	p_contrast->resize(eSize(240, fd+4));

	update();

	ok=new eButton(this);
	ok->setText(_("[SAVE]"));
	ok->move(ePoint(20, 125));
	ok->resize(eSize(90, fd+4));
	
	CONNECT(ok->selected, eZapLCDSetup::okPressed);

	abort=new eButton(this);
	abort->setText(_("[ABORT]"));
	abort->move(ePoint(140, 125));
	abort->resize(eSize(100, fd+4));

	CONNECT(abort->selected, eZapLCDSetup::abortPressed);

}

eZapLCDSetup::~eZapLCDSetup()
{
}

void eZapLCDSetup::fieldSelected(int *number)
{
	focusNext(eWidget::focusDirNext);
}

void eZapLCDSetup::okPressed()
{
	eConfig::getInstance()->setKey("/ezap/lcd/brightness", lcdbrightness);
	eConfig::getInstance()->setKey("/ezap/lcd/contrast", lcdcontrast);
	close(1);
}

void eZapLCDSetup::abortPressed()
{
	eConfig::getInstance()->getKey("/ezap/lcd/brightness", lcdbrightness);
	eConfig::getInstance()->getKey("/ezap/lcd/contrast", lcdcontrast);
	eDBoxLCD::getInstance()->setLCDParameter(lcdbrightness, lcdcontrast);
	close(0);
}
