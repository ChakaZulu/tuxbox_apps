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


#ifndef __FILEBROWSER_HPP__
#define __FILEBROWSER_HPP__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <features.h> /* make sure off_t has size 8 in __USE_FILE_OFFSET64 mode */

#ifndef __USE_FILE_OFFSET64
#warning not using 64 bit file offsets
#endif

#include <driver/framebuffer.h>
#include <driver/fontrenderer.h>
#include <driver/rcinput.h>

#include <gui/color.h>
#include <gui/infoviewer.h>

#include <gui/widget/menue.h>
#include <gui/widget/progresswindow.h>

#include <system/settings.h>

#include <string>
#include <vector>

#include <string.h>

#include <unistd.h> /* off_t */


#define VLC_URI "vlc://"

/**
 * Converts input of numeric keys to SMS style char input.
 */
class SMSKeyInput
{
	// time since last input
	time_t m_oldKeyTime;

	// last key input
	unsigned char m_oldKey;

	// keypresses within this period are taken as a sequence
	int m_timeout;
public:
	SMSKeyInput();

	/**
	 * Returns the SMS char calculated with respect to the new input.
	 * @param msg the current RC input
	 * @return the calculated SMS char
	 */
	unsigned char handleMsg(const neutrino_msg_t msg);

	/**
	 * Resets the key history which is needed for proper calculation
	 * of the SMS char by #handleMsg(neutrino_msg_t)
	 */
	void resetOldKey();

	/**
	 * @return the last key calculated by #handleMsg(neutrino_msg_t)
	 */
	unsigned char getOldKey();
	
	time_t getOldKeyTime();

	int getTimeout();

	/**
	 * Sets the timeout.
	 * @param timeout keypresses within this period are taken as a
	 * sequence. unit: msecs
	 */
	void setTimeout(int timeout);
};
//------------------------------------------------------------------------


class CFileFilter
{
	std::vector<std::string> Filter;
public:
	void addFilter(const std::string & filter){Filter.push_back(filter);};
	bool matchFilter(const std::string & name)
	{
		int ext_pos = 0;
		ext_pos = name.rfind('.');
		if( ext_pos > 0)
		{
			std::string extension;
			extension = name.substr(ext_pos + 1, name.length() - ext_pos);
			for(unsigned int i = 0; i < Filter.size();i++)
				if(strcasecmp(Filter[i].c_str(),extension.c_str()) == 0)
					return true;
		}
		return false;
	};
};
//------------------------------------------------------------------------

class CFile
{
public:
	enum FileType
	{
		FILE_UNKNOWN = 0,
		FILE_DIR,
		FILE_TEXT,
		FILE_CDR,
		FILE_MP3,
		FILE_OGG,
		FILE_WAV,
		FILE_PLAYLIST,
		STREAM_MP3,
		FILE_PICTURE,
		STREAM_PICTURE
	};

	FileType	getType(void) const;
	std::string	getFileName(void) const;
	std::string	getPath(void) const;

	CFile(){Marked = false; Size=0;Mode=0;Time=0;};
	off_t Size;
	std::string Name;
	mode_t Mode;
	bool Marked;
	time_t Time;
};
//------------------------------------------------------------------------

typedef std::vector<CFile> CFileList;

#define FILEBROWSER_NUMBER_OF_SORT_VARIANTS 5

class CFileBrowser
{
	private:
		CFrameBuffer	*frameBuffer;

		CFileList		selected_filelist;
		bool			readDir(const std::string & dirname, CFileList* flist);
		bool			readDir_vlc(const std::string & dirname, CFileList* flist);
		bool			readDir_std(const std::string & dirname, CFileList* flist);
		void			addRecursiveDir(CFileList * re_filelist, std::string path, bool bRootCall, CProgressWindow * progress = NULL);
		void SMSInput(const neutrino_msg_t msg);

		unsigned int		selected;
		unsigned int		liststart;
		unsigned int		listmaxshow;
		int 			fheight;	// Fonthoehe Filelist-Inhalt
		int 			theight;	// Fonthoehe Filelist-Titel
		int			foheight;	// Hoehe der button leiste
		std::string		name;
		std::string		base;
		std::string		m_baseurl;
		int 			width;
		int 			height;
		bool			use_filter;
		bool			bCancel;

		int 			x;
		int 			y;

		SMSKeyInput m_SMSKeyInput;

		void paintItem(unsigned pos);
		void paint();
		void paintHead();
		void paintFoot();
		void recursiveDelete(const char* file);

	protected:
		void commonInit();

	public:
		CFileList		filelist;

		void ChangeDir(const std::string & filename);
		void hide();

		std::string		Path;
		bool			Multi_Select;
		bool			Dirs_Selectable;
		bool        Dir_Mode;
		CFileFilter *	Filter;

		CFileBrowser();
		CFileBrowser(const char * const _base);
		~CFileBrowser();

		bool		exec(const char * const dirname);
		CFile		*getSelectedFile();
		CFileList	*getSelectedFiles();
		std::string getCurrentDir() {return Path;}
//		size_t CurlWriteToString(void *ptr, size_t size, size_t nmemb, void *data);
};




#endif
