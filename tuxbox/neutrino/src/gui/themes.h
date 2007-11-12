/*
	Neutrino-GUI  -   DBoxII-Project

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

	$Id: themes.h,v 1.2 2007/11/12 08:54:43 ecosys Exp $ 

*/

#ifndef __cthemes__
#define __cthemes__
#include <string>
#include <configfile.h>
#include <driver/framebuffer.h>
#include <gui/filebrowser.h>
#include <system/setting_helpers.h>

class CThemeFile
{
public:
	std::string Filename;
	std::string Path;
};

typedef std::vector<CThemeFile> CFileViewList;

class CThemes : public CMenuTarget, CChangeObserver
{
	private:
		CFrameBuffer *frameBuffer;
		CConfigFile themefile;
		CColorSetupNotifier *notifier;
		CFileFilter themeFilter;
		CThemeFile tf;

		std::string Path;

		int x, y, width, height, hheight, mheight;
		std::string file_name;

		void Show();
		void readFile(char* themename);
		void saveFile(char* themename);
		void fileChooser();

	public:
		CThemes();
		void hide();
		int exec(CMenuTarget* parent, const std::string & actionKey);
};

#endif
