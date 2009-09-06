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


#ifndef __infoview__
#define __infoview__

#include <sectionsdclient/sectionsdclient.h>

#include <driver/rcinput.h>
#include <driver/framebuffer.h>
#include <driver/fontrenderer.h>
#include <system/settings.h>

#include "gui/color.h"
#include "widget/menue.h"

#include <string>


class CInfoViewer
{
 private:
	CFrameBuffer * frameBuffer;
	
	bool           gotTime;
	bool           recordModeActive;
	bool           CA_Status;
	
	int            InfoHeightY;
	int            InfoHeightY_Info;
	bool           showButtonBar;

	int            BoxEndX;
	int            BoxEndY;
	int            BoxStartX;
	int            BoxStartY;
	int            ButtonWidth;

#ifdef ENABLE_RADIOTEXT
	// dimensions of radiotext window
	int 		rt_dx;
	int 		rt_dy;
	int 		rt_x;
	int 		rt_y;
	int 		rt_h;
	int 		rt_w;
#endif

 	std::string ChannelName;
	char strChanNum[10]; 
 
	int		ChanNameX;	
	int		ChanNameY;
	int		ChanNameW; 
	int		ChanWidth;
	int		ChanHeight;
	int		ChanInfoX;

	CSectionsdClient::CurrentNextInfo info_CurrentNext;
        t_channel_id   channel_id;

	char           aspectRatio;
		
	uint           sec_timer_id;
	uint           fadeTimer;
	bool           virtual_zap_mode;
	CChannelEventList 		evtlist;
	CChannelEventList::iterator 	eli;
	

	void paintBackground(int col_Numbox);
	void show_Data( bool calledFromEvent = false );
	void display_Info(const char *current, const char *next, bool UTF8 = true,
			  bool starttimes = true, const int pb_pos = -1,
			  const char *runningStart = NULL, const char *runningRest = NULL,
			  const char *nextStart = NULL, const char *nextDuration = NULL,
			  bool update_current = true, bool update_next = true);
	void paintTime( bool show_dot, bool firstPaint );
	void infobarLoop(bool calledFromNumZap, bool fadeIn);
	
	void showButton_Audio();
	void showButton_SubServices();
	
	void showIcon_16_9()      const;
	void showIcon_CA_Status() const;
	void showIcon_VTXT()      const;
	void showIcon_SubT()      const;
	void showRecordIcon(const bool show);

	void showInfoIcons();

	void showFailure();
	void showMotorMoving(int duration);
   	void showLcdPercentOver();
	int showChannelLogo(  const t_channel_id logo_channel_id );
	void showInfoFile();
#ifdef ENABLE_RADIOTEXT
	void showIcon_RadioText(bool rt_available) const;
	void showRadiotext();
	void killRadiotext();
#endif
	
	std::string eventname;

 public:
	bool	is_visible;
	uint	lcdUpdateTimer;

	CInfoViewer();

	void	start();
	void 	showEpgInfo();
	void	showTitle(const int ChanNum, const std::string & Channel, const t_satellite_position satellitePosition, const t_channel_id new_channel_id = 0, const bool calledFromNumZap = false, int epgpos = 0); // Channel must be UTF-8 encoded
	void	showMovieTitle(const int playstate, const std::string &title, const std::string &sub_title,
			       const int percent, const int ac3state, const int num_apids);
	void	lookAheadEPG(const int ChanNum, const std::string & Channel, const t_channel_id new_channel_id = 0, const bool calledFromNumZap = false); //alpha: fix for nvod subchannel update
	void	killTitle();
	void	getEPG(const t_channel_id for_channel_id, CSectionsdClient::CurrentNextInfo &info);
	CSectionsdClient::CurrentNextInfo getCurrentNextInfo() { return info_CurrentNext; }
	
	void	showSubchan();
	void	Set_CA_Status(int Status);
	
	int	handleMsg(const neutrino_msg_t msg, neutrino_msg_data_t data);
	void	clearVirtualZapMode() {virtual_zap_mode = false;}

};




class CInfoViewerHandler : public CMenuTarget
{
	public:
		int  exec( CMenuTarget* parent,  const std::string &actionkey);
		int  doMenu();

};


#endif
