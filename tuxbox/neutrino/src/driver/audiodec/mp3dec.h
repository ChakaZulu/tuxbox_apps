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


#ifndef __MP3_DEC__
#define __MP3_DEC__

#include <mad.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <driver/audioplay.h>

class CMP3Dec : public CBaseDec
{
private:
	enum mad_layer m_layer;
   enum mad_mode m_mode;
   enum mad_emphasis m_emphasis;
   unsigned long m_bitrate;
   unsigned int m_samplerate;
   bool m_vbr;
	unsigned int m_filesize;

	const char*  MadErrorString(const struct mad_stream *Stream);
	signed short MadFixedToSShort(const mad_fixed_t Fixed);
	void			 CreateInfo();
	void         GetMP3Info(FILE* in, bool nice, CAudioMetaData* m);
	bool         GetID3(FILE* in, CAudioMetaData* m);

public:
	static CMP3Dec* getInstance();
	virtual int Decoder(FILE *InputFp,int OutputFd, State* state);
	bool GetMetaData(FILE *in, bool nice, CAudioMetaData* m);
	CMP3Dec(){};

};


#endif

