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

#ifndef __mp3playergui__
#define __mp3playergui__


#include "driver/framebuffer.h"
#include "driver/audioplay.h"
#include "gui/filebrowser.h"
#include "gui/widget/menue.h"


#include <string>
#include <vector>


class CAudiofile
{
 public:
	std::string Filename;
	std::string Title;
	std::string Artist;
	std::string Album;
	std::string Year;
#ifdef INCLUDE_UNUSED_STUFF
	std::string Comment;
	std::string ChannelMode;
	std::string Bitrate;
	std::string Samplerate;
	std::string Layer;
#endif
	time_t Duration;
	std::string Genre;
	bool VBR;
	CFile::FileType FileType;
};

typedef std::vector<CAudiofile> CPlayList;


class CAudioPlayerGui : public CMenuTarget
{
 public:
	enum State
		{
			PLAY=0,
			STOP,
			PAUSE,
			FF,
			REV
		};
   enum DisplayOrder {ARTIST_TITLE = 0, TITLE_ARTIST=1};

 private:
	CFrameBuffer * frameBuffer;
	CFileBrowser * filebrowser;
	unsigned int   selected;
	int            current;
	unsigned int   liststart;
	unsigned int   listmaxshow;
	int            fheight; // Fonthoehe Playlist-Inhalt
	int            theight; // Fonthoehe Playlist-Titel
	int            sheight; // Fonthoehe MP Info
	int            buttonHeight;
	int            title_height;
	int            info_height;
	int            key_level;
	bool           visible;			
	State          m_state;
	time_t         m_time_total;
	time_t         m_time_played;
	std::string    m_metainfo;

	CPlayList      playlist;
	CAudiofile     curr_audiofile;
	std::string    Path;

	int            width;
	int            height;
	int            x;
	int            y;
	int            m_title_w;

	int            m_LastMode;
   int            m_idletime;
   bool           m_screensaver;
	bool           m_vol_ost;

	void paintItem(int pos);
	void paint();
	void paintHead();
	void paintFoot();
	void paintInfo();
	void paintLCD();
	void hide();

	void get_id3(CAudiofile * audiofile);
	void get_mp3info(CAudiofile * audiofile);
	CFileFilter audiofilefilter;
	void paintItemID3DetailsLine (int pos);
	void clearItemID3DetailsLine ();
	void play(int pos);
	void stop();
	void pause();
	void ff();
	void rev();
	int getNext();
	void GetMetaData(CAudiofile *File);
	void updateMetaData();
	void updateTimes(const bool force = false);
	void showMetaData();
	void screensaver(bool on);
	void savePlaylist();

	/**
	 * Converts an absolute filename to a relative one
	 * as seen from a file in fromDir.
	 * Example:
	 * absFilename: /mnt/audio/A/abc.mp3
	 * fromDir: /mnt/audio/B
	 * => ../A/abc.mp3 will be returned 
	 * @param fromDir the directory from where we want to
	 * access the file
	 * @param absFilename the file we want to access in a
	 * relative way from fromDir (given as an absolute path)
	 * @return the location of absFilename as seen from fromDir
	 * (relative path)
	 */
	std::string absPath2Rel(const std::string& fromDir,
				const std::string& absFilename);
	
	/** 
	 * Asks the user if the file filename should be overwritten or not
	 * @param filename the name of the file
	 * @return true if file should be overwritten, false otherwise
	 */
	bool CAudioPlayerGui::askToOverwriteFile(const std::string& filename);
	
 public:
	CAudioPlayerGui();
	~CAudioPlayerGui();
	int show();
	int exec(CMenuTarget* parent, const std::string & actionKey);
};


#endif


