/*
	Neutrino-GUI  -   DBoxII-Project

	audioplayer
	Copyright (C) 2002 Bjoern Kalkbrenner <terminar@cyberphoria.org>
	Copyright (C) 2002,2003 Dirch
	Copyright (C) 2002,2003,2004 Zwen
	Homepage: http://www.dbox2.info/

	Kommentar:

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

#define DBOX

/****************************************************************************
 * Includes																	*
 ****************************************************************************/
#include "global.h"
#include <stdio.h>
#include <fcntl.h>
#include <sched.h>

#include <neutrino.h>
#include <driver/audioplay.h>
#include <driver/netfile.h>

void CAudioPlayer::stop()
{
	state = CBaseDec::STOP_REQ;
	pthread_join(thrPlay,NULL);
}
void CAudioPlayer::pause()
{
   if(state==CBaseDec::PLAY || state==CBaseDec::FF || state==CBaseDec::REV)
      state=CBaseDec::PAUSE;
   else if(state==CBaseDec::PAUSE)
      state=CBaseDec::PLAY;
}
void CAudioPlayer::ff(unsigned int seconds)
{
	m_SecondsToSkip = seconds;
	if(state==CBaseDec::PLAY || state==CBaseDec::PAUSE || state==CBaseDec::REV)
		state=CBaseDec::FF;
	else if(state==CBaseDec::FF)
		state=CBaseDec::PLAY;
}
void CAudioPlayer::rev(unsigned int seconds)
{
	m_SecondsToSkip = seconds;
	if(state==CBaseDec::PLAY || state==CBaseDec::PAUSE || state==CBaseDec::FF)
		state=CBaseDec::REV;
	else if(state==CBaseDec::REV)
		state=CBaseDec::PLAY;
}
CAudioPlayer* CAudioPlayer::getInstance()
{
	static CAudioPlayer* AudioPlayer = NULL;
	if(AudioPlayer == NULL)
	{
		AudioPlayer = new CAudioPlayer();
	}
	return AudioPlayer;
}

void ShoutcastCallback(void *arg)
{
	CAudioPlayer::getInstance()->sc_callback(arg);
}

void* CAudioPlayer::PlayThread(void * filename)
{
	FILE *fp;
	int soundfd;
	soundfd=::open("/dev/sound/dsp",O_WRONLY);
	if (soundfd != -1)
	{
		fp = ::fopen((char *)filename,"r");
		if (fp!=NULL)
		{
			/* add callback function for shoutcast */
			if (fstatus(fp, ShoutcastCallback) < 0)
			{
				//fprintf(stderr,"Error adding shoutcast callback!\n%s",err_txt);
			}
         
			/* Decode stdin to stdout. */
			CBaseDec::RetCode Status;
			printf("CAudioPlayer: Decoding %s ", (char*) filename);
			Status = CBaseDec::DecoderBase(fp,soundfd,&getInstance()->state, 
						       &getInstance()->m_MetaData,
						       &getInstance()->m_played_time,
						       &getInstance()->m_SecondsToSkip);
			fclose(fp);
			if(Status != CBaseDec::OK)
				fprintf(stderr,"Error %d occured during decoding.\n",(int)Status);
	
		}
		else
			fprintf(stderr,"Error opening file %s\n",(char *) filename);
		close(soundfd);
	}
	else
		fprintf(stderr,"Error opening /dev/sound/dsp\n");
		
	getInstance()->state = CBaseDec::STOP;
	pthread_exit(0);
	return NULL;
}

bool CAudioPlayer::play(const char *filename, bool highPrio)
{
	stop();
	getInstance()->clearMetaData();
	state = CBaseDec::PLAY;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	if(highPrio)
	{
		struct sched_param param;
		pthread_attr_setschedpolicy(&attr, SCHED_RR);
		param.sched_priority=1;
		pthread_attr_setschedparam(&attr, &param);
		usleep(100000); // give the event thread some time to handle his stuff
							 // without this sleep there were duplicated events...
	}
	if (pthread_create (&thrPlay, &attr, PlayThread,(void *) filename) != 0 )
	{
		perror("mp3play: pthread_create(PlayThread)");
		return false;
	}
	return true;
}

CAudioPlayer::CAudioPlayer()
{
	init();
}

void CAudioPlayer::init()
{
	CBaseDec::Init();
	state = CBaseDec::STOP;
}

void CAudioPlayer::sc_callback(void *arg)
{
  bool changed=false;
  CSTATE *stat = (CSTATE*)arg;
  if(m_MetaData.artist != stat->artist)
  {
	  m_MetaData.artist = stat->artist;
	  changed=true;
  }
  if (m_MetaData.title != stat->title)
  {
	  m_MetaData.title = stat->title;
	  changed=true;
  }
  if (m_MetaData.sc_station != stat->station)
  {
	  m_MetaData.sc_station = stat->station;
	  changed=true;
  }
  if (m_MetaData.genre != stat->genre)
  {
	  m_MetaData.genre = stat->genre;
	  changed=true;
  }
  if(changed)
  {
	  m_played_time = 0;
  }
  m_sc_buffered = stat->buffered;
  m_MetaData.changed = changed;
  //printf("Callback %s %s %s %d\n",stat->artist, stat->title, stat->station, stat->buffered);
}

void CAudioPlayer::clearMetaData()
{
	m_MetaData.clear();
	m_played_time=0;
	m_sc_buffered=0;
}

CAudioMetaData CAudioPlayer::getMetaData()
{
	CAudioMetaData m = m_MetaData;
	m_MetaData.changed=false;
	return m;
}

CAudioMetaData CAudioPlayer::readMetaData(const char* filename, bool nice)
{
	FILE* fp;
	CAudioMetaData m;
	m.clear();
	fp = ::fopen((char *)filename,"r");
	if (fp!=NULL)
	{
		/* add callback function for shoutcast */
		if (fstatus(fp, ShoutcastCallback) < 0)
		{
			//fprintf(stderr,"Error adding shoutcast callback!\n%s",err_txt);
		}

		/* Decode stdin to stdout. */
		bool Status;
		Status = CBaseDec::GetMetaDataBase(fp, nice, &m);
		if(!Status)
			fprintf(stderr,"Error occured during meta data reading.\n");

		fclose(fp);
	}
	else
		fprintf(stderr,"Error opening file %s\n",(char *) filename);
	
	return m;
}

