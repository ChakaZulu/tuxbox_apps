/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	Kommentar:

	Diese GUI wurde von Grund auf neu programmiert und sollte nun vom
	Aufbau und auch den Ausbaumoeglichkeiten gut aussehen. Neutrino basiert
	auf der Client-Server Idee, diese GUI ist also von der direkten DBox-
	Steuerung getrennt. Diese wird dann von Daemons uebernommen.


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
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gui/update.h>

#include <global.h>
#include <neutrino.h>

#include <driver/encoding.h>
#include <driver/fontrenderer.h>
#include <driver/rcinput.h>

#include <gui/color.h>
#include <gui/filebrowser.h>
#include <gui/nfs.h>

#include <gui/widget/messagebox.h>
#include <gui/widget/hintbox.h>

#include <system/flashtool.h>
#include <system/httptool.h>

#include <libcramfs.h>

#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <dirent.h>

#include <fstream>


#define gTmpPath "/var/tmp/"
#define gUserAgent "neutrino/softupdater 1.0"
#define cramfs_list_filename "cramfs.list"
#define local_cramfs_filename "update.cramfs"


CFlashUpdate::CFlashUpdate()
	:CProgressWindow()
{
	setTitle(g_Locale->getText("flashupdate.head")); // UTF-8

	installedVersion = g_settings.softupdate_currentversion;
}



class CUpdateMenuTarget : public CMenuTarget
{
	int    myID;
	int *  myselectedID;

public:
	CUpdateMenuTarget(const int id, int * const selectedID)
		{
			myID = id;
			myselectedID = selectedID;
		}

	virtual int exec(CMenuTarget* parent, const std::string & actionKey)
		{
			*myselectedID = myID;
			return menu_return::RETURN_EXIT_ALL;
		}
};


bool CFlashUpdate::selectHttpImage(void)
{
	CHTTPTool httpTool;
	std::string url;
	std::string name;
	std::string version;
	std::vector<std::string> urls, names, versions, descriptions;

	httpTool.setStatusViewer(this);
	showStatusMessageUTF(g_Locale->getText("flashupdate.getinfofile")); // UTF-8

	std::ifstream urlFile(g_settings.softupdate_url_file);

	while (urlFile >> url)
	{
		if (httpTool.downloadFile(url, gTmpPath cramfs_list_filename, 20))
		{
			std::ifstream in(gTmpPath cramfs_list_filename);
			while (in >> url >> version)
			{
				urls.push_back(url);
				versions.push_back(version);
				std::getline(in, name);
				names.push_back(name);
			}
		}
	}

	hide();

	if (urls.empty())
	{
		ShowHintUTF("messagebox.error", g_Locale->getText("flashupdate.getinfofileerror")); // UTF-8
		return false;
	}
		
	CMenuWidget SelectionWidget("flashupdate.selectimage", "softupdate.raw");
	
	SelectionWidget.addItem(GenericMenuSeparator);
	SelectionWidget.addItem(GenericMenuBack);
	SelectionWidget.addItem(GenericMenuSeparatorLine);
		
	int selected = -1;
	for (unsigned int i = 0; i < urls.size(); i++)
	{
		CFlashVersionInfo versionInfo(versions[i]);

		std::string description = versionInfo.getType();
		description += ' ';
		description += versionInfo.getDate();
		description += ' ';
		description += versionInfo.getTime();

		descriptions.push_back(description); /* workaround since CMenuForwarder does not store the Option String itself */

		SelectionWidget.addItem(new CMenuForwarder(names[i].c_str(), true, descriptions[i].c_str(), new CUpdateMenuTarget(i, &selected)));
	}

	SelectionWidget.exec(NULL, "");

	if (selected == -1)
		return false;

	filename = urls[selected];
	newVersion = versions[selected];

	return true;
}

bool CFlashUpdate::getUpdateImage(const std::string & version)
{
	CHTTPTool httpTool;
	httpTool.setStatusViewer(this);

	showStatusMessageUTF(std::string(g_Locale->getText("flashupdate.getupdatefile")) + ' ' + version); // UTF-8

	printf("get update (url): %s - %s\n", filename.c_str(), gTmpPath local_cramfs_filename);
	return httpTool.downloadFile(filename, gTmpPath local_cramfs_filename, 40 );
}

bool CFlashUpdate::checkVersion4Update()
{
	char msg[400];
	CFlashVersionInfo * versionInfo;
	const char * msg_body;

	if(g_settings.softupdate_mode==1) //internet-update
	{
		if(!selectHttpImage())
			return false;

		showLocalStatus(100);
		showGlobalStatus(20);
		showStatusMessageUTF(g_Locale->getText("flashupdate.versioncheck")); // UTF-8

		printf("internet version: %s\n", newVersion.c_str());

		if (newVersion == installedVersion)
		{
			ShowHintUTF("messagebox.error", g_Locale->getText("flashupdate.nonewversion")); // UTF-8
			return false;
		}
	
		showLocalStatus(100);
		showGlobalStatus(20);
		hide();
		
		versionInfo = new CFlashVersionInfo(newVersion);

		msg_body = "flashupdate.msgbox";
	}
	else
	{
		CFileBrowser CramfsBrowser;

		CFileFilter CramfsFilter;
		CramfsFilter.addFilter("cramfs");

		CramfsBrowser.Filter = &CramfsFilter;

		CFile * CFileSelected = NULL;

		CramfsBrowser.ChangeDir(gTmpPath);
		for (CFileList::iterator file = CramfsBrowser.filelist.begin(); file != CramfsBrowser.filelist.end(); file++)
		{
			if (!(S_ISDIR(file->Mode)))
			{
				if (CFileSelected == NULL)
					CFileSelected = &(*file);
				else
				{
					CFileSelected = NULL;
					break;
				}
			}
		}
		CramfsBrowser.hide();
		
		if (CFileSelected == NULL)
		{
			if (!(CramfsBrowser.exec(gTmpPath)))
				return false;
		
			CFileSelected = CramfsBrowser.getSelectedFile();

			if (CFileSelected == NULL)
				return false;
		}

		filename = CFileSelected->Name;

		FILE* fd = fopen(filename.c_str(), "r");
		if(fd)
		{
			fclose(fd);
		}
		else
		{
			hide();
			printf("flash-file not found: %s\n", filename.c_str());
			ShowHintUTF("messagebox.error", g_Locale->getText("flashupdate.cantopenfile")); // UTF-8
			return false;
		}
		hide();
		
		//bestimmung der CramfsDaten
		char cramfsName[30];
		cramfs_name((char *) filename.c_str(), (char *) &cramfsName);

		versionInfo = new CFlashVersionInfo(cramfsName);

		msg_body = "flashupdate.msgbox_manual";
	}
	sprintf(msg, g_Locale->getText(msg_body), versionInfo->getDate(), versionInfo->getTime(), versionInfo->getBaseImageVersion(), versionInfo->getType());

	if (strcmp("1.7", versionInfo->getBaseImageVersion()))
	{
		delete versionInfo;
		ShowHintUTF("messagebox.error", g_Locale->getText("flashupdate.wrongbase")); // UTF-8
		return false;
	}
	delete versionInfo;
	return (ShowMsgUTF("messagebox.info", msg, CMessageBox::mbrYes, CMessageBox::mbYes | CMessageBox::mbNo, "softupdate.raw") == CMessageBox::mbrYes); // UTF-8
}

int CFlashUpdate::exec(CMenuTarget* parent, const std::string &)
{
	if(parent)
	{
		parent->hide();
	}
	paint();

	if(!checkVersion4Update())
	{
		hide();
		return menu_return::RETURN_REPAINT;
	}
	showGlobalStatus(19);
	paint();
	showGlobalStatus(20);

	if(g_settings.softupdate_mode==1) //internet-update
	{
		if(!getUpdateImage(newVersion))
		{
			hide();
			ShowHintUTF("messagebox.error", g_Locale->getText("flashupdate.getupdatefileerror")); // UTF-8
			return menu_return::RETURN_REPAINT;
		}
	}

	showGlobalStatus(40);

	CFlashTool ft;
	ft.setMTDDevice("/dev/mtd/2");
	ft.setStatusViewer(this);

	//image-check
	showStatusMessageUTF(g_Locale->getText("flashupdate.md5check")); // UTF-8
	if(!ft.check_cramfs(filename))
	{
		hide();
		ShowHintUTF( "messagebox.error", g_Locale->getText("flashupdate.md5sumerror")); // UTF-8
		return menu_return::RETURN_REPAINT;
	}

	CNeutrinoApp::getInstance()->exec(NULL, "savesettings");
	sleep(2);
	showGlobalStatus(60);

	//flash it...
	if(!ft.program(filename, 80, 100))
	{
		hide();
		ShowHintUTF("messagebox.error", ft.getErrorMessage().c_str()); // UTF-8
		return menu_return::RETURN_REPAINT;
	}

	//status anzeigen
	showGlobalStatus(100);
	showStatusMessageUTF(g_Locale->getText("flashupdate.ready")); // UTF-8

	hide();

	// Unmount all NFS & CIFS volumes
	nfs_mounted_once = false; /* needed by update.cpp to prevent removal of modules after flashing a new cramfs, since rmmod (busybox) might no longer be available */
	CNFSUmountGui::umount();

	ShowHintUTF("messagebox.info", g_Locale->getText("flashupdate.flashreadyreboot")); // UTF-8
	ft.reboot();
	sleep(20000);

	hide();
	return menu_return::RETURN_REPAINT;
}


//--------------------------------------------------------------------------------------------------------------


CFlashExpert::CFlashExpert()
	:CProgressWindow()
{
	selectedMTD = -1;
}

void CFlashExpert::readmtd(int readmtd)
{
	char tmp[10];
	sprintf(tmp, "%d", readmtd);
	std::string filename = "/tmp/mtd";
	filename += tmp;
	filename += ".img"; // US-ASCII (subset of UTF-8 and ISO8859-1)
	if (readmtd == -1)
	{
		//ganzes flashimage lesen
		filename = "/tmp/flashimage.img"; // US-ASCII (subset of UTF-8 and ISO8859-1)
		readmtd = 4;
	}
	setTitle(g_Locale->getText("flashupdate.titlereadflash")); // UTF-8
	paint();
	showGlobalStatus(0);
	showStatusMessageUTF((std::string(g_Locale->getText("flashupdate.actionreadflash")) + " (" + CMTDInfo::getInstance()->getMTDName(readmtd) + ')')); // UTF-8
	CFlashTool ft;
	ft.setStatusViewer( this );
	ft.setMTDDevice(CMTDInfo::getInstance()->getMTDFileName(readmtd));
	if(!ft.readFromMTD(filename, 100))
	{
		showStatusMessageUTF(ft.getErrorMessage()); // UTF-8
		sleep(10);
	}
	else
	{
		showGlobalStatus(100);
		showStatusMessageUTF(g_Locale->getText("flashupdate.ready")); // UTF-8
		char message[500];
		sprintf(message, g_Locale->getText("flashupdate.savesuccess"), filename.c_str() );
		sleep(1);
		hide();
		ShowHintUTF("messagebox.info", message);
	}
}

void CFlashExpert::writemtd(const std::string & filename, int mtdNumber)
{
	char message[500];
#ifdef FILESYSTEM_IS_ISO8859_1_ENCODED
	sprintf(message, g_Locale->getText("flashupdate.reallyflashmtd"), Latin1_to_UTF8(filename).c_str(), CMTDInfo::getInstance()->getMTDName(mtdNumber).c_str());
#else
	sprintf(message, g_Locale->getText("flashupdate.reallyflashmtd"), filename.c_str(), CMTDInfo::getInstance()->getMTDName(mtdNumber).c_str());
#endif
	if (ShowMsgUTF("messagebox.info", message, CMessageBox::mbrNo, CMessageBox::mbYes | CMessageBox::mbNo, "softupdate.raw") != CMessageBox::mbrYes) // UTF-8
		return;

	setTitle(g_Locale->getText("flashupdate.titlewriteflash")); // UTF-8
	paint();
	showGlobalStatus(0);
	CFlashTool ft;
	ft.setStatusViewer( this );
	ft.setMTDDevice( CMTDInfo::getInstance()->getMTDFileName(mtdNumber) );
	if(!ft.program( "/tmp/" + filename, 50, 100))
	{
		showStatusMessageUTF(ft.getErrorMessage()); // UTF-8
		sleep(10);
	}
	else
	{
		showGlobalStatus(100);
		showStatusMessageUTF(g_Locale->getText("flashupdate.ready")); // UTF-8
		sleep(1);
		hide();
		ShowHintUTF("messagebox.info", g_Locale->getText("flashupdate.flashreadyreboot")); // UTF-8
		ft.reboot();
	}
}


void CFlashExpert::showMTDSelector(const std::string & actionkey)
{
	//mtd-selector erzeugen
	CMenuWidget* mtdselector = new CMenuWidget("flashupdate.mtdselector", "softupdate.raw");
	mtdselector->addItem(GenericMenuSeparator);
	mtdselector->addItem(new CMenuForwarder("messagebox.cancel"));
	mtdselector->addItem(GenericMenuSeparatorLine);
	CMTDInfo* mtdInfo =CMTDInfo::getInstance();
	for(int x=0;x<mtdInfo->getMTDCount();x++)
	{
		char sActionKey[20];
		sprintf(sActionKey, "%s%d", actionkey.c_str(), x);
		mtdselector->addItem(new CMenuForwarder(mtdInfo->getMTDName(x).c_str(), true, NULL, this, sActionKey));
	}
	mtdselector->exec(NULL,"");
}

void CFlashExpert::showFileSelector(const std::string & actionkey)
{
	CMenuWidget* fileselector = new CMenuWidget("flashupdate.fileselector", "softupdate.raw");
	fileselector->addItem(GenericMenuSeparator);
	fileselector->addItem(new CMenuForwarder("messagebox.cancel"));
	fileselector->addItem(GenericMenuSeparatorLine);
	struct dirent **namelist;
	int n = scandir("/tmp", &namelist, 0, alphasort);
	if (n < 0)
	{
		perror("no flashimages available");
		//should be available...
	}
	else
	{
		for(int count=0;count<n;count++)
		{
			std::string filen = namelist[count]->d_name;
			int pos = filen.find(".img");
			if(pos!=-1)
			{
				fileselector->addItem(new CMenuForwarder(filen.c_str(), true, NULL, this, (actionkey + filen).c_str()));
#warning TODO: make sure filen is UTF-8 encoded
			}
			free(namelist[count]);
		}
		free(namelist);
	}
	fileselector->exec(NULL,"");
}


int CFlashExpert::exec(CMenuTarget* parent, const std::string & actionKey)
{
	if(parent)
	{
		parent->hide();
	}

	if(actionKey=="readflash")
	{
		readmtd(-1);
	}
	else if(actionKey=="writeflash")
	{
		showFileSelector("");
	}
	else if(actionKey=="readflashmtd")
	{
		showMTDSelector("readmtd");
	}
	else if(actionKey=="writeflashmtd")
	{
		showMTDSelector("writemtd");
	}
	else
	{
		int iReadmtd = -1;
		int iWritemtd = -1;
		sscanf(actionKey.c_str(), "readmtd%d", &iReadmtd);
		sscanf(actionKey.c_str(), "writemtd%d", &iWritemtd);
		if(iReadmtd!=-1)
		{
			readmtd(iReadmtd);
		}
		else if(iWritemtd!=-1)
		{
			printf("mtd-write\n\n");
			selectedMTD = iWritemtd;
			showFileSelector("");
		}
		else
		{
			if(selectedMTD==-1)
			{
				//ganzes Image schreiben -> mtd 4
				writemtd("flashimage.img", 4);
			}
			else
			{
				writemtd(actionKey, selectedMTD);
				selectedMTD=-1;
			}
		}
		hide();
		return menu_return::RETURN_EXIT_ALL;
	}

	hide();
	return menu_return::RETURN_REPAINT;
}
