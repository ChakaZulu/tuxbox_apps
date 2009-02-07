/* 
$Id: swapmanager.cpp, V1.00 2008/01/08 21:23:00 Dre
Swap Manager for Enigma 1
Copyright (C) 2007 - 2008 Dre

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

#include <swapmanager.h>
#include <sselect.h>

using namespace std;

//Main GUI
eSwapManager::eSwapManager(): eWindow(0), createSwapfile(0)
{
	init_eSwapManager();
}
void eSwapManager::init_eSwapManager()
{


	setHelpID(155);

//Boxmodell for hdd-path
	getBoxType();

//Config laden
	swap=0;
	int pos =0;
	int length = 0;
	eString file;
	file = "myswap";
	filename=0;
	eString path;
	path = eString().sprintf("%susb/", mntbase.c_str() );
	eConfig::getInstance()->getKey("/extras/swapfilename", swap);

	cb_file=new eComboBox(this,4);cb_file->setName("file");

	new eListBoxEntryText( *cb_file, "swap", (void*)0, 0, eString().sprintf("%s%s", _("Filename: "), "swap"));
	new eListBoxEntryText( *cb_file, "swapfile", (void*)1, 0, eString().sprintf("%s%s", _("Filename: "), "swapfile"));
	new eListBoxEntryText( *cb_file, "myswap", (void*)2, 0, eString().sprintf("%s%s", _("Filename: "), "myswap"));
	if(swap != 0)
	{
		eString s = eString(swap);
		pos = s.find_last_of("/");
		length = s.length()-1;
		file = s.substr(pos+1, length-1);
		if ( file == "swapfile")
		{
			filename=1;
		}
		else if (file == "myswap")
		{
			filename=2;
		}
		else if (file == "swap")
		{
			filename=0;
		}
		else
		{
			filename=3;
			new eListBoxEntryText( *cb_file, file.c_str(), (void*)3, 0, eString().sprintf("%s%s", _("Filename: "), file.c_str() ) );
		}
		path = s.substr(0, pos+1);
	}

	
	tb_path=new eTextInputField(this);tb_path->setName("path");
	tb_path->setText(path);
	bt_seldir=new eButton(this); bt_seldir->setName("seldir");

	cb_size=new eComboBox(this, 4);cb_size->setName("filesize");

	new eListBoxEntryText( *cb_size, "0 MB", (void*)0, 0, eString().sprintf(_("Filesize: %d MB"),0).c_str());
	new eListBoxEntryText( *cb_size, "8 MB", (void*)8, 0, eString().sprintf(_("Filesize: %d MB"),8).c_str());
	new eListBoxEntryText( *cb_size, "16 MB", (void*)16, 0, eString().sprintf(_("Filesize: %d MB"),16).c_str());
	new eListBoxEntryText( *cb_size, "32 MB", (void*)32, 0, eString().sprintf(_("Filesize: %d MB"),32).c_str());
	new eListBoxEntryText( *cb_size, "64 MB", (void*)64, 0, eString().sprintf(_("Filesize: %d MB"),64).c_str());
	new eListBoxEntryText( *cb_size, "128 MB", (void*)128, 0, eString().sprintf(_("Filesize: %d MB"),128).c_str());
	new eListBoxEntryText( *cb_size, "256 MB", (void*)256, 0, eString().sprintf(_("Filesize: %d MB"),256).c_str());
	new eListBoxEntryText( *cb_size, "512 MB", (void*)512, 0, eString().sprintf(_("Filesize: %d MB"),512).c_str());
	new eListBoxEntryText( *cb_size, "1024 MB", (void*)1024, 0, eString().sprintf(_("Filesize: %d MB"),1024).c_str());

	bt_delswap = new eButton(this);bt_delswap->setName("delete");

	bt_stswap = new eButton(this);bt_stswap->setName("stop");

	bt_crswap = new eButton(this);bt_crswap->setName("create");

	bt_acswap = new eButton(this);bt_acswap->setName("start");

	lb_found= new eLabel(this);lb_found->setName("found");

	lb_status= new eLabel(this);lb_status->setName("status");

	statusbar = new eStatusBar(this); statusbar->setName("statusbar");

	if (eSkin::getActive()->build(this, "SwapManager"))
		eFatal("skin load of \"SwapManager\" failed");

	bt_delswap->hide();
	bt_stswap->hide();
	bt_crswap->hide();
	bt_acswap->hide();
	cb_file->setCurrent(filename, true);
	CONNECT(cb_file->selchanged, eSwapManager::searchSwap);
	getSize();
	CONNECT(bt_seldir->selected, eSwapManager::selectDir);
	CONNECT(bt_delswap->selected, eSwapManager::deleteSwap);
	CONNECT(bt_stswap->selected, eSwapManager::stopSwap);
	CONNECT(bt_crswap->selected, eSwapManager::createSwap);
	CONNECT(bt_acswap->selected, eSwapManager::activateSwap);

//Abfragen, ob Swapfile existiert
	searchSwap(NULL);
}
void eSwapManager::selectDir()
{
	eFileSelector sel(tb_path->getText());
#ifndef DISABLE_LCD
	sel.setLCD(LCDTitle, LCDElement);
#endif
	hide();

	const eServiceReference *ref = sel.choose(-1);

	if (ref)
		tb_path->setText(sel.getPath().current().path);
	show();
	setFocus(bt_seldir);

}

void eSwapManager::getBoxType()
{
	int boxtype = eSystemInfo::getInstance()->getHwType();

	switch( boxtype )
	{
		case eSystemInfo::DM500:
		case eSystemInfo::DM5600:
		case eSystemInfo::DM5620:
		case eSystemInfo::TR_DVB272S:
		case eSystemInfo::DM7000:
		default:
			mntbase = "/var/mnt/";
			break;
		case eSystemInfo::DM500PLUS:
		case eSystemInfo::DM600PVR:
		case eSystemInfo::DM7020:
			mntbase = "/media/";
			break; 

	}
}

//Abfragen der aktuell aktiven Swaps
void eSwapManager::getCurrentSwaps()
{
	s=0;

	char swapbuffer[150];

	FILE *j = fopen("/proc/swaps", "r");
	if(j)
	{
		while(fgets(swapbuffer, 75, j))
		{
			eString dummystring;
			dummystring = swapbuffer;
			std::stringstream tmp;
			tmp.str(dummystring);
			tmp >> dummystring;
			swaps[s] = dummystring;
			s++;
		}
	fclose(j);
	}
}

//Aktiv/Inaktiv abfragen
void eSwapManager::getState()
{
	getCurrentSwaps();

	swapstatus=0;

	for(int m=0; m < s; m++)
	{
		if(swaps[m] == store)
		{
			swapstatus = 1;
			break;
		}
	}
}

//Grösse abfragen
void eSwapManager::getSize()
{

	eString tmp = tb_path->getText() + cb_file->getCurrent()->getText();

	struct stat sw;
	int used=0;

	if (stat(tmp.c_str(), &sw) != -1)
		used=sw.st_size;
		used/=(1024*1024);

	if( used > 1024 ) 
	{
		new eListBoxEntryText( *cb_size, eString().sprintf("%d MB", used), (void*)used, 0, eString().sprintf(_("Filesize: %d MB"), used));
	}
	cb_size->setCurrent((void*)used, true);
}

//Buttons handeln
void eSwapManager::setButtons()
{
	if(swapfound == 0)
	{
		bt_stswap->hide();
		bt_acswap->hide();
		bt_delswap->hide();
		bt_crswap->show();
		found = eString().sprintf(_("Not found"));
		status = eString().sprintf(_("Inactive"));
	}
	else if(swapfound == 1 && swapstatus == 0)
	{
		bt_stswap->hide();
		bt_acswap->show();
		bt_delswap->show();
		bt_crswap->hide();
		found = eString().sprintf(_("Found"));
		status = eString().sprintf(_("Inactive"));
	}
	else if(swapfound == 1 && swapstatus == 1)
	{
		bt_stswap->show();
		bt_acswap->hide();
		bt_delswap->hide();
		bt_crswap->hide();
		found = eString().sprintf(_("Found"));
		status = eString().sprintf(_("Active"));
	}

	lb_found->setText(eString().sprintf(_("Swap-File: %s"), found.c_str()));
	lb_status->setText(eString().sprintf(_("Swap-Status: %s"), status.c_str()));
}

//Swapfile suchen
void eSwapManager::searchSwap(eListBoxEntryText *seltype)
{
	store = tb_path->getText() + cb_file->getCurrent()->getText();

	struct stat st;
	if (lstat(store.c_str(),&st) != -1)
	{
		swapfound=1;
		getState();
		getSize();
	}
	else
	{
		swapfound=0;
		swapstate=0;
	}
	setButtons();
}

//Swapfile erstellen
void eSwapManager::createSwap()
{
	int bytesize=(int)cb_size->getCurrent()->getKey() * 1024;
	
	store = tb_path->getText() + cb_file->getCurrent()->getText();

	info = new eMessageBox (_("The swapfile will now be created. This may take some time depending on the size of your swapfile"), _("Swap Manager"), 0);
		info->zOrderRaise();
		info->show();

	eString command = eString().sprintf("dd if=/dev/zero of=%s bs=1024 count=%d", store.c_str(), bytesize );

		if (!createSwapfile)
		{
			createSwapfile = new eConsoleAppContainer(command.c_str());
			CONNECT(createSwapfile->appClosed, eSwapManager::appClosed);
		}

}

//Swapfile erstellt
void eSwapManager::appClosed(int)
{
	delete createSwapfile;
	createSwapfile=0;

	int byte=(int)cb_size->getCurrent()->getKey();

	eString tmp = tb_path->getText() + cb_file->getCurrent()->getText();

	struct stat crsize;
	int created=0;

	if (stat(tmp.c_str(), &crsize) != -1)
		created=crsize.st_size;
		created/=(1024*1024);

	info->hide();

	if(created == byte)
	{
		eMessageBox::ShowBox((eString().sprintf(_("The swapfile has been created!"))), _("Swap Manager"), eMessageBox::btOK);
	
		searchSwap(NULL);
	
		lb_found->setText(eString().sprintf(_("Swap-File: %s"), found.c_str()));
		int mkvalue = ( system(eString().sprintf("mkswap %s", store.c_str() ).c_str()) == 0) ? 1 : 0;
		eConfig::getInstance()->setKey("/extras/mkswap", mkvalue);
	}
	else
	{
		eMessageBox::ShowBox((eString().sprintf("%s\n%s",_("There has been an error during creation!"),strerror(errno))), _("Swap Manager"), eMessageBox::iconWarning|eMessageBox::btOK);
	}
	setButtons();
}

//Swapfile aktivieren
void eSwapManager::activateSwap()
{
	store = tb_path->getText() + cb_file->getCurrent()->getText();
	int mkvalue = 0;
	eConfig::getInstance()->getKey("/extras/mkswap", mkvalue);
	if (mkvalue == 0)
	{
		int mkvalue = ( system(eString().sprintf("mkswap %s", store.c_str() ).c_str()) == 0) ? 1 : 0;
		eConfig::getInstance()->setKey("/extras/mkswap", mkvalue);
	}
	if ( system(eString().sprintf("swapon %s%s", tb_path->getText().c_str(), cb_file->getCurrent()->getText().c_str() ).c_str())  == 0 )
	{
		system("echo 0 > /proc/sys/vm/swappiness");
		eConfig::getInstance()->setKey("/extras/swapfilename", store.c_str());
		eConfig::getInstance()->setKey("/extras/swapfile", 1);
		getState();
		setButtons();
	}
	else
	{
		eMessageBox::ShowBox((eString().sprintf("%s\n%s",_("Error during activation!"),strerror(errno))), _("Swap Manager"), eMessageBox::iconWarning|eMessageBox::btOK);
	}
}

//Swapfile deaktivieren
void eSwapManager::stopSwap()
{
	eString selectedSwap = eString().sprintf("%s%s", tb_path->getText().c_str(), cb_file->getCurrent()->getText().c_str() );

	if ( system(eString().sprintf("swapoff %s", selectedSwap.c_str() ).c_str())  == 0 )
	{
		if ( selectedSwap == swap)
		{
			eConfig::getInstance()->setKey("/extras/swapfile", 0);
		}
		getState();
		setButtons();
	}
	else
	{
		eMessageBox::ShowBox((eString().sprintf("%s\n%s",_("Error during inactivation!"),strerror(errno))), _("Swap Manager"), eMessageBox::iconWarning|eMessageBox::btOK);
	}
}

//swapfile löschen
void eSwapManager::deleteSwap()
{
	if ( system(eString().sprintf("rm -f %s%s", tb_path->getText().c_str(), cb_file->getCurrent()->getText().c_str() ).c_str()) == 0 )
	{
// 		eConfig::getInstance()->setKey("/extras/swapfilename", "/var/mnt/usb/myswap");
		eMessageBox::ShowBox((_("Swapfile has been deleted!")), ("Swap Manager"), eMessageBox::iconInfo|eMessageBox::btOK);

		searchSwap(NULL);
	}
	else
	{
		eMessageBox::ShowBox(eString().sprintf("%s\n%s",_("File has not been deleted due to an error!"),strerror(errno)), _("Swap Manager"), eMessageBox::iconWarning|eMessageBox::btOK);
	}
}

eSwapManager::~eSwapManager()
{
	if (createSwapfile)
		delete createSwapfile;
}
#endif
#endif
