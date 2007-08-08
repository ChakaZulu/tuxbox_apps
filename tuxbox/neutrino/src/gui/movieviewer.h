/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Copyright (C) 2007 Guenther
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
	
	Purpose: Show info bar for movie playback. Displays further informations from *.xml if available.
		Adapted from infoviewer.h/.cpp
*/


#ifndef __movieviewer__
#define __movieviewer__

#include <sectionsdclient/sectionsdclient.h>

#include <driver/rcinput.h>
#include <driver/framebuffer.h>
#include <driver/fontrenderer.h>
#include <system/settings.h>
#include <gui/movieplayer.h>

#include "gui/color.h"
#include "widget/menue.h"

#include <string>


class CMovieViewer
{
 private:  // Variables
	CFrameBuffer * frameBuffer;
	
	bool           gotTime;
	
	int            InfoHeightY;
	int            InfoHeightY_Info;
	bool           isButtonBar;
	bool           isButtonBarExtended;

	int            BoxEndX;
	int            BoxEndY;
	int            BoxStartX;
	int            BoxStartY;
	int            BoxEndInfoY;
	int            ButtonWidth;

	int            ChanWidth;
	int            ChanHeight;
	int            ChanInfoX;

	char           aspectRatio;

	uint           sec_timer_id;
	uint           fadeTimer;

	CMoviePlayerGui::state player_mode;
	bool is_ac3;
	bool has_ac3;
	uint apid_count;
	uint runningPercent ;
	std::string currentTitle;
	std::string currentInfo1;
	char current_name[100];
	char next_name[100];

	int time_left_width;
	int time_dot_width;
	int time_width;
	int time_height;
	char old_timestr[10];
	
 private:  // Functions
	void init();

	void showIcon_Audio();
	void showIcon_16_9()      const;
	void showIcon_VTXT()      const;
	void showRecordIcon(const bool show);
	void showData();
	void showButtonBar();
	void showButtonBarExtended();
   	void showLcdPercentOver();
	void paintTime( bool show_dot, bool firstPaint );
	
 public:  // Variables
	bool	is_visible;

 public:  // Functions
	CMovieViewer();
	~CMovieViewer();
	void	exec(); 
	void	show(); 
	void	kill();
	void	setData(uint _aspectRatio, CMoviePlayerGui::state _player_mode, bool _is_ac3, bool _has_ac3, uint _apid_count, uint _runningPercent, const char* _current_name, const char* _next_name = NULL);
	void 	getData();
	
	int	handleMsg(const neutrino_msg_t msg, neutrino_msg_data_t data);
};


#endif
