/*
 * lcdmenu.cpp
 *
 * A startup menu for the d-box 2 linux project
 *
 * Copyright (C) 2001 Andreas Oberritter <obi@saftware.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#include "lcdmenu.h"

CLCDMenu::CLCDMenu()
{
#ifndef X86_BUILD
    rc = new CRCInput();
#endif /* X86_BUILD */
    fontRenderer = new fontRenderClass(this);
    entryCount = 0;
    menuFont = fontRenderer->getFont("Arial", "Bold", fontSize);

    config = new CConfigManager();
    if (config->loadConfig("/var/etc/lcdmenu.conf"))
    {
#ifdef X86_BUILD
	config->dump();
#endif /* XF86_BUILD */

	/* user defineable settings */
	fontSize = config->getInt("font_size");
	lineSpacing = config->getInt("line_spacing");
	defaultEntry = config->getInt("default_entry") - 1;
	textAlign = config->getInt("text_align");
	showNumbers = config->getBool("show_numbers");
	cryptedPin = config->getString("pin");
	entries = config->getStringVector("menu_items");
	pinEntries = config->getIntVector("pin_protect");
    }
    else
    {
	/* defaults */
	fontSize = 12;
	lineSpacing = 3;
	defaultEntry = 0;
	textAlign = 0;
	showNumbers = false;
	cryptedPin = string("__lUISdFwUYjg");
	
	addEntry("EliteDVB");
	addEntry("Neutrino");
	addEntry("Multicast Zap");
	addEntry("Maintenance");
	
	addPinProtection(4);
    }

    /* get salt from old password */
    strncpy(oldSalt, cryptedPin.c_str(), 2);

    /* get salt for new password */
    FILE *random = fopen("/dev/urandom", "r");
    fread(&newSalt, 1, 2, random);
    fclose(random);
}

CLCDMenu::~CLCDMenu()
{
#ifndef X86_BUILD
    delete rc;
#endif /* X86_BUILD */
    delete menuFont;
    delete fontRenderer;
    delete config;
}

void CLCDMenu::addEntry(string title)
{
    entryCount++;
    if ((showNumbers) && (entryCount < 10))
    {
	char *entryCountChar = (char *) malloc(3);
	sprintf(entryCountChar, "%d) ", entryCount);
	entries.push_back(entryCountChar + title);
    }
    else
    {
        entries.push_back(title);
    }
}

void CLCDMenu::addPinProtection(int index)
{
    pinEntries.push_back(index);
}

bool CLCDMenu::selectEntry(int index)
{
    if ((index <= 3) && (index >= 0))
    {
	selectedEntry = index;

	drawMenu();

	int border = (entryCount * fontSize + (entryCount - 1) * lineSpacing - 63) / -2;
	int top = border + (index+1) * fontSize + (index) * lineSpacing;

	draw_fill_rect (0, top-fontSize-1, 119, top+3, CLCDDisplay::PIXEL_ON);
	drawString(entries[index], top, textAlign, CLCDDisplay::PIXEL_OFF);

#ifndef X86_BUILD
	update();
#else
	cout << "selectEntry(" << index << "): " << entries[index] << endl;
#endif /* X86_BUILD */
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
#ifdef X86_BUILD
	    cout << "drawString(\"" << entries[i] << "\"," << top << "," << textAlign << ",CLCDDisplay::PIXEL_ON)" << endl;
#endif /* X86_BUILD */
	    drawString(entries[i], top, textAlign, CLCDDisplay::PIXEL_ON);
	}
#ifndef X86_BUILD
	update();
#endif /* X86_BUILD */

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
    
    int width = menuFont->getRenderWidth(text.c_str())+5;

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
	cout << "string exceeded " << maxWidth << " pixels. width: " << width << endl;
        return false;
    }
    else
    {
#ifdef X86_BUILD
    cout << "menufont->RenderString(" << left << "," << top << "," << width << ",\"" << text.c_str() << "\"," << color << ")" << endl;
#endif
	menuFont->RenderString(left, top, width, text.c_str(), color);
        return true;
    }
}

bool CLCDMenu::rcLoop()
{
    int timeoutValue = 100; // zehntelsekunden?

    bool selected = false;

    while (!selected)
    {
#ifndef X86_BUILD
	int pressedKey = rc->getKey(timeoutValue);
#else
	cout << "rc->getKey(" << timeoutValue << ")" << endl;
	int pressedKey = 4;
#endif /* X86_BUILD */

	switch (pressedKey)
	{
	    /* 0-9: number keys */
	    case 1:
	    case 2:
	    case 3:
	    case 4:
		selected = selectEntry(pressedKey-1);
		break;

	    case 11: /* left arrow */
	    case 12: /* up arrow */
		if (selectedEntry > 0)
		    selectEntry(selectedEntry-1);
		break;

	    case 10: /* right arrow */
	    case 13: /* down arrow */
		if (selectedEntry < 3)
		    selectEntry(selectedEntry+1);
		break;

	    case -1: /* timeout */
	    case 14: /* ok button */
		selected = true;
		break;

	    case 23: /* question mark */
		changePin();
		drawMenu();
		selectEntry(selectedEntry);
		break;

	    default:
		cout << "pressedKey: " << pressedKey << endl;
	}

	/* check pin for maintenance mode */
	if ((selected) && (isPinProtected(selectedEntry)) && (!checkPin("Enter PIN")))
	{
	    selectEntry(defaultEntry);
	    selected = false;
	}
    }
    return true;
}

bool CLCDMenu::isPinProtected(int entry)
{
	unsigned int i;
	for (i=0; i<pinEntries.size(); i++)
	{
		if (pinEntries[i] == entry)
		{
			return true;
		}
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
	    cryptedPin = newCryptedPin;
	    config->setModifiedFlag();
	    // TODO: notify successful change via lcd
	    printf("pin changed successfully.\n");
	    return true;
	}
	else
	{
		return false;
	}
    }
    else
    {
	return false;
    }
}

string CLCDMenu::pinScreen(string title, bool isNewPin)
{
    string pin = "";

    /* clear display */
    draw_fill_rect (0, 0, 119, 63, CLCDDisplay::PIXEL_OFF);

    /* render title string */
    drawString(title, fontSize+1, CENTERED, CLCDDisplay::PIXEL_ON);
    drawString("_ _ _ _", 3*fontSize, CENTERED, CLCDDisplay::PIXEL_ON);
#ifndef X86_BUILD
    update();
#endif /* X86_BUILD */

    int i, pin_length = 4;
    int left = 120 - (pin_length * fontSize * 7/4); //wie das wohl ausschaut?

    for (i = 0; i < pin_length; i++)
    {

#ifndef X86_BUILD
	pin += rc->getKey(300);
#else
	pin[i] = '0';
	cout << "pin[" << i << "]=" << pin[i] << endl;
#endif /* X86_BUILD */

	menuFont->RenderString(left, 3*fontSize, fontSize, "*", CLCDDisplay::PIXEL_ON);
#ifdef X86_BUILD
    cout << "menuFont->RenderString(" << left << "," << 3*fontSize << "," << fontSize << ",*,CLCDDisplay::PIXEL_ON)" << endl;
#else
	update();
#endif /* X86_BUILD */
	left+=fontSize;
    }

    if (isNewPin)
    {
        return string(crypt(pin.c_str(), newSalt));
    }
    else
    {
        return string(crypt(pin.c_str(), oldSalt));
    }
}

bool CLCDMenu::checkPin(string title)
{
    if (cryptedPin ==  pinScreen(title, false))
    {
        /* TODO: complain about invalid pin on lcd */
	cout << "invalid pin entered." << endl;
        return false;
    }
    else
    {
        return true;
    }
}

int main(int argc, char **argv)
{
    /* create menu instance */
    CLCDMenu *menu = new CLCDMenu();

    /* draw the menu */
    menu->drawMenu();

    /* select default entry */
    menu->selectEntry(menu->getDefaultEntry());

    /* get command from remote control */
    menu->rcLoop();
    
    if (menu->getConfig()->getModifiedFlag())
    {
	cout << "saving configuration" << endl;

#ifndef X86_BUILD
	menu->getConfig()->dumpToFile("lcdmenu.conf");
#endif /* XF86_BUILD */

	/* reset modified flag */
	menu->getConfig()->setModifiedFlag(false);
    }

#ifdef X86_BUILD
    menu->getConfig()->dumpToFile("lcdmenu.conf.x86");
#endif /* X86_BUILD */

    /* clear screen before exit */
    menu->draw_fill_rect(0, 0, 119, 63, CLCDDisplay::PIXEL_OFF);
#ifdef X86_BUILD
    cout << "menu->draw_fill_rect(0, 0, 119, 63, CLCDDisplay::PIXEL_OFF)" << endl;
    cout << "return (" << menu->getSelectedEntry() << ")" << endl;
#else
    menu->update();
#endif /* XF86_BUILD */

    return menu->getSelectedEntry();
}

