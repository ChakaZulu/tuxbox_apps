/*
 * enigma_setup.cpp
 *
 * Copyright (C) 2002 Felix Domke <tmbinc@tuxbox.org>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Id: setupskin.cpp,v 1.2 2002/05/16 15:52:06 tmbinc Exp $
 */

#include "setupskin.h"
#include <core/gui/ebutton.h>
#include <core/gui/elistbox.h>
#include <core/gui/emessage.h>
#include <core/system/econfig.h>
#include "config.h"
#include <dirent.h>

extern eString getInfo(const char *file, const char *info);

class eListboxSkin: public eListboxEntry
{
	eString name, esml;
public:
	eString getText(int col) const;
	const eString &getESML() const { return esml; };
	eListboxSkin(eListbox *parent, eString name, eString esml);
};

eString eListboxSkin::getText(int col) const
{
	return name;
}

eListboxSkin::eListboxSkin(eListbox *parent, eString name, eString esml):
		eListboxEntry(parent), name(name), esml(esml)
{
}

void eSkinSetup::loadSkins()
{
	const char *skinPath = DATADIR "/enigma/skins/";
	struct dirent **namelist;
	char *current_skin="";
	eConfig::getInstance()->getKey("/ezap/ui/skin", current_skin);

	int n = scandir(skinPath, &namelist, 0, alphasort);

	if (n<0)
	{
		eDebug("error reading skin directory");
		eMessageBox msg("error reading skin directory", "error");
		msg.show();
		msg.exec();
		msg.hide();
		return;
	}
	for(int count=0;count<n;count++)
	{
		eString	fileName=eString(skinPath) + eString(namelist[count]->d_name);

		if (fileName.find(".info") != -1)
		{
			eString esml=getInfo(fileName.c_str(), "esml");
			eString name=getInfo(fileName.c_str(), "name");
			if (esml.size() && name.size())
			{
				eListboxSkin *s=new eListboxSkin(lskins, name, esml);
				if (esml == current_skin)
				{
					eDebug("got current");
					lskins->setCurrent(s);
				}
			}
		}

		free(namelist[count]);
  }
  free(namelist);
}

void eSkinSetup::accept()
{
	skinSelected(lskins->getCurrent());
}

void eSkinSetup::skinSelected(eListboxEntry *l)
{
	if (!l)
		close(1);
	eListboxSkin *skin=(eListboxSkin*)l;
	eConfig::getInstance()->setKey("/ezap/ui/skin", skin->getESML().c_str());
	close(0);
}

eSkinSetup::eSkinSetup()
{
	baccept=new eButton(this);
	baccept->setName("accept");
	breject=new eButton(this);
	breject->setName("reject");
	lskins=new eListbox(this);
	lskins->setName("skins");
	CONNECT(baccept->selected, eSkinSetup::accept);
	CONNECT(breject->selected, eSkinSetup::reject);
	CONNECT(lskins->selected, eSkinSetup::skinSelected);
	
	setFocus(lskins);

	eSkin *skin=eSkin::getActive();
	if (skin->build(this, "setup.skins"))
		qFatal("skin load of \"setup.skins\" failed");

	loadSkins();
}

eSkinSetup::~eSkinSetup()
{
}

int eSkinSetup::eventFilter(const eWidgetEvent &event)
{
	int inlist=0;
	if (focusList()->current() == lskins)
		inlist=1;
	switch (event.type)
	{
	case eWidgetEvent::keyDown:
		switch(event.parameter)
		{
		case eRCInput::RC_RIGHT:
			focusNext(eWidget::focusDirE);
			return 1;
		case eRCInput::RC_DOWN:
			if (inlist)
				break;
			focusNext(eWidget::focusDirS);
			return 1;
		case eRCInput::RC_LEFT:
			focusNext(eWidget::focusDirW);
			return 1;
		case eRCInput::RC_UP:
			if (inlist)
				break;
			focusNext(eWidget::focusDirN);
			return 1;
		}
	}
	return 0;
}
