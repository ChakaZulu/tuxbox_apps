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

#ifndef __controldclient__
#define __controldclient__

#include <stdio.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <string>

using namespace std;


#define CONTROLD_UDS_NAME "/tmp/controld.sock"


class CControldClient
{
		int sock_fd;

		bool controld_connect();
		bool send(char* data, int size);
		bool receive(char* data, int size);
		bool controld_close();

	public:

		static const char ACTVERSION = 2;

		enum commands
		{
			CMD_SETVOLUME = 1,
			CMD_GETVOLUME,
			CMD_MUTE,
			CMD_UNMUTE,
			CMD_GETMUTESTATUS,
			CMD_SETVIDEOFORMAT,
			CMD_GETVIDEOFORMAT,
			CMD_SETVIDEOOUTPUT,
			CMD_GETVIDEOOUTPUT,
			CMD_SETBOXTYPE,
			CMD_GETBOXTYPE,
			CMD_SETSCARTMODE,
			CMD_GETSCARTMODE,
			CMD_SHUTDOWN,
			CMD_SETVIDEOPOWERDOWN,

			CMD_REGISTEREVENT,
			CMD_UNREGISTEREVENT,
			CMD_EVENT
		};

		//command structures
		struct commandHead
		{
			unsigned char messageType;
			unsigned char version;
			unsigned char cmd;
		};

		struct commandVolume
		{
			unsigned char volume;
		};

		struct commandVideoFormat
		{
			unsigned char format;
		};

		struct commandVideoOutput
		{
			unsigned char output;
		};

		struct commandBoxType
		{
			unsigned char boxtype;
		};

		struct commandScartMode
		{
			unsigned char mode;
		};

		//response structures
		struct responseVolume
		{
			unsigned char volume;
		};

		struct responseMute
		{
			bool mute;
		};

		struct responseVideoFormat
		{
			unsigned char format;
		};

		struct responseVideoOutput
		{
			unsigned char output;
		};

		struct responseBoxType
		{
			unsigned char boxtype;
		};

		struct responseScartMode
		{
			unsigned char mode;
		};

		struct responseVideoPowerSave
		{
			bool powerdown;
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

		//BoxType
		static const char BOXTYPE_NOKIA   = 1;
		static const char BOXTYPE_SAGEM   = 2;
		static const char BOXTYPE_PHILIPS = 3;
	
		//scartmode
		static const char SCARTMODE_ON  = 1;
		static const char SCARTMODE_OFF = 0;

		/*
			Konstruktor
		*/
		CControldClient();

		/*
			setVolume(char) : Setzten der Lautstärke
			Parameter: 0..100 - 0=leise 100=laut
		*/
		void setVolume(char volume );
		char getVolume();

		/*
			setMute(bool) : setzen von Mute
			Parameter: VOLUME_MUTE   = ton aus
			           VOLUME_UNMUTE = ton an
		*/
		void setMute( bool );
		bool getMute();

		/*
			Mute() : Ton ausschalten
		*/
		void Mute();

		/*
			UnMute() : Ton wieder einschalten
		*/
		void UnMute();

		/*
			setVideoFormat(char) : Setzten des Bildformates ( 4:3 / 16:9 )
			Parameter: VIDEOFORMAT_AUTO = auto
			           VIDEOFORMAT_4_3  = 4:3
			           VIDEOFORMAT_16_9 = 16:9
		*/
		void setVideoFormat(char);
		char getVideoFormat();

		/*
			setVideoOutput(char) : Setzten des Videooutputs ( composite / svhs / rgb )
			Parameter: VIDEOOUTPUT_COMPOSITE = composite video
			           VIDEOOUTPUT_SVIDEO    = svhs video
			           VIDEOOUTPUT_RGB       = rgb
		*/
		void setVideoOutput(char);
		char getVideoOutput();

		/*
			setVideoOutput(char) : Setzten des Boxentyps ( nokia / sagem / philips )
			Parameter: BOXTYPE_NOKIA   = nokia dbox
			           BOXTYPE_SAGEM   = sagem
			           BOXTYPE_PHILIPS = philips
	
		*/
		void setBoxType(char);
		char getBoxType();

		/*
			setVideoOutput(char) : Scartmode ( an / aus )
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
};

#endif
