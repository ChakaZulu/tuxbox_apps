/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

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

#ifndef __controldclient__
#define __controldclient__

#include <connection/basicclient.h>


#define VCR_STATUS_OFF 0
#define VCR_STATUS_ON 1
#define VCR_STATUS_16_9 2

class CControldClient:private CBasicClient
{
 private:
	virtual const unsigned char   getVersion   () const;
	virtual const          char * getSocketName() const;

 public:

		enum events
		{
			EVT_VOLUMECHANGED,
			EVT_MUTECHANGED,
			EVT_MODECHANGED,
			EVT_VCRCHANGED
		};

		//VideoFormat
		static const char VIDEOFORMAT_AUTO = 0;
		static const char VIDEOFORMAT_16_9 = 1;
		static const char VIDEOFORMAT_4_3  = 2;

		//VideoOutput
		static const char VIDEOOUTPUT_COMPOSITE = 0;
		static const char VIDEOOUTPUT_SVIDEO    = 2;
		static const char VIDEOOUTPUT_RGB       = 1;

		//mute
		static const bool VOLUME_MUTE = true;
		static const bool VOLUME_UNMUTE = false;

		//BoxType  /* cf. driver/include/tuxbox/tuxbox_info.h */
		typedef enum tuxbox_maker
			{
				TUXBOX_MAKER_UNKNOWN			= 0,
				TUXBOX_MAKER_NOKIA			= 1,
				TUXBOX_MAKER_PHILIPS			= 2,
				TUXBOX_MAKER_SAGEM			= 3,
				TUXBOX_MAKER_DREAM_MM			= 4,
				TUXBOX_MAKER_TECHNOTREND		= 5,
			}
		tuxbox_maker_t;

		//scartmode
		static const char SCARTMODE_ON  = 1;
		static const char SCARTMODE_OFF = 0;

		/*
			setVolume(char) : Setzten der Lautstärke
			Parameter: 0..100 - 0=leise 100=laut
			           avs == true : mute avs device
			           avs == false: mute audio device
		*/
		void setVolume(const char volume, const bool avs = true);
		char getVolume(const bool avs = true);

		/*
			setMute(bool, bool) : setzen von Mute
			Parameter: VOLUME_MUTE   = ton aus
			           VOLUME_UNMUTE = ton an
			           avs == true : mute avs device
			           avs == false: mute audio device
		*/
		void setMute(const bool mute, const bool avs = true);
		bool getMute(const bool avs = true);

		/*
			Mute(bool) : Ton ausschalten
			Parameter: avs == true : mute avs device
			           avs == false: mute audio device
		*/
		void Mute(const bool avs = true);

		/*
			UnMute(bool) : Ton wieder einschalten
			Parameter: avs == true : mute avs device
			           avs == false: mute audio device
		*/
		void UnMute(const bool avs = true);


		/*
			setVideoFormat(char) : Setzten des Bildformates ( 4:3 / 16:9 )
			Parameter: VIDEOFORMAT_AUTO = auto
			           VIDEOFORMAT_4_3  = 4:3
			           VIDEOFORMAT_16_9 = 16:9
		*/
		void setVideoFormat(char);
		char getVideoFormat();
		/*
			getAspectRatio : Aktueller Wert aus dem Bitstream
					2: 4:3
					3: 16:9
					4: 2:2.1
		*/
		char getAspectRatio();

		/*
			setVideoOutput(char) : Setzten des Videooutputs ( composite / svhs / rgb )
			Parameter: VIDEOOUTPUT_COMPOSITE = composite video
			           VIDEOOUTPUT_SVIDEO    = svhs video
			           VIDEOOUTPUT_RGB       = rgb
		*/
		void setVideoOutput(char);
		char getVideoOutput();

		/*
			setBoxType(CControldClient::tuxbox_vendor_t) : Setzten des Boxentyps ( nokia / sagem / philips )
		*/
		void setBoxType(const CControldClient::tuxbox_maker_t);
		CControldClient::tuxbox_maker_t getBoxType();

		/*
			setScartMode(char) : Scartmode ( an / aus )
			Parameter: SCARTMODE_ON  = auf scartinput schalten
			           SCARTMODE_OFF = wieder dvb anzeigen


		*/
		void setScartMode(bool);


		/*
			die Dbox herunterfahren
		*/
		void videoPowerDown(bool);

		/*
			die Dbox herunterfahren
		*/
		void shutdown();


		/*
			ein beliebiges Event anmelden
		*/
		void registerEvent(unsigned int eventID, unsigned int clientID, const char * const udsName);

		/*
			ein beliebiges Event abmelden
		*/
		void unRegisterEvent(unsigned int eventID, unsigned int clientID);

		void saveSettings();

		/*
		  setzen der sync correction im RGB mode des saa7126 (csync) 0=aus, 31=max
		*/
		
		void setRGBCsync(char csync);
		/*
		  lesen der sync correction im RGB mode des saa7126 (csync) 0=aus, 31=max
		*/
		char getRGBCsync();

		  
};

#endif
