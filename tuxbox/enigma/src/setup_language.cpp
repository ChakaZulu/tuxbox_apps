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
 * $Id: setup_language.cpp,v 1.4 2002/06/23 15:51:24 Ghostrider Exp $
 */

#include "setup_language.h"

#include <core/base/i18n.h>
#include <core/gui/elabel.h>
#include <core/gui/enumber.h>
#include <core/gui/ebutton.h>
#include <core/gui/echeckbox.h>
#include <core/gui/eskin.h>
#include <core/system/econfig.h>

void eZapLanguageSetup::setLanguage(std::map<std::string, std::string>::iterator&it)
{
	language->setText(it->second.c_str());
}

eZapLanguageSetup::eZapLanguageSetup(): eWindow(0), v_language(0)
{
	int fd=eSkin::getActive()->queryValue("fontsize", 20);

	setText(_("Language Setup"));
	move(ePoint(150, 136));
	resize(eSize(350, 190));

	eLabel *l=new eLabel(this);
	l->setText(_("Language:"));
	l->move(ePoint(10, 20));
	l->resize(eSize(150, fd+4));
	
	language=new eButton(this, l);
	language->setText("[language]");
	language->move(ePoint(160, 20));
	language->resize(eSize(150, fd+4));
	CONNECT(language->selected, eZapLanguageSetup::toggleLanguage);

	ok=new eButton(this);
	ok->setText(_("[OK]"));
	ok->move(ePoint(10, 90));
	ok->resize(eSize(50, fd+4));
	CONNECT(ok->selected, eZapLanguageSetup::okPressed);

	abort=new eButton(this);
	abort->setText(_("[ABORT]"));
	abort->move(ePoint(80, 90));
	abort->resize(eSize(100, fd+4));
	CONNECT(abort->selected, eZapLanguageSetup::abortPressed);

	languages.insert(std::pair<std::string,std::string>("C","Englisch"));
	languages.insert(std::pair<std::string,std::string>("de_DE","Deutsch"));

	char *temp;
	if (eConfig::getInstance()->getKey("/elitedvb/language", temp))
		temp=NULL;

	if(!temp)
		v_language=languages.find("C");
	else
		v_language=languages.find(temp);

	setLanguage(v_language);
}

void eZapLanguageSetup::okPressed()
{
	eConfig::getInstance()->setKey("/elitedvb/language", v_language->first.c_str());

	setlocale(LC_ALL,v_language->first.c_str());

	close(1);
}

void eZapLanguageSetup::abortPressed()
{
	close(0);
}

void eZapLanguageSetup::toggleLanguage()
{
	++v_language;
	if (v_language==languages.end())
		v_language=languages.begin();
	setLanguage(v_language);
}

int eZapLanguageSetup::eventFilter(const eWidgetEvent &event)
{
#if 0
	switch (event.type)
	{
	case eWidgetEvent::keyDown:
		switch(event.parameter)
		{
		case eRCInput::RC_RIGHT:
			focusNext(eWidget::focusDirE);
			return 1;
		case eRCInput::RC_DOWN:
			focusNext(eWidget::focusDirS);
			return 1;
		case eRCInput::RC_LEFT:
			focusNext(eWidget::focusDirW);
			return 1;
		case eRCInput::RC_UP:
			focusNext(eWidget::focusDirN);
			return 1;
		}
	}
#endif
	return 0;
}
