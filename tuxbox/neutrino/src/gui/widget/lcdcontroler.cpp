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

#define BRIGHTNESSFACTOR 255
#define CONTRASTFACTOR 63


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

#ifdef HAVE_TRIPLEDRAGON
#define NUM_LCD_SLIDERS 1
#else
#define NUM_LCD_SLIDERS 3
#endif

int CLcdControler::exec(CMenuTarget* parent, const std::string &)
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	int c_rad_mid = RADIUS_MID;
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
		g_RCInput->getMsgAbsoluteTimeout(&msg, &data, &timeoutEnd);
		unsigned char step = 1;

		if ( msg <= CRCInput::RC_MaxRC )
		{
			if (msg & CRCInput::RC_Repeat)
				step = 5;
			timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_MENU]);
		}

		switch (msg & ~CRCInput::RC_Repeat)
		{
		case CRCInput::RC_down:
			if(selected < NUM_LCD_SLIDERS) // max entries
			{
				paintSlider(x + 10, y + hheight              , contrast         , CONTRASTFACTOR  , LOCALE_LCDCONTROLER_CONTRAST         , false);
#ifndef HAVE_TRIPLEDRAGON
				paintSlider(x + 10, y + hheight + mheight    , brightness       , BRIGHTNESSFACTOR, LOCALE_LCDCONTROLER_BRIGHTNESS       , false);
				paintSlider(x + 10, y + hheight + mheight * 2, brightnessstandby, BRIGHTNESSFACTOR, LOCALE_LCDCONTROLER_BRIGHTNESSSTANDBY, false);
#endif
				selected++;
				switch (selected)
				{
				case 0:
					paintSlider(x+ 10, y+ hheight, contrast, CONTRASTFACTOR, LOCALE_LCDCONTROLER_CONTRAST, true);
					break;
#ifndef HAVE_TRIPLEDRAGON
				case 1:
					paintSlider(x+ 10, y+ hheight+ mheight, brightness, BRIGHTNESSFACTOR, LOCALE_LCDCONTROLER_BRIGHTNESS, true);
					break;
				case 2:
					paintSlider(x+ 10, y+ hheight+ mheight* 2, brightnessstandby, BRIGHTNESSFACTOR, LOCALE_LCDCONTROLER_BRIGHTNESSSTANDBY, true);
					CLCD::getInstance()->setMode(CLCD::MODE_STANDBY);
					break;
#endif
				default:
					frameBuffer->paintBoxRel(x, y + hheight + mheight * 3 + mheight / 2, width, mheight, COL_MENUCONTENTSELECTED_PLUS_0, c_rad_mid, CORNER_BOTTOM);
					g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+10, y+hheight+mheight*4+mheight/2, width, g_Locale->getText(LOCALE_OPTIONS_DEFAULT), COL_MENUCONTENTSELECTED, 0, true); // UTF-8
					break;
				}
			}
			break;

		case CRCInput::RC_up:
			if (selected > 0)
			{
				paintSlider(x + 10, y + hheight              , contrast         , CONTRASTFACTOR  , LOCALE_LCDCONTROLER_CONTRAST         , false);
#ifndef HAVE_TRIPLEDRAGON
				paintSlider(x + 10, y + hheight + mheight    , brightness       , BRIGHTNESSFACTOR, LOCALE_LCDCONTROLER_BRIGHTNESS       , false);
				paintSlider(x + 10, y + hheight + mheight * 2, brightnessstandby, BRIGHTNESSFACTOR, LOCALE_LCDCONTROLER_BRIGHTNESSSTANDBY, false);
#endif
				selected--;
				switch (selected)
				{
				case 0:
					paintSlider(x+ 10, y+ hheight, contrast, CONTRASTFACTOR, LOCALE_LCDCONTROLER_CONTRAST, true);
#ifdef HAVE_TRIPLEDRAGON
					frameBuffer->paintBoxRel(x, y + hheight + mheight * 3 + mheight / 2, width, mheight, COL_MENUCONTENT_PLUS_0, c_rad_mid, CORNER_BOTTOM);
					g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+10, y+hheight+mheight*4+mheight/2, width, g_Locale->getText(LOCALE_OPTIONS_DEFAULT), COL_MENUCONTENT, 0, true); // UTF-8
#endif
					break;
#ifndef HAVE_TRIPLEDRAGON
				case 1:
					paintSlider(x+ 10, y+ hheight+ mheight, brightness, BRIGHTNESSFACTOR, LOCALE_LCDCONTROLER_BRIGHTNESS, true);
					CLCD::getInstance()->setMode(CLCD::MODE_TVRADIO);
					break;
				case 2:
					paintSlider(x+10, y+hheight+mheight*2, brightnessstandby, BRIGHTNESSFACTOR, LOCALE_LCDCONTROLER_BRIGHTNESSSTANDBY, true);
					CLCD::getInstance()->setMode(CLCD::MODE_STANDBY);
					frameBuffer->paintBoxRel(x, y + hheight + mheight * 3 + mheight / 2, width, mheight, COL_MENUCONTENT_PLUS_0, c_rad_mid, CORNER_BOTTOM);
					g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+10, y+hheight+mheight*4+mheight/2, width, g_Locale->getText(LOCALE_OPTIONS_DEFAULT), COL_MENUCONTENT, 0, true); // UTF-8
					break;
#endif
				default:
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
							contrast++;
							paintSlider(x+10, y+hheight, contrast, CONTRASTFACTOR, LOCALE_LCDCONTROLER_CONTRAST, true);
							setLcd();
						}
						break;
#ifndef HAVE_TRIPLEDRAGON
					case 1:
						brightness += step;
						if (brightness < step) // check for overflow.
							brightness = 255;
						paintSlider(x+10, y+hheight+mheight, brightness, BRIGHTNESSFACTOR, LOCALE_LCDCONTROLER_BRIGHTNESS, true);
						setLcd();
						break;
					case 2:
						brightnessstandby += step;
						if (brightnessstandby < step)
							brightnessstandby = 255;
						paintSlider(x+10, y+hheight+mheight*2, brightnessstandby, BRIGHTNESSFACTOR, LOCALE_LCDCONTROLER_BRIGHTNESSSTANDBY, true);
						setLcd();
						break;
#endif
				}
				break;

			case CRCInput::RC_left:
				switch (selected)
				{
					case 0:
						if (contrast > 0)
						{
							contrast--;
							paintSlider(x+10, y+hheight, contrast, CONTRASTFACTOR, LOCALE_LCDCONTROLER_CONTRAST, true);
							setLcd();
						}
						break;
#ifndef HAVE_TRIPLEDRAGON
					case 1:
						brightness -= step;
						if (brightness > 255 - step) // overflow
							brightness = 0;
						paintSlider(x+10, y+hheight+mheight, brightness, BRIGHTNESSFACTOR, LOCALE_LCDCONTROLER_BRIGHTNESS, true);
						setLcd();
						break;
					case 2:
						brightnessstandby -= step;
						if (brightnessstandby > 255 - step)
							brightnessstandby = 0;
						paintSlider(x+10, y+hheight+mheight*2, brightnessstandby, BRIGHTNESSFACTOR, LOCALE_LCDCONTROLER_BRIGHTNESSSTANDBY, true);
						setLcd();
						break;
#endif
				}
				break;

			case CRCInput::RC_home:
				if ( ( (contrast != contrast_alt) || (brightness != brightness_alt) || (brightnessstandby != brightnessstandby_alt) ) &&
				     (ShowLocalizedMessage(name, LOCALE_MESSAGEBOX_DISCARD, CMessageBox::mbrYes, CMessageBox::mbYes | CMessageBox::mbCancel) == CMessageBox::mbrCancel))
					break;

				// sonst abbruch...
				contrast = contrast_alt;
				brightness = brightness_alt;
				brightnessstandby = brightnessstandby_alt;
				setLcd();
				loop = false;
				break;

			case CRCInput::RC_ok:
				if (msg != CRCInput::RC_ok) // do not react on repeat...
					break;
				if (selected == NUM_LCD_SLIDERS)	// default Werte benutzen
				{
					brightness		= DEFAULT_LCD_BRIGHTNESS;
					brightnessstandby	= DEFAULT_LCD_STANDBYBRIGHTNESS;
					contrast		= DEFAULT_LCD_CONTRAST;
					selected		= 0;
					setLcd();
					paint();
					break;
				}
				/* else fallthrough */
			case CRCInput::RC_timeout:
				loop = false;
				break;

			default:
				if (CNeutrinoApp::getInstance()->handleMsg(msg, data) & messages_return::cancel_all)
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

	int c_rad_mid = RADIUS_MID;
	frameBuffer->paintBoxRel(x, y, width, hheight, COL_MENUHEAD_PLUS_0, c_rad_mid, CORNER_TOP);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->RenderString(x+10,y+hheight, width, g_Locale->getText(name), COL_MENUHEAD, 0, true); // UTF-8
	frameBuffer->paintBoxRel(x, y + hheight, width, height - hheight, COL_MENUCONTENT_PLUS_0, c_rad_mid, CORNER_BOTTOM);

	paintSlider(x+10, y+hheight, contrast, CONTRASTFACTOR, LOCALE_LCDCONTROLER_CONTRAST, true);
#ifndef HAVE_TRIPLEDRAGON
	paintSlider(x+10, y+hheight+mheight, brightness, BRIGHTNESSFACTOR, LOCALE_LCDCONTROLER_BRIGHTNESS, false);
	paintSlider(x+10, y+hheight+mheight*2, brightnessstandby, BRIGHTNESSFACTOR, LOCALE_LCDCONTROLER_BRIGHTNESSSTANDBY, false);
#endif

	frameBuffer->paintHLineRel(x+10, width-20, y+hheight+mheight*3+mheight/4, COL_MENUCONTENT_PLUS_3);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+10, y+hheight+mheight*4+mheight/2, width, g_Locale->getText(LOCALE_OPTIONS_DEFAULT), COL_MENUCONTENT, 0, true); // UTF-8
}

void CLcdControler::paintSlider(int _x, int _y, unsigned int spos, int factor, const neutrino_locale_t text, bool selected)
{
	int startx = 200;
	char wert[5];

	frameBuffer->paintBoxRel(_x + startx, _y, 120, mheight, COL_MENUCONTENT_PLUS_0);
	frameBuffer->paintIcon("volumebody.raw", _x + startx, _y+2+mheight/4);
	frameBuffer->paintIcon(selected ? "volumeslider2blue.raw" : "volumeslider2.raw", (int)(_x + (startx+3)+(100*spos / factor)), _y+mheight/4);

	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(_x, _y+mheight, width, g_Locale->getText(text), COL_MENUCONTENT, 0, true); // UTF-8
	sprintf(wert, "%3d", spos); // UTF-8 encoded
	frameBuffer->paintBoxRel(_x + startx + 120 + 10, _y, 50, mheight, COL_MENUCONTENT_PLUS_0);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(_x + startx + 120 + 10, _y+mheight, width, wert, COL_MENUCONTENT, 0, true); // UTF-8
}
