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

#include "gui/color.h"
#include "gui/widget/messagebox.h"
#include "gui/widget/hintbox.h"
#include "system/flashtool.h"
#include "system/httptool.h"
#include "driver/fontrenderer.h"
#include "driver/rcinput.h"

#include "libmd5sum/libmd5sum.h"

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

	return httpTool.downloadFile( gURL, sFileName, 25 );
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

	int pos = x+10;
	if(prog!=0)
	{
		pos += int( float(width-20)/100.0*prog);
		//vordergrund
		frameBuffer->paintBox(x+10, globalstatusY,pos, globalstatusY+10, COL_MENUCONTENT +7);
	}
	//hintergrund
	frameBuffer->paintBox(pos, globalstatusY, x+width-10, globalstatusY+10, COL_MENUCONTENT +2);
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

	int pos = x+10;
	if(prog!=0)
	{
		pos += int( float(width-20)/100.0*prog);
		//vordergrund
		frameBuffer->paintBox(x+10, localstatusY,pos, localstatusY+10, COL_MENUCONTENT +7);
	}
	//hintergrund
	frameBuffer->paintBox(pos, localstatusY, x+width-10, localstatusY+10, COL_MENUCONTENT +2);

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
		if(!httpUpdater.getFile( new_minor))
		{
			showStatusMessage(g_Locale->getText("flashupdate.getupdatefileerror") );
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
		return;
	}

	showGlobalStatus(50);
	//flash it...
	CFlashTool ft;
	ft.setMTDDevice("/dev/mtd/3");
	ft.setStatusViewer(this);
	if(!ft.program(sFileName, 75, 100))
	{
		showStatusMessage( ft.getErrorMessage() );
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
	ft.reboot();
	sleep(20000);
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
	string filename = "/tmp/mtd" + string(tmp) + string(".img");
	if(readmtd==-1)
	{
		//ganzes flashimage lesen
		filename = "/tmp/flashimage.img";
		readmtd = 5;
	}
	setTitle(g_Locale->getText("flashupdate.titlereadflash"));
	paint();
	showGlobalStatus(0);
	showStatusMessage(g_Locale->getText("flashupdate.actionreadflash") + " (" + string(CMTDInfo::getInstance()->getMTDName(readmtd)) + ")");
	CFlashTool ft;
	ft.setStatusViewer( this );
	ft.setMTDDevice(CMTDInfo::getInstance()->getMTDFileName(readmtd));
	if(!ft.readFromMTD(filename, 100))
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

void CFlashExpert::writemtd(string filename, int mtdNumber)
{
	char message[500];
	sprintf(message, g_Locale->getText("flashupdate.reallyflashmtd").c_str(), filename.c_str(), CMTDInfo::getInstance()->getMTDName(mtdNumber).c_str());

    if ( ShowMsg ( "messagebox.info", message , CMessageBox::mbrNo, CMessageBox::mbYes | CMessageBox::mbNo, "softupdate.raw" ) != CMessageBox::mbrYes )
    {
    	return;
    }
	setTitle( g_Locale->getText("flashupdate.titlewriteflash"));
	paint();
	showGlobalStatus(0);
	CFlashTool ft;
	ft.setStatusViewer( this );
	ft.setMTDDevice( CMTDInfo::getInstance()->getMTDFileName(mtdNumber) );
	if(!ft.program( "/tmp/" + filename, 50, 100))
	{
		showStatusMessage( ft.getErrorMessage() );
		sleep(10);
	}
	else
	{
		showGlobalStatus(100);
		showStatusMessage( g_Locale->getText("flashupdate.ready"));
		sleep(1);
		hide();
		ShowHint ( "messagebox.info",  g_Locale->getText("flashupdate.flashreadyreboot") );
		ft.reboot();
	}
}


void CFlashExpert::showMTDSelector(string actionkey)
{
	//mtd-selector erzeugen
	CMenuWidget* mtdselector = new CMenuWidget("flashupdate.mtdselector", "softupdate.raw");
	mtdselector->addItem( new CMenuSeparator() );
	mtdselector->addItem( new CMenuForwarder("messagebox.cancel") );
	mtdselector->addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	CMTDInfo* mtdInfo =CMTDInfo::getInstance();
	for(int x=0;x<mtdInfo->getMTDCount();x++)
	{
		char sActionKey[20];
		sprintf(sActionKey, "%s%d", actionkey.c_str(), x);
		mtdselector->addItem(  new CMenuForwarder( mtdInfo->getMTDName(x), true, "", this, sActionKey ) );
	}
	mtdselector->exec(NULL,"");
}

void CFlashExpert::showFileSelector(string actionkey)
{
	CMenuWidget* fileselector = new CMenuWidget("flashupdate.fileselector", "softupdate.raw");
	fileselector->addItem( new CMenuSeparator() );
	fileselector->addItem( new CMenuForwarder("messagebox.cancel") );
	fileselector->addItem( new CMenuSeparator(CMenuSeparator::LINE) );
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
			string filen = namelist[count]->d_name;
			int pos = filen.find(".img");
			if(pos!=-1)
			{
				fileselector->addItem(  new CMenuForwarder( filen, true, "", this, actionkey + filen ) );
			}
			free(namelist[count]);
		}
		free(namelist);
	}
	fileselector->exec(NULL,"");
}


int CFlashExpert::exec( CMenuTarget* parent, string actionKey )
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
				//ganzes Image schreiben -> mtd 5
				writemtd("flashimage.img", 5);
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
