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

#include "gui/widget/progressstatus.h"
#include "gui/widget/progresswindow.h"
#include "gui/widget/menue.h"


#include <string>

using namespace std;


class CHTTPUpdater
{
		string						BasePath;
		CProgress_StatusViewer*	statusViewer;

	public:
		string						ImageFile;
		string						VersionFile;

		CHTTPUpdater();
		void setStatusViewer( CProgress_StatusViewer* statusview );

		bool getInfo();
		bool getFile( string version );

};


class CFlashUpdate : public CMenuTarget, CProgress_StatusViewer
{
	private:
		CFrameBuffer	*frameBuffer;
		int fd_fp;

		CHTTPUpdater httpUpdater;

		int x;
		int y;
		int width;
		int height;
		int hheight,mheight; // head/menu font height

		int globalstatus, globalstatusX, globalstatusY, localstatusY;
		int statusTextX, statusTextY;

		int installed_major, installed_provider;
		int new_major, new_provider;
		char installed_minor[50], new_minor[50];

		char new_md5sum[50];

		void paint();

		void showGlobalStatus(int prog);
		int getGlobalStatus();
		void showLocalStatus(int prog);
		void showStatusMessage(string text);

		bool checkVersion4Update(int ypos, string &sFileName);

	public:
		CFlashUpdate();

		void hide();
		int exec( CMenuTarget* parent, string actionKey );

};


class CFlashExpert : public CProgressWindow
{
	private:
		int selectedMTD;

		void showMTDSelector(string actionkey);
		void showFileSelector(string actionkey);

		void readmtd(int readmtd);
		void writemtd(string filename, int mtdNumber);

	public:
		CFlashExpert();
		int exec( CMenuTarget* parent, string actionKey );

};


#endif
