/*
 * setup_language.h
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
 * $Id: setup_language.h,v 1.2 2002/06/23 15:51:24 Ghostrider Exp $
 */

#ifndef __APPS__ENIGMA__SETUP_LANGUAGE_H
#define __APPS__ENIGMA__SETUP_LANGUAGE_H

#include <map>
#include <string>

#include <core/gui/ewindow.h>

class eNumber;
class eButton;
class eCheckbox;

class eZapLanguageSetup: public eWindow
{
public:
	eZapLanguageSetup();

protected:
	int eventFilter(const eWidgetEvent &event);

private:
	void okPressed();
	void abortPressed();

	void setLanguage(std::map<std::string, std::string>::iterator &);
	void toggleLanguage();

	eButton *language, *ok, *abort;

	std::map<std::string, std::string> languages;
	std::map<std::string, std::string>::iterator v_language;
};

#endif
