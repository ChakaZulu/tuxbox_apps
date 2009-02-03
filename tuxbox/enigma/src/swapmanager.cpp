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

using namespace std;

//Main GUI
eSwapManager::eSwapManager(): eWindow(0), createSwapfile(0)
{
	init_eSwapManager();
}
void eSwapManager::init_eSwapManager()
{
	cmove(ePoint(100,120));
	cresize(eSize(500, 370)); 
	setText(_("Swap Manager"));	

//Statusbar
	statusbar=new eStatusBar(this);
	statusbar->move( ePoint(0, clientrect.height()-50 ) );
	statusbar->resize( eSize( clientrect.width(), 50) );
	statusbar->loadDeco();

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


//Label und Combobox für den Dateinamen
	lb_file=new eLabel(this);
	lb_file->setText(_("Filename: "));//Dateiname
	lb_file->move(ePoint(20, 20));
	lb_file->resize(eSize(200, 30));

	cb_file=new eComboBox(this, 4, lb_file);
	cb_file->move(ePoint(250, 20));
	cb_file->resize(eSize(240, 30));
	cb_file->loadDeco();
	cb_file->setHelpText(_("Push OK to select Filename"));

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

	cb_file->setCurrent(filename, true);

	CONNECT(cb_file->selchanged, eSwapManager::searchSwap);

//Label und Combobox für das Verzeichnis
	lb_mnt=new eLabel(this);
	lb_mnt->setText(_("Directory:"));//Speicherort
	lb_mnt->move(ePoint(20, 70));
	lb_mnt->resize(eSize(200, 30));
	
	cb_mnt=new eComboBox(this,4, lb_mnt);
	cb_mnt->move(ePoint(250, 70));
	cb_mnt->resize(eSize(240, 30));
	cb_mnt->loadDeco();
	cb_mnt->setHelpText(_("Push OK to select directory"));
	
	new eListBoxEntryText( *cb_mnt, eString().sprintf("%susb/",mntbase.c_str()), (void*)0, 0, eString().sprintf( "%s %susb/",_("Directory:"),mntbase.c_str()));
	new eListBoxEntryText( *cb_mnt, eString().sprintf("%s", hdddir.c_str()), (void*)1, 0, eString().sprintf( "%s %s",_("Directory:"), hdddir.c_str() ));
	new eListBoxEntryText( *cb_mnt, eString().sprintf("%scf/",mntbase.c_str()), (void*)2, 0, eString().sprintf( "%s %scf/",_("Directory:"),mntbase.c_str()));

	if(path == eString().sprintf("%susb/",mntbase.c_str()))
	{
		pathname=0;
	}
	else if(path == hdddir)
	{
		pathname=1;
	}
	else if(path == eString().sprintf("%scf/",mntbase.c_str()))
	{
		pathname=2;
	}
	else
	{
		pathname=3;
		new eListBoxEntryText( *cb_mnt, path.c_str(), (void*)3, 0, eString().sprintf( "%s%s",_("Directory:"), path.c_str() ) );
	}

	cb_mnt->setCurrent(pathname, true);

//Suchen nach dem Swapfile
	CONNECT(cb_mnt->selchanged, eSwapManager::searchSwap);

//Label und Combobox für die Dateigrösse
	lb_size=new eLabel(this);
	lb_size->setText(_("Filesize:"));//Dateigroesse
	lb_size->move(ePoint(20, 120));
	lb_size->resize(eSize(200, 30));

	cb_size=new eComboBox(this, 4, lb_size);
	cb_size->move(ePoint(250, 120));
	cb_size->resize(eSize(240, 30));
	cb_size->loadDeco();
	cb_size->setHelpText(_("Push OK to select Filesize"));

	new eListBoxEntryText( *cb_size, "0 MB", (void*)0, 0, eString().sprintf(_("Filesize: %d MB"),0).c_str());
	new eListBoxEntryText( *cb_size, "8 MB", (void*)8, 0, eString().sprintf(_("Filesize: %d MB"),8).c_str());
	new eListBoxEntryText( *cb_size, "16 MB", (void*)16, 0, eString().sprintf(_("Filesize: %d MB"),16).c_str());
	new eListBoxEntryText( *cb_size, "32 MB", (void*)32, 0, eString().sprintf(_("Filesize: %d MB"),32).c_str());
	new eListBoxEntryText( *cb_size, "64 MB", (void*)64, 0, eString().sprintf(_("Filesize: %d MB"),64).c_str());
	new eListBoxEntryText( *cb_size, "128 MB", (void*)128, 0, eString().sprintf(_("Filesize: %d MB"),128).c_str());
	new eListBoxEntryText( *cb_size, "256 MB", (void*)256, 0, eString().sprintf(_("Filesize: %d MB"),256).c_str());
	new eListBoxEntryText( *cb_size, "512 MB", (void*)512, 0, eString().sprintf(_("Filesize: %d MB"),512).c_str());
	new eListBoxEntryText( *cb_size, "1024 MB", (void*)1024, 0, eString().sprintf(_("Filesize: %d MB"),1024).c_str());

//Abfragen der Grösse
	getSize();

//Button zum Löschen des Swapfiles
	bt_delswap = new eButton(this);
	bt_delswap->move(ePoint(250, 270));
	bt_delswap->resize(eSize(240, 30));
	bt_delswap->setShortcut("yellow");
	bt_delswap->setShortcutPixmap("yellow");
	bt_delswap->setText(_("Delete Swap-File"));//Swap-File loeschen
	bt_delswap->setHelpText(_("Delete Swap-File"));
	bt_delswap->loadDeco();
	bt_delswap->hide();

	CONNECT(bt_delswap->selected, eSwapManager::deleteSwap);

//Button zum Deaktivieren des Swapfiles
	bt_stswap = new eButton(this);
	bt_stswap->move(ePoint(10, 270));
	bt_stswap->resize(eSize(230, 30));
	bt_stswap->setShortcut("red");
	bt_stswap->setShortcutPixmap("red");
	bt_stswap->setText(_("Stop Swap"));
	bt_stswap->setHelpText(_("Stop Swap"));//ausschalten
	bt_stswap->loadDeco();
	bt_stswap->hide();

	CONNECT(bt_stswap->selected, eSwapManager::stopSwap);

//Button zum Erstellen des Swapfiles
	bt_crswap = new eButton(this);
	bt_crswap->move(ePoint(250, 270));
	bt_crswap->resize(eSize(240, 30));
	bt_crswap->setShortcut("blue");
	bt_crswap->setShortcutPixmap("blue");
	bt_crswap->setText(_("Create Swap-File"));//erstellen
	bt_crswap->setHelpText(_("Create Swap-File"));
	bt_crswap->loadDeco();
	bt_crswap->hide();

	CONNECT(bt_crswap->selected, eSwapManager::createSwap);

//Button zum Aktivieren des Swapfiles
	bt_acswap = new eButton(this);
	bt_acswap->move(ePoint(10, 270));
	bt_acswap->resize(eSize(230, 30));
	bt_acswap->setShortcut("green");
	bt_acswap->setShortcutPixmap("green");
	bt_acswap->setText(_("Start Swap"));
	bt_acswap->setHelpText(_("Start Swap"));// einschalten
	bt_acswap->loadDeco();
	bt_acswap->hide();

	CONNECT(bt_acswap->selected, eSwapManager::activateSwap);

//Label für die Gefunden-/Nicht gefunden-Anzeige
	lb_found= new eLabel(this);
	lb_found->move(ePoint(20, 220));
	lb_found->resize(eSize(400, 30));

//Label für die Aktiv-/Inaktiv-Anzeige	
	lb_status= new eLabel(this);
	lb_status->move(ePoint(20, 170));
	lb_status->resize(eSize(400, 30));

//Abfragen, ob Swapfile existiert
	searchSwapLight();
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
			hdddir = "/hdd/";
			mntbase = "/var/mnt/";
			break;
		case eSystemInfo::DM500PLUS:
		case eSystemInfo::DM600PVR:
		case eSystemInfo::DM7020:
			hdddir = "/media/hdd/";
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

	eString tmp = cb_mnt->getCurrent()->getText() + cb_file->getCurrent()->getText();

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

//Swapfile suchen (mit Listbox)
void eSwapManager::searchSwap(eListBoxEntryText *seltype)
{
	store = cb_mnt->getCurrent()->getText() + cb_file->getCurrent()->getText();

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

//Swapfile suchen (ohne Listbox)
void eSwapManager::searchSwapLight()
{
	store = cb_mnt->getCurrent()->getText() + cb_file->getCurrent()->getText();
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
	
	store = cb_mnt->getCurrent()->getText() + cb_file->getCurrent()->getText();

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

	eString tmp = cb_mnt->getCurrent()->getText() + cb_file->getCurrent()->getText();

	struct stat crsize;
	int created=0;

	if (stat(tmp.c_str(), &crsize) != -1)
		created=crsize.st_size;
		created/=(1024*1024);

	info->hide();

	if(created == byte)
	{
	eMessageBox debug((eString().sprintf(_("The swapfile has been created!"))), _("Swap Manager"), eMessageBox::btOK);
	debug.show();
	debug.exec();
	debug.hide();	

	searchSwapLight();

	lb_found->setText(eString().sprintf(_("Swap-File: %s"), found.c_str()));
		if( system(eString().sprintf("mkswap %s", store.c_str() ).c_str()) == 0)
			{
				int mkvalue = 1;
				eConfig::getInstance()->setKey("/extras/mkswap", mkvalue);
			}
		else
			{
				int mkvalue = 0;
				eConfig::getInstance()->setKey("/extras/mkswap", mkvalue);
			}
	}
	else
	{
			eMessageBox debug((eString().sprintf("%s\n%s",_("There has been an error during creation!"),strerror(errno))), _("Swap Manager"), eMessageBox::iconWarning|eMessageBox::btOK);
			debug.show();
			debug.exec();
			debug.hide();

	}
	setButtons();
}

//Swapfile aktivieren
void eSwapManager::activateSwap()
{
	store = cb_mnt->getCurrent()->getText() + cb_file->getCurrent()->getText();
	int mkvalue = 0;
	eConfig::getInstance()->getKey("/extras/mkswap", mkvalue);
	if (mkvalue == 0)
	{
		if( system(eString().sprintf("mkswap %s", store.c_str() ).c_str()) == 0)
			{
				int mkvalue = 1;
				eConfig::getInstance()->setKey("/extras/mkswap", mkvalue);
			}
		else
			{
				int mkvalue = 0;
				eConfig::getInstance()->setKey("/extras/mkswap", mkvalue);
			}
	}
	if ( system(eString().sprintf("swapon %s%s", cb_mnt->getCurrent()->getText().c_str(), cb_file->getCurrent()->getText().c_str() ).c_str())  == 0 )
	{
		system("echo 0 > /proc/sys/vm/swappiness");
		eConfig::getInstance()->setKey("/extras/swapfilename", store.c_str());
		eConfig::getInstance()->setKey("/extras/swapfile", 1);
		getState();
		setButtons();
	}
	else
	{
		eMessageBox debug((eString().sprintf("%s\n%s",_("Error during activation!"),strerror(errno))), _("Swap Manager"), eMessageBox::iconWarning|eMessageBox::btOK);
		debug.show();
		debug.exec();
		debug.hide();
	}
}

//Swapfile deaktivieren
void eSwapManager::stopSwap()
{
	eString selectedSwap = eString().sprintf("%s%s", cb_mnt->getCurrent()->getText().c_str(), cb_file->getCurrent()->getText().c_str() );

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
		eMessageBox debug((eString().sprintf("%s\n%s",_("Error during inactivation!"),strerror(errno))), _("Swap Manager"), eMessageBox::iconWarning|eMessageBox::btOK);
		debug.show();
		debug.exec();
		debug.hide();
	}
}

//swapfile löschen
void eSwapManager::deleteSwap()
{
	if ( system(eString().sprintf("rm -f %s%s", cb_mnt->getCurrent()->getText().c_str(), cb_file->getCurrent()->getText().c_str() ).c_str()) == 0 )
	{
// 		eConfig::getInstance()->setKey("/extras/swapfilename", "/var/mnt/usb/myswap");
		eMessageBox debug((_("Swapfile has been deleted!")), ("Swap Manager"), eMessageBox::iconInfo|eMessageBox::btOK);
		debug.show();
		debug.exec();
		debug.hide();

		searchSwapLight();
	}
	else
	{
		eMessageBox debug(eString().sprintf("%s\n%s",_("File has not been deleted due to an error!"),strerror(errno)), _("Swap Manager"), eMessageBox::iconWarning|eMessageBox::btOK);
		debug.show();
		debug.exec();
		debug.hide();
	}
}

eSwapManager::~eSwapManager()
{
	if (createSwapfile)
		delete createSwapfile;
}
#endif
#endif
