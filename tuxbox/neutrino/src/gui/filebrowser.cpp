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

/* include <config.h> before <gui/filebrowser.h> to enable 64 bit file offsets */
#include <gui/filebrowser.h>

#include <gui/widget/buttons.h>
#include <gui/widget/icons.h>
#include <gui/widget/messagebox.h>

#include <driver/encoding.h>

#include <algorithm>
#include <iostream>
#include <cctype>

#include <global.h>
#include <neutrino.h>

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>

#include <sys/stat.h>
#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>

#include <driver/encoding.h>

#ifdef __USE_FILE_OFFSET64
typedef struct dirent64 dirent_struct;
#define my_alphasort alphasort64
#define my_scandir scandir64
typedef struct stat64 stat_struct;
#define my_stat stat64
#define my_lstat lstat64
#else
typedef struct dirent dirent_struct;
#define my_alphasort alphasort
#define my_scandir scandir
typedef struct stat stat_struct;
#define my_stat stat
#define my_lstat lstat
#error not using 64 bit file offsets
#endif

#define SMSKEY_TIMEOUT 2000
//------------------------------------------------------------------------
size_t CurlWriteToString(void *ptr, size_t size, size_t nmemb, void *data)
{
	std::string* pStr = (std::string*) data;
	*pStr += (char*) ptr;
	return size*nmemb;
}

//------------------------------------------------------------------------
//------------------------------------------------------------------------

SMSKeyInput::SMSKeyInput()
{
	resetOldKey();
	m_timeout = SMSKEY_TIMEOUT;
}
//------------------------------------------------------------------------

unsigned char SMSKeyInput::handleMsg(const neutrino_msg_t msg)
{
	timeval keyTime;
	gettimeofday(&keyTime,NULL);
	bool timeoutNotReached = (keyTime.tv_sec*1000+keyTime.tv_usec/1000
				  <= m_oldKeyTime.tv_sec*1000+m_oldKeyTime.tv_usec/1000 + m_timeout);

// 	printf("act: %ld , old: %ld (diff: %ld ) , timeout: %ld => timout= %d\n",
// 	       keyTime.tv_sec*1000+keyTime.tv_usec/1000,
// 	       m_oldKeyTime.tv_sec*1000+m_oldKeyTime.tv_usec/1000,
// 	       keyTime.tv_sec*1000+keyTime.tv_usec/1000 -
// 	       m_oldKeyTime.tv_sec*1000+m_oldKeyTime.tv_usec/1000,
// 	       m_timeout,!timeoutNotReached);

	unsigned char key = 0;
	if(msg == CRCInput::RC_1)
	{
			key = '1';
	}
	if(msg == CRCInput::RC_2)
	{
		if(m_oldKey == 'a' && timeoutNotReached)
			key = 'b';
		else if(m_oldKey == 'b' && timeoutNotReached)
			key = 'c';
		else if(m_oldKey == 'c' && timeoutNotReached)
			key = '2';
		else
			key = 'a';
	}
	else if(msg == CRCInput::RC_3)
	{
		if(m_oldKey == 'd' && timeoutNotReached)
			key = 'e';
		else if(m_oldKey == 'e' && timeoutNotReached)
			key = 'f';
		else if(m_oldKey == 'f' && timeoutNotReached)
			key = '3';
		else
			key = 'd';
	}
	else if(msg == CRCInput::RC_4)
	{
		if(m_oldKey == 'g' && timeoutNotReached)
			key = 'h';
		else if(m_oldKey == 'h' && timeoutNotReached)
			key = 'i';
		else if(m_oldKey == 'i' && timeoutNotReached)
			key = '4';
		else
			key = 'g';
	}
	else if(msg == CRCInput::RC_5)
	{
		if(m_oldKey == 'j' && timeoutNotReached)
			key = 'k';
		else if(m_oldKey == 'k' && timeoutNotReached)
			key = 'l';
		else if(m_oldKey == 'l' && timeoutNotReached)
			key = '5';
		else
			key = 'j';
	}
	else if(msg == CRCInput::RC_6)
	{
		if(m_oldKey == 'm' && timeoutNotReached)
			key = 'n';
		else if(m_oldKey == 'n' && timeoutNotReached)
			key = 'o';
		else if(m_oldKey == 'o' && timeoutNotReached)
			key = '6';
		else
			key = 'm';
	}
	else if(msg == CRCInput::RC_7)
	{
		if(m_oldKey == 'p' && timeoutNotReached)
			key = 'q';
		else if(m_oldKey == 'q' && timeoutNotReached)
			key = 'r';
		else if(m_oldKey == 'r' && timeoutNotReached)
			key = 's';
		else if(m_oldKey == 's' && timeoutNotReached)
			key = 's';
		else
			key = 'p';
	}
	else if(msg == CRCInput::RC_8)
	{
		if(m_oldKey == 't' && timeoutNotReached)
			key = 'u';
		else if(m_oldKey == 'u' && timeoutNotReached)
			key = 'v';
		else if(m_oldKey == 'v' && timeoutNotReached)
			key = '8';
		else
			key = 't';
	}
	else if(msg == CRCInput::RC_9)
	{
		if(m_oldKey == 'w' && timeoutNotReached)
			key = 'x';
		else if(m_oldKey == 'x' &&timeoutNotReached)
			key = 'y';
		else if(m_oldKey == 'y' &&timeoutNotReached)
			key = 'z';
		else if(m_oldKey == 'z' && timeoutNotReached)
			key = '9';
		else
			key = 'w';
	}
	else if(msg == CRCInput::RC_0)
	{
		key = '0';
	}
	m_oldKeyTime=keyTime;
	m_oldKey=key;
	return key;
}
//------------------------------------------------------------------------

void SMSKeyInput::resetOldKey()
{
	m_oldKeyTime.tv_sec = 0;
	m_oldKeyTime.tv_usec = 0;
	m_oldKey = 0;
}

unsigned char SMSKeyInput::getOldKey() const
{
	return m_oldKey;
}

const timeval* SMSKeyInput::getOldKeyTime() const
{
 	return &m_oldKeyTime;
}

time_t SMSKeyInput::getOldKeyTimeSec() const
{
	return m_oldKeyTime.tv_sec;
}


int SMSKeyInput::getTimeout() const
{
	return m_timeout;
}

void SMSKeyInput::setTimeout(int timeout)
{
	m_timeout = timeout;
}


//------------------------------------------------------------------------
//------------------------------------------------------------------------

bool comparetolower(const char a, const char b)
{
	return tolower(a) < tolower(b);
};

// sort operators
bool sortByName (const CFile& a, const CFile& b)
{
	if (std::lexicographical_compare(a.Name.begin(), a.Name.end(), b.Name.begin(), b.Name.end(), comparetolower))
		return true;

	if (std::lexicographical_compare(b.Name.begin(), b.Name.end(), a.Name.begin(), a.Name.end(), comparetolower))
		return false;

	return a.Mode < b.Mode;
/*
	int result = __gnu_cxx::lexicographical_compare_3way(a.Name.begin(), a.Name.end(), b.Name.begin(), b.Name.end(), comparetolower);

	if (result == 0)
		return a.Mode < b.Mode;
	else
		return result < 0;
*/
}

bool sortByNameDirsFirst(const CFile& a, const CFile& b)
// Sorts alphabetically with Directories first
{
	int typea, typeb;
	typea = a.getType();
	typeb = b.getType();

	if (typea == CFile::FILE_DIR)
		if (typeb == CFile::FILE_DIR)
			//both directories
			return sortByName(a, b);
		else
			//only a is directory
			return true;
	else if (typeb == CFile::FILE_DIR)
		//only b is directory
		return false;
	else
		//no directory
		return sortByName(a, b);
}

bool sortByType (const CFile& a, const CFile& b)
{
	if(a.Mode == b.Mode)
		return sortByName(a, b);
	else
		return a.Mode < b.Mode;

}

bool sortByDate (const CFile& a, const CFile& b)
{
   if(a.getFileName()=="..")
      return true;
   if(b.getFileName()=="..")
      return false;
	return a.Time < b.Time ;
}

bool sortBySize (const CFile& a, const CFile& b)
{
   if(a.getFileName()=="..")
      return true;
   if(b.getFileName()=="..")
      return false;
	return a.Size < b.Size;
}

bool (* const sortBy[FILEBROWSER_NUMBER_OF_SORT_VARIANTS])(const CFile& a, const CFile& b) =
{
	&sortByName,
	&sortByNameDirsFirst,
	&sortByType,
	&sortByDate,
	&sortBySize
};

const neutrino_locale_t sortByNames[FILEBROWSER_NUMBER_OF_SORT_VARIANTS] =
{
	LOCALE_FILEBROWSER_SORT_NAME,
	LOCALE_FILEBROWSER_SORT_NAMEDIRSFIRST,
	LOCALE_FILEBROWSER_SORT_TYPE,
	LOCALE_FILEBROWSER_SORT_DATE,
	LOCALE_FILEBROWSER_SORT_SIZE
};

//------------------------------------------------------------------------

CFileBrowser::CFileBrowser()
{
	commonInit();
	base = "";
}

CFileBrowser::CFileBrowser(const char * const _base)
{
	commonInit();
	base = _base;
}

void CFileBrowser::commonInit()
{
	frameBuffer = CFrameBuffer::getInstance();

	Filter = NULL;
	use_filter = true;
	Multi_Select = false;
	Dirs_Selectable = false;
	Dir_Mode = false;
	selected = 0;
	selections.clear();

	x = g_settings.screen_StartX + 20;
	y = g_settings.screen_StartY + 20;

	width = (g_settings.screen_EndX - g_settings.screen_StartX - 40);
	height = (g_settings.screen_EndY - g_settings.screen_StartY - 40);

	theight = g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_TITLE]->getHeight();
	fheight = g_Font[SNeutrinoSettings::FONT_TYPE_FILEBROWSER_ITEM]->getHeight();
	foheight = 30;

	liststart = 0;
	listmaxshow = std::max(1,(int)(height - theight - 2 * foheight)/fheight);

	//recalc height
	height = theight + listmaxshow * fheight + 2 * foheight;

	m_SMSKeyInput.setTimeout(SMSKEY_TIMEOUT);
}

//------------------------------------------------------------------------


CFileBrowser::~CFileBrowser()
{
}

//------------------------------------------------------------------------

CFile *CFileBrowser::getSelectedFile()
{
	if ((!(filelist.empty())) && (!(filelist[selected].Name.empty())))
		return &filelist[selected];
	else
		return NULL;
}

//------------------------------------------------------------------------

void CFileBrowser::ChangeDir(const std::string & filename, int selection)
{
	std::string newpath;
	if(filename == "..")
	{
		std::string::size_type pos = Path.substr(0,Path.length()-1).rfind('/');

		bool is_vlc = (strncmp(Path.c_str(), VLC_URI, strlen(VLC_URI)) == 0);

		if (pos == std::string::npos)
		{
			newpath = Path;
		}
		else
		{
			if (is_vlc && (pos < strlen(VLC_URI) - 1))
				newpath = VLC_URI;
			else
				newpath = Path.substr(0, pos + 1);
		}

		if (strncmp(is_vlc ? &(newpath.c_str()[strlen(VLC_URI)]) : newpath.c_str(), base.c_str(), base.length()) != 0)
			return;
	}
	else
	{
		newpath=filename;
	}
	if(newpath.rfind('/') != newpath.length()-1 ||
      newpath.length() == 0 ||
		newpath == VLC_URI)
	{
		newpath += '/';
	}
	filelist.clear();
	Path = newpath;
	name = newpath;
	CFileList allfiles;
	readDir(newpath, &allfiles);
	// filter
	CFileList::iterator file = allfiles.begin();
	for(; file != allfiles.end() ; file++)
	{
		if(Filter != NULL && (!S_ISDIR(file->Mode)) && use_filter)
		{
			if(!Filter->matchFilter(file->Name))
			{
				continue;
			}
		}
		if(Dir_Mode && (!S_ISDIR(file->Mode)))
		{
			continue;
		}
		filelist.push_back(*file);
	}
	// sort result
	sort(filelist.begin(), filelist.end(), sortBy[g_settings.filebrowser_sortmethod]);

	selected = 0;
	if ((selection != -1) && (selection < (int)filelist.size()))
		selected = selection;
	paintHead();
	paint();
}

//------------------------------------------------------------------------
bool CFileBrowser::readDir(const std::string & dirname, CFileList* flist)
{
	bool ret;

	if (strncmp(dirname.c_str(), VLC_URI, strlen(VLC_URI)) == 0)
	{
		ret = readDir_vlc(dirname, flist);
	}
	else
	{
		ret = readDir_std(dirname, flist);
	}
	return ret;
}

bool CFileBrowser::readDir_vlc(const std::string & dirname, CFileList* flist)
{
//	printf("readDir_vlc %s\n",dirname.c_str());
	std::string answer="";
	char *dir_escaped = curl_escape(dirname.substr(strlen(VLC_URI)).c_str(), 0);
	std::string url = m_baseurl;
	url += dir_escaped;
	curl_free(dir_escaped);
	std::cout << "[FileBrowser] vlc URL: " << url << std::endl;
	CURL *curl_handle;
	CURLcode httpres;
	/* init the curl session */
	curl_handle = curl_easy_init();
	/* specify URL to get */
	curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
	/* send all data to this function  */
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION,
						  CurlWriteToString);
	/* we pass our 'chunk' struct to the callback function */
	curl_easy_setopt(curl_handle, CURLOPT_FILE, (void *)&answer);
	/* error handling */
	char error[CURL_ERROR_SIZE];
	curl_easy_setopt(curl_handle, CURLOPT_ERRORBUFFER, error);
	curl_easy_setopt(curl_handle, CURLOPT_USERPWD, "admin:admin"); /* !!! make me customizable */
	/* get it! */
	httpres = curl_easy_perform(curl_handle);
	/* cleanup curl stuff */
	curl_easy_cleanup(curl_handle);

	/* Convert \ to / */
	std::replace(answer.begin(), answer.end(), '\\', '/');

	// std::cout << "Answer:" << std::endl << "----------------" << std::endl << answer << std::endl;
	/*!!! TODO check httpres and display error */
	if (!answer.empty() && !httpres)
	{
		unsigned int start=0;
		for (unsigned int pos=answer.find('\n',0) ; pos != std::string::npos ; pos=answer.find('\n',start))
		{
			CFile file;
			std::string entry = answer.substr(start, pos-start);
			//std::cout << "Entry" << entry << std::endl;
			if (entry.find("DIR:")==0)
				file.Mode = S_IFDIR + 0777 ;
			else
				file.Mode = S_IFREG + 0777 ;
			unsigned int spos = entry.rfind('/');
			if(spos!=std::string::npos)
			{
				file.Name = dirname;
				file.Name += entry.substr(spos+1);

				file.Size = 0;
				file.Time = 0;

				flist->push_back(file);
			}
			else
				std::cout << "Error misformed path " << entry << std::endl;
			start=pos+1;
		}
		return true;
	}
	else
	{
		std::cout << "Error reading vlc dir" << std::endl;
		/* since all CURL error messages use only US-ASCII characters, when can safely print them as if they were UTF-8 encoded */
		DisplayErrorMessage(error); // UTF-8
		CFile file;

		file.Name = dirname;
		file.Name += "..";

		file.Mode = S_IFDIR + 0777;
		file.Size = 0;
		file.Time = 0;
		flist->push_back(file);
	}
	return false;
}

bool CFileBrowser::readDir_std(const std::string & dirname, CFileList* flist)
{
//	printf("readDir_std %s\n",dirname.c_str());
	stat_struct statbuf;
	dirent_struct **namelist;
	int n;

	n = my_scandir(dirname.c_str(), &namelist, 0, my_alphasort);
	if (n < 0)
	{
		perror(("Filebrowser scandir: "+dirname).c_str());
		return false;
	}
	for(int i = 0; i < n;i++)
	{
		CFile file;
		if(strcmp(namelist[i]->d_name,".") != 0)
		{
			file.Name = dirname;
			file.Name += namelist[i]->d_name;

//			printf("file.Name: '%s', getFileName: '%s' getPath: '%s'\n",file.Name.c_str(),file.getFileName().c_str(),file.getPath().c_str());
			if(my_stat((file.Name).c_str(),&statbuf) != 0)
				perror("stat error");
			else
			{
				file.Mode = statbuf.st_mode;
				file.Size = statbuf.st_size;
				file.Time = statbuf.st_mtime;
				flist->push_back(file);
			}
		}
		free(namelist[i]);
	}

	free(namelist);

	return true;
}

//------------------------------------------------------------------------

bool CFileBrowser::exec(const char * const dirname)
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	bool res = false;

	m_baseurl = "http://";
	m_baseurl += g_settings.streaming_server_ip;
	m_baseurl += ':';
	m_baseurl += g_settings.streaming_server_port;
	m_baseurl += "/admin/dboxfiles.html?dir=";

	name = dirname;
	std::replace(name.begin(), name.end(), '\\', '/');

	paintHead();
	ChangeDir(name);
	paint();
	paintFoot();

	int oldselected = selected;

	unsigned long long timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_FILEBROWSER]);

	bool loop=true;
	while (loop)
	{
		g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );

		if ( msg <= CRCInput::RC_MaxRC )
			timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_FILEBROWSER]);

		if(!CRCInput::isNumeric(msg))
		{
			m_SMSKeyInput.resetOldKey();
		}

		if (msg == CRCInput::RC_yellow)
		{
			if ((Multi_Select) && (selected < filelist.size()))
			{
				if(filelist[selected].getFileName() != "..")
				{
					if( (S_ISDIR(filelist[selected].Mode) && Dirs_Selectable) || !S_ISDIR(filelist[selected].Mode) )
					{
						filelist[selected].Marked = !filelist[selected].Marked;
						paintItem(selected - liststart);
					}
				}
				msg = CRCInput::RC_down;	// jump to next item
			}
		}

		if (msg == CRCInput::RC_red)
		{
			selected += listmaxshow;
			if (selected >= filelist.size())
				selected = 0;
			liststart = (selected / listmaxshow) * listmaxshow;
			paint();
		}
		else if ( msg == CRCInput::RC_green )
		{
			if ((int(selected)-int(listmaxshow))<0)
				selected=filelist.size()-1;
			else
				selected -= listmaxshow;
			liststart = (selected/listmaxshow)*listmaxshow;
			paint();
		}
		else if ( msg == CRCInput::RC_up )
		{
			int prevselected=selected;
			if(selected==0)
			{
				selected = filelist.size()-1;
			}
			else
				selected--;
			paintItem(prevselected - liststart);
			unsigned int oldliststart = liststart;
			liststart = (selected/listmaxshow)*listmaxshow;
			if(oldliststart!=liststart)
			{
				paint();
			}
			else
			{
				paintItem(selected - liststart);
			}
		}
		else if ( msg == CRCInput::RC_down )
		{
			if (!(filelist.empty()))
			{
				int prevselected=selected;
				selected = (selected + 1) % filelist.size();
				paintItem(prevselected - liststart);
				unsigned int oldliststart = liststart;
				liststart = (selected/listmaxshow)*listmaxshow;
				if(oldliststart!=liststart)
					paint();
				else
					paintItem(selected - liststart);
			}
		}
		else if ( ( msg == CRCInput::RC_timeout ) )
		{
			selected = oldselected;
			loop=false;
		}
		else if ( msg == CRCInput::RC_right )
		{
			if (!(filelist.empty()))
			{
				if (S_ISDIR(filelist[selected].Mode) && (filelist[selected].getFileName() != ".."))
				{
					selections.push_back(selected);
					ChangeDir(filelist[selected].Name);
				}
			}
		}
		else if ( msg == CRCInput::RC_left )
		{
			if (selections.size() > 0)
			{
				ChangeDir("..",selections.back());
				selections.pop_back();
			} else
			{
				ChangeDir("..");
			}
		}
		else if ( msg == CRCInput::RC_blue )
		{
			if(Filter != NULL)
			{
				use_filter = !use_filter;
				ChangeDir(Path);
			}
		}
		else if ( msg == CRCInput::RC_home )
		{
			loop = false;
		}
		else if ( msg == CRCInput::RC_spkr )
		{
			if(".." !=(filelist[selected].getFileName().substr(0,2))) // das darf man nicht löschen
			{
				std::string msg = g_Locale->getText(LOCALE_FILEBROWSER_DODELETE1);
				msg += ' ';
				if (filelist[selected].getFileName().length() > 10)
				{
					msg += filelist[selected].getFileName().substr(0,10);
					msg += "...";
				}
				else
					msg += filelist[selected].getFileName();
				msg += ' ';
				msg += g_Locale->getText(LOCALE_FILEBROWSER_DODELETE2);
				if (ShowMsgUTF(LOCALE_FILEBROWSER_DELETE, msg, CMessageBox::mbrNo, CMessageBox::mbYes|CMessageBox::mbNo)==CMessageBox::mbrYes)
				{
					recursiveDelete(filelist[selected].Name.c_str());
					if(".ts" ==(filelist[selected].getFileName().substr(filelist[selected].getFileName().length()-3,filelist[selected].getFileName().length())))//if bla.ts
					{
						recursiveDelete((filelist[selected].Name.substr(0,filelist[selected].Name.length()-7)+".xml").c_str());//remove bla.xml von bla.ts
					}
					ChangeDir(Path);

				}
			}
		}
		else if (msg == CRCInput::RC_ok)
		{
			if (!(filelist.empty()))
			{
				if (filelist[selected].getFileName() == "..")
				{
					if (selections.size() > 0)
					{
						ChangeDir("..",selections.back());
						selections.pop_back();
					} else
					{
						ChangeDir("..");
					}
				}
				else
				{
					std::string filename = filelist[selected].Name;
					if ( filename.length() > 1 )
					{
						if((!Multi_Select) && S_ISDIR(filelist[selected].Mode) && !Dir_Mode)
						{
							ChangeDir(filelist[selected].Name);
						}
						else
						{
							filelist[selected].Marked = true;
							loop = false;
							res = true;
						}
					}
				}
			}
		}
		else if (msg==CRCInput::RC_help)
		{
			if (g_settings.filebrowser_sortmethod >= 4)
				g_settings.filebrowser_sortmethod = 0;
			else
				g_settings.filebrowser_sortmethod++;

			sort(filelist.begin(), filelist.end(), sortBy[g_settings.filebrowser_sortmethod]);

			paint();
		}
		else if (CRCInput::isNumeric(msg))
		{
			if (!(filelist.empty()))
				SMSInput(msg);
		}
		else
	  	{
			if ( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all )
			{
				loop = false;
			}
		}
	}

	hide();

	selected_filelist.clear();

	if(res && Multi_Select)
	{
		CProgressWindow * progress = new CProgressWindow();
		progress->setTitle(LOCALE_FILEBROWSER_SCAN);
		progress->exec(NULL,"");
		for(unsigned int i = 0; i < filelist.size();i++)
			if(filelist[i].Marked)
			{
				if(S_ISDIR(filelist[i].Mode))
					addRecursiveDir(&selected_filelist,filelist[i].Name, true, progress);
				else
					selected_filelist.push_back(filelist[i]);
			}
		progress->hide();
		delete progress;
	}

	return res;
}

//------------------------------------------------------------------------

void CFileBrowser::addRecursiveDir(CFileList * re_filelist, std::string rpath, bool bRootCall, CProgressWindow * progress)
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	int n;

	printf("addRecursiveDir %s\n",rpath.c_str());

	if (bRootCall) bCancel=false;

	g_RCInput->getMsg_us(&msg, &data, 1);
	if (msg==CRCInput::RC_home)
	{
		// home key cancel scan
		bCancel=true;
	}
	else if (msg!=CRCInput::RC_timeout)
	{
		// other event, save to low priority queue
		g_RCInput->postMsg( msg, data, false );
	}
	if(bCancel)
		return;

	if ((rpath.empty()) || ((*rpath.rbegin()) != '/'))
	{
		rpath += '/';
	}

	CFileList tmplist;
	if(!readDir(rpath, &tmplist))
	{
		perror(("Recursive scandir: "+rpath).c_str());
	}
	else
	{
		n = tmplist.size();
		if(progress)
		{
			progress->showStatusMessageUTF(FILESYSTEM_ENCODING_TO_UTF8_STRING(rpath));
		}
		for(int i = 0; i < n;i++)
		{
			if(progress)
			{
				progress->showGlobalStatus(100/n*i);
			}
			std::string basename = tmplist[i].Name.substr(tmplist[i].Name.rfind('/')+1);
			if( basename != ".." )
			{
				if(Filter != NULL && (!S_ISDIR(tmplist[i].Mode)) && use_filter)
				{
					if(!Filter->matchFilter(tmplist[i].Name))
					{
						continue;
					}
				}
				if(!S_ISDIR(tmplist[i].Mode))
					re_filelist->push_back(tmplist[i]);
				else
					addRecursiveDir(re_filelist,tmplist[i].Name, false, progress);
			}
		}
	}
}


//------------------------------------------------------------------------

void CFileBrowser::hide()
{
	frameBuffer->paintBackgroundBoxRel(x,y, width,height);
}

//------------------------------------------------------------------------

void CFileBrowser::paintItem(unsigned int pos)
{
	int colwidth1, colwidth2, colwidth3, colwidth1_dir, colwidth2_dir;
	uint8_t color;
	fb_pixel_t bgcolor;
	int ypos = y+ theight+0 + pos*fheight;
	CFile * actual_file = NULL;
	std::string fileicon;

	colwidth2_dir = 180;
	colwidth1_dir = width - 35 - colwidth2_dir - 10;

	if (liststart + pos == selected)
	{
		color   = COL_MENUCONTENTSELECTED;
		bgcolor = COL_MENUCONTENTSELECTED_PLUS_0;
		paintFoot();
	}
	else
	{
		color   = COL_MENUCONTENT;//DARK;
		bgcolor = COL_MENUCONTENT_PLUS_0;//DARK;
	}

	if( (liststart + pos) < filelist.size() )
	{
		actual_file = &filelist[liststart+pos];
		if (actual_file->Marked)
		{
			color += 2;
			bgcolor = (liststart + pos == selected) ? COL_MENUCONTENTSELECTED_PLUS_2 : COL_MENUCONTENT_PLUS_2;
		}

		if (g_settings.filebrowser_showrights == 0 && S_ISREG(actual_file->Mode))
		{
			colwidth2 = 0;
			colwidth3 = 90;
		}
		else
		{
			colwidth2 = 90;
			colwidth3 = 90;
		}
		colwidth1 = width - 35 - colwidth2 - colwidth3 - 10;

		frameBuffer->paintBoxRel(x,ypos, width- 15, fheight, bgcolor);

		if ( actual_file->Name.length() > 0 )
		{
			if (liststart+pos==selected)
				CLCD::getInstance()->showMenuText(0, FILESYSTEM_ENCODING_TO_UTF8_STRING(actual_file->getFileName()).c_str(), -1, true); // UTF-8

			switch(actual_file->getType())
			{
			case CFile::FILE_CDR:
			case CFile::FILE_MP3:
			case CFile::FILE_OGG:
			case CFile::FILE_WAV:
				fileicon = "mp3.raw";
//				color = COL_MENUCONTENT;
				break;

			case CFile::FILE_DIR:
				fileicon = "folder.raw";
				break;

			case CFile::FILE_PICTURE:
			case CFile::FILE_TEXT:
			default:
				fileicon = "file.raw";
			}
			frameBuffer->paintIcon(fileicon, x+5 , ypos + (fheight-16) / 2 );

			g_Font[SNeutrinoSettings::FONT_TYPE_FILEBROWSER_ITEM]->RenderString(x + 35, ypos + fheight, colwidth1 - 10 , FILESYSTEM_ENCODING_TO_UTF8_STRING(actual_file->getFileName()), color, 0, true); // UTF-8

			if( S_ISREG(actual_file->Mode) )
			{
				if (g_settings.filebrowser_showrights != 0)
				{
					const char * attribute = "xwr";
					char modestring[9 + 1];
					for (int m = 8; m >= 0; m--)
					{
						modestring[8 - m] = (actual_file->Mode & (1 << m)) ? attribute[m % 3] : '-';
					}
					modestring[9] = 0;

					g_Font[SNeutrinoSettings::FONT_TYPE_FILEBROWSER_ITEM]->RenderString(x + 35 + colwidth1 , ypos+ fheight, colwidth2 - 10, modestring, color, 0, true); // UTF-8
				}

				char tmpstr[256];
				if (actual_file->Size >= 1073741824LL)
				{
					snprintf(tmpstr,sizeof(tmpstr),"%.4gG",
						 (double)actual_file->Size / (1024. * 1024 * 1024));
				}
				else if (actual_file->Size >= 1048576LL)
				{
					snprintf(tmpstr,sizeof(tmpstr),"%.4gM",
						 (double)actual_file->Size / (1024. * 1024));
				}
				else if (actual_file->Size >= 1024LL)
				{
					snprintf(tmpstr,sizeof(tmpstr),"%.4gK",
						 (double)actual_file->Size / (1024.));
				}
				else
					snprintf(tmpstr,sizeof(tmpstr),"%d", (int)actual_file->Size);

				g_Font[SNeutrinoSettings::FONT_TYPE_FILEBROWSER_ITEM]->RenderString(x + 35 + colwidth1 + colwidth2, ypos+ fheight, colwidth3 - 10, tmpstr, color);
			}

			if( S_ISDIR(actual_file->Mode) )
			{
				char timestring[18];
				time_t rawtime;

				rawtime = actual_file->Time;
				strftime(timestring, 18, "%d-%m-%Y %H:%M", gmtime(&rawtime));

				g_Font[SNeutrinoSettings::FONT_TYPE_FILEBROWSER_ITEM]->RenderString(x + 35 + colwidth1_dir, ypos+ fheight, colwidth2_dir - 10, timestring, color);
			}
		}
	}
	else
		frameBuffer->paintBoxRel(x,ypos, width- 15, fheight, COL_MENUCONTENT_PLUS_0/*DARK*/);
}

//------------------------------------------------------------------------

void CFileBrowser::paintHead()
{
	char l_name[100];

	frameBuffer->paintBoxRel(x,y, width,theight+0, COL_MENUHEAD_PLUS_0);

	snprintf(l_name, sizeof(l_name), "%s %s", g_Locale->getText(LOCALE_FILEBROWSER_HEAD), FILESYSTEM_ENCODING_TO_UTF8_STRING(name).c_str()); // UTF-8

	g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_TITLE]->RenderString(x+10,y+theight+1, width-11, l_name, COL_MENUHEAD, 0, true); // UTF-8
}

//------------------------------------------------------------------------

const struct button_label FileBrowserButtons[3] =
{
	{ NEUTRINO_ICON_BUTTON_RED   , LOCALE_FILEBROWSER_NEXTPAGE        },
	{ NEUTRINO_ICON_BUTTON_GREEN , LOCALE_FILEBROWSER_PREVPAGE        },
	{ NEUTRINO_ICON_BUTTON_YELLOW, LOCALE_FILEBROWSER_MARK            },
};
const struct button_label FileBrowserFilterButton[2] =
{
	{ NEUTRINO_ICON_BUTTON_BLUE  , LOCALE_FILEBROWSER_FILTER_INACTIVE },
	{ NEUTRINO_ICON_BUTTON_BLUE  , LOCALE_FILEBROWSER_FILTER_ACTIVE   },
};

void CFileBrowser::paintFoot()
{
	int dx = (width-20) / 4;
  //Second Line (bottom, top)
	int by2 = y + height - (foheight - 4);
	int ty2 = by2 + g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getHeight();

	//Background
	frameBuffer->paintBoxRel(x, y + height - (2 * foheight ), width, (2 * foheight ), COL_MENUHEAD_PLUS_0);

	if (!(filelist.empty()))
	{
		int by = y + height - 2 * (foheight - 4);

		::paintButtons(frameBuffer, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL], g_Locale, x + 10, by, dx, Multi_Select ? 3 : 2, FileBrowserButtons);

		if(Filter != NULL)
			::paintButtons(frameBuffer, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL], g_Locale, x + 10 + (3 * dx), by, dx, 1, &(FileBrowserFilterButton[use_filter?0:1]));

		//OK-Button
		if( (filelist[selected].getType() != CFile::FILE_UNKNOWN) || (S_ISDIR(filelist[selected].Mode)) )
		{
			frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_OKAY, x +3 , by2 - 3);
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(x + 35, ty2, dx - 35, g_Locale->getText(LOCALE_FILEBROWSER_SELECT), COL_INFOBAR, 0, true); // UTF-8

		}

		//?-Button
		frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_HELP, x + (1 * dx), by2 - 3);
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(x + 35 + (1 * dx), ty2, dx - 35, g_Locale->getText(sortByNames[(g_settings.filebrowser_sortmethod + 1) % FILEBROWSER_NUMBER_OF_SORT_VARIANTS]), COL_INFOBAR, 0, true); // UTF-8

		//Mute-Button
		frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_MUTE_SMALL, x + (2 * dx), by2 - 3);
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(x + 35 + (2 * dx), ty2, dx - 35, g_Locale->getText(LOCALE_FILEBROWSER_DELETE), COL_INFOBAR, 0, true); // UTF-8

		if(m_SMSKeyInput.getOldKey()!=0)
		{
			char cKey[2]={m_SMSKeyInput.getOldKey(),0};
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(x + width - 16, by2 , 16, cKey, COL_MENUHEAD, 0, true); // UTF-8
		}
	}
}

//------------------------------------------------------------------------

void CFileBrowser::paint()
{
	liststart = (selected/listmaxshow)*listmaxshow;

//	if (filelist[0].Name.length() != 0)
//		frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_HELP, x+ width- 30, y+ 5 );
	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, g_Locale->getText(LOCALE_FILEBROWSER_HEAD));

	for(unsigned int count=0;count<listmaxshow;count++)
		paintItem(count);

	int ypos = y+ theight;
	int sb = fheight* listmaxshow;
	frameBuffer->paintBoxRel(x+ width- 15,ypos, 15, sb,  COL_MENUCONTENT_PLUS_1);

	int sbc= ((filelist.size()- 1)/ listmaxshow)+ 1;
	float sbh= (sb- 4)/ sbc;
	int sbs= (selected/listmaxshow);

	frameBuffer->paintBoxRel(x+ width- 13, ypos+ 2+ int(sbs* sbh) , 11, int(sbh),  COL_MENUCONTENT_PLUS_3);
}

//------------------------------------------------------------------------
void CFileBrowser::SMSInput(const neutrino_msg_t msg)
{
	unsigned char key = m_SMSKeyInput.handleMsg(msg);

	unsigned int i;
	for(i=(selected+1) % filelist.size(); i != selected ; i= (i+1) % filelist.size())
	{
		if(tolower(filelist[i].getFileName()[0]) == key)
		{
			break;
		}
	}
	int prevselected=selected;
	selected=i;
	paintItem(prevselected - liststart);
	unsigned int oldliststart = liststart;
	liststart = (selected/listmaxshow)*listmaxshow;
	if(oldliststart!=liststart)
	{
		paint();
	}
	else
	{
		paintItem(selected - liststart);
	}
}
void CFileBrowser::recursiveDelete(const char* file)
{
	stat_struct statbuf;
	dirent_struct **namelist;
	int n;
	printf("Delete %s\n", file);
	if(my_lstat(file,&statbuf) == 0)
	{
		if(S_ISDIR(statbuf.st_mode))
		{
			n = my_scandir(file, &namelist, 0, my_alphasort);
			while(n--)
			{
				if(strcmp(namelist[n]->d_name, ".")!=0 && strcmp(namelist[n]->d_name, "..")!=0)
				{
					std::string fullname = file;
					fullname += "/";
					fullname += namelist[n]->d_name;
					recursiveDelete(fullname.c_str());
				}
				free(namelist[n]);
			}
			free(namelist);
			rmdir(file);
		}
		else
		{
			unlink(file);
		}
	}
	else
		perror(file);
}
