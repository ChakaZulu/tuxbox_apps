/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2002 Bjoern Kalkbrenner <terminar@cyberphoria.org>
	Copyright (C) 2002,2003 Dirch
	Copyright (C) 2002,2003,2004 Zwen
	
	libmad MP3 low-level core
	Homepage: http://www.dbox2.info/

	Kommentar:

	based on
	************************************
	*** madlld -- Mad low-level      ***  v 1.0p1, 2002-01-08
	*** demonstration/decoder        ***  (c) 2001, 2002 Bertrand Petit
	************************************

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
#include <string>


#ifndef __AUDIO_METADATA__
#define __AUDIO_METADATA__

class CAudioMetaData
{
public:
	enum AudioType
   {
		NONE,
		CDR,
		MP3,
		OGG,
		WAV
	};
	AudioType type;
	std::string type_info;
	unsigned int bitrate;
	unsigned int samplerate;
	time_t total_time;
	bool vbr;
	std::string artist;
	std::string title;
	std::string album;
	std::string sc_station;
	std::string date;
	std::string genre;
	std::string track;
	bool changed;
	void clear()
	{
		type=NONE;
		type_info="";
		bitrate=0;
		samplerate=0;
		total_time=0;
		vbr=false;
		artist="";
		title="";
		album="";
		sc_station="";
		date="";
		genre="";
		track="";
		changed=false;
	}
};
#endif
