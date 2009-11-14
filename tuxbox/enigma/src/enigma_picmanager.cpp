/*
 * $Id: enigma_picmanager.cpp,v 1.12 2009/11/14 16:27:47 dbluelle Exp $
 *
 * (C) 2005 by digi_casi <digi_casi@tuxbox.org>
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
 
#include <lib/base/i18n.h>
#include <lib/gui/elabel.h>
#include <lib/gui/enumber.h>
#include <lib/gui/ebutton.h>
#include <lib/gui/echeckbox.h>
#include <lib/gui/eskin.h>
#include <lib/system/econfig.h>
#include <lib/gui/numberactions.h>
#include <lib/system/info.h>
#include <lib/system/init.h>
#include <lib/system/init_num.h>
#include "enigma_picmanager.h"

ePicViewerSettings::ePicViewerSettings():eWindow(0)
{
	init_ePicViewerSettings();
}
void ePicViewerSettings::init_ePicViewerSettings()
{
	int slideshowtimeout = 5;
	eConfig::getInstance()->getKey("/picviewer/slideshowtimeout", slideshowtimeout);

	timeout = new eListBox<eListBoxEntryText>(this);timeout->setName("timeout");
	timeout->setFlags(eListBox<eListBoxEntryText>::flagNoUpDownMovement);
	eListBoxEntryText* entries[30];
	for (int i = 0; i < 30; i++)
	{
		eString num = eString().sprintf("%d", i + 1);
		entries[i] = new eListBoxEntryText(timeout, num.c_str(), (void *)new eString(num.c_str()));
	}

	subdirs =CreateSkinnedCheckbox("subdirs",0,"/picviewer/includesubdirs");
	busy =CreateSkinnedCheckbox("busy",0,"/picviewer/showbusysign");
	format_169 =CreateSkinnedCheckbox("format_169",0,"/picviewer/format169");

	CONNECT(CreateSkinnedButton("store")->selected, ePicViewerSettings::okPressed);

	BuildSkin("picmanager");

	timeout->setCurrent(entries[slideshowtimeout - 1]);

}

ePicViewerSettings::~ePicViewerSettings()
{
}

void ePicViewerSettings::fieldSelected(int *number)
{
	focusNext(eWidget::focusDirNext);
}

void ePicViewerSettings::okPressed()
{
	eConfig::getInstance()->setKey("/picviewer/slideshowtimeout", atoi(((eString *)timeout->getCurrent()->getKey())->c_str()));
	eConfig::getInstance()->setKey("/picviewer/includesubdirs", (int)subdirs->isChecked());
	eConfig::getInstance()->setKey("/picviewer/showbusysign", (int)busy->isChecked());
	eConfig::getInstance()->setKey("/picviewer/format169", (int)format_169->isChecked());

	close(1);
}

void ePicViewerSettings::abortPressed()
{
	close(0);
}

