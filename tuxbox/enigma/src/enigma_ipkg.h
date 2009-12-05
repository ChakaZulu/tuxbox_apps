/*
 * Ripper's setup for dreambox
 * Copyright (c) 2009 Ripper <Mario.Senska@gmx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
#ifndef __enigma_ipkg_h
#define __enigma_ipkg_h
#include <plugin.h> 
#include <dirent.h>
#include <sys/vfs.h>
#include <lib/driver/rc.h>
#include <lib/gdi/font.h>
#include <lib/gui/echeckbox.h>
#include <lib/gui/enumber.h>
#include <src/enigma.h>
#include <src/setup_window.h>
#include <src/enigma_main.h>

class ePackageManagerRunApp:public eWindow
{
	int output;
	eConsoleAppContainer *app;
	eButton *bCancel, *bClose;
	int eventHandler (const eWidgetEvent & event);
	void onCancel ();
	void getData (eString);
	void appClosed (int);
	eLabel *label;
	eProgress *scrollbar;
	int pageHeight, total;
	void updateScrollbar();
	void init_ePackageManagerRunApp();
public:
	ePackageManagerRunApp();
};

class eMainmenuPackageManager: public eSetupWindow
{
private:
	void PackageManagerSettings();
	void PackageManagerUpdate();
	void PackageManagerInstallOnline();
	void PackageManagerInstall();
	void PackageManagerRemove();
	eString command;
	void init_eMainmenuPackageManager();
public:
	eMainmenuPackageManager();
};

class ePackageSettings: public eWindow
{
	struct selectComboEntry;
	eComboBox *IPKGInstallOption;
	eTextInputField *path, *lServer;
	eButton *seldir;
	void LoadSettings();
	void SaveSettings();
	void selectDir();
	void init_ePackageSettings();
public:
	ePackageSettings();
};

class ePackageHandler: public eWindow
{
	int type;
	eListBox<eListBoxEntryText> *items;
	void PackageChanged( eListBoxEntryText* );
	void ListPackage();
	void init_ePackageHandler();
public:
	ePackageHandler(int type);
};
#endif
