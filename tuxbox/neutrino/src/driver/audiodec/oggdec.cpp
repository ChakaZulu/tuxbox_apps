/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2004 Sania, Zwen
	
	Homepage: http://www.cyberphoria.org/

	Kommentar:

	ogg vorbis audio decoder
	uses tremor libvorbisidec
	
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
#include <algorithm>
#include <stdio.h>
#include <oggdec.h>
#include <driver/netfile.h>
#include <linux/soundcard.h>
#include <sstream>

#define ProgName "OggDec"

/* at first, define our own callback functions used in */
/* tremor to access the data. These functions are simple mappers */
size_t ogg_read(void *buf, size_t size, size_t nmemb, void *data)
{
  return fread(buf, size, nmemb, (FILE*)data);
}

int ogg_seek(void *data, ogg_int64_t offset, int whence)
{
  return fseek((FILE*)data, (long)offset, whence);
}

int ogg_close(void *data)
{
  return fclose((FILE*)data);
}

long ogg_tell(void *data)
{
  return ftell((FILE*)data);
}

#define PCMBUFFER 4096 //max for libtremor
#define MAX_OUTPUT_SAMPLES 1022 /* AVIA_GT_PCM_MAX_SAMPLES-1 */

int COggDec::Decoder(FILE *in, int OutputFd, State* state)
{
  OggVorbis_File vf;
  int bitstream, rval;
  int Status=0;
  char pcmbuf[PCMBUFFER];
  
  if (!Open(in, &vf))
  {
	  fclose(in);
	  return false;
  }

  SetMetaData(&vf);

  if (SetDSP(OutputFd, AFMT_S16_BE, 
				 ov_info(&vf,-1)->rate , ov_info(&vf,-1)->channels))
  {
	  Status=1;
	  return Status;
  }
  
  /* up and away ... */
  const int OUTPUT_BUFFER_SIZE = MAX_OUTPUT_SAMPLES * 2 * ov_info(&vf,-1)->channels;
  char outbuf[OUTPUT_BUFFER_SIZE];
  char* pOutPtr=outbuf;
  const char *pOutPtrEnd=outbuf+OUTPUT_BUFFER_SIZE;
  char* pPcmPtr;
  int pcm_rest;
  do
  {
	  rval = ov_read(&vf, pcmbuf, PCMBUFFER, &bitstream);
	  // why does this segfault? CAudioPlayer::getInstance()->setTimePlayed((int)(ov_time_tell(&vf)/1000));
	  pcm_rest=rval;
	  pPcmPtr = pcmbuf;
	  while(pcm_rest > 0)
	  {
			  *((unsigned short*) pOutPtr) = *((unsigned short*) pPcmPtr);
			  pOutPtr+=2;
			  pPcmPtr+=2;
			  pcm_rest-=2;
           
			  if(pOutPtr == pOutPtrEnd)
			  {
				  if (write(OutputFd, outbuf, OUTPUT_BUFFER_SIZE) != OUTPUT_BUFFER_SIZE)
				  {
					  fprintf(stderr,"%s: PCM write error (%s).\n", ProgName, strerror(errno));
					  Status = 2;
					  break;
				  }
				  pOutPtr=outbuf;
			  }
	  }
  } while (rval != 0 && *state!=STOP_REQ && Status==0);

  /* clean up the junk from the party */
  ov_clear(&vf);
  
  /* and drive home ;) */
  return Status;
}

bool COggDec::GetMetaData(FILE *in, bool nice)
{
	OggVorbis_File vf;
	if (!Open(in, &vf))
	{
		fclose(in);
		return false;
	}
	SetMetaData(&vf);
	ov_clear(&vf);
	return true;
}

COggDec* COggDec::getInstance()
{
	static COggDec* OggDec = NULL;
	if(OggDec == NULL)
	{
		OggDec = new COggDec();
	}
	return OggDec;
}

void COggDec::ParseUserComments(vorbis_comment* vc)
{
	for(int i=0; i < vc->comments ; i++)
	{
		char* search;
		if((search=strstr(vc->user_comments[i],"Artist"))!=NULL)
			CAudioPlayer::getInstance()->m_MetaData.artist = search+7;
		else if((search=strstr(vc->user_comments[i],"Album"))!=NULL)
			CAudioPlayer::getInstance()->m_MetaData.album = search+6;
		else if((search=strstr(vc->user_comments[i],"Title"))!=NULL)
			CAudioPlayer::getInstance()->m_MetaData.title = search+6;
		else if((search=strstr(vc->user_comments[i],"Genre"))!=NULL)
			CAudioPlayer::getInstance()->m_MetaData.genre = search+6;
		else if((search=strstr(vc->user_comments[i],"Date"))!=NULL)
			CAudioPlayer::getInstance()->m_MetaData.date = search+5;
		else if((search=strstr(vc->user_comments[i],"TrackNumber"))!=NULL)
			CAudioPlayer::getInstance()->m_MetaData.track = search+12;
	}
}


void COggDec::SetMetaData(OggVorbis_File* vf)
{
	/* Set Metadata */
	CAudioPlayer::getInstance()->m_MetaData.type = CAudioPlayer::MetaData::OGG;
	CAudioPlayer::getInstance()->m_MetaData.bitrate = ov_info(vf,-1)->bitrate_nominal;
	CAudioPlayer::getInstance()->m_MetaData.samplerate = ov_info(vf,-1)->rate;
	// Why does this segfault? CAudioPlayer::getInstance()->m_MetaData.total_time = (time_t) ov_time_total(vf, -1);
	std::stringstream ss;
	ss << "OGG V." << ov_info(vf,-1)->version << " / " <<  ov_info(vf,-1)->channels << "channel(s)";
	CAudioPlayer::getInstance()->m_MetaData.type_info = ss.str();
	ParseUserComments(ov_comment(vf, -1));
	CAudioPlayer::getInstance()->m_MetaData.changed=true;
}

bool COggDec::Open(FILE* in, OggVorbis_File* vf)
{
	int rval;
	ov_callbacks cb;
	/* we need to use our own functions, because we have */
	/* the netfile layer hooked in here. If we would not */
	/* provide callbacks, the tremor lib and the netfile */
	/* layer would clash and steal each other the data   */
	/* from the stream !                                 */ 

	cb.read_func  = ogg_read;
	cb.seek_func  = ogg_seek;
	cb.close_func = ogg_close;
	cb.tell_func  = ogg_tell;

	/* test the dope ... */
	rval = ov_test_callbacks((void*)in, vf, NULL, 0, cb);

	/* and tell our friends about the quality of the stuff */
	// initialize the sound device here

	if(rval<0)
	{
	  switch(rval)
	  {
	  case OV_EREAD:	sprintf(err_txt, "media read error"); break;
	  case OV_ENOTVORBIS:	sprintf(err_txt, "no vorbis stream"); break;
	  case OV_EVERSION:	sprintf(err_txt, "incompatible vorbis version"); break;
	  case OV_EBADHEADER:	sprintf(err_txt, "invalid bvorbis bitstream header"); break;
	  case OV_EFAULT:	sprintf(err_txt, "internal logic fault (tremor)"); break;
	  default:		sprintf(err_txt, "unknown error, code: %d", rval);
	  }
	  fprintf(stderr,"%s: %s\n", ProgName, err_txt);
	  return false;
	}

	/* finish the opening and ignite the joint */
	ov_test_open(vf);
	return true;
}
