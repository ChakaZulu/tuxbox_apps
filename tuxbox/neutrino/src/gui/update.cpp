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

#include "global.h"

#include "update.h"
#include "neutrino.h"
#include "gui/widget/messagebox.h"
#include "gui/widget/hintbox.h"
#include "system/flashtool.h"
#include "system/httptool.h"

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <dirent.h>

#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>


#define gTmpPath "/var/tmp/"
#define gUserAgent "neutrino/softupdater 1.0"



CHTTPUpdater::CHTTPUpdater()
{
	BasePath = "http://dboxupdate.berlios.de/update/";
	ImageFile = "cramfs.img";
	VersionFile = "version";
	//use other path?
	FILE* fd = fopen("/var/etc/update.conf", "r");
	if(fd)
	{
		char buf[1000];
		char buf2[1000];
		if(fgets(buf,sizeof(buf),fd)!=NULL)
		{
			sscanf(buf, "basepath: %s\n", &buf2);
		}
		BasePath = buf2;
		if(fgets(buf,sizeof(buf),fd)!=NULL)
		{
			sscanf(buf, "imagefile: %s\n", &buf2);
			if (strlen(buf2)> 0)
			{
				ImageFile = buf2;
			}

			if(fgets(buf,sizeof(buf),fd)!=NULL)
			{
				sscanf(buf, "versionfile: %s\n", &buf2);
				if (strlen(buf2)> 0)
				{
					VersionFile = buf2;
				}
			}
		}
		fclose(fd);

		printf("[CHTTPUpdater] HTTP-Basepath: %s\n", BasePath.c_str() );
		printf("[CHTTPUpdater] Image-Filename: %s\n", ImageFile.c_str() );
		printf("[CHTTPUpdater] Version-Filename: %s\n", VersionFile.c_str() );
	}
	statusViewer = NULL;
}

void CHTTPUpdater::setStatusViewer( CProgress_StatusViewer* statusview )
{
	statusViewer = statusview;
}


int CHTTPUpdater::show_progress( void *clientp, size_t dltotal, size_t dlnow, size_t ultotal, size_t ulnow)
{

	((CProgress_StatusViewer*)clientp)->showLocalStatus( int( dlnow*100.0/dltotal ) );
	return 0;
}

bool CHTTPUpdater::getInfo()
{
	CHTTPTool httpTool;
	httpTool.setStatusViewer( statusViewer );

	statusViewer->showStatusMessage( g_Locale->getText("flashupdate.getinfofile") );
	string gURL = BasePath + VersionFile;
	string sFileName = gTmpPath+ VersionFile;

	return httpTool.downloadFile( gURL, sFileName );
}

bool CHTTPUpdater::getFile( string version )
{
	CHTTPTool httpTool;
	httpTool.setStatusViewer( statusViewer );

	statusViewer->showStatusMessage( g_Locale->getText("flashupdate.getupdatefile")+ " v"+ version );
	string gURL = BasePath + ImageFile;
	string sFileName = gTmpPath+ ImageFile;

	return httpTool.downloadFile( gURL, sFileName );
}


CFlashUpdate::CFlashUpdate()
{
	frameBuffer = CFrameBuffer::getInstance();
	width = 430;
	hheight = g_Fonts->menu_title->getHeight();
	mheight = g_Fonts->menu->getHeight();
	height = hheight+5*mheight+20;

	globalstatus = -1;

	x= ( ( ( g_settings.screen_EndX- g_settings.screen_StartX ) - width ) >> 1 ) + g_settings.screen_StartX;
	y=(576-height)>>1;
}


int CFlashUpdate::exec(CMenuTarget* parent, string)
{
	if (parent)
	{
		parent->hide();
	}
	g_Sectionsd->setPauseScanning( true );
	paint();
	g_Sectionsd->setPauseScanning( false );

	int res = g_RCInput->messageLoop();

	hide();
	return res;
}

void CFlashUpdate::hide()
{
	frameBuffer->paintBackgroundBoxRel(x,y, width,height);
}

int CFlashUpdate::getGlobalStatus()
{
	return globalstatus;
}

void CFlashUpdate::showGlobalStatus(int prog)
{
	if(prog>100)
	{
		prog = 100;
	}
	if(prog<0)
	{
		prog=0;
	}
	if(globalstatus==prog)
	{
		return;
	}
	globalstatus = prog;

	frameBuffer->paintBox(x+10, globalstatusY, x+width-10, globalstatusY+10, COL_MENUCONTENT +2);
	if(prog!=0)
	{
		int pos = x+10+( (width- 20)/100* prog);
		frameBuffer->paintBox(x+10, globalstatusY,pos, globalstatusY+10, COL_MENUCONTENT +7);
	}
}

void CFlashUpdate::showLocalStatus(int prog)
{
	static int lastprog = -1;

	if(prog>100)
	{
		prog = 100;
	}
	if(prog<0)
	{
		prog=0;
	}

	if(lastprog==prog)
	{
		return;
	}
	lastprog = prog;

	frameBuffer->paintBox(x+10, localstatusY, x+width-10, localstatusY+10, COL_MENUCONTENT +2);
	if(prog!=0)
	{
		int pos = x+10+((width- 20)/100* prog);
		frameBuffer->paintBox(x+10, localstatusY, pos, localstatusY+10, COL_MENUCONTENT +7);
	}
}

void CFlashUpdate::showStatusMessage(string text)
{
	frameBuffer->paintBox(statusTextX-5, statusTextY-mheight, x+width, statusTextY,  COL_MENUCONTENT);
	g_Fonts->menu->RenderString(statusTextX, statusTextY, width -(statusTextX- x), text.c_str(), COL_MENUCONTENT);
}

bool CFlashUpdate::checkVersion4Update(int ypos, string &sFileName)
{
	//installierte version...
	installed_major = installed_provider = 0;
	strcpy(installed_minor, "0");
	sscanf(g_settings.softupdate_currentversion, "%d.%d.%s", &installed_major, &installed_provider, (char*) &installed_minor);

	//neue version?
	new_major = new_provider = 0;
	strcpy(new_minor, "0");
	strcpy(new_md5sum, "");

	sFileName = gTmpPath+ httpUpdater.VersionFile;
	FILE* fd = fopen(sFileName.c_str(), "r");
	if(!fd)
	{
		sFileName= sFileName+ ".txt";
		fd = fopen(sFileName.c_str(), "r");
	}

	if(!fd)
	{
		showStatusMessage( g_Locale->getText("flashupdate.getinfofileerror") );
		return false;
	}
	char buf[100];
	if(fgets(buf,sizeof(buf),fd)!=NULL)
	{
		//printf("vstr: %s\n", buf);
		buf[28]= 0;
		sscanf(buf, "version: %d.%d.%s\n", &new_major, &new_provider, (char*) &new_minor);
	}
	else
	{
		showStatusMessage( g_Locale->getText("flashupdate.getinfofileerror") );
		return false;
	}
	if(fgets(buf,sizeof(buf),fd)!=NULL)
	{
		sscanf(buf, "md5sum: %s\n", (char*) &new_md5sum);
	}
	fclose(fd);

	//printf("installed - %d : %d : %s\n", installed_major, installed_provider, installed_minor);
	//printf("new - %d : %d : %s\n", new_major, new_provider, new_minor);
	if(installed_major!=new_major)
	{
		g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width- 10, g_Locale->getText("flashupdate.majorversiondiffer1").c_str() , COL_MENUCONTENT);
		ypos+= mheight;
		g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width- 10, g_Locale->getText("flashupdate.majorversiondiffer2").c_str() , COL_MENUCONTENT);
		return false;
	}
	if(installed_provider!=new_provider)
	{
		g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width- 10, g_Locale->getText("flashupdate.providerversiondiffer1").c_str() , COL_MENUCONTENT);
		ypos+= mheight;
		g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width- 10, g_Locale->getText("flashupdate.providerversiondiffer2").c_str() , COL_MENUCONTENT);
		return false;
	}
	if(strcmp(installed_minor,new_minor)== 0)
	{
		g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width- 10, g_Locale->getText("flashupdate.nonewversion1").c_str() , COL_MENUCONTENT);
		ypos+= mheight;
		g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width- 10, g_Locale->getText("flashupdate.nonewversion2").c_str() , COL_MENUCONTENT);
		return false;
	}
	return true;
}

void CFlashUpdate::paint()
{
	int fp_fd = 0;
	if ((fp_fd = open("/dev/dbox/fp0",O_RDWR)) <= 0)
	{
		perror("[neutrino] open fp0");
		return;
	}

	int ypos=y;
	frameBuffer->paintBoxRel(x, ypos, width, hheight, COL_MENUHEAD);
	g_Fonts->menu_title->RenderString(x+10, ypos+ hheight, width- 10, g_Locale->getText("flashupdate.head").c_str(), COL_MENUHEAD);
	frameBuffer->paintBoxRel(x, ypos+ hheight, width, height- hheight, COL_MENUCONTENT);

	ypos+= hheight + (mheight >>1);

	g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width- 10,  g_Locale->getText("flashupdate.action").c_str(), COL_MENUCONTENT);
	statusTextY = ypos+mheight;
	statusTextX = x+10 + g_Fonts->menu->getRenderWidth(g_Locale->getText("flashupdate.action").c_str())+10;
	ypos+= mheight;

	localstatusY = ypos+ mheight-20;
	showLocalStatus(0);
	ypos+= mheight+10;

	httpUpdater.setStatusViewer(this);

	if(g_settings.softupdate_mode==1) //internet-update
	{
		if(!httpUpdater.getInfo())
		{
			showStatusMessage( g_Locale->getText("flashupdate.getinfofileerror") );
			close(fp_fd);
			return;
		}
	}
	//versionsprüfung.....
	showLocalStatus(100);
	showStatusMessage(g_Locale->getText("flashupdate.versioncheck").c_str());

	string sFileName = "";
	if(!checkVersion4Update(ypos, sFileName))
	{
		return;
	}

	char msg[250];
	sprintf( (char*) &msg, g_Locale->getText("flashupdate.msgbox").c_str(), new_major, new_provider, new_minor);
    if ( ShowMsg ( "messagebox.info", msg, CMessageBox::mbrYes, CMessageBox::mbYes | CMessageBox::mbNo, "softupdate.raw" ) != CMessageBox::mbrYes )
    {
    	return;
    }

	//start update
	g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width- 10, g_Locale->getText("flashupdate.globalprogress").c_str() , COL_MENUCONTENT);
	ypos+= mheight;

	globalstatusY = ypos+ mheight-20;
	ypos+= mheight >>1;
	showGlobalStatus(0);

	if(g_settings.softupdate_mode==1) //internet-update
	{
		if(!httpUpdater.getFile( new_minor ))
		{
			showStatusMessage(g_Locale->getText("flashupdate.getupdatefileerror") );
			close(fp_fd);
			return;
		}
	}

	showGlobalStatus(25);

	//md5check...
	unsigned char   md5buffer[16];
	char            md5string[40]="";

	sFileName = gTmpPath+ httpUpdater.ImageFile;
	showStatusMessage(g_Locale->getText("flashupdate.md5check") );
	if( md5_file(sFileName.c_str(), 1, (unsigned char*) &md5buffer))
	{
		showStatusMessage(g_Locale->getText("flashupdate.cantopenfile") );
		close(fp_fd);
		return;
	}
	for(int count=0;count<16;count++)
	{
		char tmp[6];
		sprintf((char*) &tmp, "%02x", md5buffer[count] );
		strcat(md5string, tmp);
	}
	printf("%s\n%s\n\n", new_md5sum, md5string);
	if(strcmp(md5string, new_md5sum)!=0)
	{
		showStatusMessage(g_Locale->getText("flashupdate.md5sumerror") );
		close(fp_fd);
		return;
	}

	showGlobalStatus(50);
	//flash it...
	CFlashTool ft;
	ft.setMTDDevice("/dev/mtd/3");
	ft.setStatusViewer(this);
	if(!ft.program(sFileName))
	{
		showStatusMessage( ft.getErrorMessage() );
		close(fp_fd);
		return;
	}

	//versionsinfo schreiben
	FILE* fd2 = fopen("/var/etc/version", "w");
	fprintf(fd2, "%d.%d.%s\n", new_major, new_provider, new_minor);
	fflush(fd2);
	fclose(fd2);


	showGlobalStatus(100);
	showStatusMessage( g_Locale->getText("flashupdate.ready") );

	CNeutrinoApp::getInstance()->exec(NULL, "savesettings");

	sleep(2);

	frameBuffer->paintBoxRel(x, y+ hheight, width, height- hheight, COL_MENUCONTENT);
	g_Fonts->menu->RenderString(x+ 10, y+ mheight*3, width- 10, g_Locale->getText("flashupdate.reboot").c_str() , COL_MENUCONTENT);

	sleep(2);
	if (ioctl(fp_fd,FP_IOCTL_REBOOT)< 0)
	{
		perror("FP_IOCTL_REBOOT:");
	}
	close(fp_fd);
	sleep(20000);
}


//--------------------------------------------------------------------------------------------------------------


CFlashExpert::CFlashExpert()
	:CProgressWindow()
{

}

void CFlashExpert::readflash()
{
	setTitle( g_Locale->getText("flashupdate.titlereadflash"));
	paint();
	showGlobalStatus(0);
	showStatusMessage(g_Locale->getText("flashupdate.actionreadflash"));
	CFlashTool ft;
	ft.setStatusViewer( this );
	ft.setMTDDevice("/dev/mtd/5");
	if(!ft.readFromMTD("/tmp/flashimage.img"))
	{
		showStatusMessage( ft.getErrorMessage() );
		sleep(10);
	}
	else
	{
		showGlobalStatus(100);
		showStatusMessage( g_Locale->getText("flashupdate.ready"));
		char message[500];
		sprintf(message, g_Locale->getText("flashupdate.savesuccess").c_str(), "/tmp/flashimage.img");
		sleep(1);
		hide();
		ShowHint ( "messagebox.info", message );
	}
}

void CFlashExpert::writeflash()
{
		CMenuWidget* fileselector = new CMenuWidget("flashupdate.fileselector", "softupdate.raw");
		fileselector->addItem( new CMenuSeparator() );
		fileselector->addItem( new CMenuForwarder("messagebox.cancel") );
		fileselector->addItem( new CMenuSeparator(CMenuSeparator::LINE) );
/*
		CMTDInfo* mtdInfo =CMTDInfo::getInstance();
		for(int x=0;x<mtdInfo->getMTDCount();x++)
		{
			char actionKey[20];
			sprintf(actionKey, "readmtd%d", x);
			mtdselector->addItem(  new CMenuForwarder( mtdInfo->getMTDName(x), true, "", this, actionKey ) );
		}
		*/
		fileselector->exec(NULL,"");
}


void CFlashExpert::readmtd(int readmtd)
{
	char tmp[10];
	sprintf(tmp, "%d", readmtd);
	string filename = "/tmp/mtd" + string(tmp) + string(".img");
	setTitle(g_Locale->getText("flashupdate.titlereadflash"));
	paint();
	showGlobalStatus(0);
	showStatusMessage(g_Locale->getText("flashupdate.actionreadflash") + " (" + string(CMTDInfo::getInstance()->getMTDName(readmtd)) + ")");
	CFlashTool ft;
	ft.setStatusViewer( this );
	ft.setMTDDevice("/dev/mtd/" + string(tmp));
	if(!ft.readFromMTD(filename))
	{
		showStatusMessage( ft.getErrorMessage() );
		sleep(10);
	}
	else
	{
		showGlobalStatus(100);
		showStatusMessage( g_Locale->getText("flashupdate.ready"));
		char message[500];
		sprintf(message, g_Locale->getText("flashupdate.savesuccess").c_str(), filename.c_str() );
		sleep(1);
		hide();
		ShowHint ( "messagebox.info", message );
	}
}

int CFlashExpert::exec( CMenuTarget* parent, string actionKey )
{
	if(parent)
	{
		parent->hide();
	}

	if(actionKey=="readflash")
	{
		readflash();
	}
	else if(actionKey=="writeflash")
	{
		CMenuWidget* fileselector = new CMenuWidget("flashupdate.fileselector", "softupdate.raw");
		fileselector->addItem( new CMenuSeparator() );
		fileselector->addItem( new CMenuForwarder("messagebox.cancel") );
		fileselector->addItem( new CMenuSeparator(CMenuSeparator::LINE) );
		struct dirent **namelist;
		int n;
		//		printf("scanning locale dir now....(perhaps)\n");

		n = scandir("/tmp", &namelist, 0, alphasort);
		if (n < 0)
		{
			perror("no flashimages available");
			//should be available...
		}
		else
		{
			for(int count=0;count<n;count++)
			{
				string filen = namelist[count]->d_name;
				int pos = filen.find(".img");
				if(pos!=-1)
				{
					fileselector->addItem(  new CMenuForwarder( filen, true, "", this, filen ) );
				}
				free(namelist[count]);
			}
			free(namelist);
		}
		fileselector->exec(NULL,"");
	}
	else if(actionKey=="readflashmtd")
	{
		//mtd-selector erzeugen
		CMenuWidget* mtdselector = new CMenuWidget("flashupdate.mtdselector", "softupdate.raw");
		mtdselector->addItem( new CMenuSeparator() );
		mtdselector->addItem( new CMenuForwarder("messagebox.cancel") );
		mtdselector->addItem( new CMenuSeparator(CMenuSeparator::LINE) );
		CMTDInfo* mtdInfo =CMTDInfo::getInstance();
		for(int x=0;x<mtdInfo->getMTDCount();x++)
		{
			char actionKey[20];
			sprintf(actionKey, "readmtd%d", x);
			mtdselector->addItem(  new CMenuForwarder( mtdInfo->getMTDName(x), true, "", this, actionKey ) );
		}
		mtdselector->exec(NULL,"");
	}
	else if(actionKey=="writeflashmtd")
	{/*
		CMenuWidget* mtdselector = new CMenuWidget("flashupdate.mtdselector", "softupdate.raw");
		mtdselector->addItem( new CMenuSeparator() );
		mtdselector->addItem( new CMenuForwarder("messagebox.cancel") );
		mtdselector->addItem( new CMenuSeparator(CMenuSeparator::LINE) );
		CMTDInfo* mtdInfo =CMTDInfo::getInstance();
		for(int x=0;x<mtdInfo->getMTDCount();x++)
		{
			char actionKey[20];
			sprintf(actionKey, "readmtd%d", x);
			mtdselector->addItem(  new CMenuForwarder( mtdInfo->getMTDName(x), true, "", this, actionKey ) );
		}
		mtdselector->exec(NULL,"");
		*/
	}
	else
	{
		int iReadmtd = -1;
		sscanf(actionKey.c_str(), "readmtd%d", &iReadmtd);
		if(iReadmtd!=-1)
		{
			readmtd(iReadmtd);
		}
		hide();
		return menu_return::RETURN_EXIT_ALL;
	}

	hide();
	return menu_return::RETURN_REPAINT;
}
