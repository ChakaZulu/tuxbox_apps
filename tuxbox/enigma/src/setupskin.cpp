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
 * $Id: setupskin.cpp,v 1.23 2008/10/25 14:24:36 dbluelle Exp $
 */

#include <setupskin.h>

#include <dirent.h>
#include <lib/gui/ebutton.h>
#include <lib/gui/emessage.h>
#include <lib/system/econfig.h>
#include <lib/gdi/epng.h>
#include <lib/gdi/gfbdc.h>

#include <lib/gui/ewidget.h>


void eSkinSetup::loadSkins()
{
	eListBoxEntrySkin* selection=0;
        lskins->beginAtomic();
	const char *skinPaths[] =
	{
		CONFIGDIR "/enigma/skins/",
		TUXBOXDATADIR "/enigma/skins/",
		0
	};

	char *current_skin=0;
	eConfig::getInstance()->getKey("/ezap/ui/skin", current_skin);

	std::set<eString> parsedSkins;

	for (int i=0; skinPaths[i]; ++i)
	{
		DIR *d=opendir(skinPaths[i]);
		if (!d)
		{
			if (i)
			{
				eDebug("error reading skin directory");
				eMessageBox msg("error reading skin directory", "error");
				msg.show();
				msg.exec();
				msg.hide();
			}
			continue;
		}

		while(struct dirent *e=readdir(d))
		{
			if (i && parsedSkins.find(e->d_name) != parsedSkins.end() )
				// ignore loaded skins in var... (jffs2)
				continue;

			eString fileName=skinPaths[i];
			fileName+=e->d_name;

			if (fileName.find(".info") != eString::npos)
			{
				eSimpleConfigFile config(fileName.c_str());
				eString esml = skinPaths[i] + config.getInfo("esml");
				eString name = config.getInfo("name");
				eDebug("esml = %s, name = %s", esml.c_str(), name.c_str());
				if (esml.size() && name.size())
				{
					parsedSkins.insert(e->d_name);
					eListBoxEntrySkin *s=new eListBoxEntrySkin(lskins, name, esml);
					if (current_skin && esml == current_skin)
						selection=s;
				}
			}
		}
		closedir(d);
	}
	if ( lskins->getCount() )
		lskins->sort();
	if (selection)
		lskins->setCurrent(selection);
	if ( current_skin )
		free(current_skin);
        lskins->endAtomic();
}

void eSkinSetup::accept()
{
	skinSelected(lskins->getCurrent());
}

void eSkinSetup::skinchanged(eListBoxEntrySkin *skin)
{
        eString icon;
        eString iconname; 
        eString finalname;
        int len;

        iconname = skin->getESML();
        len = iconname.find(".esml");
        finalname= iconname.substr(0,len)+ ".png";

        lb3->clear();

        gPixmap *img = 0;
        img = loadPNG(finalname.c_str());
        if(img)
        { 
           gPixmap * mp = &gFBDC::getInstance()->getPixmap();
           gPixmapDC mydc(img);
           gPainter p(mydc);
           p.mergePalette(*mp);

           lb3->move(ePoint(280, 15));
           lb3->resize(eSize(256, 200));
           lb3->setBlitFlags(BF_ALPHATEST);
           lb3->setProperty("align", "center");
           lb3->setPixmap(img);
           lb3->setPixmapPosition(ePoint(1, 1));
        }
        else
        {
           gPixmap *img = loadPNG("/share/tuxbox/enigma/pictures/nopreview.png");
           if(img)
           {
              gPixmap * mp = &gFBDC::getInstance()->getPixmap();
              gPixmapDC mydc(img);
              gPainter p(mydc);
              p.mergePalette(*mp);

              lb3->move(ePoint(280, 15));
              lb3->resize(eSize(256, 200));
              lb3->setBlitFlags(BF_ALPHATEST);
              lb3->setProperty("align", "center");
              lb3->setPixmap(img);
              lb3->setPixmapPosition(ePoint(1, 1));
              lb3->loadDeco();
           }
        }
        lb3->show();
}
void eSkinSetup::skinSelected(eListBoxEntrySkin *skin)
{
	if (!skin)
		close(1);
	else
	{
		eConfig::getInstance()->setKey("/ezap/ui/skin", skin->getESML().c_str());
		eConfig::getInstance()->flush();
		close(0);
	}
}

eSkinSetup::eSkinSetup()
{
        lb3     = new eLabel(this);
        move(ePoint(50,50));
        cresize(eSize(590,376));
        setText("Skin Selector");
	baccept=new eButton(this);
	baccept->setName("accept");
	baccept->setText("Save");
        baccept->setShortcut("green");
        baccept->setShortcutPixmap("green");
        baccept->resize(eSize(250,40));
        baccept->move(ePoint(10,320)); 
        baccept->loadDeco(); 

	lskins=new eListBox<eListBoxEntrySkin>(this);
	lskins->setName("skins");
	lskins->setFlags(eListBoxBase::flagLostFocusOnLast);
        lskins->move(ePoint(10,10));
        lskins->resize(eSize(clientrect.width()-350,clientrect.height() -60));
        lskins->loadDeco();

	statusbar=new eStatusBar(this);
	statusbar->setName("statusbar");

	CONNECT(baccept->selected, eSkinSetup::accept);
	CONNECT(lskins->selected, eSkinSetup::skinSelected);

	setFocus(lskins);

	CONNECT(lskins->selchanged, eSkinSetup::skinchanged);
	loadSkins();
	
	setHelpID(88);
}

eSkinSetup::~eSkinSetup()
{
}

int eSkinSetup::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
		case eWidgetEvent::evtAction:
			if (event.action == &i_cursorActions->left)
				focusNext(eWidget::focusDirW);
			else if (event.action == &i_cursorActions->right)
				focusNext(eWidget::focusDirE);
			else
				break;
			return 1;
		default:
			break;
	}
	return eWidget::eventHandler(event);
}
int  eListBoxEntrySkin::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
		default:
			break;
	}
	return eListBoxEntry::eventHandler(event);
}
