/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	Kommentar:

	Diese GUI wurde von Grund auf neu programmiert und sollte nun vom
	Aufbau und auch den Ausbaumoeglichkeiten gut aussehen. Neutrino basiert
	auf der Client-Server Idee, diese GUI ist also von der direkten DBox-
	Steuerung getrennt. Diese wird dann von Daemons uebernommen.


	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gui/widget/lcdcontroler.h>

#include <driver/fontrenderer.h>
#include <driver/rcinput.h>

#include <gui/color.h>
#include <gui/widget/messagebox.h>

#include <global.h>
#include <neutrino.h>

#include <math.h>


#define BRIGHTNESSFACTOR 2.55
#define CONTRASTFACTOR 0.63


CLcdControler::CLcdControler(const neutrino_locale_t Name, CChangeObserver* Observer)
{
	frameBuffer = CFrameBuffer::getInstance();
	hheight     = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	mheight     = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
	observer = Observer;
	name = Name;
	width = 390;
	height = hheight+ mheight* 4+ +mheight/2;
	x=((720-width) >> 1) -20;
	y=(576-height)>>1;

	contrast = CLCD::getInstance()->getContrast();
	brightness = CLCD::getInstance()->getBrightness();
	brightnessstandby = CLCD::getInstance()->getBrightnessStandby();
}

void CLcdControler::setLcd()
{
//	printf("contrast: %d brightness: %d brightness standby: %d\n", contrast, brightness, brightnessstandby);
	CLCD::getInstance()->setBrightness(brightness);
	CLCD::getInstance()->setBrightnessStandby(brightnessstandby);
	CLCD::getInstance()->setContrast(contrast);
}

int CLcdControler::exec(CMenuTarget* parent, const std::string &)
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	int selected, res = menu_return::RETURN_REPAINT;
	unsigned int contrast_alt, brightness_alt, brightnessstandby_alt, autodimm_alt;

	if (parent)
	{
		parent->hide();
	}
	contrast_alt = CLCD::getInstance()->getContrast();
	brightness_alt = CLCD::getInstance()->getBrightness();
	brightnessstandby_alt = CLCD::getInstance()->getBrightnessStandby();
	autodimm_alt = CLCD::getInstance()->getAutoDimm();
	selected = 0;

	setLcd();
	CLCD::getInstance()->setAutoDimm(0);	// autodimm deactivated to control and see the real settings
	paint();

	unsigned long long timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_MENU]);

	bool loop=true;
	while (loop)
	{
		g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd, true );

		if ( msg <= CRCInput::RC_MaxRC )
			timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_MENU]);

		switch ( msg )
		{
			case CRCInput::RC_down:
				if(selected<3)	// max entries
				{
					paintSlider(x+10, y+ hheight, contrast, CONTRASTFACTOR, "lcdcontroler.contrast", "contrast", false);
					paintSlider(x+10, y+ hheight+ mheight, brightness, BRIGHTNESSFACTOR, "lcdcontroler.brightness", "brightness", false);
					paintSlider(x+10, y+ hheight+ mheight* 2, brightnessstandby, BRIGHTNESSFACTOR, "lcdcontroler.brightnessstandby", "brightnessstandby", false);
					selected++;
					switch (selected)
					{
						case 0:
							paintSlider(x+ 10, y+ hheight, contrast, CONTRASTFACTOR, "lcdcontroler.contrast", "contrast", true);
							break;
						case 1:
							paintSlider(x+ 10, y+ hheight+ mheight, brightness, BRIGHTNESSFACTOR, "lcdcontroler.brightness", "brightness", true);
							break;
						case 2:
							paintSlider(x+ 10, y+ hheight+ mheight* 2, brightnessstandby, BRIGHTNESSFACTOR, "lcdcontroler.brightnessstandby", "brightnessstandby", true);
							CLCD::getInstance()->setMode(CLCD::MODE_STANDBY);
							break;
						case 3:
							frameBuffer->paintBoxRel(x, y+hheight+mheight*3+mheight/2, width, mheight, COL_MENUCONTENTSELECTED_PLUS_0);
							g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+10, y+hheight+mheight*4+mheight/2, width, g_Locale->getText("options.default"), COL_MENUCONTENTSELECTED, 0, true); // UTF-8
							break;
					}
				}
				break;

			case CRCInput::RC_up:
				if(selected>0)
				{
					paintSlider(x+10, y+hheight, contrast, CONTRASTFACTOR, "lcdcontroler.contrast", "contrast", false);
					paintSlider(x+10, y+hheight+mheight, brightness, BRIGHTNESSFACTOR, "lcdcontroler.brightness", "brightness", false);
					paintSlider(x+10, y+hheight+mheight*2, brightnessstandby, BRIGHTNESSFACTOR, "lcdcontroler.brightnessstandby", "brightnessstandby", false);
					selected--;
					switch (selected)
					{
						case 0:
							paintSlider(x+10, y+hheight, contrast, CONTRASTFACTOR, "lcdcontroler.contrast", "contrast", true);
							break;
						case 1:
							paintSlider(x+10, y+hheight+mheight, brightness, BRIGHTNESSFACTOR, "lcdcontroler.brightness", "brightness", true);
							CLCD::getInstance()->setMode(CLCD::MODE_TVRADIO);
							break;
						case 2:
							paintSlider(x+10, y+hheight+mheight*2, brightnessstandby, BRIGHTNESSFACTOR, "lcdcontroler.brightnessstandby", "brightnessstandby", true);
							CLCD::getInstance()->setMode(CLCD::MODE_STANDBY);
							frameBuffer->paintBoxRel(x, y+hheight+mheight*3+mheight/2, width, mheight, COL_MENUCONTENT_PLUS_0);
							g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+10, y+hheight+mheight*4+mheight/2, width, g_Locale->getText("options.default"), COL_MENUCONTENT, 0, true); // UTF-8
							break;
						case 3:
							break;
					}
				}
				break;

			case CRCInput::RC_right:
				switch (selected)
				{
					case 0:
						if (contrast < 63)
						{
							int val = lrint(::log(contrast+1));

							if (contrast + val < 63)
								contrast += val;
							else
								contrast = 63;

							paintSlider(x+10, y+hheight, contrast, CONTRASTFACTOR, "lcdcontroler.contrast", "contrast", true);
							setLcd();
						}
						break;
					case 1:
						if (brightness < 255)
						{
							if (brightness < 250)
								brightness += 5;
							else
								brightness = 255;

							paintSlider(x+10, y+hheight+mheight, brightness, BRIGHTNESSFACTOR, "lcdcontroler.brightness", "brightness", true);
							setLcd();
						}
						break;
					case 2:
						if (brightnessstandby < 255)
						{
							if (brightnessstandby < 250)
								brightnessstandby += 5;
							else
								brightnessstandby = 255;

							paintSlider(x+10, y+hheight+mheight*2, brightnessstandby, BRIGHTNESSFACTOR, "lcdcontroler.brightnessstandby", "brightnessstandby", true);
							setLcd();
						}
						break;
				}
				break;

			case CRCInput::RC_left:
				switch (selected)
				{
					case 0:
						if (contrast > 0)
						{
							contrast -= lrint(::log(contrast));

							paintSlider(x+10, y+hheight, contrast, CONTRASTFACTOR, "lcdcontroler.contrast", "contrast", true);
							setLcd();
						}
						break;
					case 1:
						if (brightness > 0)
						{
							if (brightness > 5)
								brightness -= 5;
							else
								brightness = 0;

							paintSlider(x+10, y+hheight+mheight, brightness, BRIGHTNESSFACTOR, "lcdcontroler.brightness", "brightness", true);
							setLcd();
						}
						break;
					case 2:
						if (brightnessstandby > 0)
						{
							if (brightnessstandby > 5)
								brightnessstandby -= 5;
							else
								brightnessstandby = 0;

							paintSlider(x+10, y+hheight+mheight*2, brightnessstandby, BRIGHTNESSFACTOR, "lcdcontroler.brightnessstandby", "brightnessstandby", true);
							setLcd();
						}
						break;
				}
				break;

			case CRCInput::RC_home:
				if ( ( (contrast != contrast_alt) || (brightness != brightness_alt) || (brightnessstandby != brightnessstandby_alt) ) &&
				     (ShowMsgUTF(name, g_Locale->getText(LOCALE_MESSAGEBOX_DISCARD), CMessageBox::mbrYes, CMessageBox::mbYes | CMessageBox::mbCancel) == CMessageBox::mbrCancel)) // UTF-8
					break;

				// sonst abbruch...
				contrast = contrast_alt;
				brightness = brightness_alt;
				brightnessstandby = brightnessstandby_alt;
				setLcd();
				loop = false;
				break;

			case CRCInput::RC_ok:
				if (selected==3)	// default Werte benutzen
				{
					brightness		= DEFAULT_LCD_BRIGHTNESS;
					brightnessstandby	= DEFAULT_LCD_STANDBYBRIGHTNESS;
					contrast		= DEFAULT_LCD_CONTRAST;
					selected		= 0;
					setLcd();
					paint();
					break;
				}

			case CRCInput::RC_timeout:
				loop = false;
				break;

			default:
				if ( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all )
				{
					loop = false;
					res = menu_return::RETURN_EXIT_ALL;
				}
		}
	}

	CLCD::getInstance()->setAutoDimm(autodimm_alt);
	hide();

	if(observer)
		observer->changeNotify(name, NULL);

	return res;
}

void CLcdControler::hide()
{
	frameBuffer->paintBackgroundBoxRel(x,y, width,height);
}

void CLcdControler::paint()
{
	CLCD::getInstance()->setMode(CLCD::MODE_TVRADIO);

	frameBuffer->paintBoxRel(x,y, width,hheight, COL_MENUHEAD_PLUS_0);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->RenderString(x+10,y+hheight, width, g_Locale->getText(name), COL_MENUHEAD, 0, true); // UTF-8
	frameBuffer->paintBoxRel(x,y+hheight, width,height-hheight, COL_MENUCONTENT_PLUS_0);

	paintSlider(x+10, y+hheight, contrast, CONTRASTFACTOR, "lcdcontroler.contrast", "contrast", true);
	paintSlider(x+10, y+hheight+mheight, brightness, BRIGHTNESSFACTOR, "lcdcontroler.brightness", "brightness", false);
	paintSlider(x+10, y+hheight+mheight*2, brightnessstandby, BRIGHTNESSFACTOR, "lcdcontroler.brightnessstandby", "brightnessstandby",false);

	frameBuffer->paintHLineRel(x+10, width-20, y+hheight+mheight*3+mheight/4, COL_MENUCONTENT_PLUS_3);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+10, y+hheight+mheight*4+mheight/2, width, g_Locale->getText("options.default"), COL_MENUCONTENT, 0, true); // UTF-8
}

void CLcdControler::paintSlider(int x, int y, unsigned int spos, float factor, const char * const text, const char * const iconname, bool selected)
{
	int startx = 200;
	char wert[5];

	frameBuffer->paintBoxRel(x + startx, y, 120, mheight, COL_MENUCONTENT_PLUS_0);
	frameBuffer->paintIcon("volumebody.raw", x + startx, y+2+mheight/4);
	std::string iconfile = "volumeslider2";
	if (selected)
		iconfile += "blue";
	iconfile +=".raw";
	frameBuffer->paintIcon(iconfile, (int)(x + (startx+3)+(spos / factor)), y+mheight/4);

	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x, y+mheight, width, g_Locale->getText(text), COL_MENUCONTENT, 0, true); // UTF-8
	sprintf(wert, "%3d", spos); // UTF-8 encoded
	frameBuffer->paintBoxRel(x + startx + 120 + 10, y, 50, mheight, COL_MENUCONTENT_PLUS_0);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x + startx + 120 + 10, y+mheight, width, wert, COL_MENUCONTENT, 0, true); // UTF-8
}
