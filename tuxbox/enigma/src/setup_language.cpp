/*
 * setup_language.cpp
 *
 * Copyright (C) 2002 Bastian Blank <waldi@tuxbox.org>
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
 * $Id: setup_language.cpp,v 1.8 2002/10/03 18:13:36 Ghostrider Exp $
 */

#include "setup_language.h"

#include <core/base/i18n.h>
#include <core/gui/eskin.h>
#include <core/system/econfig.h>

eZapLanguageSetup::eZapLanguageSetup(): eWindow(0)
{
	int fd=eSkin::getActive()->queryValue("fontsize", 20);

	setText(_("Language Setup"));
	move(ePoint(150, 136));
	resize(eSize(350, 230));

	eLabel *l=new eLabel(this);
	l->setText(_("Language:"));
	l->move(ePoint(10, 20));
	l->resize(eSize(150, fd+4));

	language=new eListBox<eListBoxEntryText>(this, l);
	language->loadDeco();
	language->setFlags( eListBoxBase::flagNoUpDownMovement );
	language->move(ePoint(140, 20));
	language->resize(eSize(150, 35));
	language->setHelpText(_("select your language (left, right)"));
	new eListBoxEntryText(language, "Englisch", (void*) new eString("C"));
	new eListBoxEntryText(language, "Deutsch", (void*) new eString("de_DE"));
	char *temp;
	if ( eConfig::getInstance()->getKey("/elitedvb/language", temp) )
		temp=NULL;
	if (temp && !strcmp(temp, "de_DE") )
		language->goNext();

	ok=new eButton(this);
	ok->setText(_("save"));
	ok->move(ePoint(20, 80));
	ok->resize(eSize(90, fd+4));
	ok->setHelpText(_("save changes and close window"));
	ok->loadDeco();
	CONNECT(ok->selected, eZapLanguageSetup::okPressed);

	abort=new eButton(this);
	abort->setText(_("abort"));
	abort->move(ePoint(140, 80));
	abort->resize(eSize(100, fd+4));
	abort->setHelpText(_("leave language setup (no changes are saved)"));
	abort->loadDeco();
	CONNECT(abort->selected, eZapLanguageSetup::abortPressed);

	statusbar = new eStatusBar(this);
	statusbar->move( ePoint(0, clientrect.height()-30) );
	statusbar->resize( eSize( clientrect.width(), 30) );
	statusbar->loadDeco();
}

void eZapLanguageSetup::okPressed()
{
	eConfig::getInstance()->setKey("/elitedvb/language", ((eString*) language->getCurrent()->getKey())->c_str() );
	eConfig::getInstance()->flush();
	setlocale(LC_ALL, ((eString*)language->getCurrent()->getKey())->c_str() );

	close(1);
}

void eZapLanguageSetup::abortPressed()
{
	close(0);
}

eZapLanguageSetup::~eZapLanguageSetup()
{
	while ( language->getCurrent()->getKey() )
	{
		delete (eString*)language->getCurrent()->getKey();
		language->getCurrent()->getKey()=0;
		language->goNext();
	}
}

