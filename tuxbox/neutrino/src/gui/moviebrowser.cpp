/***************************************************************************
	Neutrino-GUI  -   DBoxII-Project

 	Homepage: http://dbox.cyberphoria.org/

	$Id: moviebrowser.cpp,v 1.8 2006/02/20 01:10:34 guenther Exp $

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

	***********************************************************

	Module Name: moviebrowser.cpp .

	Description: Implementation of the CMovieBrowser class
	             This class provides a filebrowser window to view, select and start a movies from HD.
	             This class does replace the Filebrowser

	Date:	   Nov 2005

	Author: Günther@tuxbox.berlios.org
		based on code of Steffen Hehn 'McClean'

	$Log: moviebrowser.cpp,v $
	Revision 1.8  2006/02/20 01:10:34  guenther
	- temporary parental lock updated - remove 1s debug prints in movieplayer- Delete file without rescan of movies- Crash if try to scroll in list with 2 movies only- UTF8XML to UTF8 conversion in preview- Last file selection recovered- use of standard folders adjustable in config- reload and remount option in config
	
	Revision 1.7  2006/01/05 03:58:49  Arzka
	Hopefully fixed a memory leak
	  fb_window.cpp:61: warning: deleting `void*' is undefined
	
	Removed few minor compilation warnings about used data types with printf formatters in moviebrowser.cpp and movieinfo.cpp
	
	Revision 1.6  2005/12/23 18:45:42  metallica
	Günther moviebrowser.cpp update
	
	Revision 1.5  2005/12/18 09:23:53  metallica
	fix compil warnings
	
	Revision 1.4  2005/12/12 07:58:02  guenther
	- fix bug on deleting CMovieBrowser 
	- speed up parse time (20 ms per .ts file now)
	- update stale function
	- refresh directories on reload
	- print scan time in debug console
	

****************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef MOVIEBROWSER  			
#endif /* MOVIEBROWSER */


#include "stdlib.h"
#include "moviebrowser.h"
#include "widget/hintbox.h"
#include "widget/helpbox.h"
#include "widget/messagebox.h"
#include <dirent.h>
#include <sys/stat.h>
#include <gui/nfs.h>
#include "neutrino.h"
#include <gui/widget/stringinput.h>
#include <sys/vfs.h> // for statfs
#include <gui/widget/icons.h>
#include <sys/mount.h>

#define my_scandir scandir64
#define my_alphasort alphasort64
typedef struct stat64 stat_struct;
typedef struct dirent64 dirent_struct;
#define my_stat stat64

#define TRACE  printf
#define TRACE_1 printf

#define VLC_URI "vlc://"

#define NUMBER_OF_MOVIES_LAST 40 // This is the number of movies shown in last recored and last played list
 
#define MESSAGEBOX_PARENTAL_LOCK_OPTIONS_COUNT 3
const CMenuOptionChooser::keyval MESSAGEBOX_PARENTAL_LOCK_OPTIONS[MESSAGEBOX_PARENTAL_LOCK_OPTIONS_COUNT] =
{
	{ 1, LOCALE_MOVIEBROWSER_MENU_PARENTAL_LOCK_ACTIVATED_YES        },
	{ 0, LOCALE_MOVIEBROWSER_MENU_PARENTAL_LOCK_ACTIVATED_NO         },
	{ 2, LOCALE_MOVIEBROWSER_MENU_PARENTAL_LOCK_ACTIVATED_NO_TEMP   }
};

#define MESSAGEBOX_PARENTAL_LOCKAGE_OPTION_COUNT 6
const CMenuOptionChooser::keyval MESSAGEBOX_PARENTAL_LOCKAGE_OPTIONS[MESSAGEBOX_PARENTAL_LOCKAGE_OPTION_COUNT] =
{
	{ 0,  LOCALE_MOVIEBROWSER_INFO_PARENTAL_LOCKAGE_0YEAR },
	{ 6,  LOCALE_MOVIEBROWSER_INFO_PARENTAL_LOCKAGE_6YEAR },
	{ 12, LOCALE_MOVIEBROWSER_INFO_PARENTAL_LOCKAGE_12YEAR },
	{ 16, LOCALE_MOVIEBROWSER_INFO_PARENTAL_LOCKAGE_16YEAR },
	{ 18, LOCALE_MOVIEBROWSER_INFO_PARENTAL_LOCKAGE_18YEAR },
	{ 99, LOCALE_MOVIEBROWSER_INFO_PARENTAL_LOCKAGE_ALWAYS }
};

#define MAX_WINDOW_WIDTH  (g_settings.screen_EndX - g_settings.screen_StartX - 40)
#define MAX_WINDOW_HEIGHT (g_settings.screen_EndY - g_settings.screen_StartY - 40)	

#define MIN_WINDOW_WIDTH  ((g_settings.screen_EndX - g_settings.screen_StartX)>>1)
#define MIN_WINDOW_HEIGHT 200	

#define TITLE_BACKGROUND_COLOR ((CFBWindow::color_t)COL_MENUHEAD_PLUS_0)
#define TITLE_FONT_COLOR ((CFBWindow::color_t)COL_MENUHEAD)

#define TEXT_FONT g_Font[SNeutrinoSettings::FONT_TYPE_MENU]
#define TITLE_FONT g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]
#define FOOT_FONT g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]

#define INTER_FRAME_SPACE 6  // space between e.g. upper and lower window
#define TEXT_BORDER_WIDTH 8

const neutrino_locale_t m_localizedItemName[MB_INFO_MAX_NUMBER+1] =
{
	LOCALE_MOVIEBROWSER_SHORT_FILENAME,
	LOCALE_MOVIEBROWSER_SHORT_PATH,
	LOCALE_MOVIEBROWSER_SHORT_TITLE ,
	LOCALE_MOVIEBROWSER_SHORT_SERIE,
	LOCALE_MOVIEBROWSER_SHORT_INFO1,
	LOCALE_MOVIEBROWSER_SHORT_GENRE_MAJOR,
	LOCALE_MOVIEBROWSER_SHORT_GENRE_MINOR,
	LOCALE_MOVIEBROWSER_SHORT_INFO2,
	LOCALE_MOVIEBROWSER_SHORT_PARENTAL_LOCKAGE	,
	LOCALE_MOVIEBROWSER_SHORT_CHANNEL ,
	LOCALE_MOVIEBROWSER_SHORT_BOOK,
	LOCALE_MOVIEBROWSER_SHORT_QUALITY,
	LOCALE_MOVIEBROWSER_SHORT_PREVPLAYDATE,
	LOCALE_MOVIEBROWSER_SHORT_RECORDDATE,
	LOCALE_MOVIEBROWSER_SHORT_PRODYEAR,
	LOCALE_MOVIEBROWSER_SHORT_COUNTRY,
	LOCALE_MOVIEBROWSER_SHORT_FORMAT ,
	LOCALE_MOVIEBROWSER_SHORT_AUDIO ,
	LOCALE_MOVIEBROWSER_SHORT_LENGTH,
	LOCALE_MOVIEBROWSER_SHORT_SIZE, 
	NONEXISTANT_LOCALE
};

// default row size in pixel for any element
#define	MB_ROW_WIDTH_FILENAME 		150
#define	MB_ROW_WIDTH_FILEPATH		150
#define	MB_ROW_WIDTH_TITLE			300
#define	MB_ROW_WIDTH_SERIE			100
#define	MB_ROW_WIDTH_INFO1			100
#define	MB_ROW_WIDTH_MAJOR_GENRE 	100
#define	MB_ROW_WIDTH_MINOR_GENRE 	30
#define	MB_ROW_WIDTH_INFO2 			30
#define	MB_ROW_WIDTH_PARENTAL_LOCKAGE 25 
#define	MB_ROW_WIDTH_CHANNEL		80
#define	MB_ROW_WIDTH_BOOKMARK		50
#define	MB_ROW_WIDTH_QUALITY 		25
#define	MB_ROW_WIDTH_PREVPLAYDATE	80
#define	MB_ROW_WIDTH_RECORDDATE 	80
#define	MB_ROW_WIDTH_PRODDATE 		50
#define	MB_ROW_WIDTH_COUNTRY 		50
#define	MB_ROW_WIDTH_GEOMETRIE		50
#define	MB_ROW_WIDTH_AUDIO			50 	
#define	MB_ROW_WIDTH_LENGTH			40
#define	MB_ROW_WIDTH_SIZE 			45

const int m_defaultRowWidth[MB_INFO_MAX_NUMBER+1] = 
{
	MB_ROW_WIDTH_FILENAME ,
	MB_ROW_WIDTH_FILEPATH,
	MB_ROW_WIDTH_TITLE,
	MB_ROW_WIDTH_SERIE,
	MB_ROW_WIDTH_INFO1,
	MB_ROW_WIDTH_MAJOR_GENRE ,
	MB_ROW_WIDTH_MINOR_GENRE ,
	MB_ROW_WIDTH_INFO2 ,
	MB_ROW_WIDTH_PARENTAL_LOCKAGE ,
	MB_ROW_WIDTH_CHANNEL,
	MB_ROW_WIDTH_BOOKMARK,
	MB_ROW_WIDTH_QUALITY ,
	MB_ROW_WIDTH_PREVPLAYDATE,
	MB_ROW_WIDTH_RECORDDATE ,
	MB_ROW_WIDTH_PRODDATE ,
	MB_ROW_WIDTH_COUNTRY ,
	MB_ROW_WIDTH_GEOMETRIE,
	MB_ROW_WIDTH_AUDIO 	,
	MB_ROW_WIDTH_LENGTH,
	MB_ROW_WIDTH_SIZE, 
	0 //MB_ROW_WIDTH_MAX_NUMBER 
};

//------------------------------------------------------------------------
// sorting
//------------------------------------------------------------------------
#define FILEBROWSER_NUMBER_OF_SORT_VARIANTS 5

bool sortDirection = 0;

bool compare_to_lower(const char a, const char b)
{
	return tolower(a) < tolower(b);
};

// sort operators
bool sortByTitle (const MI_MOVIE_INFO* a, const MI_MOVIE_INFO* b)
{
	if (std::lexicographical_compare(a->epgTitle.begin(), a->epgTitle.end(), b->epgTitle.begin(), b->epgTitle.end(), compare_to_lower))
		return true;
	if (std::lexicographical_compare(b->epgTitle.begin(), b->epgTitle.end(), a->epgTitle.begin(), a->epgTitle.end(), compare_to_lower))
		return false;
	return a->file.Time < b->file.Time;
}
bool sortByGenre (const MI_MOVIE_INFO* a, const MI_MOVIE_INFO* b)
{
	if (std::lexicographical_compare(a->epgInfo1.begin(), a->epgInfo1.end(), b->epgInfo1.begin(), b->epgInfo1.end(), compare_to_lower))
		return true;
	if (std::lexicographical_compare(b->epgInfo1.begin(), b->epgInfo1.end(), a->epgInfo1.begin(), a->epgInfo1.end(), compare_to_lower))
		return false;
	return sortByTitle(a,b);
}
bool sortByChannel (const MI_MOVIE_INFO* a, const MI_MOVIE_INFO* b)
{
	if (std::lexicographical_compare(a->epgChannel.begin(), a->epgChannel.end(), b->epgChannel.begin(), b->epgChannel.end(), compare_to_lower))
		return true;
	if (std::lexicographical_compare(b->epgChannel.begin(), b->epgChannel.end(), a->epgChannel.begin(), a->epgChannel.end(), compare_to_lower))
		return false;
	return sortByTitle(a,b);
}
bool sortByFileName (const MI_MOVIE_INFO* a, const MI_MOVIE_INFO* b)
{
	if (std::lexicographical_compare(a->file.getFileName().begin(), a->file.getFileName().end(), b->file.getFileName().begin(), b->file.getFileName().end(), compare_to_lower))
		return true;
	if (std::lexicographical_compare(b->file.getFileName().begin(), b->file.getFileName().end(), a->file.getFileName().begin(), a->file.getFileName().end(), compare_to_lower))
		return false;
	return a->file.Time < b->file.Time;
}
bool sortByRecordDate (const MI_MOVIE_INFO* a, const MI_MOVIE_INFO* b)
{
	if(sortDirection)
		return a->file.Time > b->file.Time ;
	else
		return a->file.Time < b->file.Time ;
}
bool sortBySize (const MI_MOVIE_INFO* a, const MI_MOVIE_INFO* b)
{
	if(sortDirection)
		return a->file.Size > b->file.Size;
	else
		return a->file.Size < b->file.Size;
}
bool sortByAge (const MI_MOVIE_INFO* a, const MI_MOVIE_INFO* b)
{
	if(sortDirection)
		return a->parentalLockAge > b->parentalLockAge;
	else
		return a->parentalLockAge < b->parentalLockAge;
}
bool sortByQuality (const MI_MOVIE_INFO* a, const MI_MOVIE_INFO* b)
{
	if(sortDirection)
		return a->quality > b->quality;
	else
		return a->quality < b->quality;
}
bool sortByDir (const MI_MOVIE_INFO* a, const MI_MOVIE_INFO* b)
{
	if(sortDirection)
		return a->dirItNr > b->dirItNr;
	else
		return a->dirItNr < b->dirItNr;
}

bool sortByLastPlay (const MI_MOVIE_INFO* a, const MI_MOVIE_INFO* b)
{
	if(sortDirection)
		return a->dateOfLastPlay > b->dateOfLastPlay;
	else
		return a->dateOfLastPlay < b->dateOfLastPlay;
}

bool (* const sortBy[MB_INFO_MAX_NUMBER+1])(const MI_MOVIE_INFO* a, const MI_MOVIE_INFO* b) =
{
	&sortByFileName ,	//MB_INFO_FILENAME		= 0,
	&sortByDir, 		//MB_INFO_FILEPATH		= 1,
	&sortByTitle, 		//MB_INFO_TITLE			= 2,
	NULL, 				//MB_INFO_SERIE 		= 3,
	&sortByGenre, 		//MB_INFO_INFO1			= 4,
	NULL, 				//MB_INFO_MAJOR_GENRE 	= 5,
	NULL, 				//MB_INFO_MINOR_GENRE 	= 6,
	NULL, 				//MB_INFO_INFO2 			= 7,
	&sortByAge, 		//MB_INFO_PARENTAL_LOCKAGE			= 8,
	&sortByChannel, 	//MB_INFO_CHANNEL		= 9,
	NULL, 				//MB_INFO_BOOKMARK		= 10,
	&sortByQuality, 	//MB_INFO_QUALITY		= 11,
	&sortByLastPlay, 	//MB_INFO_PREVPLAYDATE 	= 12,
	&sortByRecordDate, 	//MB_INFO_RECORDDATE	= 13,
	NULL, 				//MB_INFO_PRODDATE 		= 14,
	NULL, 				//MB_INFO_COUNTRY 		= 15,
	NULL, 				//MB_INFO_GEOMETRIE 	= 16,
	NULL, 				//MB_INFO_AUDIO 		= 17,
	NULL, 				//MB_INFO_LENGTH 		= 18,
	&sortBySize, 		//MB_INFO_SIZE 			= 19, 
	NULL				//MB_INFO_MAX_NUMBER		= 20
};
/************************************************************************
 Public API
************************************************************************/


/************************************************************************

************************************************************************/
CMovieBrowser::CMovieBrowser(const char* path): configfile ('\t')
{
	m_selectedDir = path; 
	addRepositoryDir(m_selectedDir);
	CMovieBrowser();
}
/************************************************************************

************************************************************************/
CMovieBrowser::CMovieBrowser(): configfile ('\t')
{
	TRACE("$Id: moviebrowser.cpp,v 1.8 2006/02/20 01:10:34 guenther Exp $\r\n");
	init();
}

/************************************************************************

************************************************************************/
CMovieBrowser::~CMovieBrowser()
{
	//TRACE("[mb] del\r\n");
	//saveSettings(&m_settings);		
	hide();
	m_repositoryDir.clear();

	m_dirNames.clear();
	for(unsigned int i=0; i < m_vMovieInfo.size(); i++)
	{
		m_vMovieInfo[i].audioPids.clear();
	}
	m_vMovieInfo.clear();
	m_vHandleBrowserList.clear();
	m_vHandleRecordList.clear();
	m_vHandlePlayList.clear();
	m_serienames.clear();

	for(int i = 0; i < LF_MAX_ROWS; i++)
	{
		m_browserListLines.lineArray[i].clear();
		m_recordListLines.lineArray[i].clear();
		m_playListLines.lineArray[i].clear();
		m_FilterLines.lineArray[i].clear();
	}
}


/************************************************************************

************************************************************************/
void CMovieBrowser::fileInfoStale(void)
{
	m_file_info_stale = true;
	m_seriename_stale = true;
	
	 // Also release memory buffers, since we have to reload this stuff next time anyhow 
	m_dirNames.clear();
	
	for(unsigned int i=0; i < m_vMovieInfo.size(); i++)
	{
		m_vMovieInfo[i].audioPids.clear();
	}
	
	m_vMovieInfo.clear();
	m_vHandleBrowserList.clear();
	m_vHandleRecordList.clear();
	m_vHandlePlayList.clear();
	m_serienames.clear();
	
	for(int i = 0; i < LF_MAX_ROWS; i++)
	{
		m_browserListLines.lineArray[i].clear();
		m_recordListLines.lineArray[i].clear();
		m_playListLines.lineArray[i].clear();
		m_FilterLines.lineArray[i].clear();
	}

}; 


/************************************************************************

************************************************************************/
void CMovieBrowser::init(void)
{
	//TRACE("[mb]->init\r\n");
	initGlobalSettings();
	loadSettings(&m_settings);
		
	m_file_info_stale = true;
	m_seriename_stale = true;

	m_pcWindow = NULL;
	m_pcBrowser = NULL;
	m_pcLastPlay = NULL;
	m_pcLastRecord = NULL;
	m_pcInfo = NULL;
	
	m_windowFocus = MB_FOCUS_BROWSER;

	m_pcFontFoot  = FOOT_FONT;
	m_pcFontTitle = TITLE_FONT;
	
	m_textTitle = g_Locale->getText(LOCALE_MOVIEBROWSER_HEAD);
	
	m_currentStartPos = 0;
	
	m_movieSelectionHandler = NULL;
	m_currentBrowserSelection = 0;
	m_currentRecordSelection = 0;
	m_currentPlaySelection = 0;
 	m_prevBrowserSelection = 0;
	m_prevRecordSelection = 0;
	m_prevPlaySelection = 0;
	
	m_storageType = MB_STORAGE_TYPE_NFS;
	m_repositoryDir.clear();
	for(int i = 0; i < MB_MAX_DIRS; i++)
	{
		if(!m_settings.storageDir[i].empty())
			addRepositoryDir(m_settings.storageDir[i]);
	}
	if(g_settings.network_nfs_moviedir[0] != 0)
	{
		std::string name = g_settings.network_nfs_moviedir;
		addRepositoryDir(name);
	}
	if(g_settings.network_nfs_recordingdir[0] != 0)
	{
		std::string name = g_settings.network_nfs_recordingdir;
		addRepositoryDir(name);
	}
	m_parentalLock = m_settings.parentalLock;
	
	// check g_setting values 
	if(m_settings.gui >= MB_GUI_MAX_NUMBER)
		m_settings.gui = MB_GUI_MOVIE_INFO;
	
	if(m_settings.sorting.direction >= MB_DIRECTION_MAX_NUMBER)
		m_settings.sorting.direction = MB_DIRECTION_DOWN;
	if(m_settings.sorting.item 	>=  MB_INFO_MAX_NUMBER)
		m_settings.sorting.item =  MB_INFO_TITLE;

	if(m_settings.filter.item >= MB_INFO_MAX_NUMBER)
		m_settings.filter.item = MB_INFO_MAX_NUMBER;
	
	if(m_settings.parentalLockAge >= MI_PARENTAL_MAX_NUMBER)
		m_settings.parentalLockAge = MI_PARENTAL_OVER18;
	if(m_settings.parentalLock >= MB_PARENTAL_LOCK_MAX_NUMBER)
		m_settings.parentalLock = MB_PARENTAL_LOCK_OFF;
	
	if(m_settings.browserFrameHeight < 100 || m_settings.browserFrameHeight > 400)
		m_settings.browserFrameHeight = 250;
	/***** Browser List **************/
	if(m_settings.browserRowNr == 0)
	{
		TRACE(" row error\r\n");
		// init browser row elements if not configured correctly by neutrino.config
		m_settings.browserRowNr = 6;
		m_settings.browserRowItem[0] = MB_INFO_TITLE;
		m_settings.browserRowItem[1] = MB_INFO_INFO1;
		m_settings.browserRowItem[2] = MB_INFO_RECORDDATE;
		m_settings.browserRowItem[3] = MB_INFO_SIZE;
		m_settings.browserRowItem[4] = MB_INFO_PARENTAL_LOCKAGE;
		m_settings.browserRowItem[5] = MB_INFO_QUALITY;
		m_settings.browserRowWidth[0] = m_defaultRowWidth[m_settings.browserRowItem[0]];		//300;
		m_settings.browserRowWidth[1] = m_defaultRowWidth[m_settings.browserRowItem[1]]; 		//100;
		m_settings.browserRowWidth[2] = m_defaultRowWidth[m_settings.browserRowItem[2]]; 		//80;
		m_settings.browserRowWidth[3] = m_defaultRowWidth[m_settings.browserRowItem[3]]; 		//50;
		m_settings.browserRowWidth[4] = m_defaultRowWidth[m_settings.browserRowItem[4]]; 		//30;
		m_settings.browserRowWidth[5] = m_defaultRowWidth[m_settings.browserRowItem[5]]; 		//30;
	}

	initFrames();
	initRows();
	//initDevelopment();

	refreshLastPlayList();	
	refreshLastRecordList();	
	refreshBrowserList();	
	refreshFilterList();	
#if 0
	TRACE_1("Frames\r\n\tScren:\t%3d,%3d,%3d,%3d\r\n\tMain:\t%3d,%3d,%3d,%3d\r\n\tTitle:\t%3d,%3d,%3d,%3d \r\n\tBrowsr:\t%3d,%3d,%3d,%3d \r\n\tPlay:\t%3d,%3d,%3d,%3d \r\n\tRecord:\t%3d,%3d,%3d,%3d\r\n\r\n",
	g_settings.screen_StartX,
	g_settings.screen_StartY,
	g_settings.screen_EndX,
	g_settings.screen_EndY,
	m_cBoxFrame.iX,
	m_cBoxFrame.iY,
	m_cBoxFrame.iWidth,
	m_cBoxFrame.iHeight,
	m_cBoxFrameTitleRel.iX,
	m_cBoxFrameTitleRel.iY,
	m_cBoxFrameTitleRel.iWidth,
	m_cBoxFrameTitleRel.iHeight,
	m_cBoxFrameBrowserList.iX,
	m_cBoxFrameBrowserList.iY,
	m_cBoxFrameBrowserList.iWidth,
	m_cBoxFrameBrowserList.iHeight,
	m_cBoxFrameLastPlayList.iX,
	m_cBoxFrameLastPlayList.iY,
	m_cBoxFrameLastPlayList.iWidth,
	m_cBoxFrameLastPlayList.iHeight,
	m_cBoxFrameLastRecordList.iX,
	m_cBoxFrameLastRecordList.iY,
	m_cBoxFrameLastRecordList.iWidth,
	m_cBoxFrameLastRecordList.iHeight
	);
#endif
}

/************************************************************************

************************************************************************/
void CMovieBrowser::initGlobalSettings(void)
{
	//TRACE("[mb]->initGlobalSettings\r\n");
	
	m_settings.gui = MB_GUI_MOVIE_INFO;
	
	m_settings.sorting.direction = MB_DIRECTION_DOWN;
	m_settings.sorting.item 	=  MB_INFO_TITLE;

	m_settings.filter.item = MB_INFO_MAX_NUMBER;
	m_settings.filter.optionString = "";
	m_settings.filter.optionVar = 0;
	
	m_settings.parentalLockAge = MI_PARENTAL_OVER18;
	m_settings.parentalLock = MB_PARENTAL_LOCK_OFF;
	
	for(int i = 0; i < MB_MAX_DIRS; i++)
		m_settings.storageDir[i] = "";

	/***** Browser List **************/
	m_settings.browserFrameHeight = g_settings.screen_EndY - g_settings.screen_StartY - 20 - ((g_settings.screen_EndY - g_settings.screen_StartY - 20)>>1) - (INTER_FRAME_SPACE>>1);

	m_settings.browserFrameHeight = 250;
	
	m_settings.browserRowNr = 6;
	m_settings.browserRowItem[0] = MB_INFO_TITLE;
	m_settings.browserRowItem[1] = MB_INFO_INFO1;
	m_settings.browserRowItem[2] = MB_INFO_RECORDDATE;
	m_settings.browserRowItem[3] = MB_INFO_SIZE;
	m_settings.browserRowItem[4] = MB_INFO_PARENTAL_LOCKAGE;
	m_settings.browserRowItem[5] = MB_INFO_QUALITY;
	m_settings.browserRowWidth[0] = m_defaultRowWidth[m_settings.browserRowItem[0]];		//300;
	m_settings.browserRowWidth[1] = m_defaultRowWidth[m_settings.browserRowItem[1]]; 		//100;
	m_settings.browserRowWidth[2] = m_defaultRowWidth[m_settings.browserRowItem[2]]; 		//80;
	m_settings.browserRowWidth[3] = m_defaultRowWidth[m_settings.browserRowItem[3]]; 		//50;
	m_settings.browserRowWidth[4] = m_defaultRowWidth[m_settings.browserRowItem[4]]; 		//30;
	m_settings.browserRowWidth[5] = m_defaultRowWidth[m_settings.browserRowItem[5]]; 		//30;

	m_settings.storageDir_movie = true;
	m_settings.storageDir_rec = true;
	m_settings.reload = false;
	m_settings.remount = false;
}

/************************************************************************
 
************************************************************************/
void CMovieBrowser::initFrames(void)
{
	//TRACE("[mb]->initFrames\r\n");
	m_cBoxFrame.iX = 					g_settings.screen_StartX + 10;
	m_cBoxFrame.iY = 					g_settings.screen_StartY + 10;
	m_cBoxFrame.iWidth = 				g_settings.screen_EndX - g_settings.screen_StartX - 20;
	m_cBoxFrame.iHeight = 				g_settings.screen_EndY - g_settings.screen_StartY - 20;

	m_cBoxFrameTitleRel.iX = 			0;
	m_cBoxFrameTitleRel.iY = 			0;
	m_cBoxFrameTitleRel.iWidth = 		m_cBoxFrame.iWidth;
	m_cBoxFrameTitleRel.iHeight = 		m_pcFontTitle->getHeight();

	m_cBoxFrameBrowserList.iX = 		m_cBoxFrame.iX;
	m_cBoxFrameBrowserList.iY = 		m_cBoxFrame.iY + m_cBoxFrameTitleRel.iHeight;
	m_cBoxFrameBrowserList.iWidth = 	m_cBoxFrame.iWidth;
	m_cBoxFrameBrowserList.iHeight = 	m_settings.browserFrameHeight; //m_cBoxFrame.iHeight - (m_cBoxFrame.iHeight>>1) - (INTER_FRAME_SPACE>>1);

	m_cBoxFrameFootRel.iX = 			0;
	m_cBoxFrameFootRel.iY = 			m_cBoxFrame.iHeight - m_pcFontFoot->getHeight();
	m_cBoxFrameFootRel.iWidth = 		m_cBoxFrameBrowserList.iWidth;
	m_cBoxFrameFootRel.iHeight = 		m_pcFontFoot->getHeight();

	m_cBoxFrameLastPlayList.iX = 		m_cBoxFrameBrowserList.iX;
	m_cBoxFrameLastPlayList.iY = 		m_cBoxFrameBrowserList.iY ;
	m_cBoxFrameLastPlayList.iWidth = 	(m_cBoxFrameBrowserList.iWidth>>1) - (INTER_FRAME_SPACE>>1);
	m_cBoxFrameLastPlayList.iHeight = 	m_cBoxFrameBrowserList.iHeight;
	
	m_cBoxFrameLastRecordList.iX = 		m_cBoxFrameLastPlayList.iX + m_cBoxFrameLastPlayList.iWidth + INTER_FRAME_SPACE;
	m_cBoxFrameLastRecordList.iY = 		m_cBoxFrameLastPlayList.iY;
	m_cBoxFrameLastRecordList.iWidth = 	m_cBoxFrame.iWidth - m_cBoxFrameLastPlayList.iWidth - INTER_FRAME_SPACE;
	m_cBoxFrameLastRecordList.iHeight = m_cBoxFrameLastPlayList.iHeight;
	
	m_cBoxFrameInfo.iX = 				m_cBoxFrameBrowserList.iX;
	m_cBoxFrameInfo.iY = 				m_cBoxFrameBrowserList.iY + m_cBoxFrameBrowserList.iHeight + INTER_FRAME_SPACE;
	m_cBoxFrameInfo.iWidth = 			m_cBoxFrameBrowserList.iWidth;
	m_cBoxFrameInfo.iHeight = 			m_cBoxFrame.iHeight - m_cBoxFrameBrowserList.iHeight - INTER_FRAME_SPACE - m_cBoxFrameFootRel.iHeight - m_cBoxFrameTitleRel.iHeight;
	
	m_cBoxFrameFilter.iX = 				m_cBoxFrameInfo.iX;
	m_cBoxFrameFilter.iY = 				m_cBoxFrameInfo.iY;
	m_cBoxFrameFilter.iWidth = 			m_cBoxFrameInfo.iWidth;
	m_cBoxFrameFilter.iHeight = 		m_cBoxFrameInfo.iHeight;
}
	



/************************************************************************
 
************************************************************************/
void CMovieBrowser::initRows(void)
{
	//TRACE("[mb]->initRows\r\n");

	/***** Last Play List **************/
	m_settings.lastPlayRowNr = 2;
	m_settings.lastPlayRow[0] = MB_INFO_TITLE;
	m_settings.lastPlayRow[1] = MB_INFO_PREVPLAYDATE;
	m_settings.lastPlayRow[2] = MB_INFO_MAX_NUMBER;
	m_settings.lastPlayRow[3] = MB_INFO_MAX_NUMBER;
	m_settings.lastPlayRow[4] = MB_INFO_MAX_NUMBER;
	m_settings.lastPlayRow[5] = MB_INFO_MAX_NUMBER;
	m_settings.lastPlayRowWidth[0] = 190;
	m_settings.lastPlayRowWidth[1]  = m_defaultRowWidth[m_settings.lastPlayRow[1]];
	m_settings.lastPlayRowWidth[2] = m_defaultRowWidth[m_settings.lastPlayRow[2]];
	m_settings.lastPlayRowWidth[3] = m_defaultRowWidth[m_settings.lastPlayRow[3]];
	m_settings.lastPlayRowWidth[4] = m_defaultRowWidth[m_settings.lastPlayRow[4]];
	m_settings.lastPlayRowWidth[5] = m_defaultRowWidth[m_settings.lastPlayRow[5]];
	
	/***** Last Record List **************/
	m_settings.lastRecordRowNr = 2;
	m_settings.lastRecordRow[0] = MB_INFO_TITLE;
	m_settings.lastRecordRow[1] = MB_INFO_RECORDDATE;
	m_settings.lastRecordRow[2] = MB_INFO_MAX_NUMBER;
	m_settings.lastRecordRow[3] = MB_INFO_MAX_NUMBER;
	m_settings.lastRecordRow[4] = MB_INFO_MAX_NUMBER;
	m_settings.lastRecordRow[5] = MB_INFO_MAX_NUMBER;
	m_settings.lastRecordRowWidth[0] = 190;
	m_settings.lastRecordRowWidth[1] = m_defaultRowWidth[m_settings.lastRecordRow[1]];
	m_settings.lastRecordRowWidth[2] = m_defaultRowWidth[m_settings.lastRecordRow[2]];
	m_settings.lastRecordRowWidth[3] = m_defaultRowWidth[m_settings.lastRecordRow[3]];
	m_settings.lastRecordRowWidth[4] = m_defaultRowWidth[m_settings.lastRecordRow[4]];
	m_settings.lastRecordRowWidth[5] = m_defaultRowWidth[m_settings.lastRecordRow[5]];
}
/************************************************************************
 
************************************************************************/
void CMovieBrowser::initDevelopment(void)
{
	TRACE("[mb]->initDevelopment\r\n");
	std::string name;
	name = "/mnt/movies/";
	//addRepositoryDir(name);
	name = "/mnt/record/";
	//addRepositoryDir(name);
	name = "/mnt/nfs/";
	//addRepositoryDir(name);
	
}

/************************************************************************
 
************************************************************************/
bool CMovieBrowser::loadSettings(MB_SETTINGS* settings)
{
	bool result = true;
	//TRACE("CMovieBrowser::loadSettings\r\n"); 
	if(configfile.loadConfig(MOVIEBROWSER_SETTINGS_FILE))
	{
		settings->gui = (MB_GUI)configfile.getInt32("mb_gui", MB_GUI_MOVIE_INFO);
		
		settings->sorting.item = (MB_INFO_ITEM)configfile.getInt32("mb_sorting_item", MB_INFO_TITLE);
		settings->sorting.direction = (MB_DIRECTION)configfile.getInt32("mb_sorting_direction", MB_DIRECTION_UP);
		
		settings->filter.item = (MB_INFO_ITEM)configfile.getInt32("mb_filter_item", MB_INFO_INFO1);
		settings->filter.optionString = configfile.getString("mb_filter_optionString", "");
		settings->filter.optionVar = configfile.getInt32("mb_filter_optionVar", 0);
		
		settings->parentalLockAge = (MI_PARENTAL_LOCKAGE)configfile.getInt32("mb_parentalLockAge", MI_PARENTAL_OVER18);
		settings->parentalLock = (MB_PARENTAL_LOCK)configfile.getInt32("mb_parentalLock", MB_PARENTAL_LOCK_ACTIVE);
	
		settings->storageDir_rec = (bool)configfile.getInt32("mb_storageDir_rec", true );
		settings->storageDir_movie = (bool)configfile.getInt32("mb_storageDir_movie", true );

		settings->reload = (bool)configfile.getInt32("mb_reload", false );
		settings->remount = (bool)configfile.getInt32("mb_remount", false );

		char cfg_key[81];
		for(int i = 0; i < MB_MAX_DIRS; i++)
		{
			sprintf(cfg_key, "mb_dir_%d", i);
			settings->storageDir[i] = configfile.getString( cfg_key, "" );
		}
		/* these variables are used for the listframes */	
		settings->browserFrameHeight  = configfile.getInt32("mb_browserFrameHeight", 250);
		settings->browserRowNr  = configfile.getInt32("mb_browserRowNr", 0);
		for(int i = 0; i < MB_MAX_ROWS && i < settings->browserRowNr; i++)
		{
			sprintf(cfg_key, "mb_browserRowItem_%d", i);
			settings->browserRowItem[i] = (MB_INFO_ITEM)configfile.getInt32(cfg_key, MB_INFO_MAX_NUMBER);
			sprintf(cfg_key, "mb_browserRowWidth_%d", i);
			settings->browserRowWidth[i] = configfile.getInt32(cfg_key, 50);
		}
	}
	else
	{
		TRACE("CMovieBrowser::loadSettings failed\r\n"); 
		configfile.clear();
		result = false;
	}
	return (result);
}

/************************************************************************
 
************************************************************************/
bool CMovieBrowser::saveSettings(MB_SETTINGS* settings)
{
	bool result = true;
	TRACE("[mb] saveSettings\r\n"); 
	configfile.setInt32("mb_gui", settings->gui);
	
	configfile.setInt32("mb_sorting_item", settings->sorting.item);
	configfile.setInt32("mb_sorting_direction", settings->sorting.direction);
	
	configfile.setInt32("mb_filter_item", settings->filter.item);
	configfile.setString("mb_filter_optionString", settings->filter.optionString);
	configfile.setInt32("mb_filter_optionVar", settings->filter.optionVar);
	
	configfile.setInt32("mb_storageDir_rec", settings->storageDir_rec );
	configfile.setInt32("mb_storageDir_movie", settings->storageDir_movie );

	configfile.setInt32("mb_parentalLockAge", settings->parentalLockAge);
	configfile.setInt32("mb_parentalLock", settings->parentalLock);

	configfile.setInt32("mb_reload", settings->reload);
	configfile.setInt32("mb_remount", settings->remount);

	char cfg_key[81];
	for(int i = 0; i < MB_MAX_DIRS; i++)
	{
		sprintf(cfg_key, "mb_dir_%d", i);
		configfile.setString( cfg_key, settings->storageDir[i] );
	}
	/* these variables are used for the listframes */	
	configfile.setInt32("mb_browserFrameHeight", settings->browserFrameHeight);
	configfile.setInt32("mb_browserRowNr",settings->browserRowNr);
	for(int i = 0; i < MB_MAX_ROWS && i < settings->browserRowNr; i++)
	{
		sprintf(cfg_key, "mb_browserRowItem_%d", i);
		configfile.setInt32(cfg_key, settings->browserRowItem[i]);
		sprintf(cfg_key, "mb_browserRowWidth_%d", i);
		configfile.setInt32(cfg_key, settings->browserRowWidth[i]);
	}

	configfile.saveConfig(MOVIEBROWSER_SETTINGS_FILE);
	return (result);
}
/************************************************************************

************************************************************************/
int CMovieBrowser::exec(const char* path)
{
	bool res = false;

	TRACE("[mb] start MovieBrowser\r\n");
	int timeout = -1;
	int returnDefaultOnTimeout = false;
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	// might be removed, for development it is good to reload the settings at any startup for testing
	//loadSettings(&m_settings);
	
	// Clear all, to avoid 'jump' in screen 
	m_vHandleBrowserList.clear();
	m_vHandleRecordList.clear();
	m_vHandlePlayList.clear();

	for(int i = 0; i < LF_MAX_ROWS; i++)
	{
		m_browserListLines.lineArray[i].clear();
		m_recordListLines.lineArray[i].clear();
		m_playListLines.lineArray[i].clear();
	}

	m_selectedDir = path; 
	//addRepositoryDir(m_selectedDir);
	
	if(paint() == false)
		return res;// paint failed due to less memory , exit 

	if ( timeout == -1 )
		timeout = g_settings.timing[SNeutrinoSettings::TIMING_FILEBROWSER];

	unsigned long long timeoutEnd = CRCInput::calcTimeoutEnd( timeout );
	
	if(m_settings.remount == true)
	{
		TRACE("[mb] remount\r\n");
		//umount automount dirs
		for(int i = 0; i < NETWORK_NFS_NR_OF_ENTRIES; i++)
		{
			if(g_settings.network_nfs_automount[i])
				umount2(g_settings.network_nfs_local_dir[i],MNT_FORCE);
		}
		CFSMounter::automount();
	}

	if(m_file_info_stale == true)
	{
		TRACE("[mb] reload\r\n");
		loadMovies();
	}
	else
	{
		// since we cleared everything above, we have to refresh the list now.
		refreshBrowserList();	
		refreshLastPlayList();	
		refreshLastRecordList();
	}
	
	// get old movie selection and set position in windows	
 	m_currentBrowserSelection = m_prevBrowserSelection;
	m_currentRecordSelection = m_prevRecordSelection;
	m_currentPlaySelection = m_prevPlaySelection;

	m_pcBrowser->setSelectedLine(m_currentBrowserSelection);
	m_pcLastRecord->setSelectedLine(m_currentRecordSelection);
	m_pcLastPlay->setSelectedLine(m_currentPlaySelection);
	
	updateMovieSelection();
	//refreshMovieInfo();
 
 	onSetGUIWindow(m_settings.gui);
 
 	bool loop = true;
	bool result;
	while (loop)
	{
		g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );
		
		if ( msg <= CRCInput::RC_MaxRC )
			timeoutEnd = CRCInput::calcTimeoutEnd(timeout);
		
		result = onButtonPress(msg);
		
		if(result == false)
		{
			if (msg == CRCInput::RC_timeout && returnDefaultOnTimeout)
			{
				TRACE("[mb]->exec: RC_timeout\r\n");
				loop = false;
			}
			else if (msg == CRCInput::RC_timeout) 
			{
				TRACE("[mb]->exec: RC_timeout\r\n");
				loop = false;
			}

			else if(msg == CRCInput::RC_ok)
			{
				m_currentStartPos = 0; 
				
				if(m_movieSelectionHandler != NULL)
				{
					// If there is any available bookmark, show the bookmark menu
					if( m_movieSelectionHandler->bookmarks.lastPlayStop != 0 ||
						m_movieSelectionHandler->bookmarks.start != 0)
						{
							TRACE("[mb] stop: %d start:%d \r\n",m_movieSelectionHandler->bookmarks.lastPlayStop,m_movieSelectionHandler->bookmarks.start);
							m_currentStartPos = showStartPosSelectionMenu(); // display start menu m_currentStartPos = 
						}
				}
				TRACE("[mb] start pos: %d s\r\n",m_currentStartPos);
					res = true;
					loop = false;
				}
			else if (msg == CRCInput::RC_home)
			{
				loop = false;
			}
			else if (CNeutrinoApp::getInstance()->handleMsg(msg, data) & messages_return::cancel_all)
			{
				TRACE("[mb]->exec: getInstance\r\n");
				//res  = menu_return::RETURN_EXIT_ALL;
				loop = false;
			}
		}
	}
	hide();
	//TRACE(" return %d\r\n",res);
	
 	m_prevBrowserSelection = m_currentBrowserSelection;
	m_prevRecordSelection = m_currentRecordSelection;
	m_prevPlaySelection = m_currentPlaySelection;

	saveSettings(&m_settings);	// might be better done in ~CMovieBrowser, but for any reason this does not work if MB is killed by neutrino shutdown	

	// make stale if we should reload the next time, but not if movie has to be played
	if(m_settings.reload == true && res == false)
	{
		TRACE("[mb] force reload next time\r\n");
		fileInfoStale();
	}
		
	return (res);
}

/************************************************************************

************************************************************************/
bool CMovieBrowser::hide(void)
{
	//TRACE("[mb]->Hide\r\n");

	if (m_pcFilter != NULL)
	{
		m_currentFilterSelection  = m_pcFilter->getSelectedLine();
		delete m_pcFilter;
		m_pcFilter = NULL;
	}
	if (m_pcBrowser != NULL)
	{
		m_currentBrowserSelection = m_pcBrowser->getSelectedLine();
		delete m_pcBrowser;
		m_pcBrowser = NULL;
	}
	if (m_pcLastPlay != NULL)
	{
		m_currentPlaySelection    = m_pcLastPlay->getSelectedLine();
		delete m_pcLastPlay;
		m_pcLastPlay = NULL;
	}
	if (m_pcLastRecord != NULL) 
	{
		m_currentRecordSelection  = m_pcLastRecord->getSelectedLine();
		delete m_pcLastRecord;
		m_pcLastRecord = NULL;
	}
	if (m_pcInfo != NULL) delete m_pcInfo;
	if (m_pcWindow != NULL) delete m_pcWindow;

	m_pcWindow = NULL;
	m_pcInfo = NULL;
	
	return (true);
}

/************************************************************************

************************************************************************/
int CMovieBrowser::paint(void)
{
	if (m_pcWindow != NULL)
	{
		TRACE("[mb]  return -> window already exists\r\n");
		return (false);
	}
	TRACE("[mb]->Paint\r\n");

	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, g_Locale->getText(LOCALE_MOVIEBROWSER_HEAD));

	//Font* font = TEXT_FONT;
	Font* font = NULL;

	// create new window
	m_pcWindow = new CFBWindow( m_cBoxFrame.iX,
								m_cBoxFrame.iY,
								m_cBoxFrame.iWidth,
								m_cBoxFrame.iHeight+10);

	m_pcBrowser = new CListFrame(&m_browserListLines,
								font,
								CListFrame::SCROLL | CListFrame::HEADER_LINE, 
								&m_cBoxFrameBrowserList);
	m_pcLastPlay = new CListFrame(&m_playListLines,
								font,
								CListFrame::SCROLL | CListFrame::HEADER_LINE | CListFrame::TITLE, 
								&m_cBoxFrameLastPlayList,
								g_Locale->getText(LOCALE_MOVIEBROWSER_HEAD_PLAYLIST),
								g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]);
	m_pcLastRecord = new CListFrame(&m_recordListLines,
								font,
								CListFrame::SCROLL | CListFrame::HEADER_LINE | CListFrame::TITLE, 
								&m_cBoxFrameLastRecordList,
								g_Locale->getText(LOCALE_MOVIEBROWSER_HEAD_RECORDLIST),
								g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]);
	m_pcFilter = new CListFrame(&m_FilterLines,
								font,
								CListFrame::SCROLL | CListFrame::TITLE, 
								&m_cBoxFrameFilter,
								g_Locale->getText(LOCALE_MOVIEBROWSER_HEAD_FILTER),
								g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]);
	m_pcInfo = new CTextBox(" ",
							NULL,
							CTextBox::SCROLL,
							&m_cBoxFrameInfo);							


	if(  m_pcWindow == NULL || 
		m_pcBrowser == NULL || 
		m_pcLastPlay == NULL || 
		m_pcLastRecord == NULL || 
		m_pcInfo == NULL || 
		m_pcFilter == NULL)
	{
		TRACE("[mb] paint, ERROR: not enought memory to allocate windows");
		if (m_pcFilter != NULL)delete m_pcFilter;
		if (m_pcBrowser != NULL)delete m_pcBrowser;
		if (m_pcLastPlay != NULL) delete m_pcLastPlay;
		if (m_pcLastRecord != NULL)delete m_pcLastRecord;
		if (m_pcInfo != NULL) delete m_pcInfo;
		if (m_pcWindow != NULL) delete m_pcWindow;

		m_pcWindow = NULL;
		m_pcInfo = NULL;
		m_pcLastPlay = NULL;
		m_pcLastRecord = NULL;
		m_pcBrowser = NULL;
		m_pcFilter = NULL;

		return (false);
	}  
	//onSetGUIWindow(m_settings.gui);	
							
	refreshTitle();
	refreshFoot();
	refreshLCD();
	//refresh();
	return (true);
}

/************************************************************************

************************************************************************/
void CMovieBrowser::refresh(void)
{
	//TRACE("[mb]->refresh\r\n");
	
	if (m_pcBrowser != NULL && m_showBrowserFiles == true )
		 m_pcBrowser->refresh();
	if (m_pcLastPlay != NULL && m_showLastPlayFiles == true ) 
		m_pcLastPlay->refresh();
	if (m_pcLastRecord != NULL && m_showLastRecordFiles == true)
		 m_pcLastRecord->refresh();
	if (m_pcInfo != NULL && m_showMovieInfo == true) 
		m_pcInfo->refresh();
	if (m_pcFilter != NULL && m_showFilter == true) 
		m_pcFilter->refresh();
		
	refreshTitle();
	refreshFoot();
	refreshLCD();
}

/************************************************************************

************************************************************************/
std::string CMovieBrowser::getCurrentDir(void)
{
	return(m_selectedDir);
}

/************************************************************************

************************************************************************/
CFile* CMovieBrowser::getSelectedFile(void)
{
	//TRACE("[mb]->getSelectedFile: %s\r\n",m_movieSelectionHandler->file.Name.c_str());

	if(m_movieSelectionHandler != NULL)
		return(&m_movieSelectionHandler->file);
	else
		return(NULL);
}

/************************************************************************

************************************************************************/
void CMovieBrowser::refreshMovieInfo(void)
{
	//TRACE("[mb]->refreshMovieInfo \r\n");
	if(m_vMovieInfo.size() <= 0) return;
	if (m_movieSelectionHandler == NULL)
	{
		// There is no selected element, clear LCD
		std::string emptytext = " ";
		m_pcInfo->setText(&emptytext);
	}
	else
	{
		m_pcInfo->setText(&m_movieSelectionHandler->epgInfo2);
	}
}

/************************************************************************

************************************************************************/
void CMovieBrowser::refreshLCD(void)
{
	if(m_vMovieInfo.size() <= 0) return;

	CLCD * lcd = CLCD::getInstance();
	if(m_movieSelectionHandler == NULL)
	{
		// There is no selected element, clear LCD
		lcd->showMenuText(0, " ", -1, true); // UTF-8
		lcd->showMenuText(1, " ", -1, true); // UTF-8
	}
	else
	{
		lcd->showMenuText(0, m_movieSelectionHandler->epgTitle.c_str(), -1, true); // UTF-8
		lcd->showMenuText(1, m_movieSelectionHandler->epgInfo1.c_str(), -1, true); // UTF-8
	} 
}

/************************************************************************

************************************************************************/
void CMovieBrowser::refreshFilterList(void)
{
	TRACE("[mb]->refreshFilterList %d\r\n",m_settings.filter.item);
		
	std::string string_item;
	
	m_FilterLines.rows = 1;
	m_FilterLines.lineArray[0].clear();
	m_FilterLines.rowWidth[0] = 400;
	m_FilterLines.lineHeader[0]= "";

	if(m_vMovieInfo.size() <= 0) 
		return; // exit here if nothing else is to do

	if(m_settings.filter.item == MB_INFO_MAX_NUMBER)
	{
		// show Main List
		string_item = g_Locale->getText(LOCALE_MOVIEBROWSER_INFO_GENRE_MAJOR);
		m_FilterLines.lineArray[0].push_back(string_item);
		string_item = g_Locale->getText(LOCALE_MOVIEBROWSER_INFO_INFO1);
		m_FilterLines.lineArray[0].push_back(string_item);
		string_item = g_Locale->getText(LOCALE_MOVIEBROWSER_INFO_PATH);
		m_FilterLines.lineArray[0].push_back(string_item);
		string_item = g_Locale->getText(LOCALE_MOVIEBROWSER_INFO_SERIE);
		m_FilterLines.lineArray[0].push_back(string_item);
	}
	else
	{
		std::string tmp = g_Locale->getText(LOCALE_MENU_BACK);
		m_FilterLines.lineArray[0].push_back(tmp);
		
		 if(m_settings.filter.item == MB_INFO_FILEPATH)
		{
			for(unsigned int i =0 ; i < m_dirNames.size() ;i++)
			{
				m_FilterLines.lineArray[0].push_back(m_dirNames[i]);
			}
		}
		else if(m_settings.filter.item == MB_INFO_INFO1)
		{
			for(unsigned int i = 0; i < m_vMovieInfo.size(); i++)
			{
				bool found = false;
				for(unsigned int t = 0; t < m_FilterLines.lineArray[0].size() && found == false; t++)
				{
					if(strcmp(m_FilterLines.lineArray[0][t].c_str(),m_vMovieInfo[i].epgInfo1.c_str()) == 0)
						found = true;
				}
				if(found == false)
					m_FilterLines.lineArray[0].push_back(m_vMovieInfo[i].epgInfo1);
			}
		}
		else if(m_settings.filter.item == MB_INFO_MAJOR_GENRE)
		{
			for(int i = 0; i < GENRE_ALL_COUNT; i++)
			{
				std::string tmp = g_Locale->getText(GENRE_ALL[i].value);
				m_FilterLines.lineArray[0].push_back(tmp);
			}
		}
		else if(m_settings.filter.item == MB_INFO_SERIE)
		{
			updateSerienames();
			for(unsigned int i = 0; i < m_serienames.size(); i++)
			{
				m_FilterLines.lineArray[0].push_back(m_serienames[i]);
			}
		}
	}
	m_pcFilter->setLines(&m_FilterLines);
}

/************************************************************************

************************************************************************/
void CMovieBrowser::refreshLastPlayList(void) //P2
{
	//TRACE("[mb]->refreshlastPlayList \r\n");
	std::string string_item;

	// Initialise and clear list array
	m_playListLines.rows = m_settings.lastPlayRowNr;
	for(int row = 0 ;row < m_settings.lastPlayRowNr; row++)
	{
		m_playListLines.lineArray[row].clear();
		m_playListLines.rowWidth[row] = m_settings.lastPlayRowWidth[row];
		m_playListLines.lineHeader[row]= g_Locale->getText(m_localizedItemName[m_settings.lastPlayRow[row]]);
	}
	m_vHandlePlayList.clear();

	if(m_vMovieInfo.size() <= 0) 
		return; // exit here if nothing else is to do

	MI_MOVIE_INFO* movie_handle;
	// prepare Browser list for sorting and filtering
	for(unsigned int file=0; file < m_vMovieInfo.size(); file++)
	{
		if(	/*isFiltered(m_vMovieInfo[file]) 	   == false &&*/
			isParentalLock(m_vMovieInfo[file]) == false) 
		{
			movie_handle = &(m_vMovieInfo[file]);
			m_vHandlePlayList.push_back(movie_handle);
		}
	}
	// sort the not filtered files
	onSortMovieInfoHandleList(m_vHandlePlayList,MB_INFO_PREVPLAYDATE,MB_DIRECTION_DOWN);

	for(unsigned int handle=0; handle < m_vHandlePlayList.size() && handle < NUMBER_OF_MOVIES_LAST ;handle++)
	{
		for(int row = 0; row < m_settings.lastPlayRowNr ;row++)
		{
			if ( getMovieInfoItem(*m_vHandlePlayList[handle], m_settings.lastPlayRow[row], &string_item) == false)
			{
				string_item = "n/a";
				if(m_settings.lastPlayRow[row] == MB_INFO_TITLE)
					getMovieInfoItem(*m_vHandlePlayList[handle], MB_INFO_FILENAME, &string_item);
			}
			m_playListLines.lineArray[row].push_back(string_item);
		}
	}
	m_pcLastPlay->setLines(&m_playListLines);

	m_currentPlaySelection = m_pcLastPlay->getSelectedLine();
	// update selected movie if browser is in the focus
	if (m_windowFocus == MB_FOCUS_LAST_PLAY)
	{
		updateMovieSelection();	
	}
}

/************************************************************************

************************************************************************/
void CMovieBrowser::refreshLastRecordList(void) //P2
{
	//TRACE("[mb]->refreshLastRecordList \r\n");
	std::string string_item;

	// Initialise and clear list array
	m_recordListLines.rows = m_settings.lastRecordRowNr;
	for(int row = 0 ;row < m_settings.lastRecordRowNr; row++)
	{
		m_recordListLines.lineArray[row].clear();
		m_recordListLines.rowWidth[row] = m_settings.lastRecordRowWidth[row];
		m_recordListLines.lineHeader[row]= g_Locale->getText(m_localizedItemName[m_settings.lastRecordRow[row]]);
	}
	m_vHandleRecordList.clear();

	if(m_vMovieInfo.size() <= 0) 
		return; // exit here if nothing else is to do

	MI_MOVIE_INFO* movie_handle;
	// prepare Browser list for sorting and filtering
	for(unsigned int file=0; file < m_vMovieInfo.size()  ;file++)
	{
		if(	/*isFiltered(m_vMovieInfo[file]) 	   == false &&*/
			isParentalLock(m_vMovieInfo[file]) == false) 
		{
			movie_handle = &(m_vMovieInfo[file]);
			m_vHandleRecordList.push_back(movie_handle);
		}
	}
	// sort the not filtered files
	onSortMovieInfoHandleList(m_vHandleRecordList,MB_INFO_RECORDDATE,MB_DIRECTION_DOWN);

	for(unsigned int handle=0; handle < m_vHandleRecordList.size() && handle < NUMBER_OF_MOVIES_LAST ;handle++)
	{
		for(int row = 0; row < m_settings.lastRecordRowNr ;row++)
		{
			if ( getMovieInfoItem(*m_vHandleRecordList[handle], m_settings.lastRecordRow[row], &string_item) == false)
			{
				string_item = "n/a";
				if(m_settings.lastRecordRow[row] == MB_INFO_TITLE)
					getMovieInfoItem(*m_vHandleRecordList[handle], MB_INFO_FILENAME, &string_item);
			}
			m_recordListLines.lineArray[row].push_back(string_item);
		}
	}

	m_pcLastRecord->setLines(&m_recordListLines);

	m_currentRecordSelection = m_pcLastRecord->getSelectedLine();
	// update selected movie if browser is in the focus
	if (m_windowFocus == MB_FOCUS_LAST_RECORD)
	{
		updateMovieSelection();	
	}
}

/************************************************************************

************************************************************************/
void CMovieBrowser::refreshBrowserList(void) //P1
{
	//TRACE("[mb]->refreshBrowserList \r\n");
	std::string string_item;

	// Initialise and clear list array
	m_browserListLines.rows = m_settings.browserRowNr;
	for(int row = 0; row < m_settings.browserRowNr; row++)
	{
		m_browserListLines.lineArray[row].clear();
		m_browserListLines.rowWidth[row] = m_settings.browserRowWidth[row];
		m_browserListLines.lineHeader[row]= g_Locale->getText(m_localizedItemName[m_settings.browserRowItem[row]]);
	}
	m_vHandleBrowserList.clear();
	
	if(m_vMovieInfo.size() <= 0) 
		return; // exit here if nothing else is to do
	
	MI_MOVIE_INFO* movie_handle;
	// prepare Browser list for sorting and filtering
	for(unsigned int file=0; file < m_vMovieInfo.size(); file++)
	{
		if(	isFiltered(m_vMovieInfo[file]) 	   == false &&
			isParentalLock(m_vMovieInfo[file]) == false) 
		{
			movie_handle = &(m_vMovieInfo[file]);
			m_vHandleBrowserList.push_back(movie_handle);
		}
	}
	// sort the not filtered files
	onSortMovieInfoHandleList(m_vHandleBrowserList,m_settings.sorting.item,MB_DIRECTION_AUTO);

	for(unsigned int handle=0; handle < m_vHandleBrowserList.size() ;handle++)
	{
		for(int row = 0; row < m_settings.browserRowNr ;row++)
		{
			if ( getMovieInfoItem(*m_vHandleBrowserList[handle], m_settings.browserRowItem[row], &string_item) == false)
			{
				string_item = "n/a";
				if(m_settings.browserRowItem[row] == MB_INFO_TITLE)
					getMovieInfoItem(*m_vHandleBrowserList[handle], MB_INFO_FILENAME, &string_item);
			}
			m_browserListLines.lineArray[row].push_back(string_item);
		}
	}
	m_pcBrowser->setLines(&m_browserListLines);

	m_currentBrowserSelection = m_pcBrowser->getSelectedLine();
	// update selected movie if browser is in the focus
	if (m_windowFocus == MB_FOCUS_BROWSER)
	{
		updateMovieSelection();	
	}
}

/************************************************************************

************************************************************************/
void CMovieBrowser::refreshBookmarkList(void) // P3
{
	//TRACE("[mb]->refreshBookmarkList \r\n");
}

/************************************************************************

************************************************************************/
void CMovieBrowser::refreshTitle(void) 
{
	if( m_pcWindow == NULL) return;
	//Paint Text Background
	//TRACE("[mb]->refreshTitle : %s\r\n",m_textTitle.c_str());
	m_pcWindow->paintBoxRel(	m_cBoxFrameTitleRel.iX, 
								m_cBoxFrameTitleRel.iY, 
								m_cBoxFrameTitleRel.iWidth, 
								m_cBoxFrameTitleRel.iHeight, 
								TITLE_BACKGROUND_COLOR);
									
	m_pcWindow->RenderString(	m_pcFontTitle,
								m_cBoxFrameTitleRel.iX + TEXT_BORDER_WIDTH, 
								m_cBoxFrameTitleRel.iY + m_cBoxFrameTitleRel.iHeight, 
								m_cBoxFrameTitleRel.iWidth - TEXT_BORDER_WIDTH<<1, 
								m_textTitle.c_str(), 
								TITLE_FONT_COLOR, 
								0, 
								true); // UTF-8
}

/************************************************************************

************************************************************************/
#define ADD_FOOT_HEIGHT 4
void CMovieBrowser::refreshFoot(void) 
{
	//TRACE("[mb]->refreshButtonLine \r\n");
	int	color   = (CFBWindow::color_t)COL_INFOBAR_SHADOW;
	int	bgcolor = (CFBWindow::color_t)COL_MENUHEAD_PLUS_0;

	std::string filter_text = g_Locale->getText(LOCALE_MOVIEBROWSER_FOOT_FILTER);
	filter_text += m_settings.filter.optionString;
	std::string sort_text = g_Locale->getText(LOCALE_MOVIEBROWSER_FOOT_SORT);
	sort_text += g_Locale->getText(m_localizedItemName[m_settings.sorting.item]);
	std::string ok_text = g_Locale->getText(LOCALE_MOVIEBROWSER_FOOT_PLAY);
	
	// draw the background first
	m_pcWindow->paintBoxRel(	m_cBoxFrameFootRel.iX, 
								m_cBoxFrameFootRel.iY, 
								m_cBoxFrameFootRel.iWidth, 
								m_cBoxFrameFootRel.iHeight,  
								(CFBWindow::color_t)COL_MENUHEAD_PLUS_0);

	int width = m_cBoxFrameFootRel.iWidth>>2;
	
	int xpos1 = m_cBoxFrameFootRel.iX;
	int width1 = width;
	int xpos2 = xpos1 + width1;
	int width2 = width + width;
	int xpos4 = xpos2 + width2;
	int width4 = width;
	//int xpos4 = xpos3 + width3;
	//int width4 = width;

	// draw Button blue (filter)
	//xpos += ButtonWidth + ButtonSpacing;
	// draw yellow (sort)
	if (m_settings.gui != MB_GUI_LAST_PLAY && m_settings.gui != MB_GUI_LAST_RECORD)
	{
		m_pcWindow->paintBoxRel(xpos1, m_cBoxFrameFootRel.iY + (ADD_FOOT_HEIGHT>>1), width1, m_cBoxFrameFootRel.iHeight + 4, (CFBWindow::color_t)bgcolor);
		m_pcWindow->paintIcon(NEUTRINO_ICON_BUTTON_RED, xpos1, m_cBoxFrameFootRel.iY + (ADD_FOOT_HEIGHT>>1));
		m_pcWindow->RenderString(m_pcFontFoot, xpos1 + 20, m_cBoxFrameFootRel.iY + m_cBoxFrameFootRel.iHeight + 4 , width1-30, sort_text.c_str(), (CFBWindow::color_t)color, 0, true); // UTF-8
	}

	if (m_settings.gui != MB_GUI_LAST_PLAY && m_settings.gui != MB_GUI_LAST_RECORD)
	{
		m_pcWindow->paintBoxRel(xpos2, m_cBoxFrameFootRel.iY + (ADD_FOOT_HEIGHT>>1), width2, m_cBoxFrameFootRel.iHeight + 4, (CFBWindow::color_t)bgcolor);
		m_pcWindow->paintIcon(NEUTRINO_ICON_BUTTON_GREEN, xpos2, m_cBoxFrameFootRel.iY + (ADD_FOOT_HEIGHT>>1));
		m_pcWindow->RenderString(m_pcFontFoot, xpos2 + 20, m_cBoxFrameFootRel.iY + m_cBoxFrameFootRel.iHeight + 4 , width2 -30, filter_text.c_str(), (CFBWindow::color_t)color, 0, true); // UTF-8
	}
	//xpos += ButtonWidth + ButtonSpacing;
	// draw 
	if(1)
	{
		std::string ok_text;
		if(m_settings.gui == MB_GUI_FILTER && m_windowFocus == MB_FOCUS_FILTER)
		{
			ok_text = "select";
		}
		else
		{
			ok_text = g_Locale->getText(LOCALE_MOVIEBROWSER_FOOT_PLAY);
		}
		m_pcWindow->paintBoxRel(xpos4, m_cBoxFrameFootRel.iY + (ADD_FOOT_HEIGHT>>1), width4, m_cBoxFrameFootRel.iHeight + 4, (CFBWindow::color_t)bgcolor);
		m_pcWindow->paintIcon(NEUTRINO_ICON_BUTTON_OKAY, xpos4, m_cBoxFrameFootRel.iY + (ADD_FOOT_HEIGHT>>1));
		m_pcWindow->RenderString(m_pcFontFoot, xpos4 + 30, m_cBoxFrameFootRel.iY + m_cBoxFrameFootRel.iHeight + 4 , width4-30, ok_text.c_str(), (CFBWindow::color_t)color, 0, true); // UTF-8
	}	
}

/************************************************************************

************************************************************************/
bool CMovieBrowser::onButtonPress(neutrino_msg_t msg)
{
//	TRACE("[mb]->onButtonPress %d\r\n",msg);
	bool result = false;
	
	result = onButtonPressMainFrame(msg);
	if(result == false)
	{
		// if Main Frame didnot process the button, the focused window may do
		switch(m_windowFocus)
		{
			case MB_FOCUS_BROWSER:
			 	result = onButtonPressBrowserList(msg);		
				break;
			case MB_FOCUS_LAST_PLAY:
			 	result = onButtonPressLastPlayList(msg);		
				break;
			case MB_FOCUS_LAST_RECORD:
			 	result = onButtonPressLastRecordList(msg);		
				break;
			case MB_FOCUS_MOVIE_INFO:
			 	result = onButtonPressMovieInfoList(msg);		
				break;
			case MB_FOCUS_FILTER:
			 	result = onButtonPressFilterList(msg);		
				break;
			default:
				break;
		}
	}
	return (result);
}

/************************************************************************

************************************************************************/
bool CMovieBrowser::onButtonPressMainFrame(neutrino_msg_t msg)
{
	//TRACE("[mb]->onButtonPressMainFrame: %d\r\n",msg);
	bool result = true;

	if (msg == CRCInput::RC_home)
	{
		if(m_settings.gui == MB_GUI_FILTER)
			onSetGUIWindow(MB_GUI_MOVIE_INFO);
		else
			result = false;
	}
	else if (msg == CRCInput::RC_minus) 
	{
		onSetGUIWindowPrev();
		//refreshMovieInfo();
	}
	else if (msg == CRCInput::RC_plus) 
	{
		onSetGUIWindowNext();
		//refreshMovieInfo();
	}
	else if (msg == CRCInput::RC_green) 
	{
		if(m_settings.gui == MB_GUI_MOVIE_INFO)
			onSetGUIWindow(MB_GUI_FILTER);
		else if(m_settings.gui == MB_GUI_FILTER)
			onSetGUIWindow(MB_GUI_MOVIE_INFO);
		// no effect if gui is last play or record			
	}
	else if (msg == CRCInput::RC_yellow) 
	{
		onSetFocusNext();
	}
	else if (msg == CRCInput::RC_blue) 
	{
		loadMovies();
		refresh();
	}
	else if (msg == CRCInput::RC_red ) 
	{
		if(m_settings.gui != MB_GUI_LAST_PLAY && m_settings.gui != MB_GUI_LAST_RECORD)
		{
			// sorting is not avialable for last play and record
			do
			{
				if(m_settings.sorting.item + 1 >= MB_INFO_MAX_NUMBER)
					m_settings.sorting.item = (MB_INFO_ITEM)0;
				else
					m_settings.sorting.item = (MB_INFO_ITEM)(m_settings.sorting.item + 1);
			}while(sortBy[m_settings.sorting.item] == NULL);
					
			TRACE("[mb]->new sorting %d,%s\r\n",m_settings.sorting.item,g_Locale->getText(m_localizedItemName[m_settings.sorting.item]));
			refreshBrowserList();	
			refreshFoot();
		}
	}
	else if (msg == CRCInput::RC_spkr) 
	{
		if(m_vMovieInfo.size() > 0)
		{	
			if(m_movieSelectionHandler != NULL)
			{
			 	onDeleteFile(*m_movieSelectionHandler);
			}
		}
	}
	else if (msg == CRCInput::RC_help) 
	{
		if(m_movieSelectionHandler != NULL)
		{
			m_movieInfo.showMovieInfo(*m_movieSelectionHandler);
			//m_movieInfo.printDebugMovieInfo(*m_movieSelectionHandler);
		}
	}
	else if (msg == CRCInput::RC_setup) 
	{
		if(m_movieSelectionHandler != NULL);
			showMenu(m_movieSelectionHandler);
	}
	else
	{
		//TRACE("[mb]->onButtonPressMainFrame none\r\n");
		result = false;
	}

	return (result);
}
/************************************************************************

************************************************************************/
bool CMovieBrowser::onButtonPressBrowserList(neutrino_msg_t msg) 
{
	//TRACE("[mb]->onButtonPressBrowserList %d\r\n",msg);
	bool result = true;
	
	if(msg==CRCInput::RC_up)
	{
		m_pcBrowser->scrollLineUp(1);
	}
	else if (msg == CRCInput::RC_down)
	{
		m_pcBrowser->scrollLineDown(1);
	}
	else if (msg == CRCInput::RC_page_up)
	{
		m_pcBrowser->scrollPageUp(1);
	}
	else if (msg == CRCInput::RC_page_down)
	{
		m_pcBrowser->scrollPageDown(1);
	}
	else if (msg == CRCInput::RC_left)
	{
		m_pcBrowser->scrollPageUp(1);
	}
	else if (msg == CRCInput::RC_right)
	{
		m_pcBrowser->scrollPageDown(1);
	}
	else
	{
		// default
		result = false;
	}
	if(result == true)
		updateMovieSelection();

	return (result);
}

/************************************************************************

************************************************************************/
bool CMovieBrowser::onButtonPressLastPlayList(neutrino_msg_t msg) 
{
	//TRACE("[mb]->onButtonPressLastPlayList %d\r\n",msg);
	bool result = true;
	
	if(msg==CRCInput::RC_up)
	{
		m_pcLastPlay->scrollLineUp(1);
	}
	else if (msg == CRCInput::RC_down)
	{
		m_pcLastPlay->scrollLineDown(1);
	}
	else if (msg == CRCInput::RC_left)
	{
		m_pcLastPlay->scrollPageUp(1);
	}
	else if (msg == CRCInput::RC_right)
	{
		m_pcLastPlay->scrollPageDown(1);
	}
	else
	{
		// default
		result = false;
	}
	if(result == true)
		updateMovieSelection();

	return (result);
}

/************************************************************************

************************************************************************/
bool CMovieBrowser::onButtonPressLastRecordList(neutrino_msg_t msg) 
{
	//TRACE("[mb]->onButtonPressLastRecordList %d\r\n",msg);
	bool result = true;
	
	if(msg==CRCInput::RC_up)
	{
		m_pcLastRecord->scrollLineUp(1);
	}
	else if (msg == CRCInput::RC_down)
	{
		m_pcLastRecord->scrollLineDown(1);
	}
	else if (msg == CRCInput::RC_left)
	{
		m_pcLastRecord->scrollPageUp(1);
	}
	else if (msg == CRCInput::RC_right)
	{
		m_pcLastRecord->scrollPageDown(1);
	}
	else
	{
		// default
		result = false;
	}

	if(result == true)
		updateMovieSelection();

	return (result);
}

/************************************************************************

************************************************************************/
bool CMovieBrowser::onButtonPressBookmarkList(neutrino_msg_t msg) 
{
	//TRACE("[mb]->onButtonPressBookmarkList %d\r\n",msg);
	bool result = true;
	
	result = false;
	return (result);
}
/************************************************************************

************************************************************************/
bool CMovieBrowser::onButtonPressFilterList(neutrino_msg_t msg) 
{
	//TRACE("[mb]->onButtonPressFilterList %d,%d\r\n",msg,m_settings.filter.item);
	bool result = true;

	if(msg==CRCInput::RC_up)
	{
		m_pcFilter->scrollLineUp(1);
	}
	else if (msg == CRCInput::RC_down)
	{
		m_pcFilter->scrollLineDown(1);
	}
	else if (msg == CRCInput::RC_page_up)
	{
		m_pcFilter->scrollPageUp(1);
	}
	else if (msg == CRCInput::RC_page_down)
	{
		m_pcFilter->scrollPageDown(1);
	}
	else if (msg == CRCInput::RC_ok)
	{
		int selected_line = m_pcFilter->getSelectedLine();
		if(m_settings.filter.item == MB_INFO_MAX_NUMBER)
		{
			if(selected_line == 0) m_settings.filter.item = MB_INFO_MAJOR_GENRE;
			if(selected_line == 1) m_settings.filter.item = MB_INFO_INFO1;
			if(selected_line == 2) m_settings.filter.item = MB_INFO_FILEPATH;
			if(selected_line == 3) m_settings.filter.item = MB_INFO_SERIE;
			refreshFilterList();
			m_pcFilter->setSelectedLine(0);
		}
		else
		{
			if(selected_line == 0)
			{
				m_settings.filter.item = MB_INFO_MAX_NUMBER;
				m_settings.filter.optionString = "";
				m_settings.filter.optionVar = 0;
				refreshFilterList();
				m_pcFilter->setSelectedLine(0);
				refreshBrowserList();	
				refreshLastPlayList();	
				refreshLastRecordList();	
				refreshFoot();
			}
			else
			{
				updateFilterSelection();
			}
		}
	}
	else if (msg == CRCInput::RC_left)
	{
		m_pcFilter->scrollPageUp(1);
	}
	else if (msg == CRCInput::RC_right)
	{
		m_pcFilter->scrollPageDown(1);
	}
	else
	{
		// default
		result = false;
	}
	
	return (result);
}

/************************************************************************

************************************************************************/
bool CMovieBrowser::onButtonPressMovieInfoList(neutrino_msg_t msg) 
{
//	TRACE("[mb]->onButtonPressEPGInfoList %d\r\n",msg);
	bool result = true;
	
	if(msg == CRCInput::RC_up)
	{
		m_pcInfo->scrollPageUp(1);
	}
	else if (msg == CRCInput::RC_down)
	{
		m_pcInfo->scrollPageDown(1);
	}
	else
	{
		// default
		result = false;
	}	

	return (result);
}
/************************************************************************

************************************************************************/
void CMovieBrowser::onDeleteFile(MI_MOVIE_INFO& movieSelectionHandler)
{
	//TRACE( "[onDeleteFile] ");
	int test= movieSelectionHandler.file.Name.find(".ts");
	if(test == -1) 
	{ 
		// not a TS file, return!!!!! 
		TRACE( "show_ts_info: not a TS file ");
	}
	else
	{
		std::string msg = g_Locale->getText(LOCALE_FILEBROWSER_DODELETE1);
		msg += "\r\n ";
		if (movieSelectionHandler.file.Name.length() > 40)
		{
			msg += movieSelectionHandler.file.Name.substr(0,40);
			msg += "...";
		}
		else
			msg += movieSelectionHandler.file.Name;
			
		msg += "\r\n ";
		msg += g_Locale->getText(LOCALE_FILEBROWSER_DODELETE2);
		if (ShowMsgUTF(LOCALE_FILEBROWSER_DELETE, msg, CMessageBox::mbrNo, CMessageBox::mbYes|CMessageBox::mbNo)==CMessageBox::mbrYes)
		{
			delFile(movieSelectionHandler.file);
			
			CFile file_xml  = movieSelectionHandler.file; 
			if(m_movieInfo.convertTs2XmlName(&file_xml.Name) == true)  
			{
				delFile(file_xml);
	    	}
	    	
	    	m_vMovieInfo.erase( (std::vector<MI_MOVIE_INFO>::iterator)&movieSelectionHandler);
			updateSerienames();
			refreshBrowserList();
			refreshLastPlayList();	
			refreshLastRecordList();	
	    		
			//loadMovies(); // //TODO we might remove the handle from the handle list only, to avoid reload .....
			refresh();
		}
    } 
}

/************************************************************************

************************************************************************/
void CMovieBrowser::onSetGUIWindow(MB_GUI gui)
{
	m_settings.gui = gui;
	
	if(gui == MB_GUI_MOVIE_INFO)
	{
		TRACE("[mb] browser info\r\n");
		// Paint these frames ...
		m_showMovieInfo = true;
		m_showBrowserFiles = true;

		// ... and hide these frames
		m_showLastRecordFiles = false;
		m_showLastPlayFiles = false;
		m_showFilter = false;

		m_pcLastPlay->hide();
		m_pcLastRecord->hide();
		m_pcFilter->hide();
		m_pcBrowser->paint();
		onSetFocus(MB_FOCUS_BROWSER);
		refreshMovieInfo();
		m_pcInfo->paint();
	}
	else if(gui == MB_GUI_LAST_PLAY)
	{
		TRACE("[mb] last play \r\n");
		// Paint these frames ...
		m_showLastRecordFiles = true;
		m_showLastPlayFiles = true;
		m_showMovieInfo = true;

		// ... and hide these frames
		m_showBrowserFiles = false;
		m_showFilter = false;

		m_pcBrowser->hide();
		m_pcFilter->hide();
		m_pcLastRecord->paint();
		m_pcLastPlay->paint();

		onSetFocus(MB_FOCUS_LAST_PLAY);
		refreshMovieInfo();
		m_pcInfo->paint();
	}
	else if(gui == MB_GUI_LAST_RECORD)
	{
		TRACE("[mb] last record \r\n");
		// Paint these frames ...
		m_showLastRecordFiles = true;
		m_showLastPlayFiles = true;
		m_showMovieInfo = true;

		// ... and hide these frames
		m_showBrowserFiles = false;
		m_showFilter = false;

		m_pcBrowser->hide();
		m_pcFilter->hide();
		m_pcLastRecord->paint();
		m_pcLastPlay->paint();

		onSetFocus(MB_FOCUS_LAST_RECORD);
		refreshMovieInfo();
		m_pcInfo->paint();
	}
 	else if(gui == MB_GUI_FILTER)
	{
		TRACE("[mb] filter \r\n");
		// Paint these frames ...
		m_showFilter = true;
		// ... and hide these frames
		m_showMovieInfo = false;
		m_pcInfo->hide();
		m_pcFilter->paint();
		
		onSetFocus(MB_FOCUS_FILTER);
	}
}

/************************************************************************

************************************************************************/
void CMovieBrowser::onSetGUIWindowNext(void) 
{
	if(m_settings.gui == MB_GUI_MOVIE_INFO )
	{
		onSetGUIWindow(MB_GUI_LAST_PLAY);
	}
	else if(m_settings.gui == MB_GUI_LAST_PLAY)
	{
		onSetGUIWindow(MB_GUI_LAST_RECORD);
	}
	else
	{
		onSetGUIWindow(MB_GUI_MOVIE_INFO);
	}
}
/************************************************************************

************************************************************************/
void CMovieBrowser::onSetGUIWindowPrev(void) 
{
	if(m_settings.gui == MB_GUI_MOVIE_INFO )
	{
		onSetGUIWindow(MB_GUI_LAST_RECORD);
	}
	else if(m_settings.gui == MB_GUI_LAST_RECORD)
	{
		onSetGUIWindow(MB_GUI_LAST_PLAY);
	}
	else
	{
		onSetGUIWindow(MB_GUI_MOVIE_INFO);
	}
}

/************************************************************************

************************************************************************/
void CMovieBrowser::onSetFocus(MB_FOCUS new_focus)
{
	//TRACE("[mb]->onSetFocus %d \r\n",new_focus);
	m_windowFocus = new_focus;
	if(m_windowFocus == MB_FOCUS_BROWSER)
	{
			m_pcBrowser->showSelection(true);
			m_pcLastRecord->showSelection(false);
			m_pcLastPlay->showSelection(false);
			m_pcFilter->showSelection(false);
			//m_pcInfo->showSelection(false);
	}
	else if(m_windowFocus == MB_FOCUS_LAST_PLAY)
	{
			m_pcBrowser->showSelection(false);
			m_pcLastRecord->showSelection(false);
			m_pcLastPlay->showSelection(true);
			m_pcFilter->showSelection(false);
			//m_pcInfo->showSelection(false);
	}
	else if(m_windowFocus == MB_FOCUS_LAST_RECORD)
	{
			m_pcBrowser->showSelection(false);
			m_pcLastRecord->showSelection(true);
			m_pcLastPlay->showSelection(false);
			m_pcFilter->showSelection(false);
			//m_pcInfo->showSelection(false);
	}
	else if(m_windowFocus == MB_FOCUS_MOVIE_INFO)
	{
			m_pcBrowser->showSelection(false);
			m_pcLastRecord->showSelection(false);
			m_pcLastPlay->showSelection(false);
			m_pcFilter->showSelection(false);
			//m_pcInfo->showSelection(true);
	}
	else if(m_windowFocus == MB_FOCUS_FILTER)
	{
			m_pcBrowser->showSelection(false);
			m_pcLastRecord->showSelection(false);
			m_pcLastPlay->showSelection(false);
			m_pcFilter->showSelection(true);
			//m_pcInfo->showSelection(false);
	}
	updateMovieSelection();
	refreshFoot();
}
/************************************************************************

************************************************************************/
void CMovieBrowser::onSetFocusNext(void) 
{
	//TRACE("[mb]->onSetFocusNext \r\n");
	
	if(m_settings.gui == MB_GUI_FILTER)
	{
		if(m_windowFocus == MB_FOCUS_BROWSER)
		{
			TRACE("[mb] MB_FOCUS_FILTER\r\n");
			onSetFocus(MB_FOCUS_FILTER);
		}
		else
		{
			TRACE("[mb] MB_FOCUS_BROWSER\r\n");
			onSetFocus(MB_FOCUS_BROWSER);
		}
	}
	else if(m_settings.gui == MB_GUI_MOVIE_INFO)
	{
		if(m_windowFocus == MB_FOCUS_BROWSER)
		{
			TRACE("[mb] MB_FOCUS_MOVIE_INFO\r\n");
			onSetFocus(MB_FOCUS_MOVIE_INFO);
			m_windowFocus = MB_FOCUS_MOVIE_INFO;
		}
		else
		{
			TRACE("[mb] MB_FOCUS_BROWSER\r\n");
			onSetFocus(MB_FOCUS_BROWSER);
		}
	}
	else if(m_settings.gui == MB_GUI_LAST_PLAY)
	{
		if(m_windowFocus == MB_FOCUS_MOVIE_INFO)
		{
			onSetFocus(MB_FOCUS_LAST_PLAY);
		}
		else if(m_windowFocus == MB_FOCUS_LAST_PLAY)
		{
			onSetFocus(MB_FOCUS_MOVIE_INFO);
		}
	}
	else if(m_settings.gui == MB_GUI_LAST_RECORD)
	{
		if(m_windowFocus == MB_FOCUS_MOVIE_INFO)
		{
			onSetFocus(MB_FOCUS_LAST_RECORD);
		}
		else if(m_windowFocus == MB_FOCUS_LAST_RECORD)
		{
			onSetFocus(MB_FOCUS_MOVIE_INFO);
		}
	}
}

/************************************************************************

************************************************************************/
bool CMovieBrowser::onSortMovieInfoHandleList(std::vector<MI_MOVIE_INFO*>& handle_list, MB_INFO_ITEM sort_item, MB_DIRECTION direction)
{
	//TRACE("sort: %d\r\n",direction);
	if(handle_list.size() <= 0) 
	    return (false); // nothing to sort, return immedately
	if(sortBy[sort_item] == NULL) 
	    return (false);
	
	if(direction == MB_DIRECTION_AUTO)
	{
		if( sort_item == MB_INFO_QUALITY || 
			sort_item == MB_INFO_PARENTAL_LOCKAGE || 
			sort_item == MB_INFO_PREVPLAYDATE || 
			sort_item == MB_INFO_RECORDDATE || 
			sort_item == MB_INFO_PRODDATE ||
			sort_item == MB_INFO_SIZE)
	 	{
			sortDirection = 1;
		}
		else
		{
			sortDirection = 0;
		}
	}
	else if(direction == MB_DIRECTION_UP)
	{
		sortDirection = 0;
	}
	else
	{
		sortDirection = 1;
	}
	
	//TRACE("sort: %d\r\n",sortDirection);
	sort(handle_list.begin(), handle_list.end(), sortBy[sort_item]);
	
	return (true);
}


/************************************************************************

************************************************************************/
void CMovieBrowser::loadAllTsFileNamesFromStorage(void)
{
	//TRACE("[mb]->loadAllTsFileNamesFromStorage \r\n");
	bool result;
	int i,size; 
	
	m_movieSelectionHandler = NULL;
	m_dirNames.clear();
	m_vMovieInfo.clear();
	
	m_repositoryDir.clear();
	//addRepositoryDir(m_selectedDir);
	for(int i = 0; i < MB_MAX_DIRS; i++)
	{
		if(!m_settings.storageDir[i].empty())
			addRepositoryDir(m_settings.storageDir[i]);
	}
	// check if there is a movie dir and if we should use it
	if(g_settings.network_nfs_moviedir[0] != 0 && m_settings.storageDir_movie == true)
	{
		std::string name = g_settings.network_nfs_moviedir;
		addRepositoryDir(name);
	}
	// check if there is a record dir and if we should use it 
	if(g_settings.network_nfs_recordingdir[0] != 0 && m_settings.storageDir_rec == true)
	{
		std::string name = g_settings.network_nfs_recordingdir;
		addRepositoryDir(name);
	}

	size = m_repositoryDir.size();
	for(i=0; i < size;i++)
	{
		result = loadTsFileNamesFromDir(m_repositoryDir[i]);
	}
	
	TRACE("[mb] Dir%d, Files:%d \r\n",m_dirNames.size(),m_vMovieInfo.size());
	if(m_vMovieInfo.size() == 0)
	{
		std::string msg = g_Locale->getText(LOCALE_MOVIEBROWSER_ERROR_NO_MOVIES);
		DisplayErrorMessage(msg.c_str());
	}
}

/************************************************************************
Note: this function is used recursive, do not add any return within the body due to the recursive counter
************************************************************************/
bool CMovieBrowser::loadTsFileNamesFromDir(const std::string & dirname)
{
	//TRACE("[mb]->loadTsFileNamesFromDir %s\r\n",dirname.c_str());

	static int recursive_counter = 0; // recursive counter to be used to avoid hanging
	bool result = false;
	int file_found_in_dir = false;

	if (recursive_counter > 10)
	{
		TRACE("[mb]loadTsFileNamesFromDir: return->recoursive error\r\n"); 
		return (false); // do not go deeper than 10 directories
	}

	/* check if directory was already searched once */
	int size = m_dirNames.size();
	for(int i = 0; i < size; i++)
	{
		if(strcmp(m_dirNames[i].c_str(),dirname.c_str()) == 0)	
		{
			// string is identical to previous one
			TRACE("[mb]Dir already in list: %s\r\n",dirname.c_str()); 
			return (false); 
		}
	}
	/* !!!!!! no return statement within the body after here !!!!*/
	recursive_counter++;

	CFileList flist;
	if(readDir(dirname, &flist) == true)
	{
		MI_MOVIE_INFO movieInfo;
		m_movieInfo.clearMovieInfo(&movieInfo); // refresh structure
		for(unsigned int i = 0; i < flist.size(); i++)
		{
			if( S_ISDIR(flist[i].Mode)) 
			{
				flist[i].Name += '/';
				//TRACE("[mb] Dir: '%s'\r\n",movieInfo.file.Name.c_str());
				loadTsFileNamesFromDir(flist[i].Name);
			}
			else
			{
				int test=flist[i].getFileName().find(".ts");
				if( test == -1)
				{
					//TRACE("[mb] other file: '%s'\r\n",movieInfo.file.Name.c_str());
				}
				else
				{
					movieInfo.file.Name = flist[i].Name;
					movieInfo.file.Mode = flist[i].Mode;
					movieInfo.file.Size = flist[i].Size;
					movieInfo.file.Time = flist[i].Time;
					//TRACE(" N:%s,s:%d,t:%d\r\n",movieInfo.file.getFileName().c_str(),movieInfo.file.Size,movieInfo.file.Time);
					//TRACE(" N:%s,s:%d\r\n",movieInfo.file.getFileName().c_str(),movieInfo.file.Size>>20);
					//TRACE(" s:%d\r\n",movieInfo.file.getFileName().c_str(),movieInfo.file.Size);
					//TRACE(" s:%llu\r\n",movieInfo.file.getFileName().c_str(),movieInfo.file.Size);
					if(file_found_in_dir == false)
					{
						// first file in directory found, add directory to list 
						m_dirNames.push_back(dirname);
						file_found_in_dir = true;
						//TRACE("[mb] new dir: :%s\r\n",dirname);
					}
					movieInfo.dirItNr = m_dirNames.size()-1;
					m_vMovieInfo.push_back(movieInfo);
				}
			}
		}
		result = true;
	}	
 	
	recursive_counter--;
	return (result);
}

/************************************************************************

************************************************************************/
bool CMovieBrowser::readDir(const std::string & dirname, CFileList* flist)
{
	bool result = false;
	if (strncmp(dirname.c_str(), VLC_URI, strlen(VLC_URI)) == 0)
	{
		result = readDir_vlc(dirname, flist);
	}
	else
	{
		result = readDir_std(dirname, flist);
	}
	return(result);
}
/************************************************************************

************************************************************************/
bool CMovieBrowser::readDir_vlc(const std::string & dirname, CFileList* flist)
{
	return false;
}
/************************************************************************

************************************************************************/
bool CMovieBrowser::readDir_std(const std::string & dirname, CFileList* flist)
{
	bool result = true;
    //TRACE("readDir_std %s\n",dirname.c_str());
	stat_struct statbuf;
	dirent_struct **namelist;
	int n;

	n = my_scandir(dirname.c_str(), &namelist, 0, my_alphasort);
	if (n < 0)
	{
		perror(("[mb] scandir: "+dirname).c_str());
		return false;
	}
	CFile file;
	for(int i = 0; i < n;i++)
	{
		if(namelist[i]->d_name[0] != '.')
		{
			file.Name = dirname;
			file.Name += namelist[i]->d_name;

//			printf("file.Name: '%s', getFileName: '%s' getPath: '%s'\n",file.Name.c_str(),file.getFileName().c_str(),file.getPath().c_str());
			if(my_stat((file.Name).c_str(),&statbuf) != 0)
				perror("stat error");
			else
			{
				file.Mode = statbuf.st_mode;
				file.Time = statbuf.st_mtime;
				file.Size = statbuf.st_size;
				flist->push_back(file);
			}
		}
		free(namelist[i]);
	}

	free(namelist);

	return(result);
}

/************************************************************************

************************************************************************/
bool CMovieBrowser::delFile(CFile& file)
{
	bool result = false;
	//only std supported yet
	if(1)
	{
		result = delFile_std(file);
	}
	else
	{
		result = delFile_vlc(file);
	}
	return(result);
}
/************************************************************************

************************************************************************/
bool CMovieBrowser::delFile_vlc(CFile& file)
{
	bool result = false;
	return(result);
}
/************************************************************************

************************************************************************/
bool CMovieBrowser::delFile_std(CFile& file)
{
	bool result = true;
	unlink(file.Name.c_str()); // fix: use full path
	TRACE("  delete file: %s\r\n",file.Name.c_str());
	return(result);
}
/************************************************************************

************************************************************************/
void CMovieBrowser::updateMovieSelection(void)
{
	TRACE("[mb]->updateMovieSelection %d\r\n",m_windowFocus);
	if (m_vMovieInfo.size() == 0) return;
	bool new_selection = false;
	 
	unsigned int old_movie_selection;
	if(m_windowFocus == MB_FOCUS_BROWSER)
	{
		if(m_vHandleBrowserList.size() == 0)
		{
			// There are no elements in the Filebrowser, clear all handles
			m_currentBrowserSelection = 0;
			m_movieSelectionHandler = NULL;
			new_selection = true;
		}
		else
		{
			old_movie_selection = m_currentBrowserSelection;
			m_currentBrowserSelection = m_pcBrowser->getSelectedLine();
			//TRACE("    sel1:%d\r\n",m_currentBrowserSelection);
			if(m_currentBrowserSelection != old_movie_selection)
				new_selection = true;
			
			if(m_currentBrowserSelection < m_vHandleBrowserList.size())
				m_movieSelectionHandler = m_vHandleBrowserList[m_currentBrowserSelection];
		}
	}
	else if(m_windowFocus == MB_FOCUS_LAST_PLAY)
	{
		if(m_vHandlePlayList.size() == 0)
		{
			// There are no elements in the Filebrowser, clear all handles
			m_currentPlaySelection = 0;
			m_movieSelectionHandler = NULL;
			new_selection = true;
		}
		else
		{
			old_movie_selection = m_currentPlaySelection;
			m_currentPlaySelection = m_pcLastPlay->getSelectedLine();
			//TRACE("    sel2:%d\r\n",m_currentPlaySelection);
			if(m_currentPlaySelection != old_movie_selection)
				new_selection = true;
	
			 if(m_currentPlaySelection < m_vHandlePlayList.size())
				m_movieSelectionHandler = m_vHandlePlayList[m_currentPlaySelection];
		}
	}
	else if(m_windowFocus == MB_FOCUS_LAST_RECORD)
	{
		if(m_vHandleRecordList.size() == 0)
		{
			// There are no elements in the Filebrowser, clear all handles
			m_currentRecordSelection = 0;
			m_movieSelectionHandler = NULL;
			new_selection = true;
		}
		else
		{
			old_movie_selection = m_currentRecordSelection;
			m_currentRecordSelection = m_pcLastRecord->getSelectedLine();
			//TRACE("    sel3:%d\r\n",m_currentRecordSelection);
			if(m_currentRecordSelection != old_movie_selection)
				new_selection = true;
	
			if(m_currentRecordSelection < m_vHandleRecordList.size())
				m_movieSelectionHandler = m_vHandleRecordList[m_currentRecordSelection];
		}
	}	
	
	if(new_selection == true)
	{
		//TRACE("new\r\n");
		refreshMovieInfo();
		refreshLCD();
	}
	//TRACE("\r\n");
}

/************************************************************************

************************************************************************/
void CMovieBrowser::updateFilterSelection(void)
{
	//TRACE("[mb]->updateFilterSelection \r\n");
	if(m_FilterLines.lineArray[0].size() == 0) return;

	bool result = true;
	int selected_line = m_pcFilter->getSelectedLine();
	if(selected_line > 0)
		selected_line--;

	if(m_settings.filter.item == MB_INFO_FILEPATH)
	{
		m_settings.filter.optionString = m_FilterLines.lineArray[0][selected_line+1];
		m_settings.filter.optionVar = selected_line;
	}
	else if(m_settings.filter.item == MB_INFO_INFO1)
	{
		m_settings.filter.optionString = m_FilterLines.lineArray[0][selected_line+1];
	}
	else if(m_settings.filter.item == MB_INFO_MAJOR_GENRE)
	{
		m_settings.filter.optionString = g_Locale->getText(GENRE_ALL[selected_line].value);
		m_settings.filter.optionVar = GENRE_ALL[selected_line].key;
	}
	else if(m_settings.filter.item == MB_INFO_SERIE)
	{
		m_settings.filter.optionString = m_FilterLines.lineArray[0][selected_line+1];
	}
	else
	{
		result = false;
	}
	if(result == true)
	{
		refreshBrowserList();	
		refreshLastPlayList();	
		refreshLastRecordList();	
		refreshFoot();
	}
}

/************************************************************************

************************************************************************/

bool CMovieBrowser::addRepositoryDir(std::string& dirname)
{
	if(dirname.empty()) return false;
	if(dirname == "/") return false;
	
	std::string newpath = dirname;

	if(newpath.rfind('/') != newpath.length()-1 ||
      newpath.length() == 0 ||
		newpath == VLC_URI)
	{
		newpath += '/';
	}

	int size = m_repositoryDir.size();
	for(int i = 0; i < size; i++)
	{
		if(strcmp(m_repositoryDir[i].c_str(),newpath.c_str()) == 0)	
		{
			// string is identical to previous one
			TRACE("[mb] Dir already in list: %s\r\n",newpath.c_str()); 
			return (false); 
		}
	}
	TRACE("[mb] new Dir: %s\r\n",newpath.c_str()); 
	m_repositoryDir.push_back(newpath);
	m_file_info_stale = true; // we got a new Dir, search again for all movies next time
	m_seriename_stale = true;
	return (true);
}
/************************************************************************

************************************************************************/
void CMovieBrowser::loadMovies(void)
{
		time_t time_start = time(NULL);
		clock_t clock_start = clock()/10000; // CLOCKS_PER_SECOND
		clock_t clock_prev = clock_start;
		clock_t clock_act = clock_start;

		TRACE("[mb] loadMovies:  %ld,%ld\r\n",(long)clock(),(long)time_start);
		
		CHintBox loadBox(LOCALE_MOVIEBROWSER_HEAD,g_Locale->getText(LOCALE_MOVIEBROWSER_SCAN_FOR_MOVIES));
		loadBox.paint();

		loadAllTsFileNamesFromStorage(); // P1
		clock_act = clock()/10000;TRACE("[mb] *1:%3ld,%3ld*\r\n",clock_act - clock_prev, (long)time(NULL));clock_prev = clock_act;	
		loadAllMovieInfo(); // P1
		clock_act = clock()/10000;TRACE("[mb] *2:%3ld,%3ld*\r\n",clock_act - clock_prev, (long)time(NULL));clock_prev = clock_act;
		updateSerienames();
		m_file_info_stale = false;
		m_seriename_stale = true; // we reloded the movie info, so make sure the other list are  updated later on as well

		loadBox.hide();

		clock_act = clock()/10000;TRACE("[mb] *3:%3ld,%3ld*\r\n",clock_act - clock_prev, (long)time(NULL));clock_prev = clock_act;
		refreshBrowserList();	
		clock_act = clock()/10000;TRACE("[mb] *4:%3ld,%3ld*\r\n",clock_act - clock_prev, (long)time(NULL));clock_prev = clock_act;
		refreshLastPlayList();	
		refreshLastRecordList();
		refreshFilterList();
		refreshMovieInfo();	// is done by refreshBrowserList if needed
		TRACE("[mb] ***Total:%ld,%ld***\r\n",(time(NULL)-time_start), ((clock()/10000)-clock_start));
}
/************************************************************************

************************************************************************/
void CMovieBrowser::loadAllMovieInfo(void)
{
	//TRACE("[mb]->loadAllMovieInfo \r\n");

	for(unsigned int i=0; i < m_vMovieInfo.size();i++)
	{
		m_movieInfo.loadMovieInfo( &(m_vMovieInfo[i]));
	}
}


/************************************************************************

************************************************************************/
void CMovieBrowser::showHelp(void)
{
	CMovieHelp help;
	help.exec(NULL,NULL);
}

#define MAX_STRING 30
/************************************************************************

************************************************************************/
bool CMovieBrowser::showMenu(MI_MOVIE_INFO* movie_info)
{
	// very dirty usage of the menues, but it works and I already spent to much time with it, feel free to make it better ;-)

/********************************************************************/
/**  bookmark menu ******************************************************/
	CStringInputSMS* pBookNameInputSMS[MAX_NUMBER_OF_BOOKMARK_ITEMS];
	CStringInputSMS* pBookPosInputSMS[MAX_NUMBER_OF_BOOKMARK_ITEMS];
	CStringInputSMS* pBookTypeInputSMS[MAX_NUMBER_OF_BOOKMARK_ITEMS];
	CMenuWidget* pBookItemMenu[MAX_NUMBER_OF_BOOKMARK_ITEMS];

	char bookStart[100];
	char bookLast[100];
	char bookEnd[100];

	if(movie_info != NULL)
	{
		snprintf(bookStart, 20,"%5d", movie_info->bookmarks.start); 
		snprintf(bookLast, 20,"%5d", movie_info->bookmarks.lastPlayStop); 
		snprintf(bookEnd, 20,"%5d", movie_info->bookmarks.end); 
	}

	char bookName[MAX_NUMBER_OF_BOOKMARK_ITEMS][100];
	char bookPos[MAX_NUMBER_OF_BOOKMARK_ITEMS][100];
	char bookType[MAX_NUMBER_OF_BOOKMARK_ITEMS][100];

	if(movie_info != NULL)
	{
		for(int i =0 ; i < MI_MOVIE_BOOK_USER_MAX && i < MAX_NUMBER_OF_BOOKMARK_ITEMS; i++ )
		{
			strncpy(bookName[i], movie_info->bookmarks.user[i].name.c_str(),20);
			snprintf(bookPos[i], 20,"%5d", movie_info->bookmarks.user[i].pos);
			snprintf(bookType[i], 20,"%5d", movie_info->bookmarks.user[i].length);
		}	
	}
	
	CStringInputSMS bookStartInputSMS (LOCALE_MOVIEBROWSER_EDIT_BOOK, 	bookStart, 5, LOCALE_MOVIEBROWSER_EDIT_BOOK_POS_INFO1, LOCALE_MOVIEBROWSER_EDIT_BOOK_POS_INFO2, "0123456789 ");
	CStringInputSMS bookLastInputSMS (LOCALE_MOVIEBROWSER_EDIT_BOOK, 	bookLast, 5, LOCALE_MOVIEBROWSER_EDIT_BOOK_POS_INFO1, LOCALE_MOVIEBROWSER_EDIT_BOOK_POS_INFO2, "0123456789 ");
	CStringInputSMS bookEndInputSMS (LOCALE_MOVIEBROWSER_EDIT_BOOK, 	bookEnd, 5, LOCALE_MOVIEBROWSER_EDIT_BOOK_POS_INFO1, LOCALE_MOVIEBROWSER_EDIT_BOOK_POS_INFO2, "0123456789 ");

	CMenuWidget bookmarkMenu (LOCALE_MOVIEBROWSER_BOOK_HEAD , "streaming.raw");
			
	bookmarkMenu.addItem(GenericMenuSeparator);
	bookmarkMenu.addItem( new CMenuForwarder(LOCALE_MOVIEBROWSER_BOOK_MOVIESTART, 	true, bookStart,&bookStartInputSMS));
	bookmarkMenu.addItem( new CMenuForwarder(LOCALE_MOVIEBROWSER_BOOK_MOVIEEND, 	true, bookEnd,	&bookEndInputSMS));
	bookmarkMenu.addItem( new CMenuForwarder(LOCALE_MOVIEBROWSER_BOOK_LASTMOVIESTOP, true, bookLast,	&bookLastInputSMS));
	bookmarkMenu.addItem(GenericMenuSeparatorLine);

	for(int i =0 ; i < MI_MOVIE_BOOK_USER_MAX && i < MAX_NUMBER_OF_BOOKMARK_ITEMS; i++ )
	{
		pBookNameInputSMS[i] = new CStringInputSMS (LOCALE_MOVIEBROWSER_EDIT_BOOK, bookName[i], 20, LOCALE_MOVIEBROWSER_EDIT_BOOK_NAME_INFO1, LOCALE_MOVIEBROWSER_EDIT_BOOK_NAME_INFO2, "abcdefghijklmnopqrstuvwxyz0123456789-.: ");
		pBookPosInputSMS[i] =  new CStringInputSMS (LOCALE_MOVIEBROWSER_EDIT_BOOK, bookPos[i], 20, LOCALE_MOVIEBROWSER_EDIT_BOOK_POS_INFO1, LOCALE_MOVIEBROWSER_EDIT_BOOK_POS_INFO2, "0123456789 ");
		pBookTypeInputSMS[i] = new CStringInputSMS (LOCALE_MOVIEBROWSER_EDIT_BOOK, bookType[i], 20, LOCALE_MOVIEBROWSER_EDIT_BOOK_TYPE_INFO1, LOCALE_MOVIEBROWSER_EDIT_BOOK_TYPE_INFO2, "0123456789 ");

		pBookItemMenu[i] = new CMenuWidget(LOCALE_MOVIEBROWSER_BOOK_HEAD, "streaming.raw");
		pBookItemMenu[i]->addItem(GenericMenuSeparator);
		pBookItemMenu[i]->addItem( new CMenuForwarder(LOCALE_MOVIEBROWSER_BOOK_NAME, true, 		bookName[i],pBookNameInputSMS[i]));
		pBookItemMenu[i]->addItem( new CMenuForwarder(LOCALE_MOVIEBROWSER_BOOK_POSITION, true, 	bookPos[i], pBookPosInputSMS[i]));
		pBookItemMenu[i]->addItem( new CMenuForwarder(LOCALE_MOVIEBROWSER_BOOK_TYPE, true, bookType[i],pBookTypeInputSMS[i]));

		bookmarkMenu.addItem( new CMenuForwarderNonLocalized (bookName[i], 	true, bookPos[i],pBookItemMenu[i]));
	}

/********************************************************************/
/**  serie menu ******************************************************/
	char serieName[100];
	if(movie_info != NULL)
	{
		strncpy(serieName,movie_info->serieName.c_str(),100);
	}
	CStringInputSMS serieUserInputSMS(LOCALE_MOVIEBROWSER_EDIT_SERIE, serieName, 20, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "abcdefghijklmnopqrstuvwxyz0123456789-.: ");

	CMenuWidget serieMenu (LOCALE_MOVIEBROWSER_SERIE_HEAD, "streaming.raw");
	serieMenu.addItem(GenericMenuSeparator);
	serieMenu.addItem( new CMenuForwarder(LOCALE_MOVIEBROWSER_SERIE_NAME, 	true, serieName, 		&serieUserInputSMS));
	serieMenu.addItem(GenericMenuSeparatorLine);
	for(unsigned int i=0; i < m_serienames.size(); i++)
		serieMenu.addItem( new CMenuSelector(m_serienames[i].c_str(), true,  serieName));

/********************************************************************/
/**  movieInfo menu ******************************************************/
#define BUFFER_SIZE 100
	char epgTitle[BUFFER_SIZE];			
	char epgInfo1[BUFFER_SIZE]; 		
	char length[BUFFER_SIZE]; 			
	char country[BUFFER_SIZE]; 			
	char year[BUFFER_SIZE]; 			
	char epgChannel[BUFFER_SIZE]; 		
	char fileName[BUFFER_SIZE]; 		
	char dirItNr[BUFFER_SIZE]; 			
	char playDate[BUFFER_SIZE];			
	char recordDate[BUFFER_SIZE];		
	char size[BUFFER_SIZE];				
	int quality = 0;
	int parental_lockage_option  = 0;
	int genre = 0; 

	if(movie_info != NULL)
	{
		strncpy(epgTitle, movie_info->epgTitle.c_str(),BUFFER_SIZE);
		strncpy(epgInfo1,movie_info->epgInfo1.c_str(),BUFFER_SIZE);
		snprintf(length, BUFFER_SIZE,"%3d",movie_info->length); 
		strncpy(country,movie_info->productionCountry.c_str(),BUFFER_SIZE);
		snprintf(year, BUFFER_SIZE,"%4d",movie_info->productionDate + 1900); 
		strncpy(epgChannel,movie_info->epgChannel.c_str(),BUFFER_SIZE);
		strncpy(fileName, movie_info->file.getFileName().c_str(),BUFFER_SIZE);
		strncpy(dirItNr, m_dirNames[movie_info->dirItNr].c_str(),BUFFER_SIZE);
			tm* date = localtime(&movie_info->dateOfLastPlay);
		snprintf(playDate, BUFFER_SIZE,"%02d.%02d.%02d",date->tm_mday,date->tm_mon+1,date->tm_year >= 100 ? date->tm_year-100 : date->tm_year);
			date = localtime(&movie_info->file.Time);
		snprintf(recordDate, BUFFER_SIZE,"%02d.%02d.%02d",date->tm_mday,date->tm_mon+1,date->tm_year >= 100 ? date->tm_year-100 : date->tm_year);
		snprintf(size,BUFFER_SIZE,"%5llu",movie_info->file.Size>>20);
		quality = movie_info->quality;
		parental_lockage_option  = movie_info->parentalLockAge;
		genre = movie_info->genreMajor; 
	}

	CStringInputSMS titelUserInputSMS(LOCALE_MOVIEBROWSER_INFO_TITLE, epgTitle, MAX_STRING, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "abcdefghijklmnopqrstuvwxyz0123456789-.: ");
	CStringInputSMS channelUserInputSMS(LOCALE_MOVIEBROWSER_INFO_CHANNEL, epgChannel, 15, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "abcdefghijklmnopqrstuvwxyz0123456789-.: ");
	CStringInputSMS epgUserInputSMS(LOCALE_MOVIEBROWSER_INFO_INFO1, epgInfo1, 20, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "abcdefghijklmnopqrstuvwxyz0123456789-.: ");
	CStringInput lengthUserInput(LOCALE_MOVIEBROWSER_INFO_LENGTH, length, 3, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "0123456789 ");
	CStringInput countryUserInput(LOCALE_MOVIEBROWSER_INFO_PRODCOUNTRY, country, 11, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "ABCDEFGHIJKLMNOPQRSTUVWXYZ ");
	CStringInput yearUserInput(LOCALE_MOVIEBROWSER_INFO_PRODYEAR, year, 4, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "0123456789 ");

	CMenuWidget movieInfoMenu (LOCALE_MOVIEBROWSER_INFO_HEAD, "streaming.raw",m_cBoxFrame.iWidth);

	movieInfoMenu.addItem(GenericMenuSeparator);
	movieInfoMenu.addItem(GenericMenuSeparatorLine);
	movieInfoMenu.addItem( new CMenuForwarder(LOCALE_MOVIEBROWSER_INFO_TITLE, 	true, epgTitle, 	&titelUserInputSMS));
	movieInfoMenu.addItem( new CMenuForwarder(LOCALE_MOVIEBROWSER_INFO_SERIE, 		true, serieName,	&serieMenu));
	movieInfoMenu.addItem( new CMenuForwarder(LOCALE_MOVIEBROWSER_INFO_INFO1, 	true, epgInfo1,		&epgUserInputSMS));
	movieInfoMenu.addItem( new CMenuOptionChooser(LOCALE_MOVIEBROWSER_INFO_GENRE_MAJOR, &genre, GENRE_ALL, GENRE_ALL_COUNT, true ));
	movieInfoMenu.addItem( new CMenuForwarder(LOCALE_MOVIEBROWSER_INFO_CHANNEL, 		true, epgChannel,	&channelUserInputSMS));//LOCALE_TIMERLIST_CHANNEL
	movieInfoMenu.addItem(GenericMenuSeparatorLine);
	movieInfoMenu.addItem( new CMenuForwarder(LOCALE_MOVIEBROWSER_INFO_PRODYEAR, 	true, year, 		&yearUserInput));
	movieInfoMenu.addItem( new CMenuForwarder(LOCALE_MOVIEBROWSER_INFO_PRODCOUNTRY, 		true, country, 		&countryUserInput));
	movieInfoMenu.addItem( new CMenuOptionNumberChooser(LOCALE_MOVIEBROWSER_INFO_QUALITY,&quality,true,0,3));
	movieInfoMenu.addItem( new CMenuForwarder(LOCALE_MOVIEBROWSER_INFO_LENGTH, 		true, length, 		&lengthUserInput));
	movieInfoMenu.addItem( new CMenuOptionChooser(LOCALE_MOVIEBROWSER_INFO_PARENTAL_LOCKAGE, &parental_lockage_option, MESSAGEBOX_PARENTAL_LOCKAGE_OPTIONS, MESSAGEBOX_PARENTAL_LOCKAGE_OPTION_COUNT, true ));
	movieInfoMenu.addItem( new CMenuForwarder(LOCALE_MOVIEBROWSER_INFO_PREVPLAYDATE, 	false, playDate));//LOCALE_FLASHUPDATE_CURRENTVERSIONDATE
	movieInfoMenu.addItem(GenericMenuSeparatorLine);
	//movieInfoMenu.addItem( new CMenuOptionChooser(LOCALE_VIDEOMENU_VIDEOFORMAT, &video_format_option, VIDEOMENU_VIDEOFORMAT_OPTIONS, VIDEOMENU_VIDEOFORMAT_OPTION_COUNT, true ));
	movieInfoMenu.addItem( new CMenuForwarder(LOCALE_MOVIEBROWSER_INFO_PATH, 	false, dirItNr)); //LOCALE_TIMERLIST_RECORDING_DIR
	movieInfoMenu.addItem( new CMenuForwarder(LOCALE_MOVIEBROWSER_INFO_RECORDDATE, 	false, recordDate));//LOCALE_FLASHUPDATE_CURRENTVERSIONDATE
	movieInfoMenu.addItem( new CMenuForwarder(LOCALE_MOVIEBROWSER_INFO_SIZE, 		false, size,		NULL));

/********************************************************************/
/**  directory menu ******************************************************/
	unsigned int drivefree = 0;
	struct statfs s;

	CMenuWidget dirMenu (LOCALE_MOVIEBROWSER_MENU_DIRECTORIES_HEAD , "streaming.raw");
	dirMenu.addItem(GenericMenuSeparatorLine);
	char storage[10][20];
	if( m_repositoryDir.size() > 0 )
	{
		for(unsigned int i = 0; i < m_repositoryDir.size() && i < 10; i++)
		{
			if (statfs(m_repositoryDir[i].c_str(), &s) >= 0 )
			{
				drivefree = (s.f_bfree * s.f_bsize)>>30;
			}
			snprintf(storage[i], 19,"%3d GB free",drivefree);
			dirMenu.addItem( new CMenuForwarderNonLocalized	( m_repositoryDir[i].c_str(),  false, storage[i], 		NULL));
		}
	}
/********************************************************************/
/**  parental lock **************************************************/
	CMenuWidget parentalMenu (LOCALE_MOVIEBROWSER_MENU_PARENTAL_LOCK_HEAD , "streaming.raw");
	parentalMenu.addItem(GenericMenuSeparator);
	parentalMenu.addItem( new CMenuOptionChooser(LOCALE_MOVIEBROWSER_MENU_PARENTAL_LOCK_ACTIVATED, (int*)(&m_parentalLock), MESSAGEBOX_PARENTAL_LOCK_OPTIONS, MESSAGEBOX_PARENTAL_LOCK_OPTIONS_COUNT, true ));
	parentalMenu.addItem( new CMenuOptionChooser(LOCALE_MOVIEBROWSER_MENU_PARENTAL_LOCK_RATE_HEAD, (int*)(&m_settings.parentalLockAge), MESSAGEBOX_PARENTAL_LOCKAGE_OPTIONS, MESSAGEBOX_PARENTAL_LOCKAGE_OPTION_COUNT, true ));

/********************************************************************/
/**  main menu ******************************************************/
	CMenuWidget mainMenu(LOCALE_MOVIEBROWSER_MENU_MAIN_HEAD, "streaming.raw");
	mainMenu.addItem(GenericMenuSeparator);
	
	CSelectedMenu* cSelectedMenuSave = new CSelectedMenu();
	CSelectedMenu* cSelectedMenuReload = new CSelectedMenu();
	mainMenu.addItem( new CMenuForwarder(LOCALE_MOVIEBROWSER_MENU_MAIN_SAVEANDBACK, 	true,NULL,cSelectedMenuSave));
	mainMenu.addItem(GenericMenuSeparatorLine);
	mainMenu.addItem( new CMenuForwarder(LOCALE_MOVIEBROWSER_INFO_HEAD, (movie_info != NULL), NULL,		 &movieInfoMenu));
	mainMenu.addItem( new CMenuForwarder(LOCALE_MOVIEBROWSER_BOOK_HEAD, (movie_info != NULL), NULL,		 &bookmarkMenu));
	mainMenu.addItem(GenericMenuSeparatorLine);
	mainMenu.addItem( new CMenuForwarder(LOCALE_MOVIEBROWSER_MENU_DIRECTORIES_HEAD, 	true, NULL,		&dirMenu));
	mainMenu.addItem( new CMenuForwarder(LOCALE_MOVIEBROWSER_SCAN_FOR_MOVIES, 	true,NULL,cSelectedMenuReload));
	mainMenu.addItem(GenericMenuSeparatorLine);
	if(m_parentalLock != MB_PARENTAL_LOCK_OFF)
		mainMenu.addItem( new CLockedMenuForwarder(LOCALE_MOVIEBROWSER_MENU_PARENTAL_LOCK_HEAD, g_settings.parentallock_pincode, true, 	true, NULL,	&parentalMenu));
	else
		mainMenu.addItem( new CMenuForwarder(LOCALE_MOVIEBROWSER_MENU_PARENTAL_LOCK_HEAD, 	true, NULL,	&parentalMenu));
	CMovieHelp* movieHelp;
	CNFSSmallMenu* nfs;
	mainMenu.addItem( new CMenuForwarder(LOCALE_MOVIEBROWSER_MENU_NFS_HEAD, 		true, NULL,		nfs = new CNFSSmallMenu(), NULL, CRCInput::RC_setup, NEUTRINO_ICON_BUTTON_DBOX_SMALL));
	mainMenu.addItem( new CMenuForwarder(LOCALE_MOVIEBROWSER_MENU_HELP_HEAD,	true, NULL,		movieHelp = new CMovieHelp(), NULL, CRCInput::RC_setup, NEUTRINO_ICON_BUTTON_DBOX_SMALL));

	mainMenu.exec(NULL, "aabb");
	if(cSelectedMenuReload->selected)
	{
		loadMovies();
		refresh();
	}	
	
	if(cSelectedMenuSave->selected )
	{
		if(movie_info != NULL)
		{
			//TRACE(" save...\r\n");
			movie_info->epgTitle = epgTitle;			
			movie_info->epgInfo1 = epgInfo1; 		
			movie_info->genreMajor = genre; 			
			movie_info->serieName = serieName; 		
			movie_info->parentalLockAge = (MI_PARENTAL_LOCKAGE)parental_lockage_option; 			
			movie_info->length = atoi(length); 			
			movie_info->productionCountry = country; 			
			movie_info->productionDate = atoi(year )-1900; 			
			movie_info->epgChannel = epgChannel; 		
			movie_info->quality =  quality;	
			movie_info->bookmarks.end = atoi(bookEnd);
			movie_info->bookmarks.start = atoi(bookStart);
			movie_info->bookmarks.lastPlayStop = atoi(bookLast);
	
			for(int i =0 ; i < MI_MOVIE_BOOK_USER_MAX && i < MAX_NUMBER_OF_BOOKMARK_ITEMS; i++ )
			{
				movie_info->bookmarks.user[i].pos = 	atoi(bookPos[i]);
				if(movie_info->bookmarks.user[i].pos == 0)
				{
					// Bookmark cleared, clear other infos as well
					movie_info->bookmarks.user[i].length = 	0;
					movie_info->bookmarks.user[i].name = 	"";
				}
				else
				{
					movie_info->bookmarks.user[i].length = 	atoi(bookType[i]);
					movie_info->bookmarks.user[i].name = 	bookName[i];
				}
			}
			m_movieInfo.saveMovieInfo( *movie_info);
		}
		
		//save parental lock if not temporary 
		if (m_parentalLock != MB_PARENTAL_LOCK_OFF_TMP)
			m_settings.parentalLock = m_parentalLock;
	}

	updateSerienames();
	refreshBrowserList();
	refreshLastPlayList();	
	refreshLastRecordList();	
	refreshFilterList();	
	refresh();
	
	for(int i =0 ; i < MI_MOVIE_BOOK_USER_MAX && i < MAX_NUMBER_OF_BOOKMARK_ITEMS; i++ )
	{
		delete pBookNameInputSMS[i] ;
		delete pBookPosInputSMS[i] ;
		delete pBookTypeInputSMS[i];
		delete pBookItemMenu[i];
	}
	delete movieHelp;
	delete nfs;
	delete cSelectedMenuSave;
	delete cSelectedMenuReload;

	return(true);
}

/************************************************************************

************************************************************************/
int CMovieBrowser::showStartPosSelectionMenu(void) // P2
{
	//TRACE("[mb]->showStartPosSelectionMenu\r\n");
	int pos = -1;
	int result = 0;
	int menu_nr= 0;
	int position[MAX_NUMBER_OF_BOOKMARK_ITEMS];
	
	if(m_movieSelectionHandler == NULL) return(result);
	
	char start_pos[13]; snprintf(start_pos, 12,"%3d min",m_movieSelectionHandler->bookmarks.start/60); 
	char play_pos[13]; 	snprintf(play_pos, 12,"%3d min",m_movieSelectionHandler->bookmarks.lastPlayStop/60); 
	
	char book[MI_MOVIE_BOOK_USER_MAX][20];

	CMenuWidgetSelection startPosSelectionMenu(LOCALE_MOVIEBROWSER_START_HEAD , "streaming.raw");
			
	startPosSelectionMenu.addItem(GenericMenuSeparator);
	
	startPosSelectionMenu.addItem(new CMenuForwarder(LOCALE_MOVIEBROWSER_START_RECORD_START, true,NULL));
	position[menu_nr++] = 0;
	if( m_movieSelectionHandler->bookmarks.start != 0)
	{
		startPosSelectionMenu.addItem(new CMenuForwarder(LOCALE_MOVIEBROWSER_BOOK_MOVIESTART, true, start_pos));
		position[menu_nr++] = m_movieSelectionHandler->bookmarks.start;
	}
	if( m_movieSelectionHandler->bookmarks.lastPlayStop != 0) 
	{
		startPosSelectionMenu.addItem(new CMenuForwarder(LOCALE_MOVIEBROWSER_BOOK_LASTMOVIESTOP, true, play_pos));
		position[menu_nr++] = m_movieSelectionHandler->bookmarks.lastPlayStop;
	}
	startPosSelectionMenu.addItem(GenericMenuSeparatorLine);

	for(int i =0 ; i < MI_MOVIE_BOOK_USER_MAX && menu_nr < MAX_NUMBER_OF_BOOKMARK_ITEMS; i++ )
	{
		if( m_movieSelectionHandler->bookmarks.user[i].pos != 0 )
		{
			if(m_movieSelectionHandler->bookmarks.user[i].length >= 0)
				position[menu_nr] = m_movieSelectionHandler->bookmarks.user[i].pos;
			else
				position[menu_nr] = m_movieSelectionHandler->bookmarks.user[i].pos + m_movieSelectionHandler->bookmarks.user[i].length;
				
			snprintf(book[i], 19,"%5d min",position[menu_nr]/60);
			startPosSelectionMenu.addItem(new CMenuForwarderNonLocalized (m_movieSelectionHandler->bookmarks.user[i].name.c_str(), 	true, book[i]));
			menu_nr++;
		}
	}

	startPosSelectionMenu.exec(NULL, "12345");
	/* check what menu item was ok'd  and set the appropriate play offset*/
	//result = startPosSelectionMenu.getSelected();
	result = startPosSelectionMenu.getSelectedLine();
	if(result != 0 && result <= MAX_NUMBER_OF_BOOKMARK_ITEMS)
	{
		result--;
		if(result > 3) result--;
		pos = position[result];
	}
	
	TRACE("[mb] res = %d,%d \r\n",result,pos);
	
	return(pos) ;
}


/************************************************************************

************************************************************************/
bool CMovieBrowser::isParentalLock(MI_MOVIE_INFO& movie_info)
{
	bool result = false;
	if(m_parentalLock == MB_PARENTAL_LOCK_ACTIVE && m_settings.parentalLockAge <= movie_info.parentalLockAge )
	{
		result = true;
	}
	return (result);
}
/************************************************************************

************************************************************************/
bool CMovieBrowser::isFiltered(MI_MOVIE_INFO& movie_info)
{
	bool result = true;
	
	switch(m_settings.filter.item)
	{
		case MB_INFO_FILEPATH:
			if(m_settings.filter.optionVar == movie_info.dirItNr)
				result = false;
			break;
		case MB_INFO_INFO1:
			if(strcmp(m_settings.filter.optionString.c_str(),movie_info.epgInfo1.c_str()) == 0) 
				result = false;
			break;
		case MB_INFO_MAJOR_GENRE:
			if(m_settings.filter.optionVar == movie_info.genreMajor)
				result = false;
			break;
		case MB_INFO_SERIE:
			if(strcmp(m_settings.filter.optionString.c_str(),movie_info.serieName.c_str()) == 0) 
				result = false;
			break;
			break;
		default:
				result = false;
			break;
	}
	return (result);
}

/************************************************************************

************************************************************************/
bool CMovieBrowser::getMovieInfoItem(MI_MOVIE_INFO& movie_info, MB_INFO_ITEM item, std::string* item_string)
{
	#define MAX_STR_TMP 100
	char str_tmp[MAX_STR_TMP];
	bool result = true;
	*item_string="";
	tm* tm_tmp;
	
	char text[20];
	int i=0;
	int counter=0;
	int ac3_found = false;

	switch(item)
	{
		case MB_INFO_FILENAME: 				// 		= 0,
			*item_string = movie_info.file.getFileName();
			break;
		case MB_INFO_FILEPATH: 				// 		= 1,
			if(m_dirNames.size() > 0)
				*item_string = m_dirNames[movie_info.dirItNr];
			break;
		case MB_INFO_TITLE: 				// 		= 2,
			*item_string = movie_info.epgTitle;
			if(strcmp("not available",movie_info.epgTitle.c_str()) == 0)
				result = false;
			if(movie_info.epgTitle.empty())
				result = false;
			break;
		case MB_INFO_SERIE: 				// 		= 3,
			*item_string = movie_info.serieName;
			break;
		case MB_INFO_INFO1: 			//		= 4,
			*item_string = movie_info.epgInfo1;
			break;
		case MB_INFO_MAJOR_GENRE: 			// 		= 5,
			snprintf(str_tmp,MAX_STR_TMP,"%2d",movie_info.genreMajor);
			*item_string = str_tmp;
			break;
		case MB_INFO_MINOR_GENRE: 			// 		= 6,
			snprintf(str_tmp,MAX_STR_TMP,"%2d",movie_info.genreMinor);
			*item_string = str_tmp;
			break;
		case MB_INFO_INFO2: 					// 		= 7,
			*item_string = movie_info.epgInfo2;
			break;
		case MB_INFO_PARENTAL_LOCKAGE: 					// 		= 8,
			snprintf(str_tmp,MAX_STR_TMP,"%2d",movie_info.parentalLockAge);
			*item_string = str_tmp;
			break;
		case MB_INFO_CHANNEL: 				// 		= 9,
			*item_string = movie_info.epgChannel;
			break;
		case MB_INFO_BOOKMARK: 				//		= 10,
			// we just return the number of bookmarks
			for(i = 0; i < MI_MOVIE_BOOK_USER_MAX; i++)
			{
				if(movie_info.bookmarks.user[0].pos != 0) 
					counter++;
			}
			snprintf(text, 8,"%d",counter);
			text[9] = 0; // just to make sure string is terminated
			*item_string = text;
			break;
		case MB_INFO_QUALITY: 				// 		= 11,
			snprintf(str_tmp,MAX_STR_TMP,"%d",movie_info.quality);
			*item_string = str_tmp;
			break;
		case MB_INFO_PREVPLAYDATE: 			// 		= 12,
			tm_tmp = localtime(&movie_info.dateOfLastPlay);
			snprintf(str_tmp,MAX_STR_TMP,"%02d.%02d.%02d",tm_tmp->tm_mday,(tm_tmp->tm_mon)+ 1, tm_tmp->tm_year >= 100 ? tm_tmp->tm_year-100 : tm_tmp->tm_year);
			*item_string = str_tmp;
			break;
		case MB_INFO_RECORDDATE: 			// 		= 13,
			tm_tmp = localtime(&movie_info.file.Time);
			snprintf(str_tmp,MAX_STR_TMP,"%02d.%02d.%02d",tm_tmp->tm_mday,(tm_tmp->tm_mon) + 1,tm_tmp->tm_year >= 100 ? tm_tmp->tm_year-100 : tm_tmp->tm_year);
			*item_string = str_tmp;
			break;
		case MB_INFO_PRODDATE: 				// 		= 14,
			snprintf(str_tmp,MAX_STR_TMP,"%d",movie_info.productionDate);
			*item_string = str_tmp;
			break;
		case MB_INFO_COUNTRY: 				// 		= 15,
			*item_string = movie_info.productionCountry;
			break;
		case MB_INFO_GEOMETRIE: 			// 		= 16,
			result = false;
			break;
		case MB_INFO_AUDIO: 				// 		= 17,
#if 1  // MB_INFO_AUDIO test
			// we just return the number of audiopids
			char text[10];
			snprintf(text, 8,"%d",movie_info.audioPids.size());
			text[9] = 0; // just to make sure string is terminated
			*item_string = text;
#else // MB_INFO_AUDIO test
			for(i=0; i < movie_info.audioPids.size() && i < 10; i++)
			{
				if(movie_info.audioPids[i].epgAudioPidName[0].size() < 2)
				{
					text[counter++] = '?'; // two chars ??? -> strange name
					continue;
				}
				
				// check for Dolby Digital / AC3 Audio audiopids (less than 5.1 is not remarkable)
				if(	(movie_info.audioPids[i].epgAudioPidName.find("AC3") != -1 ) || 
					(movie_info.audioPids[i].epgAudioPidName.find("5.1") != -1 ))
				{
					ac3_found = true;
				}
				// Check for german audio pids
				if( movie_info.audioPids[i].epgAudioPidName[0] == 'D' || // Deutsch
					movie_info.audioPids[i].epgAudioPidName[0] == 'd' ||
					movie_info.audioPids[i].epgAudioPidName[0] == 'G' || // German
					movie_info.audioPids[i].epgAudioPidName[0] == 'g' ||
					movie_info.audioPids[i].epgAudioPidName[0] == 'M' || // for Mono, mono and Stereo, stereo we assume German ;)
					movie_info.audioPids[i].epgAudioPidName[0] == 'n' || 
					(movie_info.audioPids[i].epgAudioPidName[0] == 'S' && movie_info.audioPids[i].epgAudioPidName[1] == 't' ) || 
					(movie_info.audioPids[i].epgAudioPidName[0] == 's' && movie_info.audioPids[i].epgAudioPidName[1] == 't' ))
				{
					text[counter++] = 'D';
					continue;
				}
				// Check for english audio pids
				if( movie_info.audioPids[i].epgAudioPidName[0] == 'E' ||
					movie_info.audioPids[i].epgAudioPidName[0] == 'e')
				{
					text[counter++] = 'E';
					continue;
				}
				// Check for french audio pids
				if( movie_info.audioPids[i].epgAudioPidName[0] == 'F' ||
					movie_info.audioPids[i].epgAudioPidName[0] == 'f')
				{
					text[counter++] = 'F';
					continue;
				}
				// Check for italian audio pids
				if( movie_info.audioPids[i].epgAudioPidName[0] == 'I' ||
					movie_info.audioPids[i].epgAudioPidName[0] == 'i')
				{
					text[counter++] = 'I';
					continue;
				}
				// Check for spanish audio pids
				if( movie_info.audioPids[i].epgAudioPidName[0] == 'E' ||
					movie_info.audioPids[i].epgAudioPidName[0] == 'e' ||
					movie_info.audioPids[i].epgAudioPidName[0] == 'S' ||
					movie_info.audioPids[i].epgAudioPidName[0] == 's')
				{
					text[counter++] = 'S';
					continue;
				}
				text[counter++] = '?'; // We have not found any language for this pid
			}
			if(ac3_found == true)
			{
				text[counter++] = '5';
				text[counter++] = '.';
				text[counter++] = '1';
			}
			text[counter] = 0; // terminate string 
#endif	// MB_INFO_AUDIO test
			break;
		case MB_INFO_LENGTH: 				// 		= 18,
			snprintf(str_tmp,MAX_STR_TMP,"%4d",movie_info.length);
			*item_string = str_tmp;
			break;
		case MB_INFO_SIZE: 					// 		= 19, 
			snprintf(str_tmp,MAX_STR_TMP,"%4llu",movie_info.file.Size>>20);
			*item_string = str_tmp;
			break;
		case MB_INFO_MAX_NUMBER: 			//		= 20 
		default:
			*item_string="";
			result = false;
			break;
	}
	//TRACE("   getMovieInfoItem: %d,>%s<",item,*item_string.c_str());
	return(result);
}

/************************************************************************

************************************************************************/
void CMovieBrowser::updateSerienames(void)
{
	if(m_seriename_stale == false) 
		return;
		
	m_serienames.clear();
	for(unsigned int i = 0; i < m_vMovieInfo.size(); i++)
	{
		if(!m_vMovieInfo[i].serieName.empty())
		{
			// current series name is not empty, lets see if we already have it in the list, and if not save it to the list.
			bool found = false;
			for(unsigned int t = 0; t < m_serienames.size() && found == false; t++)
			{
				if(strcmp(m_serienames[t].c_str(),m_vMovieInfo[i].serieName.c_str()) == 0)
					found = true;
			}
			if(found == false)
				m_serienames.push_back(m_vMovieInfo[i].serieName);
		}
	}
	TRACE("[mb]->updateSerienames: %d\r\n",m_serienames.size());
	// TODO sort(m_serienames.begin(), m_serienames.end(), my_alphasort);
	m_seriename_stale = false;
}	


/************************************************************************

************************************************************************/

CMenuSelector::CMenuSelector(const char * OptionName, const bool Active , char * OptionValue, int* ReturnInt ,int ReturnIntValue ) : CMenuItem()
{
	height     = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
	optionName = 		OptionName;
	optionValue = 		OptionValue;
	active = 			Active;
	returnIntValue =	ReturnIntValue;
	returnInt = 		ReturnInt;
};

int CMenuSelector::exec(CMenuTarget* parent)
{ 
	if(returnInt != NULL)
		*returnInt= returnIntValue;
		
	if(optionValue != NULL && optionName != NULL) 
	{
		strcpy(optionValue,optionName); 
	}
	return menu_return::RETURN_EXIT;
};

int CMenuSelector::paint(bool selected)
{
	CFrameBuffer * frameBuffer = CFrameBuffer::getInstance();

	unsigned char color   = COL_MENUCONTENT;
	fb_pixel_t    bgcolor = COL_MENUCONTENT_PLUS_0;
	if (selected)
	{
		color   = COL_MENUCONTENTSELECTED;
		bgcolor = COL_MENUCONTENTSELECTED_PLUS_0;
	}
	if (!active)
	{
		color   = COL_MENUCONTENTINACTIVE;
		bgcolor = COL_MENUCONTENTINACTIVE_PLUS_0;
	}

	frameBuffer->paintBoxRel(x, y, dx, height, bgcolor);

	int stringstartposName = x + offx + 10;
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(stringstartposName,   y+height,dx- (stringstartposName - x), optionName, color, 0, true); // UTF-8

	if (selected)
		CLCD::getInstance()->showMenuText(0, optionName, -1, true); // UTF-8

	return y+height;
}


/************************************************************************

************************************************************************/
int CMovieHelp::exec(CMenuTarget* parent, const std::string & actionKey)
{
	Helpbox helpbox;
	helpbox.addLine(NEUTRINO_ICON_BUTTON_RED, "Sortierung ändern");
	helpbox.addLine(NEUTRINO_ICON_BUTTON_GREEN, "Filterfenster einblenden");
	helpbox.addLine(NEUTRINO_ICON_BUTTON_YELLOW, "Aktives Fenster wechseln");
	helpbox.addLine(NEUTRINO_ICON_BUTTON_BLUE, "Filminfos neu laden");
	helpbox.addLine(NEUTRINO_ICON_BUTTON_DBOX, "Hauptmenü");
	helpbox.addLine("+/-  Ansicht wechseln");
	helpbox.addLine("");
	helpbox.addLine("Während der Filmwiedergabe:");
	helpbox.addLine(NEUTRINO_ICON_BUTTON_BLUE, " Markierungsmenu ");
	helpbox.addLine(NEUTRINO_ICON_BUTTON_0,    " Markierungsaktion nicht ausführen");
	helpbox.addLine("");
	helpbox.addLine("MovieBrowser $Revision: 1.8 $");
	helpbox.addLine("by Günther");
	helpbox.show(LOCALE_MESSAGEBOX_INFO);
	return(0);
}



