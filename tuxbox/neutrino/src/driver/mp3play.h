/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2002 Bjoern Kalkbrenner <terminar@cyberphoria.org>
	libmad MP3 low-level core
	Homepage: http://www.cyberphoria.org/

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


#ifndef __MP3_PLAY__
#define __MP3_PLAY__

#include <mad.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <linux/soundcard.h>
#include <pthread.h>


class CMP3Player
{

	FILE		*soundfd;
	bool		do_loop;
	pthread_t	thrPlay;
	FILE		*fp;
	static void* PlayThread(void*);
   char m_mp3info[100];
   char m_timePlayed[11];
   char m_timeTotal[11];
   long m_filesize;
   enum mad_layer m_layer;
   enum mad_mode m_mode;
   enum mad_emphasis m_emphasis;
   unsigned long m_bitrate;
   unsigned int m_samplerate;
   bool m_vbr;


	const char		*MadErrorString(const struct mad_stream *Stream);
	signed short MadFixedToSShort(const mad_fixed_t Fixed);
	void				CreateInfo();
	int				MpegAudioDecoder(FILE *InputFp,int OutputFd);
	bool SetDSP(int soundfd, struct mad_header *Header);

public:
	enum State {STOP = 0, PLAY, PAUSE, FF, REV};
	State state;
	static CMP3Player* getInstance();
	bool play(const char *filename);
	void stop();
   void pause();
	void init();
   void ff();
   void rev();
   char* getMp3Info(){return m_mp3info;}
   char* getTimePlayed(){return m_timePlayed;}
   char* getTimeTotal(){return m_timeTotal;}
	bool avs_mute(bool mute);

	CMP3Player();
	~CMP3Player();

};


#endif

