/*
 * $Id: lcdmenu.cpp,v 1.25 2003/05/21 16:32:11 thegoodguy Exp $
 *
 * A startup menu for the d-box 2 linux project
 *
 * Copyright (C) 2001, 2002 Andreas Oberritter <obi@tuxbox.org>
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
 */

#include "lcdmenu.h"

#ifdef DEBUG
#include <iostream>
#endif

CLCDMenu *CLCDMenu::instance;

CLCDMenu::CLCDMenu (std::string configFilename)
{
	rc = new CRCInput();

	fontRenderer = new LcdFontRenderClass(this);
	fontRenderer->AddFont(FONTDIR "/micron.ttf");
	fontRenderer->InitFontCache();

	entryCount = 0;

	config = new CConfigFile(',');
	if (!config->loadConfig(configFilename))
	{
		/* defaults */
		config->setInt32("font_size", 12);
		config->setInt32("line_spacing", 3);
		config->setInt32("default_entry", 0);
		config->setInt32("text_align", 0);
		config->setBool("show_numbers", false);
		config->setInt32("timeout", 10);
		config->setString("pin", std::string("__lUISdFwUYjg"));
		addEntry("Enigma");
		addEntry("Neutrino");
		addEntry("Lcars");
		addEntry("Maintenance");
		config->setStringVector("menu_items", entries);
		config->setInt32("visible_entries", 4);
		//addPinProtection(3);
		config->setInt32Vector("pin_protect", pinEntries);
	}

	/* user defineable settings */
	fontSize = config->getInt32("font_size");
	lineSpacing = config->getInt32("line_spacing");
	defaultEntry = config->getInt32("default_entry");
	textAlign = config->getInt32("text_align");
	showNumbers = config->getBool("show_numbers");
	timeoutValue = config->getInt32("timeout");
	cryptedPin = config->getString("pin");
	entries = config->getStringVector("menu_items");
	pinEntries = config->getInt32Vector("pin_protect");
	visibleEntries = config->getInt32("visible_entries");
	entryCount = entries.size();

	if (entryCount < visibleEntries)
		visibleEntries = entryCount;

	if (defaultEntry >= visibleEntries)
	{
		upperRow = defaultEntry - visibleEntries + 1;
	}
	else
	{
		upperRow = 0;
	}

	if (showNumbers)
		addNumberPrefix();

	newSalt = getNewSalt();
	menuFont = fontRenderer->getFont("Micron", "Regular", fontSize);
	pinFailures = 0;
}

void CLCDMenu::addNumberPrefix ()
{
	int i;
	for(i = 0; i < entryCount; i++)
	{
		char *entryCountChar = (char *) malloc(sizeof(i+1)+2);
		sprintf(entryCountChar, "%d) ", i+1);
		entries[i] = std::string(entryCountChar) + entries[i];
	}
}

const char *CLCDMenu::getCurrentSalt ()
{
	return cryptedPin.substr(0, 2).c_str();
}

char *CLCDMenu::getNewSalt ()
{
	char *salt = (char *) malloc(2);
	FILE *fd = fopen("/dev/urandom", "r");
	fread(salt, 1, 2, fd);
	fclose(fd);
	return salt;
}

CLCDMenu::~CLCDMenu ()
{
	delete rc;
	delete menuFont;
	delete fontRenderer;
	delete config;
}

void CLCDMenu::addEntry (std::string title)
{
	entryCount++;
	entries.push_back(title);
}

void CLCDMenu::addPinProtection (int index)
{
	pinEntries.push_back(index + 1);
}

bool CLCDMenu::selectEntry (int index)
{
	if ((index < entryCount) && (index >= 0))
	{
		if (index < upperRow)
		{
			upperRow--;
		}
		else if (index >= upperRow + visibleEntries)
		{
			upperRow++;
		}

		selectedEntry = index;
		drawMenu();

		int border = (visibleEntries * fontSize + (visibleEntries - 1) * lineSpacing - 63) / -2;
		int top = border + (index-upperRow+1) * fontSize + (index-upperRow) * lineSpacing;

		draw_fill_rect (0, top-fontSize-1, 119, top+3, CLCDDisplay::PIXEL_ON);
		drawString(entries[index], top, textAlign, CLCDDisplay::PIXEL_OFF);

		update();
		return true;
	}
	else
	{
		return false;
	}
}

bool CLCDMenu::drawMenu ()
{
	if (entryCount > 0)
	{
		int i, top;
		int border = (visibleEntries * fontSize + (visibleEntries - 1) * lineSpacing - 63) / -2;

		draw_fill_rect (0, 0, 119, 63, CLCDDisplay::PIXEL_OFF);
		for (i = 0; i < visibleEntries; i++)
		{
			top = border + (i+1) * fontSize + (i) * lineSpacing;
			drawString(entries[i + upperRow], top, textAlign, CLCDDisplay::PIXEL_ON);
		}
		
		update();
		return true;
	}
	else
	{
		return false;
	}
}

bool CLCDMenu::drawString (std::string text, int top, int align, int color)
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
        return false;
    }
    else
    {
	menuFont->RenderString(left, top, width, text.c_str(), color, 0);
        return true;
    }
}

void CLCDMenu::timeout (int signal)
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
		menu->update();
		exit(menu->selectedEntry);
	}
}

bool CLCDMenu::rcLoop ()
{
	bool selected = false;

	signal(SIGALRM, &timeout);

	while (!selected)
	{
		alarm(timeoutValue);

		int pressedKey = rc->getKey();

		switch (pressedKey)
		{
			/* 1-9: number keys */
		case KEY_1:
		case KEY_2:
		case KEY_3:
		case KEY_4:
		case KEY_5:
		case KEY_6:
		case KEY_7:
		case KEY_8:
		case KEY_9:
			if (pressedKey <= entryCount)
				selected = selectEntry(pressedKey - 1);
			break;

		case KEY_LEFT: /* left arrow */
		case KEY_UP: /* up arrow */
			if (selectedEntry > 0)
				selectEntry(selectedEntry - 1);
			break;

		case KEY_RIGHT: /* right arrow */
		case KEY_DOWN: /* down arrow */
			if (selectedEntry < entryCount)
				selectEntry(selectedEntry + 1);
			break;

		//case -1: /* timeout */
		case KEY_OK: /* ok button */
			selected = true;
			break;

		case KEY_HELP: /* question mark */
			changePin();
			drawMenu();
			selectEntry(selectedEntry);
			break;
		default:
#ifdef DEBUG
			std::cout << "pressedKey: " << pressedKey << std::endl;
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
			return true;
		}
		else
			return false;
	}
	else
		return false;
}

std::string CLCDMenu::pinScreen (std::string title, bool isNewPin)
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

	menuFont->RenderString(left, 3*fontSize, fontSize, "*", CLCDDisplay::PIXEL_ON, 0);

	update();
	left+=fontSize;
    }

    if (isNewPin)
        return string(crypt(pin.c_str(), newSalt));
    else
        return string(crypt(pin.c_str(), getCurrentSalt()));
}

bool CLCDMenu::checkPin (std::string title)
{
    if (cryptedPin != pinScreen(title, false))
    {
        /* TODO: complain about invalid pin on lcd */
	if (pinFailures >= 2)
		poweroff();
	else
		pinFailures++;

        return false;
    }
    else
    {
	pinFailures = 0;
        return true;
    }
}

void CLCDMenu::poweroff ()
{
	int fd = open("/dev/dbox/fp0", O_RDWR);
	if (fd < 0)
	{
		return;
	}

	if(ioctl(fd, FP_IOCTL_POWEROFF, 0) < 0)
	{
		close(fd);
		return;
	}
}

