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
#include <tremor/ogg.h>
#include <tremor/ivorbisfile.h>
#include <driver/netfile.h>
#include <linux/soundcard.h>

#define ProgName "OggDec"

/* at first, define our own callback functions used in */
/* tremor to access the data. These functions are simple mappers */
size_t ogg_read(void *buf, size_t size, size_t nmemb, void *data)
{
  return fread(buf, size, nmemb, (FILE*)data);
}

int ogg_seek(void *data, ogg_int64_t offset, int whence)
{
  return fseek((FILE*)data, offset, whence);
}

int ogg_close(void *data)
{
  return fclose((FILE*)data);
}

long ogg_tell(void *data)
{
  return ftell((FILE*)data);
}

#define PCMBUFFER 1022*4 //max for libtremor
#define OUTPUT_BUFFER_SIZE	1022*4 /* AVIA_GT_PCM_MAX_SAMPLES-1 */

int COggDec::Decoder(FILE *in, int OutputFd, State* state)
{
  OggVorbis_File vf;
  ov_callbacks cb;
  int bitstream, rval;
  int Status=0;
  char pcmbuf[PCMBUFFER];
  
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
  rval = ov_test_callbacks((void*)in, &vf, NULL, 0, cb);
  
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
    
    fclose(in);
    return rval;
  }

  /* finish the opening and ignite the joint */
  ov_test_open(&vf);


  if (SetDSP(OutputFd, AFMT_S16_BE, 
				 ov_info(&vf,0)->rate , ov_info(&vf,0)->channels))
  {
	  Status=1;
	  return Status;
  }
  
  /* up and away ... */
  do
  {
    rval = ov_read(&vf, pcmbuf, PCMBUFFER, &bitstream);
	 int rest=rval;
	 char* bufptr = pcmbuf;
	 while(rest > 0)
	 {
		 int write_bytes = std::min(OUTPUT_BUFFER_SIZE, rest);
		 if (write(OutputFd, bufptr, write_bytes) != write_bytes)
		 {
			 fprintf(stderr,"%s: PCM write error (%s).\n", ProgName, strerror(errno));
			 Status = 2;
			 break;
		 }
		 rest-=write_bytes;
		 bufptr+=write_bytes;
	 }
  } while (rval != 0 && *state!=STOP_REQ && Status==0);

  /* clean up the junk from the party */
  ov_clear(&vf);
  
  /* and drive home ;) */
  return Status;
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

