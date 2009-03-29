/*
	$Id: update.cpp,v 1.133 2009/03/29 16:55:42 seife Exp $

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
#include <driver/screen_max.h>

#include <gui/color.h>
#include <gui/filebrowser.h>
#include <system/fsmounter.h>
#include <gui/imageinfo.h>

#include <gui/widget/messagebox.h>
#include <gui/widget/hintbox.h>

#include <system/flashtool.h>
#ifndef DISABLE_INTERNET_UPDATE
#include <system/httptool.h>
#endif

#include <sectionsdclient/sectionsdclient.h>

#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <fstream>

//#define TESTING

#define gTmpPath "/tmp/"
#define gUserAgent "neutrino/softupdater 1.2"

#define LIST_OF_UPDATES_LOCAL_FILENAME "cramfs.list"
#define UPDATE_LOCAL_FILENAME          "update.cramfs"
#define RELEASE_CYCLE                  "2.0"
#define FILEBROWSER_UPDATE_FILTER      "cramfs"
#define FILEBROWSER_UPDATE_FILTER_ALT  "squashfs"
//#define MTD_OF_WHOLE_IMAGE             4
#ifdef HAVE_DREAMBOX_HARDWARE
#define MTD_TEXT_OF_WHOLE_IMAGE		"DreamBOX cramfs+squashfs"
#else
#define MTD_TEXT_OF_WHOLE_IMAGE		"Flash without bootloader"
#define MTD_DEVICE_OF_UPDATE_PART      "/dev/mtd/2"
#endif


CFlashUpdate::CFlashUpdate()
	:CProgressWindow()
{
	setTitle(LOCALE_FLASHUPDATE_HEAD);
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

	virtual int exec(CMenuTarget *, const std::string &)
		{
			*myselectedID = myID;
			return menu_return::RETURN_EXIT_ALL;
		}
};


class CNonLocalizedMenuSeparator : public CMenuSeparator
{
	const char * the_text;

public:
	CNonLocalizedMenuSeparator(const char *_text, const neutrino_locale_t Text) : CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, Text)
		{
			the_text = _text;
		}

	virtual const char * getString(void)
		{
			return the_text;
		}
};

#ifndef DISABLE_INTERNET_UPDATE
bool CFlashUpdate::selectHttpImage(void)
{
	CHTTPTool httpTool;
	std::string url;
	std::string md5;
	std::string name;
	std::string version;
	std::vector<std::string> updates_lists, urls, md5s, names, versions, descriptions;
	int selected = -1, listWidth = w_max (710, 50);

	// get default update url from .version
	CConfigFile config('\t');
	config.loadConfig("/.version");
	const std::string updateURL  = config.getString("update",  "");

	httpTool.setStatusViewer(this);
	showStatusMessageUTF(g_Locale->getText(LOCALE_FLASHUPDATE_GETINFOFILE)); // UTF-8

	CMenuWidget SelectionWidget(LOCALE_FLASHUPDATE_SELECTIMAGE, "softupdate.raw", listWidth);
	SelectionWidget.addItem(GenericMenuSeparator);
	SelectionWidget.addItem(GenericMenuBack);

	std::ifstream urlFile(g_settings.softupdate_url_file);

	unsigned int i = 0;
	bool update_prefix_tried = false;
	while (urlFile >> url)
	{
		// add update url from .version if exists, then seek back to start
		if (updateURL.length() > 0 && !update_prefix_tried)
		{
			url = updateURL + url;
			urlFile.seekg(0, std::ios::beg);
			update_prefix_tried = true;
		}

		std::string::size_type startpos, endpos;

		/* extract domain name */
		startpos = url.find("//");
		if (startpos == std::string::npos)
		{
			startpos = 0;
			endpos   = std::string::npos;
		}
		else
		{
			startpos += 2;
			endpos    = url.find('/', startpos);
		}
		updates_lists.push_back(url.substr(startpos, endpos - startpos));

		SelectionWidget.addItem(new CNonLocalizedMenuSeparator(updates_lists.rbegin()->c_str(), LOCALE_FLASHUPDATE_SELECTIMAGE));

		if (httpTool.downloadFile(url, gTmpPath LIST_OF_UPDATES_LOCAL_FILENAME, 20))
		{
			std::ifstream in(gTmpPath LIST_OF_UPDATES_LOCAL_FILENAME);
			while (in >> url >> md5 >> version >> std::ws)
			{
				urls.push_back(url);
				md5s.push_back(md5);
				versions.push_back(version);
				std::getline(in, name);
				names.push_back(name);

				CFlashVersionInfo versionInfo(versions[i]);

				std::string description = versionInfo.getType();
				description += ' ';
				description += versionInfo.getDate();
				description += ' ';
				description += versionInfo.getTime();

				descriptions.push_back(description); /* workaround since CMenuForwarder does not store the Option String itself */

				SelectionWidget.addItem(new CMenuForwarderNonLocalized(names[i].c_str(), true, descriptions[i].c_str(), new CUpdateMenuTarget(i, &selected)));
				i++;
			}
		}
	}

	hide();

	if (urls.empty())
	{
		ShowHintUTF(LOCALE_MESSAGEBOX_ERROR, g_Locale->getText(LOCALE_FLASHUPDATE_GETINFOFILEERROR)); // UTF-8
		return false;
	}

	SelectionWidget.exec(NULL, "");

	if (selected == -1)
		return false;

	filename = urls[selected];
	filemd5 = md5s[selected];
	newVersion = versions[selected];

	return true;
}

bool CFlashUpdate::getUpdateImage(const std::string & version)
{
	CHTTPTool httpTool;
	httpTool.setStatusViewer(this);
		
	showStatusMessageUTF(std::string(g_Locale->getText(LOCALE_FLASHUPDATE_GETUPDATEFILE)) + ' ' + version); // UTF-8

	printf("[update] get update (url): %s - %s\n", filename.c_str(), gTmpPath UPDATE_LOCAL_FILENAME);
	return httpTool.downloadFile(filename, gTmpPath UPDATE_LOCAL_FILENAME, 40 );
}
#endif

bool CFlashUpdate::checkVersion4Update()
{
	char msg[400];
	CFlashVersionInfo * versionInfo=0;
	neutrino_locale_t msg_body;

#ifndef DISABLE_INTERNET_UPDATE
#ifndef HAVE_DREAMBOX_HARDWARE
	if(g_settings.softupdate_mode==1) //internet-update
	{
		if(!selectHttpImage())
			return false;

		showLocalStatus(100);
		showGlobalStatus(20);
		showStatusMessageUTF(g_Locale->getText(LOCALE_FLASHUPDATE_VERSIONCHECK)); // UTF-8

		printf("[update] internet version: %s\n", newVersion.c_str());

		showLocalStatus(100);
		showGlobalStatus(20);
		hide();

		versionInfo = new CFlashVersionInfo(newVersion);

		msg_body = LOCALE_FLASHUPDATE_MSGBOX;
	}
	else
#endif
#endif
	{
		CFileBrowser UpdatesBrowser;

		CFileFilter UpdatesFilter;
		UpdatesFilter.addFilter(FILEBROWSER_UPDATE_FILTER);
		UpdatesFilter.addFilter(FILEBROWSER_UPDATE_FILTER_ALT);

		UpdatesBrowser.Filter = &UpdatesFilter;

		CFile * CFileSelected = NULL;

		UpdatesBrowser.ChangeDir(gTmpPath);
		for (CFileList::iterator file = UpdatesBrowser.filelist.begin(); file != UpdatesBrowser.filelist.end(); file++)
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
		UpdatesBrowser.hide();

		if (CFileSelected == NULL)
		{
			if (!(UpdatesBrowser.exec(gTmpPath)))
				return false;

			CFileSelected = UpdatesBrowser.getSelectedFile();

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
			printf("[update] flash-file not found: %s\n", filename.c_str());
			ShowHintUTF(LOCALE_MESSAGEBOX_ERROR, g_Locale->getText(LOCALE_FLASHUPDATE_CANTOPENFILE)); // UTF-8
			return false;
		}
		hide();

		CFlashTool ft;
		versionInfo = new CFlashVersionInfo();
		if (!ft.GetVersionInfo(*versionInfo, filename))
		{
			ShowHintUTF(LOCALE_MESSAGEBOX_ERROR, g_Locale->getText(LOCALE_FLASHUPDATE_CANTOPENFILE)); // UTF-8
			return false;			
		}

		msg_body = LOCALE_FLASHUPDATE_MSGBOX_MANUAL;
	}

	sprintf(msg, g_Locale->getText(msg_body), versionInfo->getDate(), versionInfo->getTime(), versionInfo->getReleaseCycle(), versionInfo->getType());

	if (strcmp(RELEASE_CYCLE, versionInfo->getReleaseCycle()))
	{
		delete versionInfo;
		ShowHintUTF(LOCALE_MESSAGEBOX_ERROR, g_Locale->getText(LOCALE_FLASHUPDATE_WRONGBASE)); // UTF-8
		return false;
	}

	if ((strcmp("Release", versionInfo->getType()) != 0) &&
	    (ShowLocalizedMessage(LOCALE_MESSAGEBOX_INFO, LOCALE_FLASHUPDATE_EXPERIMENTALIMAGE, CMessageBox::mbrYes, CMessageBox::mbYes | CMessageBox::mbNo, "softupdate.raw") != CMessageBox::mbrYes))
	{
		delete versionInfo;
		return false;
	}

	delete versionInfo;

	return (ShowMsgUTF(LOCALE_MESSAGEBOX_INFO, msg, CMessageBox::mbrYes, CMessageBox::mbYes | CMessageBox::mbNo, "softupdate.raw") == CMessageBox::mbrYes); // UTF-8
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
	
#ifdef LCD_UPDATE
	CLCD::getInstance()->showProgressBar2(0,NULL,0, g_Locale->getText(LOCALE_FLASHUPDATE_GLOBALPROGRESS));
	CLCD::getInstance()->setMode(CLCD::MODE_PROGRESSBAR2);
#endif // LCD_UPDATE

	showGlobalStatus(19);
	paint();
	showGlobalStatus(20);

	bool looks_like_cramfs = filename.find(".cramfs") != unsigned(-1);
#ifndef DISABLE_INTERNET_UPDATE
	if(g_settings.softupdate_mode==1) //internet-update
	{
		if(!getUpdateImage(newVersion))
		{
			hide();
			ShowHintUTF(LOCALE_MESSAGEBOX_ERROR, g_Locale->getText(LOCALE_FLASHUPDATE_GETUPDATEFILEERROR)); // UTF-8
			return menu_return::RETURN_REPAINT;
		}
		filename = std::string(gTmpPath UPDATE_LOCAL_FILENAME);
	}
#endif

	showGlobalStatus(40);

	CFlashTool ft;
	ft.setStatusViewer(this);

#ifdef HAVE_DREAMBOX_HARDWARE
	// This check was previously used only on squashfs-images
	if(g_settings.softupdate_mode==1) //internet-update
	{
		showStatusMessageUTF(g_Locale->getText(LOCALE_FLASHUPDATE_MD5CHECK)); // UTF-8

		if(!ft.MD5Check(filename, filemd5))
		{
			hide();
			ShowHintUTF(LOCALE_MESSAGEBOX_ERROR, g_Locale->getText(LOCALE_FLASHUPDATE_MD5SUMERROR)); // UTF-8
			return menu_return::RETURN_REPAINT;
		}
	}
#endif

	// If the file name contains the string ".cramfs", check that it
	// is a valid cramfs image
	if (looks_like_cramfs)
	{
		printf("[update] Checking it for cramfs correctness\n");
		showStatusMessageUTF(g_Locale->getText(LOCALE_FLASHUPDATE_MD5CHECK)); // UTF-8
		if(!ft.check_cramfs(filename))
		{
			hide();
			ShowHintUTF(LOCALE_MESSAGEBOX_ERROR, g_Locale->getText(LOCALE_FLASHUPDATE_MD5SUMERROR)); // UTF-8
			return menu_return::RETURN_REPAINT;
		}
	}

	struct stat buf;
	stat(filename.c_str(), &buf);
	int filesize = buf.st_size;

	// Is the file size that of a full image? Then flash as such.
	unsigned int mtd_of_whole_image = CMTDInfo::getInstance()->findMTDNumberFromDescription(MTD_TEXT_OF_WHOLE_IMAGE);
#ifndef HAVE_DREAMBOX_HARDWARE
	unsigned int mtd_of_update_image = CMTDInfo::getInstance()->findMTDNumber(MTD_DEVICE_OF_UPDATE_PART);
#else
	unsigned int mtd_of_update_image = mtd_of_whole_image;
#endif
	if (mtd_of_whole_image == (unsigned int) -1 || mtd_of_update_image == (unsigned int) -1)
	{
		printf("Cannot determine partition numbers, aborting flashing\n");
		hide();
		ShowHintUTF(LOCALE_MESSAGEBOX_ERROR, "Internal error");	// I don't care to localize...
		return menu_return::RETURN_REPAINT;
	}

	if (filesize == CMTDInfo::getInstance()->getMTDSize(mtd_of_whole_image))
	{
		ft.setMTDDevice(CMTDInfo::getInstance()->getMTDFileName(mtd_of_whole_image));
		printf("full image %d %d\n", filesize,mtd_of_whole_image);
#ifndef HAVE_DREAMBOX_HARDWARE
	} else 
	// Is filesize <= root partition? Then flash as update.
	if (filesize <= CMTDInfo::getInstance()->getMTDSize(mtd_of_update_image)) {
	  	ft.setMTDDevice(MTD_DEVICE_OF_UPDATE_PART);
		printf("update image %d %d\n", filesize, mtd_of_update_image);
#endif
	} else {
	// Otherwise reject
		printf("NO update due to erroneous file size %d %d\n", filesize, CMTDInfo::getInstance()->getMTDSize(mtd_of_update_image));
		hide();
		ShowHintUTF(LOCALE_MESSAGEBOX_ERROR, g_Locale->getText(LOCALE_FLASHUPDATE_MD5SUMERROR)); // UTF-8
		return menu_return::RETURN_REPAINT;
	}

	CSectionsdClient sd;
	bool sd_scan = sd.getIsScanningActive();
	// restart sectionsd, this frees up memory
	printf("[flashtool] restarting sectionsd to free memory\n");
	sd.Restart();

	CNeutrinoApp::getInstance()->exec(NULL, "savesettings");
	sleep(2);
	showGlobalStatus(60);

#ifdef TESTING
	printf("+++++++++++++++++++ NOT flashing, just testing\n");
	hide();
	unlink(filename.c_str());
	sd.RegisterNeutrino();
	sd.setPauseScanning(!sd_scan);
	return menu_return::RETURN_REPAINT;
#endif

	//flash it...
	if(!ft.program(filename, 80, 100))
	{
		hide();
		ShowHintUTF(LOCALE_MESSAGEBOX_ERROR, ft.getErrorMessage().c_str()); // UTF-8
		sd.RegisterNeutrino();
		sd.setPauseScanning(!sd_scan);
		return menu_return::RETURN_REPAINT;
	}

	//status anzeigen
	showGlobalStatus(100);
	showStatusMessageUTF(g_Locale->getText(LOCALE_FLASHUPDATE_READY)); // UTF-8

	hide();

	// Unmount all NFS & CIFS volumes
	nfs_mounted_once = false; /* needed by update.cpp to prevent removal of modules after flashing a new cramfs, since rmmod (busybox) might no longer be available */
	CFSMounter::umount();

	ShowHintUTF(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_FLASHUPDATE_FLASHREADYREBOOT)); // UTF-8
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

void CFlashExpert::readmtd(int mtd)
{
	char tmp[10];
	sprintf(tmp, "%d", mtd);
	std::string filename = "/tmp/mtd";
	filename += tmp;
	filename += ".img"; // US-ASCII (subset of UTF-8 and ISO8859-1)
	if (mtd == -1)
	{
		filename = "/tmp/flashimage.img"; // US-ASCII (subset of UTF-8 and ISO8859-1)
		mtd = CMTDInfo::getInstance()->findMTDNumberFromDescription(MTD_TEXT_OF_WHOLE_IMAGE); //MTD_OF_WHOLE_IMAGE;
	}
	setTitle(LOCALE_FLASHUPDATE_TITLEREADFLASH);
	paint();
	showGlobalStatus(0);
	showStatusMessageUTF((std::string(g_Locale->getText(LOCALE_FLASHUPDATE_ACTIONREADFLASH)) + " (" + CMTDInfo::getInstance()->getMTDName(mtd) + ')')); // UTF-8
	CFlashTool ft;
	ft.setStatusViewer( this );
	ft.setMTDDevice(CMTDInfo::getInstance()->getMTDFileName(mtd));
	if(!ft.readFromMTD(filename, 100))
	{
		showStatusMessageUTF(ft.getErrorMessage()); // UTF-8
		sleep(10);
	}
	else
	{
		showGlobalStatus(100);
		showStatusMessageUTF(g_Locale->getText(LOCALE_FLASHUPDATE_READY)); // UTF-8
		char message[500];
		sprintf(message, g_Locale->getText(LOCALE_FLASHUPDATE_SAVESUCCESS), filename.c_str());
		sleep(1);
		hide();
		ShowHintUTF(LOCALE_MESSAGEBOX_INFO, message);
	}
}


void CFlashExpert::writemtd(const std::string & filename, int mtdNumber)
{
	
	char message[500];
	static CImageInfo imageinfo;
	const char* mtdtarget = CMTDInfo::getInstance()->getMTDName(mtdNumber).c_str();
	const char* imagefile = FILESYSTEM_ENCODING_TO_UTF8_STRING(filename).c_str();	

	if (mtdNumber >3) 
	{
		sprintf(message,
		g_Locale->getText(LOCALE_FLASHUPDATE_REALLYFLASHCHIPSET),
		imageinfo.getChipInfo().c_str(),
		imagefile, mtdtarget);
	}
	else
	{	
		sprintf(message,
		g_Locale->getText(LOCALE_FLASHUPDATE_REALLYFLASHMTD),
		imagefile, mtdtarget);
	}	
		
	if (ShowMsgUTF(LOCALE_MESSAGEBOX_INFO, message, CMessageBox::mbrNo, CMessageBox::mbYes | CMessageBox::mbNo, "softupdate.raw") != CMessageBox::mbrYes) // UTF-8
		return;

#ifdef LCD_UPDATE
	CLCD::getInstance()->showProgressBar2(0,NULL,0, g_Locale->getText(LOCALE_FLASHUPDATE_GLOBALPROGRESS));
	CLCD::getInstance()->setMode(CLCD::MODE_PROGRESSBAR2);
#endif // LCD_UPDATE

	setTitle(LOCALE_FLASHUPDATE_TITLEWRITEFLASH);
	paint();
	showGlobalStatus(0);
	CFlashTool ft;
	ft.setStatusViewer( this );
	ft.setMTDDevice( CMTDInfo::getInstance()->getMTDFileName(mtdNumber) );

	CSectionsdClient sd;
	bool sd_scan = sd.getIsScanningActive();
	// restart sectionsd, this frees up memory
	printf("[flashtool] restarting sectionsd to free memory\n");
	sd.Restart();

#ifdef TESTING
	printf("+++++++++++++++++++ NOT flashing, just testing\n");
	hide();
	sd.RegisterNeutrino();
	sd.setPauseScanning(!sd_scan);
	unlink(filename.c_str());
	return;
#endif
	if(!ft.program( "/tmp/" + filename, 50, 100))
	{
		showStatusMessageUTF(ft.getErrorMessage()); // UTF-8
		sd.RegisterNeutrino();
		sd.setPauseScanning(!sd_scan);
		sleep(10);
	}
	else
	{
		showGlobalStatus(100);
		showStatusMessageUTF(g_Locale->getText(LOCALE_FLASHUPDATE_READY)); // UTF-8
		sleep(1);
		hide();
		ShowHintUTF(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_FLASHUPDATE_FLASHREADYREBOOT)); // UTF-8
		ft.reboot();
	}
}


void CFlashExpert::showMTDSelector(const std::string & actionkey)
{
	//mtd-selector erzeugen
	CMenuWidget* mtdselector = new CMenuWidget(LOCALE_FLASHUPDATE_MTDSELECTOR, "softupdate.raw");
	mtdselector->addItem(GenericMenuSeparator);
	mtdselector->addItem(new CMenuForwarder(LOCALE_MESSAGEBOX_CANCEL));
	mtdselector->addItem(GenericMenuSeparatorLine);
	CMTDInfo* mtdInfo =CMTDInfo::getInstance();
	for (int i = 0; i < mtdInfo->getMTDCount(); i++)
	{
		char sActionKey[20];
		sprintf(sActionKey, "%s%d", actionkey.c_str(), i);
		mtdselector->addItem(new CMenuForwarderNonLocalized(mtdInfo->getMTDName(i).c_str(), true, NULL, this, sActionKey));
	}
	mtdselector->exec(NULL,"");
	delete mtdselector;
}

void CFlashExpert::showFileSelector(const std::string & actionkey)
{
	CMenuWidget* fileselector = new CMenuWidget(LOCALE_FLASHUPDATE_FILESELECTOR, "softupdate.raw");
	fileselector->addItem(GenericMenuSeparator);
	fileselector->addItem(new CMenuForwarder(LOCALE_MESSAGEBOX_CANCEL));
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
			if((int(filen.find(".img")) != -1) 
			   || (int(filen.find(".squashfs")) != -1)
			   || (int(filen.find(".cramfs")) != -1)
			   || (int(filen.find(".jffs2")) != -1)
			   || (int(filen.find(".flfs")) != -1)
			   )
			{
				fileselector->addItem(new CMenuForwarderNonLocalized(filen.c_str(), true, NULL, this, (actionkey + filen).c_str()));
#warning TODO: make sure filen is UTF-8 encoded
			}
			free(namelist[count]);
		}
		free(namelist);
	}
	fileselector->exec(NULL,"");
	delete fileselector;
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
				writemtd(actionKey, CMTDInfo::getInstance()->findMTDNumberFromDescription(MTD_TEXT_OF_WHOLE_IMAGE)/*MTD_OF_WHOLE_IMAGE*/);
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
