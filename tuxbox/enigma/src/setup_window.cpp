/*
 * setup_window.cpp
 *
 * Copyright (C) 2003 Andreas Monzner <ghostrider@tuxbox.org>
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
 * $Id: setup_window.cpp,v 1.3 2008/01/03 15:47:32 dbluelle Exp $
 */

#include <setup_window.h>

Signal2<void,eSetupWindow*,int*> eSetupWindow::setupHook;

class eSetupWindowSelectN
{
	int n;
public:
	eSetupWindowSelectN(int n): n(n) { }
	bool operator()(eListBoxEntryMenu &e)
	{
		if ( e.isSelectable()&2 && !n--)
		{
			e.selected();
			return 1;
		}
		return 0;
	}
};

eSetupWindow::eSetupWindow( const char *title, int entries, int width )
	:eListBoxWindow<eListBoxEntryMenu>(title, entries, width, true)
{
	addActionMap(&i_shortcutActions->map);
	list.setFlags(eListBoxBase::flagHasShortcuts);
}

void eSetupWindow::sel_num(int n)
{
	list.forEachEntry(eSetupWindowSelectN(n));
}

int eSetupWindow::eventHandler(const eWidgetEvent &event)
{
	int num=-1;
	switch (event.type)
	{
	case eWidgetEvent::evtAction:
		if (event.action == &i_shortcutActions->number0)
			num=9;
		else if (event.action == &i_shortcutActions->number1)
			num=0;
		else if (event.action == &i_shortcutActions->number2)
			num=1;
		else if (event.action == &i_shortcutActions->number3)
			num=2;
		else if (event.action == &i_shortcutActions->number4)
			num=3;
		else if (event.action == &i_shortcutActions->number5)
			num=4;
		else if (event.action == &i_shortcutActions->number6)
			num=5;
		else if (event.action == &i_shortcutActions->number7)
			num=6;
		else if (event.action == &i_shortcutActions->number8)
			num=7;
		else if (event.action == &i_shortcutActions->number9)
			num=8;
		else if (event.action == &i_cursorActions->cancel)
			close(0);
		else
			break;
		if (num != -1)
			sel_num(num);
		return 1;
	default:
		break;
	}
	return eWindow::eventHandler(event);
}
