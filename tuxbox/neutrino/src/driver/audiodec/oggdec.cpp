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
// nr of msecs to skip in ff/rev mode
#define MSECS_TO_SKIP 3000
// nr of msecs to play in ff/rev mode
#define MSECS_TO_PLAY 200

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

#define PCMBUFFER 4096 //4096 max for libtremor
#define MAX_OUTPUT_SAMPLES 1022 /* AVIA_GT_PCM_MAX_SAMPLES-1 */

int COggDec::Decoder(FILE *in, int OutputFd, State* state)
{
  OggVorbis_File vf;
  int bitstream, rval;
  ogg_int64_t jumptime=0;
  Status=0;
  mOutputFd = OutputFd;

  if (!Open(in, &vf))
  {
	  fclose(in);
	  return false;
  }

  SetMetaData(&vf, &CAudioPlayer::getInstance()->m_MetaData);

  if (SetDSP(OutputFd, AFMT_S16_BE, 
				 ov_info(&vf,0)->rate , ov_info(&vf,0)->channels))
  {
	  fclose(in);
	  Status=1;
	  return Status;
  }
  
  /* up and away ... */
  mSlotSize = MAX_OUTPUT_SAMPLES * 2 * ov_info(&vf,0)->channels;
  for(int i = 0 ; i < DECODE_SLOTS ; i++)
  {
	  if ((mPcmSlots[i] = (char*) malloc(mSlotSize)) == NULL)
	  {
		  fclose(in);
		  Status=4;
		  return Status;
	  }
	  mSlotTime[i]=0;
  }
  mReadSlot=mWriteSlot=0;

  pthread_t OutputThread;
  if (pthread_create (&OutputThread, 0, OutputDsp, (void *) this) != 0 )
  {
	  fclose(in);
	  Status=5;
	  return Status;
  }

  int bytes;
  State oldstate=*state;
  do
  {
	  // clear buffer on state change
	  if(oldstate!=*state)
	  {
		  mWriteSlot=mReadSlot=0;
		  oldstate=*state;
	  }
	  while((mWriteSlot+1)%DECODE_SLOTS == mReadSlot)
	  {
		  usleep(10000);
	  }
	  bytes=0;
	  if(mSeekable)
		  mSlotTime[mWriteSlot]=ov_time_tell(&vf);
	  do
	  {
		  rval = ov_read(&vf, mPcmSlots[mWriteSlot]+bytes, mSlotSize-bytes, &bitstream);
		  bytes+=rval;
	  } while (rval > 0 && bytes !=mSlotSize);
	  if((*state==FF || *state==REV) && mSeekable )
	  {
		  if((std::abs(mSlotTime[mWriteSlot]-jumptime)) > MSECS_TO_PLAY)
		  {
			  if(*state==FF)
			  {
				  ov_time_seek_page(&vf, mSlotTime[mWriteSlot] + MSECS_TO_SKIP);
				  jumptime=mSlotTime[mWriteSlot]+MSECS_TO_SKIP;
			  }
			  else
			  {
				  if(mSlotTime[mWriteSlot] < MSECS_TO_SKIP)
				  {
					  ov_time_seek(&vf, 0);
					  *state=PLAY;
				  }
				  else
				  {
					  ov_time_seek_page(&vf, mSlotTime[mWriteSlot] - MSECS_TO_SKIP);
					  jumptime=mSlotTime[mWriteSlot]-MSECS_TO_SKIP;
				  }
			  }
		  }
	  }
	  if(bytes == mSlotSize)
		  mWriteSlot=(mWriteSlot+1) % DECODE_SLOTS;
  } while (rval != 0 && *state!=STOP_REQ && Status==0);
  pthread_cancel(OutputThread);

  for(int i = 0 ; i < DECODE_SLOTS ; i++)
	  free(mPcmSlots[i]);

  /* clean up the junk from the party */
  ov_clear(&vf);
  
  /* and drive home ;) */
  return Status;
}

void* COggDec::OutputDsp(void * arg)
{
	COggDec* dec = (COggDec*) arg;
	while(true)
	{
		while(dec->mReadSlot==dec->mWriteSlot)
		{
			usleep(10000);
		}
		if (write(dec->mOutputFd, dec->mPcmSlots[dec->mReadSlot], dec->mSlotSize) != dec->mSlotSize)
		{
			fprintf(stderr,"%s: PCM write error (%s).\n", ProgName, strerror(errno));
			dec->Status=6;
			break;
		}
		CAudioPlayer::getInstance()->setTimePlayed((int)(dec->mSlotTime[dec->mReadSlot]/1000));
		dec->mReadSlot=(dec->mReadSlot+1)%DECODE_SLOTS;
	}
	return NULL;
}

bool COggDec::GetMetaData(FILE *in, bool nice, CAudioMetaData* m)
{
	OggVorbis_File vf;
	if (!Open(in, &vf))
	{
		fclose(in);
		return false;
	}
	SetMetaData(&vf, m);
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

void COggDec::ParseUserComments(vorbis_comment* vc, CAudioMetaData* m)
{
	for(int i=0; i < vc->comments ; i++)
	{
		char* search;
		if((search=strstr(vc->user_comments[i],"Artist"))!=NULL)
			m->artist = search+7;
		else if((search=strstr(vc->user_comments[i],"Album"))!=NULL)
			m->album = search+6;
		else if((search=strstr(vc->user_comments[i],"Title"))!=NULL)
			m->title = search+6;
		else if((search=strstr(vc->user_comments[i],"Genre"))!=NULL)
			m->genre = search+6;
		else if((search=strstr(vc->user_comments[i],"Date"))!=NULL)
			m->date = search+5;
		else if((search=strstr(vc->user_comments[i],"TrackNumber"))!=NULL)
			m->track = search+12;
	}
}


void COggDec::SetMetaData(OggVorbis_File* vf, CAudioMetaData* m)
{
	/* Set Metadata */
	m->type = CAudioMetaData::OGG;
	m->bitrate = ov_info(vf,0)->bitrate_nominal;
	m->samplerate = ov_info(vf,0)->rate;
	if(mSeekable)
		m->total_time = (time_t) ov_time_total(vf, 0)/1000;
	std::stringstream ss;
	ss << "OGG V." << ov_info(vf,0)->version << " / " <<  ov_info(vf,0)->channels << "channel(s)";
	m->type_info = ss.str();
	ParseUserComments(ov_comment(vf, 0), m);
	m->changed=true;
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
	//rval = ov_test_callbacks((void*)in, vf, NULL, 0, cb);
	rval = ov_open_callbacks(in, vf, NULL, 0, cb);

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
	//ov_test_open(vf);

	if(ov_seekable(vf)) 
		mSeekable = true;
	else
		mSeekable = false;
	
	return true;
}

