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
#include "driver/mp3play.h"
#include "gui/filebrowser.h"
#include "gui/widget/menue.h"


#include <string>
#include <vector>


class CMP3
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
	std::string Duration;
	std::string Genre;
	bool VBR;
	int Index;
};

typedef std::vector<CMP3> CPlayList;


class CMP3PlayerGui : public CMenuTarget
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
	std::string    m_time_total;
	std::string    m_time_played;
	std::string    m_mp3info;

	CPlayList      playlist;
	CMP3           curr_mp3;
	std::string    Path;

	int            width;
	int            height;
	int            x;
	int            y;
	int            m_title_w;

	int            m_LastMode;
   int            m_idletime;
   bool           m_screensaver;

	void paintItem(int pos);
	void paint();
	void paintHead();
	void paintFoot();
	void paintInfo();
	void paintLCD();
	void hide();

	void get_id3(CMP3 * mp3);
	void get_mp3info(CMP3 * mp3);
	CFileFilter mp3filter;
	void paintItemID3DetailsLine (int pos);
	void clearItemID3DetailsLine ();
	void play(int pos);
	void stop();
	void pause();
	void ff();
	void rev();
	int getNext();
	void updateMP3Infos();
	void updateTimes(const bool force = false);
	void showMP3Info();
   void screensaver(bool on);

 public:
	CMP3PlayerGui();
	~CMP3PlayerGui();
	int show();
	int exec(CMenuTarget* parent, const std::string & actionKey);
};


#endif


