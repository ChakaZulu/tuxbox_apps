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
 * $Id: setup_language.h,v 1.4 2002/10/15 23:31:29 Ghostrider Exp $
 */

#ifndef __APPS__ENIGMA__SETUP_LANGUAGE_H
#define __APPS__ENIGMA__SETUP_LANGUAGE_H

#include <map>
#include <string>

#include <lib/gui/ewindow.h>
#include <lib/gui/statusbar.h>
#include <lib/gui/ebutton.h>
#include <lib/gui/elabel.h>
#include <lib/gui/listbox.h>


class eZapLanguageSetup: public eWindow
{
private:
	void okPressed();
	void abortPressed();

	eButton *ok, *abort;
	eListBox<eListBoxEntryText> *language;
	eStatusBar* statusbar;

public:
	eZapLanguageSetup();
	~eZapLanguageSetup();
};

#endif
