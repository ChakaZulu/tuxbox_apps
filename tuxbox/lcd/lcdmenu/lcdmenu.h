/*
 * $Id: lcdmenu.h,v 1.8 2001/12/14 16:57:13 obi Exp $
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
 */

#ifndef __LCDMENU_H_
#define __LCDMENU_H_

#include <config.h>
#include <crypt.h>
#include <liblcddisplay.h>
#include <dbox/fp.h>
#include "configManager.h"

#ifndef X86_BUILD
#include <librcinput.h>
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
	void addNumberPrefix();

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

	const char *getCurrentSalt();
	char *getNewSalt();
	
	void poweroff();

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

	int pinFailures;
	string cryptedPin;
	char *newSalt;
};

#endif /* __LCDMENU_H_ */
