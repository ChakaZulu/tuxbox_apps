/*
 * $Id: main.cpp,v 1.1 2002/01/03 17:31:40 obi Exp $
 *
 * A startup menu for the d-box 2 linux project
 *
 * Copyright (C) 2001,2002 Andreas Oberritter <obi@saftware.de>
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
 * $Log: main.cpp,v $
 * Revision 1.1  2002/01/03 17:31:40  obi
 * moved main method out of class source
 *
 *
 */

#include "main.h"

int main(int argc, char **argv)
{
    /* create menu instance */
    //CLCDMenu *menu = new CLCDMenu(CONFIGFILE);
    CLCDMenu *menu = CLCDMenu::getInstance();

    /* draw the menu */
    menu->drawMenu();

    /* select default entry */
    menu->selectEntry(menu->getDefaultEntry());

    /* get command from remote control */
    menu->rcLoop();

    /* remember last selection */
    if (menu->getSelectedEntry() != menu->getDefaultEntry())
    {
	menu->getConfig()->setInt("default_entry", menu->getSelectedEntry());
	menu->getConfig()->setModifiedFlag(true);
    }

    if (menu->getConfig()->getModifiedFlag())
    {
	/* save configuraion */
#ifdef DEBUG
	cout << "saving configuration..." << endl;
	menu->getConfig()->dump();
#endif
	menu->getConfig()->saveConfig(CONFIGFILE);

	/* reset modified flag */
	menu->getConfig()->setModifiedFlag(false);
    }

    /* clear screen before exit */
    menu->draw_fill_rect(0, 0, 119, 63, CLCDDisplay::PIXEL_OFF);
    menu->update();

    return menu->getSelectedEntry();
}
