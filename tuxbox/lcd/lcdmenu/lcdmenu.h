/*
 * lcdmenu.h
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

#ifndef __LCDMENU_H_
#define __LCDMENU_H_

#include <crypt.h>

#include "config.h"
#include "fontrenderer.h"
#include "lcddisplay.h"

#ifndef X86_BUILD
#include "rcinput.h"
#endif /* X86_BUILD */

#include <string>
#include <vector>
using namespace std;

#define LEFTALIGNED	0
#define CENTERED	1

class CLCDMenu : public CLCDDisplay
{
    public:

	CLCDMenu();
	~CLCDMenu();

	void addEntry(string);
	bool selectEntry(int);
	int getDefaultEntry() { return defaultEntry; }
	int getSelectedEntry() { return selectedEntry; }

	bool drawMenu();
	bool drawString(string, int, int, int);
	int getTextAlign() { return textAlign; } /* 0=left, 1=centered */

#ifndef X86_BUILD
	CRCInput *getRc()  { return rc; }
#endif /* X86_BUILD */
	bool rcLoop();

	string pinScreen(string, bool);
	bool changePin();
	bool checkPin(string);
	bool isPinProtected(int);
	void addPinProtection(int);

	CConfigManager *getConfig() { return config; }

    private:
	
	CConfigManager *config;

#ifndef X86_BUILD
	CRCInput *rc;
#endif /* X86_BUILD */

	fontRenderClass *fontRenderer;
	Font *menuFont;
	int fontSize;
	int lineSpacing;
	int textAlign;
	bool showNumbers;

	int selectedEntry;
	int entryCount;
	int defaultEntry;
	vector<string> entries;
	vector<int> pinEntries;

	string cryptedPin;
	char newSalt[2];
	char oldSalt[2];

};

#endif /* __LCDMENU_H_ */
