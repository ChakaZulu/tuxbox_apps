/*
 * $Id: lcdmenu.cpp,v 1.29 2007/04/12 17:49:10 chakazulu Exp $
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

#include <sys/reboot.h>

#include "lcdmenu.h"

//#define DEBUG

#include <iostream>

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
#if 0
		// With some effort, this code can be made to work in the
		// new setting.  I simpy do not think it is worth it.

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
#else
		  std::cerr << "[lcdmenu] " << configFilename
			    << " not found, exiting\n";
		  exit(255);
#endif
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
	entries_files = config->getStringVector("item_files");
	entries_execfiles = config->getStringVector("item_execfiles");
	entries_isGUI = config->getStringVector("item_isGUI");
	pinEntries = config->getInt32Vector("pin_protect");
	visibleEntries = config->getInt32("visible_entries");

	struct stat *buf = new(struct stat);
	noGUIs = 0;
	int one_GUI = -1;
	std::vector <std::string> real_entries;
        std::vector <std::string> real_entries_execfiles;

	if (entries.size() != entries_files.size() 
	    || entries.size() != entries_execfiles.size()
	    || entries.size() != entries_isGUI.size() ) {
	  std::cerr << "[lcdmenu] Sizes of menu_items, item_isGUI, item_files, and item_execfiles differ!\n";
	  noGUIs = 999;

	} else {
	  for (unsigned int i = 0; i < entries.size(); i++) {
	    if (stat(entries_files[i].c_str(), buf)) {
	      std::cout << "[lcdmenu] " << entries_files[i] << " was not found, entry " << entries[i] << " discarded\n";
	    } else {
	      real_entries.push_back(entries[i]);
	      real_entries_execfiles.push_back(entries_execfiles[i]);
	      if (entries_isGUI[i] == "true") {
		noGUIs++;
		one_GUI = entries.size();
	      }
	    }
	  }
	  entries = real_entries;
	  entries_execfiles = real_entries_execfiles;
	}

#ifdef DEBUG
	for (std::vector<std::string>::iterator i1 = entries.begin();
	     i1 != entries.end();
	     i1++)
	  std::cout << *i1 << "\n";

	std::cout << noGUIs << " GUIs were found\n";
#endif

	entryCount = entries.size();
	if (noGUIs == 1)
		defaultEntry = one_GUI;

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
		menu->exec();
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
			if (entryCount > 0)
				selected = selectEntry(0);
			break;
		case KEY_2:
			if (entryCount > 1)
				selected = selectEntry(1);
			break;
		case KEY_3:
			if (entryCount > 2)
				selected = selectEntry(2);
			break;
		case KEY_4:
			if (entryCount > 3)
				selected = selectEntry(3);
			break;
		case KEY_5:
			if (entryCount > 4)
				selected = selectEntry(4);
			break;
		case KEY_6:
			if (entryCount > 5)
				selected = selectEntry(5);
			break;
		case KEY_7:
			if (entryCount > 6)
				selected = selectEntry(6);
			break;
		case KEY_8:
			if (entryCount > 7)
				selected = selectEntry(7);
			break;
		case KEY_9:
			if (entryCount > 8)
				selected = selectEntry(8);
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
	    char digit = 0;
	    int code = rc->getKey();
	    
	    switch (code)
	    {       
	    case KEY_0:
		    digit = '0';
		    break;
	    case KEY_1:
		    digit = '1';
		    break;
	    case KEY_2:
		    digit = '2';
		    break;
	    case KEY_3:
		    digit = '3';
		    break;
	    case KEY_4:
		    digit = '4';
		    break;
	    case KEY_5:
		    digit = '5';
		    break;
	    case KEY_6:
		    digit = '6';
		    break;
	    case KEY_7:
		    digit = '7';
		    break;
	    case KEY_8:
		    digit = '8';
		    break;
	    case KEY_9:
		    digit = '9';
		    break;
	    }

	    if (digit != 0)
		    pin += digit;

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
	reboot(RB_POWER_OFF);
	exit(0);
}

void CLCDMenu::exec()
{
  if (entries_execfiles.size() >= (unsigned) selectedEntry + 1) {
    std::string execfilename = entries_execfiles[selectedEntry];
    struct stat *buf = new(struct stat);
    if (stat(execfilename.c_str(), buf)) {
      std::cout << "[lcdmenu] " << execfilename
		<< " was not found, not exec-ing\n";
    }

    execl(execfilename.c_str(), execfilename.c_str(), NULL);
  }
}
