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

#ifndef __controld__
#define __controld__

#include <stdio.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <string>

using namespace std;


#define CONTROLD_UDS_NAME "/tmp/controld.sock"


class CControld
{

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
			CMD_SAVECONFIG,

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

		struct commandVideoPowerSave
		{
			bool powerdown;
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

};

#endif
