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


/****************************************************************************
 * Includes																	*
 ****************************************************************************/
#include "global.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <mad.h>
#include <sched.h>

#include <neutrino.h>
#include <driver/mp3play.h>
#include <dbox/avs_core.h>
#include <linux/dvb/audio.h>

#define AVS_DEVICE	"/dev/dbox/avs0"
#define ADAP		"/dev/dvb/adapter0"
#define ADEC		ADAP "/audio0"

// Frames to skip in ff/rev mode
#define FRAMES_TO_SKIP 75 
// nr of frames to play after skipping in rev/ff mode
#define FRAMES_TO_PLAY 5

#define ProgName "CMP3Player"

/****************************************************************************
 * Global variables.														*
 ****************************************************************************/
/****************************************************************************
 * Return an error string associated with a mad error code.					*
 ****************************************************************************/
/* Mad version 0.14.2b introduced the mad_stream_errorstr() function.
 * For previous library versions a replacement is provided below.
 */
#if (MAD_VERSION_MAJOR>=1) || \
    ((MAD_VERSION_MAJOR==0) && \
     (((MAD_VERSION_MINOR==14) && \
       (MAD_VERSION_PATCH>=2)) || \
      (MAD_VERSION_MINOR>14)))
#define MadErrorString(x) mad_stream_errorstr(x)
#else
const char *CMP3Player::MadErrorString(const struct mad_stream *Stream)
{
	switch(Stream->error)
	{
		/* Generic unrecoverable errors. */
		case MAD_ERROR_BUFLEN:
			return("input buffer too small (or EOF)");
		case MAD_ERROR_BUFPTR:
			return("invalid (null) buffer pointer");
		case MAD_ERROR_NOMEM:
			return("not enough memory");

		/* Frame header related unrecoverable errors. */
		case MAD_ERROR_LOSTSYNC:
			return("lost synchronization");
		case MAD_ERROR_BADLAYER:
			return("reserved header layer value");
		case MAD_ERROR_BADBITRATE:
			return("forbidden bitrate value");
		case MAD_ERROR_BADSAMPLERATE:
			return("reserved sample frequency value");
		case MAD_ERROR_BADEMPHASIS:
			return("reserved emphasis value");

		/* Recoverable errors */
		case MAD_ERROR_BADCRC:
			return("CRC check failed");
		case MAD_ERROR_BADBITALLOC:
			return("forbidden bit allocation value");
		case MAD_ERROR_BADSCALEFACTOR:
			return("bad scalefactor index");
		case MAD_ERROR_BADFRAMELEN:
			return("bad frame length");
		case MAD_ERROR_BADBIGVALUES:
			return("bad big_values count");
		case MAD_ERROR_BADBLOCKTYPE:
			return("reserved block_type");
		case MAD_ERROR_BADSCFSI:
			return("bad scalefactor selection info");
		case MAD_ERROR_BADDATAPTR:
			return("bad main_data_begin pointer");
		case MAD_ERROR_BADPART3LEN:
			return("bad audio data length");
		case MAD_ERROR_BADHUFFTABLE:
			return("bad Huffman table select");
		case MAD_ERROR_BADHUFFDATA:
			return("Huffman data overrun");
		case MAD_ERROR_BADSTEREO:
			return("incompatible block_type for JS");

		/* Unknown error. This swich may be out of sync with libmad's
		 * defined error codes.
		 */
		default:
			return("Unknown error code");
	}
}
#endif

/****************************************************************************
 * Converts a sample from mad's fixed point number format to a signed       *
 * short (16 bits).                                                         *
 ****************************************************************************/
inline signed short CMP3Player::MadFixedToSShort(const mad_fixed_t Fixed)
{
	/* A fixed point number is formed of the following bit pattern:
	 *
	 * SWWWFFFFFFFFFFFFFFFFFFFFFFFFFFFF
	 * MSB                          LSB
	 * S ==> Sign (0 is positive, 1 is negative)
	 * W ==> Whole part bits
	 * F ==> Fractional part bits
	 *
	 * This pattern contains MAD_F_FRACBITS fractional bits, one
	 * should alway use this macro when working on the bits of a fixed
	 * point number. It is not guaranteed to be constant over the
	 * different platforms supported by libmad.
	 *
	 * The unsigned short value is formed by the least significant
	 * whole part bit, followed by the 15 most significant fractional
	 * part bits. Warning: this is a quick and dirty way to compute
	 * the 16-bit number, madplay includes much better algorithms.
	 */
	if (Fixed >= MAD_F_ONE)
		return 32767;
	else if (Fixed < -MAD_F_ONE)
		return -32768;
 
	return (signed short)(Fixed >> (MAD_F_FRACBITS + 1 - 16));
}

/****************************************************************************
 * Print human readable informations about an audio MPEG frame.             *
 ****************************************************************************/
void CMP3Player::CreateInfo()
{
	const char	*Layer,
				   *Mode,
				   *Emphasis,
               *Vbr;
	/* Convert the layer number to it's printed representation. */
	switch(m_layer)
	{
		case MAD_LAYER_I:
			Layer="I";
			break;
		case MAD_LAYER_II:
			Layer="II";
			break;
		case MAD_LAYER_III:
			Layer="III";
			break;
		default:
			Layer="?";
			break;
	}

	/* Convert the audio mode to it's printed representation. */
	switch(m_mode)
	{
		case MAD_MODE_SINGLE_CHANNEL:
			Mode="single channel";
			break;
		case MAD_MODE_DUAL_CHANNEL:
			Mode="dual channel";
			break;
		case MAD_MODE_JOINT_STEREO:
			Mode="joint stereo";
			break;
		case MAD_MODE_STEREO:
			Mode="normal stereo";
			break;
		default:
			Mode="unkn. mode";
			break;
	}

	/* Convert the emphasis to it's printed representation. */
	switch(m_emphasis)
	{
		case MAD_EMPHASIS_NONE:
			Emphasis="no";
			break;
		case MAD_EMPHASIS_50_15_US:
			Emphasis="50/15 us";
			break;
		case MAD_EMPHASIS_CCITT_J_17:
			Emphasis="CCITT J.17";
			break;
		default:
			Emphasis="(unexpected emphasis value)";
			break;
	}

   if(m_vbr)
      Vbr="VBR ";
   else
      Vbr="";

//	fprintf(fp,"%s: %lu kb/s audio mpeg layer %s stream %s crc, "
//			"%s with %s emphasis at %d Hz sample rate\n",
//			ProgName,Header->bitrate,Layer,
//			Header->flags&MAD_FLAG_PROTECTION?"with":"without",
//			Mode,Emphasis,Header->samplerate);
   
   sprintf(m_mp3info,"%s%lukbs / %.1fKHz / %s / layer %s", Vbr, m_bitrate/1000,
           (float)m_samplerate/1000, Mode,Layer);
   long secs = m_filesize * 8 / m_bitrate;
   sprintf(m_timeTotal,"%lu:%02lu", secs/60, secs%60);
}

/****************************************************************************
 * Main decoding loop. This is where mad is used.                           *
 ****************************************************************************/
#define INPUT_BUFFER_SIZE	(2*8192)
#define OUTPUT_BUFFER_SIZE	1022*4 /* AVIA_GT_PCM_MAX_SAMPLES-1 */
int CMP3Player::MpegAudioDecoder(FILE *InputFp,int OutputFd)
{
	struct mad_stream	Stream;
	struct mad_frame	Frame;
	struct mad_synth	Synth;
	mad_timer_t			Timer;
	unsigned char		InputBuffer[INPUT_BUFFER_SIZE],
						OutputBuffer[OUTPUT_BUFFER_SIZE],
						*OutputPtr=OutputBuffer;
	const unsigned char	*OutputBufferEnd=OutputBuffer+OUTPUT_BUFFER_SIZE;
	int					Status=0,
						ret;
	unsigned long		FrameCount=0;

	/* First the structures used by libmad must be initialized. */
	mad_stream_init(&Stream);
	mad_frame_init(&Frame);
	mad_synth_init(&Synth);
	mad_timer_reset(&Timer);

	/* Decoding options can here be set in the options field of the
	 * Stream structure.
	 */

	/* This is the decoding loop. */
	state = PLAY;
	do
	{
		if(state==PAUSE)
		{
			// in pause mode do nothing
			usleep(100000);
			continue;
		}
		/* The input bucket must be filled if it becomes empty or if
		 * it's the first execution of the loop.
		 */
		if(Stream.buffer==NULL || Stream.error==MAD_ERROR_BUFLEN)
		{
			size_t			ReadSize,
							Remaining;
			unsigned char	*ReadStart;

			/* {1} libmad may not consume all bytes of the input
			 * buffer. If the last frame in the buffer is not wholly
			 * contained by it, then that frame's start is pointed by
			 * the next_frame member of the Stream structure. This
			 * common situation occurs when mad_frame_decode() fails,
			 * sets the stream error code to MAD_ERROR_BUFLEN, and
			 * sets the next_frame pointer to a non NULL value. (See
			 * also the comment marked {2} bellow.)
			 *
			 * When this occurs, the remaining unused bytes must be
			 * put back at the beginning of the buffer and taken in
			 * account before refilling the buffer. This means that
			 * the input buffer must be large enough to hold a whole
			 * frame at the highest observable bit-rate (currently 448
			 * kb/s). XXX=XXX Is 2016 bytes the size of the largest
			 * frame? (448000*(1152/32000))/8
			 */
			if(Stream.next_frame!=NULL)
			{
				Remaining=Stream.bufend-Stream.next_frame;
				memmove(InputBuffer,Stream.next_frame,Remaining);
				ReadStart=InputBuffer+Remaining;
				ReadSize=INPUT_BUFFER_SIZE-Remaining;
			}
			else
				ReadSize=INPUT_BUFFER_SIZE,
					ReadStart=InputBuffer,
					Remaining=0;
			
			/* Fill-in the buffer. If an error occurs print a message
			 * and leave the decoding loop. If the end of stream is
			 * reached we also leave the loop but the return status is
			 * left untouched.
			 */
			ReadSize=fread(ReadStart,1,ReadSize,InputFp);
			if(ReadSize<=0)
			{
				if(ferror(InputFp))
				{
					fprintf(stderr,"%s: read error on bitstream (%s)\n",
							ProgName,strerror(errno));
					Status=1;
				}
				if(feof(InputFp))
					fprintf(stderr,"%s: end of input stream\n",ProgName);
				break;
			}

			/* Pipe the new buffer content to libmad's stream decoder
             * facility.
			 */
			mad_stream_buffer(&Stream,InputBuffer,ReadSize+Remaining);
			Stream.error=(mad_error)0;
		}

		/* Decode the next mpeg frame. The streams is read from the
		 * buffer, its constituents are break down and stored the the
		 * Frame structure, ready for examination/alteration or PCM
		 * synthesis. Decoding options are carried in the Frame
		 * structure from the Stream structure.
		 *
		 * Error handling: mad_frame_decode() returns a non zero value
		 * when an error occurs. The error condition can be checked in
		 * the error member of the Stream structure. A mad error is
		 * recoverable or fatal, the error status is checked with the
		 * MAD_RECOVERABLE macro.
		 *
		 * {2} When a fatal error is encountered all decoding
		 * activities shall be stopped, except when a MAD_ERROR_BUFLEN
		 * is signaled. This condition means that the
		 * mad_frame_decode() function needs more input to achieve
		 * it's work. One shall refill the buffer and repeat the
		 * mad_frame_decode() call. Some bytes may be left unused at
		 * the end of the buffer if those bytes forms an incomplete
		 * frame. Before refilling, the remainign bytes must be moved
		 * to the begining of the buffer and used for input for the
q		 * next mad_frame_decode() invocation. (See the comments marked
		 * {1} earlier for more details.)
		 *
		 * Recoverable errors are caused by malformed bit-streams, in
		 * this case one can call again mad_frame_decode() in order to
		 * skip the faulty part and re-sync to the next frame.
		 */
		// decode 'FRAMES_TO_PLAY' frames each 'FRAMES_TO_SKIP' frames in ff/rev mode 
		if( (state!=FF && state!=REV) || FrameCount % FRAMES_TO_SKIP < FRAMES_TO_PLAY )
			ret=mad_frame_decode(&Frame,&Stream);
		else if(state==FF) // in FF mode just decode the header, this sets bufferptr to next frame and also gives stats about the frame for totals
			ret=mad_header_decode(&Frame.header,&Stream);
		else
		{ //REV
			// Jump back 
			long bytesBack = (Stream.bufend - Stream.this_frame) + ((ftell(InputFp)+Stream.this_frame-Stream.bufend) / FrameCount)*(FRAMES_TO_SKIP + FRAMES_TO_PLAY);
			if (fseek(InputFp, -1*(bytesBack), SEEK_CUR)!=0)
			{
				// Reached beginning
				fseek(InputFp, 0, SEEK_SET);
				Timer.fraction=0;
				Timer.seconds=0;
				FrameCount=0;
				state=PLAY;
			}
			else
			{
				// Calculate timer
				mad_timer_t m;
				mad_timer_set(&m, 0, 32 * MAD_NSBSAMPLES(&Frame.header) *(FRAMES_TO_SKIP + FRAMES_TO_PLAY), Frame.header.samplerate);
				Timer.seconds -= m.seconds;
				if(Timer.fraction < m.fraction)
				{
					Timer.seconds--;
					Timer.fraction+= MAD_TIMER_RESOLUTION - m.fraction;
				}
				else
					Timer.fraction-= m.fraction;
				// in case we calculated wrong...
				if(Timer.seconds < 0)
				{
					Timer.seconds=0;
					Timer.fraction=0;
				}
				FrameCount-=FRAMES_TO_SKIP + FRAMES_TO_PLAY;
			}
			Stream.buffer=NULL;
			Stream.next_frame=NULL;
			continue;
		}

		if(ret)
		{
			if(MAD_RECOVERABLE(Stream.error))
			{
				// no errrors in FF mode
				if(state!=FF && state!=REV)
				{
					fprintf(stderr,"%s: recoverable frame level error (%s)\n",
						ProgName,MadErrorString(&Stream));
					fflush(stderr);
				 }
				continue;
			}
			else
				if(Stream.error==MAD_ERROR_BUFLEN)
					continue;
				else
				{
					fprintf(stderr,"%s: unrecoverable frame level error (%s).\n",
						ProgName,MadErrorString(&Stream));
					Status=1;
					break;
				}
		}

		/* On first frame set DSP & save header info
		 * The first frame is representative of the entire
		 * stream.
		 */
		FrameCount++;
		if (FrameCount == 1)
		{
			if (SetDSP(OutputFd, &Frame.header))
			{
				Status=1;
				break;
			}
			m_samplerate=Frame.header.samplerate;
			m_bitrate=Frame.header.bitrate;
			m_mode=Frame.header.mode;
			m_layer=Frame.header.layer;
			m_emphasis=Frame.header.emphasis;
			m_vbr=false;
			CreateInfo();
		}
		else
		{
			if (m_bitrate != Frame.header.bitrate)
			{
				m_vbr = true;
				m_bitrate -= m_bitrate / FrameCount;
				m_bitrate += Frame.header.bitrate / FrameCount;
				CreateInfo();
			}
		}

		/* Accounting. The computed frame duration is in the frame
		 * header structure. It is expressed as a fixed point number
		 * whole data type is mad_timer_t. It is different from the
		 * samples fixed point format and unlike it, it can't directly
		 * be added or substracted. The timer module provides several
		 * functions to operate on such numbers. Be careful there, as
		 * some functions of mad's timer module receive some of their
		 * mad_timer_t arguments by value!
		 */
		mad_timer_add(&Timer,Frame.header.duration);
		mad_timer_string(Timer,m_timePlayed,"%lu:%02lu",
                       MAD_UNITS_MINUTES,MAD_UNITS_MILLISECONDS,0);

				
		// decode 5 frames each 75 frames in ff mode
		if( state!=FF || FrameCount % 75 < 5)
		{
			
			/* Once decoded the frame is synthesized to PCM samples. No errors
			 * are reported by mad_synth_frame();
			 */
			mad_synth_frame(&Synth, &Frame);
			
			
			/* Synthesized samples must be converted from mad's fixed
			 * point number to the consumer format. Here we use signed
			 * 16 bit native endian integers on two channels. Integer samples
			 * are temporarily stored in a buffer that is flushed when
			 * full.
			 */

			if (MAD_NCHANNELS(&Frame.header) == 2)
			{
				mad_fixed_t * leftchannel = Synth.pcm.samples[0];
				mad_fixed_t * rightchannel = Synth.pcm.samples[1];
				
				while (Synth.pcm.length-- > 0)
				{
					/* Left channel */
					*((signed short *)OutputPtr) = MadFixedToSShort(*(leftchannel++));
					
					/* Right channel */
					*(((signed short *)OutputPtr) + 1) = MadFixedToSShort(*(rightchannel++));
				
					OutputPtr += 4;
					
					/* Flush the buffer if it is full. */
					if (OutputPtr == OutputBufferEnd)
					{
						if (write(OutputFd, OutputBuffer, OUTPUT_BUFFER_SIZE) != OUTPUT_BUFFER_SIZE)
						{
							fprintf(stderr,"%s: PCM write error (%s).\n", ProgName, strerror(errno));
							Status = 2;
							break;
						}
						
						OutputPtr = OutputBuffer;
					}
				}
			}
			else
			{
				mad_fixed_t * leftchannel = Synth.pcm.samples[0];
				
				while (Synth.pcm.length-- > 0)
				{
					/* Left channel => copy to right channel */

					*(((signed short *)OutputPtr) + 1) = *((signed short *)OutputPtr) = MadFixedToSShort(*(leftchannel++));
				
					OutputPtr += 4;
					
					/* Flush the buffer if it is full. */
					if (OutputPtr == OutputBufferEnd)
					{
						if (write(OutputFd, OutputBuffer, OUTPUT_BUFFER_SIZE) != OUTPUT_BUFFER_SIZE)
						{
							fprintf(stderr,"%s: PCM write error (%s).\n", ProgName, strerror(errno));
							Status = 2;
							break;
						}
						
						OutputPtr = OutputBuffer;
					}
				}
			}
		}
	}while(do_loop);

	/* Mad is no longer used, the structures that were initialized must
     * now be cleared.
	 */
	mad_synth_finish(&Synth);
	mad_frame_finish(&Frame);
	mad_stream_finish(&Stream);

	/* If the output buffer is not empty and no error occured during
     * the last write, then flush it.
	 */
	if(OutputPtr!=OutputBuffer && Status!=2)
	{
		ssize_t	BufferSize=OutputPtr-OutputBuffer;

		if(write(OutputFd, OutputBuffer, BufferSize)!=BufferSize)
  		{
			fprintf(stderr,"%s: PCM write error (%s).\n",
					ProgName,strerror(errno));
			Status=2;
		}
	}

	/* Accounting report if no error occured. */
	if(!Status)
	{
		/* The duration timer is converted to a human readable string
		 * with the versatile but still constrained mad_timer_string()
		 * function, in a fashion not unlike strftime(). The main
		 * difference is that the timer is break-down into several
		 * values according some of it's arguments. The units and
		 * fracunits arguments specify the intended conversion to be
		 * executed.
		 *
		 * The conversion unit (MAD_UNIT_MINUTES in our example) also
		 * specify the order and kind of conversion specifications
		 * that can be used in the format string.
		 *
		 * It is best to examine mad's timer.c source-code for details
		 * of the available units, fraction of units, their meanings,
		 * the format arguments, etc.
		 */
		mad_timer_string(Timer,m_timePlayed,"%lu:%02lu",
						 MAD_UNITS_MINUTES,MAD_UNITS_MILLISECONDS,0);
//		fprintf(stderr,"%s: %lu frames decoded (%s).\n",
//				ProgName,FrameCount,Buffer);
	}

	/* That's the end of the world (in the H. G. Wells way). */
	return(Status);
}

bool  CMP3Player::SetDSP(int soundfd, struct mad_header *Header)
{
	int fmt = AFMT_S16_NE; /* signed 16 bit native endian */
	 unsigned int dsp_speed;
	 unsigned int channels;
	 bool crit_error=false;

	 dsp_speed = Header->samplerate;
	 // Single channel is transformed to dual channel in MpegAudioDecoder, there for set oss channels to 2 always
	 channels=2;
    
	 if (::ioctl(soundfd, SNDCTL_DSP_RESET))
		 printf("reset failed\n");
	 if(::ioctl(soundfd, SNDCTL_DSP_SETFMT, &fmt))
		 printf("setfmt failed\n");
	 if(::ioctl(soundfd, SNDCTL_DSP_CHANNELS, &channels))
		 printf("channel set failed\n");
	 if (dsp_speed != m_samplerate)
	 {
		// mute audio to reduce pops when changing samplerate (avia_reset)
		int adec;
		bool was_muted = avs_mute(true);

		if (::ioctl(soundfd, SNDCTL_DSP_SPEED, &dsp_speed))
		{
			printf("speed set failed\n");
			crit_error=true;
	 	}
	 	else
	 		m_samplerate = dsp_speed;

		// disable spdif output
		if ((adec=::open (ADEC, O_RDWR | O_NONBLOCK)) >= 0)
		{
			if (::ioctl(adec, AUDIO_SET_DA_IEC_DISABLE, 1) < 0)
				perror("AUDIO_SET_DA_IEC_DISABLE");
			close(adec);
		}

		usleep(400000);
		if (!was_muted)
			avs_mute(false);
	 }
//		  printf("Debug: SNDCTL_DSP_RESET %d / SNDCTL_DSP_SPEED %d / SNDCTL_DSP_CHANNELS %d / SNDCTL_DSP_SETFMT %d\n",
//					SNDCTL_DSP_RESET, SNDCTL_DSP_SPEED, SNDCTL_DSP_CHANNELS, SNDCTL_DSP_SETFMT);
		  return crit_error;
}

void CMP3Player::stop()
{
	do_loop = false;
	pthread_join(thrPlay,NULL);
}
void CMP3Player::pause()
{
   if(state==PLAY || state==FF || state==REV)
      state=PAUSE;
   else if(state==PAUSE)
      state=PLAY;
}
void CMP3Player::ff()
{
   if(state==PLAY || state==PAUSE || state==REV)
      state=FF;
   else if(state==FF)
      state=PLAY;
}
void CMP3Player::rev()
{
   if(state==PLAY || state==PAUSE || state==FF)
      state=REV;
   else if(state==REV)
      state=PLAY;
}
CMP3Player* CMP3Player::getInstance()
{
	static CMP3Player* mp3player = NULL;
	if(mp3player == NULL)
	{
		mp3player = new CMP3Player();
	}
	return mp3player;
}

void* CMP3Player::PlayThread(void * filename)
{
	FILE *fp;
	int soundfd;
	soundfd=::open("/dev/sound/dsp",O_WRONLY);
	if (soundfd != -1)
	{
		fp = ::fopen((char *)filename,"r");
		if (fp!=NULL)
		{
         /* Calc file length */
         fseek(fp, 0, SEEK_END);
         CMP3Player::getInstance()->m_filesize=ftell(fp);
         rewind(fp);
			/* Decode stdin to stdout. */
			int Status = CMP3Player::getInstance()->MpegAudioDecoder(fp,soundfd);
			if(Status > 0)
				fprintf(stderr,"Error %d occured during decoding.\n",Status);
	
			fclose(fp);
		}
		else
			fprintf(stderr,"Error opening file %s\n",(char *) filename);
		close(soundfd);
	}
	else
		fprintf(stderr,"Error opening /dev/sound/dsp\n");
		
	CMP3Player::getInstance()->state = STOP;
	pthread_exit(0);
	return NULL;
}

bool CMP3Player::play(const char *filename)
{
	stop();
	strcpy(m_mp3info,"");
	strcpy(m_timePlayed,"0:00");
	strcpy(m_timeTotal,"0:00");
	do_loop = true;
	state = PLAY;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	struct sched_param param;
	pthread_attr_setschedpolicy(&attr, SCHED_RR);
	param.sched_priority=1;
	pthread_attr_setschedparam(&attr, &param);
	usleep(100000); // give the event thread some time to handle his stuff
	                // without this sleep there were duplicated events...
	if (pthread_create (&thrPlay, &attr, PlayThread,(void *) filename) != 0 )
	{
		perror("mp3play: pthread_create(PlayThread)");
		return false;
	}
	return true;
}

CMP3Player::CMP3Player()
{
	init();
}

void CMP3Player::init()
{
	state = STOP;
	m_samplerate=0;
}

bool CMP3Player::avs_mute(bool mute)
{
	int fd, a, b=AVS_UNMUTE;
	a = mute ? AVS_MUTE : AVS_UNMUTE;
	if ((fd = open(AVS_DEVICE, O_RDWR)) < 0)
		 perror("[CMP3Player::avs_mute] " AVS_DEVICE);
	 else 
	 {
		 if (ioctl(fd, AVSIOGMUTE, &b) < 0)
			 perror("[CMP3Player::avs_mute] AVSIOSMUTE");
		 if(a!=b)
		 {
			 if (ioctl(fd, AVSIOSMUTE, &a) < 0)
				 perror("[CMP3Player::avs_mute] AVSIOSMUTE");
		 }
		 close(fd);
	 }
	 return (b==AVS_MUTE);
}

