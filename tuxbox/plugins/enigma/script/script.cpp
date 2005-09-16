/*
	Script - Enigma Plugin

	Simple plugin that just calls a script
	
	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA
*/

#include <plugin.h>
#include <stdio.h>
#include <signal.h>
#include <lib/gui/ewindow.h>
#include <lib/gui/ebutton.h>
#include <lib/gui/emessage.h>

class eScriptWindow: public eWindow
{
	eButton *bt_scripts[10];
	void runScript(int i);
public:
	eScriptWindow();
	~eScriptWindow();
};

extern "C" int plugin_exec( PluginParam *par )
{
	eScriptWindow dlg;
	dlg.show();
	int result=dlg.exec();
	dlg.hide();
	return result;
}

eScriptWindow::eScriptWindow(): eWindow(1)
{
	cmove(ePoint(100, 100));
	cresize(eSize(520, 376));
	setText((_("Script Plugin")));
	
	for(int i=1; i<10; i++)
	{
		bt_scripts[i-1]=new eButton(this);
		bt_scripts[i-1]->move(ePoint(10, 10+((i-1)*32)));
		bt_scripts[i-1]->resize(eSize(clientrect.width()-20, 30));
		bt_scripts[i-1]->setShortcut(eString().sprintf("%d",i));
		bt_scripts[i-1]->setShortcutPixmap(eString().sprintf("%d",i));
		bt_scripts[i-1]->loadDeco();
		bt_scripts[i-1]->setText(eString().sprintf("Script %d (/var/bin/script%.02d.sh)", i, i));
		CONNECT_1_0(bt_scripts[i-1]->selected, eScriptWindow::runScript, i);
	}
	
	setFocus(bt_scripts[0]);
}

eScriptWindow::~eScriptWindow()
{
}

void eScriptWindow::runScript(int i)
{
	char cmd[256];
	sprintf(cmd, "/var/bin/script%.02d.sh&", i);
	eDebug("Running %s\n", cmd);
	system(cmd);	
}
