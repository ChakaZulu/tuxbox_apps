/*
 * $Id: lcdmenu.cpp,v 1.13 2002/01/03 17:18:59 obi Exp $
 *
 * A startup menu for the d-box 2 linux project
 *
 * Copyright (C) 2001 Andreas Oberritter <obi@saftware.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Log: lcdmenu.cpp,v $
 * Revision 1.13  2002/01/03 17:18:59  obi
 * some reorganization.
 * removed buffer classes.
 * removed threading from rcinput.
 * moved timer from rc-class to menu-class.
 * manually add fonts to fontRendererClass.
 * fixed small bugs that noone would ever find and added new ones.
 *
 * Revision 1.12  2001/12/15 12:49:21  obi
 * do not protect any entry with pin by default
 *
 * Revision 1.11  2001/12/14 16:57:13  obi
 * power off after three pin failures
 *
 * Revision 1.10  2001/12/14 06:09:05  obi
 * moved fontrenderer and lcddisplay to a lib
 *
 * Revision 1.9  2001/12/10 06:25:34  obi
 * changed config path to tuxbox default
 *
 * Revision 1.8  2001/12/05 21:49:31  obi
 * - fix for more than four menu entries
 *
 * Revision 1.7  2001/12/05 20:35:29  obi
 * - fix save method in configManager
 * - remember last selection
 * - store default_entry c-like, beginnig from 0
 *
 * Revision 1.6  2001/11/16 20:49:04  obi
 * - option show_numbers now works with config file too
 *
 * Revision 1.5  2001/11/16 20:05:36  obi
 * - pin does really work now, can be checked, changed and saved :)
 * - default selection is visible again on startup
 * - fontsize parameter might work better now
 *
 * Revision 1.4  2001/11/16 14:07:25  McClean
 * fixed missing index-check
 *
 * Revision 1.3  2001/11/15 22:37:47  obi
 * fix pin protection settings
 * actually display words not dots :-)
 *
 * Revision 1.2  2001/11/14 22:42:06  obi
 * fall back to defaults correctly
 *
 */

#include "lcdmenu.h"

CLCDMenu *CLCDMenu::instance;

CLCDMenu::CLCDMenu(string configFilename)
{
	rc = new CRCInput();

	fontRenderer = new fontRenderClass(this);
	fontRenderer->AddFont(FONTDIR "/Arial_Bold.ttf");
	fontRenderer->InitFontCache();

	entryCount = 0;

	config = new CConfigManager();
	if (!config->loadConfig(configFilename))
	{
		/* defaults */
		config->setInt("font_size", 12);
		config->setInt("line_spacing", 3);
		config->setInt("default_entry", 0);
		config->setInt("text_align", 0);
		config->setBool("show_numbers", false);
		config->setString("pin", string("__lUISdFwUYjg"));
		addEntry("EliteDVB");
		addEntry("Neutrino");
		addEntry("Lcars");
		addEntry("Maintenance");
		config->setStringVector("menu_items", entries);
		//addPinProtection(3);
		config->setIntVector("pin_protect", pinEntries);
	}

#ifdef DEBUG
	config->dump();
#endif

	/* user defineable settings */
	fontSize = config->getInt("font_size");
	lineSpacing = config->getInt("line_spacing");
	defaultEntry = config->getInt("default_entry");
	textAlign = config->getInt("text_align");
	showNumbers = config->getBool("show_numbers");
	cryptedPin = config->getString("pin");
	entries = config->getStringVector("menu_items");
	pinEntries = config->getIntVector("pin_protect");
	entryCount = entries.size();

	if (showNumbers)
		addNumberPrefix();

	newSalt = getNewSalt();
	menuFont = fontRenderer->getFont("Arial", "Bold", fontSize);
	pinFailures = 0;
}

void CLCDMenu::addNumberPrefix()
{
	int i;
	for(i = 0; i < entryCount; i++)
	{
		char *entryCountChar = (char *) malloc(sizeof(i+1)+2);
		sprintf(entryCountChar, "%d) ", i+1);
		entries[i] = string(entryCountChar) + entries[i];
	}
}

const char *CLCDMenu::getCurrentSalt()
{
	return cryptedPin.substr(0, 2).c_str();
}

char *CLCDMenu::getNewSalt()
{
	char *salt = (char *) malloc(2);
	FILE *fd = fopen("/dev/urandom", "r");
	fread(salt, 1, 2, fd);
	fclose(fd);
	return salt;
}

CLCDMenu::~CLCDMenu()
{
	delete rc;
	delete menuFont;
	delete fontRenderer;
	delete config;
}

void CLCDMenu::addEntry(string title)
{
	entryCount++;
	entries.push_back(title);
}

void CLCDMenu::addPinProtection(int index)
{
	pinEntries.push_back(index + 1);
}

bool CLCDMenu::selectEntry(int index)
{
	if ((index < entryCount) && (index >= 0))
	{
		selectedEntry = index;
		drawMenu();

		int border = (entryCount * fontSize + (entryCount - 1) * lineSpacing - 63) / -2;
		int top = border + (index+1) * fontSize + (index) * lineSpacing;

		draw_fill_rect (0, top-fontSize-1, 119, top+3, CLCDDisplay::PIXEL_ON);
		drawString(entries[index], top, textAlign, CLCDDisplay::PIXEL_OFF);

		update();
#ifdef DEBUG
		cout << "selectEntry(" << index << "): " << entries[index] << endl;
#endif
		return true;
	}
	else
	{
		return false;
	}
}

bool CLCDMenu::drawMenu()
{
	if (entryCount > 0)
	{
		int i, top;
		int border = (entryCount * fontSize + (entryCount - 1) * lineSpacing - 63) / -2;

		draw_fill_rect (0, 0, 119, 63, CLCDDisplay::PIXEL_OFF);
		for (i=0; i<entryCount; i++)
		{
			top = border + (i+1) * fontSize + (i) * lineSpacing;
#ifdef DEBUG
			cout << "drawString(\"" << entries[i] << "\"," << top << ",";
			cout << textAlign << ",CLCDDisplay::PIXEL_ON)" << endl;
#endif
			drawString(entries[i], top, textAlign, CLCDDisplay::PIXEL_ON);
		}
		
		update();
		return true;
	}
	else
	{
		return false;
	}
}

bool CLCDMenu::drawString(string text, int top, int align, int color)
{
    int left, maxWidth;
    
    int width = menuFont->getRenderWidth(text.c_str()) + fontSize/2;

    if (align == 1)
    {
	left = (120 - width) / 2;
	maxWidth = 120;
    }
    else
    {
        left = 3;
	maxWidth = 120-2*left;
    }
    
    if (width > maxWidth)
    {
#ifdef DEBUG
	cerr << "string exceeded " << maxWidth << " pixels. width: " << width << endl;
#endif
        return false;
    }
    else
    {
#ifdef DEBUG
	cout << "menufont->RenderString(" << left << "," << top << "," << width;
	cout << ",\"" << text.c_str() << "\"," << color << ",0)" << endl;
#endif
	menuFont->RenderString(left, top, width, text.c_str(), color, 0);
        return true;
    }
}

void CLCDMenu::timeout(int signal)
{
	CLCDMenu *menu = getInstance();
	if (menu->isPinProtected(menu->selectedEntry))
	{
		/*
		 * menu item is protected with a pin
		 * timeout does not make sense here
		 */
	}
	else
	{
		menu->draw_fill_rect (0, 0, 119, 63, CLCDDisplay::PIXEL_OFF);
		exit(menu->selectedEntry);
	}
}

bool CLCDMenu::rcLoop()
{
	int timeoutValue = 10; // sekunden
	bool selected = false;

	signal(SIGALRM, &timeout);

	while (!selected)
	{
		alarm(timeoutValue);

		int pressedKey = rc->getKey();

		switch (pressedKey)
		{
			/* 1-9: number keys */
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
			if (pressedKey <= entryCount)
				selected = selectEntry(pressedKey - 1);
			break;

		case 11: /* left arrow */
		case 12: /* up arrow */
			if (selectedEntry > 0)
				selectEntry(selectedEntry - 1);
			break;

		case 10: /* right arrow */
		case 13: /* down arrow */
			if (selectedEntry < entryCount)
				selectEntry(selectedEntry + 1);
			break;

		//case -1: /* timeout */
		case 14: /* ok button */
			selected = true;
			break;

		case 23: /* question mark */
			changePin();
			drawMenu();
			selectEntry(selectedEntry);
			break;
		default:
#ifdef DEBUG
			cout << "pressedKey: " << pressedKey << endl;
#endif
			break;
		}

		/* check pin if selected entry is protected  */
		if ((selected) && (isPinProtected(selectedEntry)) && (!checkPin("Enter PIN")))
		{
			drawMenu();
			selectEntry(selectedEntry);
			selected = false;
		}
	}
	return true;
}

bool CLCDMenu::isPinProtected(int entry)
{
	unsigned int i;
	for (i = 0; i < pinEntries.size(); i++)
	{
		if (pinEntries[i] - 1 == entry)
			return true;
	}
	return false;
}

bool CLCDMenu::changePin()
{
	/* wenn der alte pin ueberprueft wurde, ... */
	if (checkPin("Old PIN"))
	{
		/* ... der neue pin eingegeben wurde ... */
		string newCryptedPin = pinScreen("New PIN", true);

		/* ... und zu sicherheit nochmal verifiziert wurde ... */
		if (newCryptedPin == pinScreen("Confirm PIN", true))
		{
			/* ... dann kann der alte pin durch den neuen ersetzt werden. */
			config->setString("pin", newCryptedPin);
			cryptedPin = config->getString("pin");

			/* und die welt soll erfahren dass die config veraendert wurde. */
			config->setModifiedFlag(true);

			/* get salt for next password */
			newSalt = getNewSalt();

			// TODO: notify successful change via lcd
#ifdef DEBUG
			cout << "pin changed successfully." << endl;
#endif
			return true;
		}
		else
			return false;
	}
	else
		return false;
}

string CLCDMenu::pinScreen(string title, bool isNewPin)
{
    string pin;

    /* clear display */
    draw_fill_rect (0, 0, 119, 63, CLCDDisplay::PIXEL_OFF);

    /* render title string */
    drawString(title, fontSize+1, CENTERED, CLCDDisplay::PIXEL_ON);
    drawString("_ _ _ _", 3*fontSize, CENTERED, CLCDDisplay::PIXEL_ON);
    update();

    int i, pin_length = 4;
    int left = 120 - (pin_length * fontSize * 7/4); //wie das wohl ausschaut?

    for (i = 0; i < pin_length; i++)
    {
	char *digit = (char *) malloc(1);
	sprintf(digit, "%d", rc->getKey());
	pin += string(digit);

#ifdef DEBUG
	cout << "pin[" << i << "]=" << pin[i] << endl;
	cout << "menuFont->RenderString(" << left << "," << 3*fontSize << "," << fontSize << ",*,CLCDDisplay::PIXEL_ON, 0)" << endl;
#endif
	menuFont->RenderString(left, 3*fontSize, fontSize, "*", CLCDDisplay::PIXEL_ON, 0);

	update();
	left+=fontSize;
    }

    if (isNewPin)
        return string(crypt(pin.c_str(), newSalt));
    else
        return string(crypt(pin.c_str(), getCurrentSalt()));
}

bool CLCDMenu::checkPin(string title)
{
    if (cryptedPin != pinScreen(title, false))
    {
        /* TODO: complain about invalid pin on lcd */
#ifdef DEBUG
	cout << "invalid pin entered." << endl;
#endif
	if (pinFailures >= 2)
		poweroff();
	else
		pinFailures++;

        return false;
    }
    else
    {
#ifdef DEBUG
	cout << "pin accepted" << endl;
#endif
	pinFailures = 0;
        return true;
    }
}

void CLCDMenu::poweroff()
{
	int fd = open("/dev/dbox/fp0", O_RDWR);
	if (fd < 0)
	{
#ifdef DEBUG
		cerr << "unable to open /dev/dbox/fp0" << endl;
#endif
		return;
	}

	int val;
	if(ioctl(fd, FP_IOCTL_POWEROFF, &val) < 0)
	{
#ifdef DEBUG
		cerr << "poweroff failed." << endl;
#endif
		return;
	}
	close(fd);
}

