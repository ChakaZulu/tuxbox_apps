/*  
	$Id: dirchooser.h,v 1.1 2007/01/24 02:04:30 guenther Exp $
	
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2006 Guenther
	Homepage: http://www.tuxbox.org/


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


#ifndef __dirchooser__
#define __dirchooser__

#include <driver/framebuffer.h>
#include <system/localize.h>
#include <gui/widget/menue.h>
#include <global.h>

#include <string>


extern int getNFSIDOfDir(const char * dir);
extern unsigned int getFreeDiscSpaceGB(const char * dir);
extern int getFirstFreeRecDirNr(int min_free_gb);

#define MAX_ALLOWED_PATHS 2
class CDirChooser : public CMenuWidget
{
	private:
		std::string* dirPath;
		char* allowedPath[MAX_ALLOWED_PATHS];

	public:
		bool new_path_selected;
		CDirChooser(std::string* path, char* allowed_path0 = NULL,char* allowed_path1= NULL);
		int exec(CMenuTarget* parent, const std::string & actionKey);
};



class CRecDirChooser : public CMenuWidget
{
	private:
		int * index;
		char * localDir;
		std::string * localDirString;
		
		int nfsIndex[MAX_RECORDING_DIR];
		std::string dir;
		std::string dirOptionText[MAX_RECORDING_DIR];
		int selectedDir;

	public:
		CRecDirChooser(const neutrino_locale_t Name,  const std::string & Icon = "", int * chosenNfsIndex = NULL, char * chosenLocalDir = NULL, const char * const selectedLocalDir = "", const int mwidth = 400, const int mheight = 576);
		CRecDirChooser(const neutrino_locale_t Name,  const std::string & Icon = "", int * chosenNfsIndex = NULL, std::string * chosenLocalDir = NULL, const char * const selectedLocalDir = "", const int mwidth = 400, const int mheight = 576);
		void initMenu(void);
		int exec(CMenuTarget* parent, const std::string & actionKey);
		std::string get_selected_dir(void){return dir;};
};


#endif

