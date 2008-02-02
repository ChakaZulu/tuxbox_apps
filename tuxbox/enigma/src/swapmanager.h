/* 
$Id: swapmanager.cpp, V1.00 2008/01/08 21:23:00 Dre
Swap Manager for Enigma 1
Copyright (C) 2007 Dre

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifndef DISABLE_HDD
#ifndef DISABLE_FILE

#ifndef __swapmanager_h_
#define __swapmanager_h_

#include <lib/gui/ebutton.h>
#include <lib/gui/listbox.h>
#include <lib/gui/textinput.h>
#include <lib/gui/combobox.h>
#include <lib/gui/elabel.h>
#include <lib/gui/emessage.h>
#include <sys/stat.h>
#include <errno.h>
#include <lib/base/console.h>
#include <lib/system/info.h>
#include <lib/system/math.h>

class eConsoleAppContainer;

class eSwapManager: public eWindow
{
	eButton *bt_crswap, *bt_acswap, *bt_delswap, *bt_stswap;
	eStatusBar *statusbar;
	eLabel *lb_file, *lb_mnt, *lb_size, *lb_status, *lb_found;
	eComboBox *cb_size, *cb_file, *cb_mnt;

	eMessageBox *info;

	void createSwap();
	void activateSwap();
	void stopSwap();
	void deleteSwap();
	void searchSwap(eListBoxEntryText *seltype);
	void searchSwapLight();
	void getSize();
	void getState();
	void appClosed(int);
	void setButtons();
	void getCurrentSwaps();
	void rewriteScript();
	void getBoxType();

	eConsoleAppContainer *createSwapfile;

	eString status, store, found, swapstring, swaps[10], hdddir, mntbase;
	int swapfound, swapstatus, swapstate, bytesize, filename, pathname, s;
	char* swap;
	
public:
	eSwapManager();
	~eSwapManager();
};

#endif

#endif
#endif
 
