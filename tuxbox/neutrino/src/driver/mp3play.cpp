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


#include "mp3play.h"

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
 * Converts a sample from mad's fixed point number format to an unsigned	*
 * short (16 bits).															*
 ****************************************************************************/
unsigned short CMP3Player::MadFixedToUshort(mad_fixed_t Fixed)
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
		Fixed = MAD_F_ONE - 1;
	else if (Fixed < -MAD_F_ONE)
		Fixed = -MAD_F_ONE;
 
	Fixed>>=MAD_F_FRACBITS-15;

	return((unsigned short)Fixed);
}

/****************************************************************************
 * Print human readable informations about an audio MPEG frame.				*
 ****************************************************************************/
int CMP3Player::PrintFrameInfo(FILE *fp, struct mad_header *Header)
{
	const char	*Layer,
				*Mode,
				*Emphasis;

	/* Convert the layer number to it's printed representation. */
	switch(Header->layer)
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
			Layer="(unexpected layer value)";
			break;
	}

	/* Convert the audio mode to it's printed representation. */
	switch(Header->mode)
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
			Mode="(unexpected mode value)";
			break;
	}

	/* Convert the emphasis to it's printed representation. */
	switch(Header->emphasis)
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

	fprintf(fp,"%s: %lu kb/s audio mpeg layer %s stream %s crc, "
			"%s with %s emphasis at %d Hz sample rate\n",
			ProgName,Header->bitrate,Layer,
			Header->flags&MAD_FLAG_PROTECTION?"with":"without",
			Mode,Emphasis,Header->samplerate);
	fprintf(fp,"This roxx!;)\n");
   sprintf(m_mp3info,"%lukbs / %.1fKHz / %s / layer %s",Header->bitrate/1000,(float)Header->samplerate/1000,
           Mode,Layer);
	return(ferror(fp));
}

/****************************************************************************
 * Main decoding loop. This is where mad is used.							*
 ****************************************************************************/
#define INPUT_BUFFER_SIZE	(5*8192)
#define OUTPUT_BUFFER_SIZE	8192 /* Must be an integer multiple of 4. */
int CMP3Player::MpegAudioDecoder(FILE *InputFp,FILE *OutputFp)
{
	printf("Start audio decoder\n");

	struct mad_stream	Stream;
	struct mad_frame	Frame;
	struct mad_synth	Synth;
	mad_timer_t			Timer;
	unsigned char		InputBuffer[INPUT_BUFFER_SIZE],
						OutputBuffer[OUTPUT_BUFFER_SIZE],
						*OutputPtr=OutputBuffer;
	const unsigned char	*OutputBufferEnd=OutputBuffer+OUTPUT_BUFFER_SIZE;
	int					Status=0,
						i;
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
	do_loop = true;
	state = PLAY;
	do
	{
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
		 * next mad_frame_decode() invocation. (See the comments marked
		 * {1} earlier for more details.)
		 *
		 * Recoverable errors are caused by malformed bit-streams, in
		 * this case one can call again mad_frame_decode() in order to
		 * skip the faulty part and re-sync to the next frame.
		 */
		if(mad_frame_decode(&Frame,&Stream))
			if(MAD_RECOVERABLE(Stream.error))
			{
				fprintf(stderr,"%s: recoverable frame level error (%s)\n",
						ProgName,MadErrorString(&Stream));
				fflush(stderr);
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

		/* The characteristics of the stream's first frame is printed
		 * on stderr. The first frame is representative of the entire
		 * stream.
		 */
		if(FrameCount==0)
			if(PrintFrameInfo(stderr,&Frame.header))
			{
				Status=1;
				break;
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
		FrameCount++;
		mad_timer_add(&Timer,Frame.header.duration);
				
		/* Once decoded the frame is synthesized to PCM samples. No errors
		 * are reported by mad_synth_frame();
		 */
		mad_synth_frame(&Synth,&Frame);
		

		/* Synthesized samples must be converted from mad's fixed
		 * point number to the consumer format. Here we use unsigned
		 * 16 bit big endian integers on two channels. Integer samples
		 * are temporarily stored in a buffer that is flushed when
		 * full.
		 */
		for(i=0;i<Synth.pcm.length;i++)
		{
			unsigned short	Sample;

			/* Left channel */
			Sample=MadFixedToUshort(Synth.pcm.samples[0][i]);
			*(OutputPtr++)=Sample>>8;
			*(OutputPtr++)=Sample&0xff;

			/* Right channel. If the decoded stream is monophonic then
			 * the right output channel is the same as the left one.
			 */
			if(MAD_NCHANNELS(&Frame.header)==2)
				Sample=MadFixedToUshort(Synth.pcm.samples[1][i]);
			*(OutputPtr++)=Sample>>8;
			*(OutputPtr++)=Sample&0xff;

			/* Flush the buffer if it is full. */
			if(OutputPtr==OutputBufferEnd)
			{
				if(fwrite(OutputBuffer,1,OUTPUT_BUFFER_SIZE,OutputFp)!=OUTPUT_BUFFER_SIZE)
				{
					fprintf(stderr,"%s: PCM write error (%s).\n",
						ProgName,strerror(errno));
					Status=2;
					break;
				}
			
				OutputPtr=OutputBuffer;
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
		size_t	BufferSize=OutputPtr-OutputBuffer;

		if(fwrite(OutputBuffer,1,BufferSize,OutputFp)!=BufferSize)
  		{
			fprintf(stderr,"%s: PCM write error (%s).\n",
					ProgName,strerror(errno));
			Status=2;
		}
	}

	/* Accounting report if no error occured. */
	if(!Status)
	{
		char	Buffer[80];

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
		mad_timer_string(Timer,Buffer,"%lu:%02lu.%03u",
						 MAD_UNITS_MINUTES,MAD_UNITS_MILLISECONDS,0);
		fprintf(stderr,"%s: %lu frames decoded (%s).\n",
				ProgName,FrameCount,Buffer);
	}

	/* That's the end of the world (in the H. G. Wells way). */
	state = STOP;
	return(Status);
}

/****************************************************************************
 * Program entry point.														*
 ****************************************************************************/
void  CMP3Player::ResetDSP(FILE *soundfd)
{
    long dsp_speed=44100;
    long channels=2;
    long fmt=AFMT_S16_BE;
    
        if (::ioctl(fileno(soundfd), SNDCTL_DSP_RESET))
                printf("reset failed\n");
        if (::ioctl(fileno(soundfd), SNDCTL_DSP_SPEED, &dsp_speed))
                printf("speed set failed\n");
        if(::ioctl(fileno(soundfd), SNDCTL_DSP_CHANNELS, &channels))
                printf("channel set failed\n");
        if(::ioctl(fileno(soundfd), SNDCTL_DSP_SETFMT, &fmt))
                printf("setfmt failed\n");
}

void CMP3Player::stop()
{
	do_loop = false;
//	pthread_join(thrPlay,NULL);
	pthread_cancel(thrPlay);
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
	FILE *fp = fopen((char *)filename,"r");
	FILE *soundfd=::fopen("/dev/sound/dsp","w");
	CMP3Player::getInstance()->ResetDSP(soundfd);

	/* Decode stdin to stdout. */
	int Status = CMP3Player::getInstance()->MpegAudioDecoder(fp,soundfd);
	if(Status > 0)
		fprintf(stderr,"Error %d occured during decoding.\n",Status);
	
	fclose(fp);
	fclose(soundfd);
	pthread_exit(0);
	return NULL;
}

bool CMP3Player::play(const char *filename)
{
	ProgName=__FILE__;
	if(true)
	{
		stop();
		CMP3Player::getInstance()->state = PLAY;
		if (pthread_create (&thrPlay, NULL, PlayThread,(void *) filename) != 0 )
		{
			perror("mp3play: pthread_create(PlayThread)");
			return false;
		}
	}
	else
		PlayThread((void *) filename);
	return true;

/* 	
	if(Status)
		fprintf(stderr,"%s: an error occured during decoding.\n",ProgName);

		fclose(fp);
		fclose(soundfd);
*/		
	/* All done. */
//	return(Status);

}

CMP3Player::CMP3Player()
{
	init();
}

void CMP3Player::init()
{
	state = STOP;
	strcpy(m_mp3info,"");
}
