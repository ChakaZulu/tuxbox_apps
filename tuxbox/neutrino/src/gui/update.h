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

#ifndef __update__
#define __update__

#include "driver/framebuffer.h"
#include "driver/fontrenderer.h"
#include "driver/rcinput.h"

#include "widget/menue.h"
#include "widget/color.h"
#include "libmd5sum/libmd5sum.h"
#include "dbox/fp.h"
#include <sys/ioctl.h>


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mount.h>

#include <linux/mtd/mtd.h>

#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>

#include <string>

using namespace std;


class CFlashTool_StatusViewer
{
	public:
		virtual void showGlobalStatus(int prog)
		{}
		;
		virtual void showLocalStatus(int prog)
		{}
		;
		virtual void showStatusMessage(string text)
		{}
		;
};


class CFlashTool
{
		CFlashTool_StatusViewer* statusViewer;
		string mtdDevice;
		string ErrorMessage;

		bool erase();

	public:
		CFlashTool();
		string getErrorMessage();

		void setMTDDevice( string mtddevice );
		void setStatusViewer( CFlashTool_StatusViewer* statusview );

		bool program( string filename );

};


class CHTTPUpdater
{
		string						BasePath;
		CFlashTool_StatusViewer*	statusViewer;

		static int show_progress( void *clientp, size_t dltotal, size_t dlnow, size_t ultotal, size_t ulnow);

	public:
		CHTTPUpdater();
		void setStatusViewer( CFlashTool_StatusViewer* statusview );

		bool getInfo();
		bool getFile();

};


class CFlashUpdate : public CMenuTarget, CFlashTool_StatusViewer
{
		int x;
		int y;
		int width;
		int height;
		int hheight,mheight; // head/menu font height

		int globalstatusX, globalstatusY, localstatusY;
		int statusTextX, statusTextY;

		int installed_major, installed_provider, installed_minor;
		int new_major, new_provider, new_minor;
		char new_md5sum[50];

		void paint();

		void showGlobalStatus(int prog);
		void showLocalStatus(int prog);
		void showStatusMessage(string text);

	public:

		CFlashUpdate();

		void hide();
		int exec( CMenuTarget* parent, string actionKey );

};


#endif
