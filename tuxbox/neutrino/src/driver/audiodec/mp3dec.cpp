/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2002 Bjoern Kalkbrenner <terminar@cyberphoria.org>
   (C) 2002,2003,2004 Zwen <Zwen@tuxbox.org>
   
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
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <mad.h>
#include <sstream>
#include <driver/audiodec/mp3dec.h>
#include <driver/netfile.h>
#include <linux/soundcard.h>
#include <assert.h>

#include <id3tag.h>

/* libid3tag extension: This is neccessary in order to call fclose
   on the file. Normally libid3tag closes the file implicit.
   For the netfile extension to work properly netfiles fclose must be called.
   To close an id3 file (usually by calling id3_file_close) without fclosing it,
   call following i3_finish_file function. It's just a copy of libid3tags
   finish_file function. */
extern "C"
{
void id3_tag_addref(struct id3_tag *);
void id3_tag_delref(struct id3_tag *);
struct filetag {
	struct id3_tag *tag;
	unsigned long location;
	id3_length_t length;
};
struct id3_file {
	FILE *iofile;
	enum id3_file_mode mode;
	char *path;
	int flags;
	struct id3_tag *primary;
	unsigned int ntags;
	struct filetag *tags;
};
void id3_finish_file(struct id3_file* file);
}

// Frames to skip in ff/rev mode
#define FRAMES_TO_SKIP 75 
// nr of frames to play after skipping in rev/ff mode
#define FRAMES_TO_PLAY 5

#define ProgName "CMP3Dec"

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
const char *CMP3Dec::MadErrorString(const struct mad_stream *Stream)
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
inline signed short CMP3Dec::MadFixedToSShort(const mad_fixed_t Fixed)
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
void CMP3Dec::CreateInfo()
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

	CAudioPlayer::getInstance()->m_MetaData.type = CAudioMetaData::MP3;
	CAudioPlayer::getInstance()->m_MetaData.bitrate = m_bitrate;
	CAudioPlayer::getInstance()->m_MetaData.samplerate = m_samplerate;
	CAudioPlayer::getInstance()->m_MetaData.total_time = m_filesize * 8 / m_bitrate;
	std::stringstream ss;
	ss << "MPEG Layer " << Layer << " / " << Mode;
	CAudioPlayer::getInstance()->m_MetaData.type_info = ss.str();
	CAudioPlayer::getInstance()->m_MetaData.changed=true;
}

/****************************************************************************
 * Main decoding loop. This is where mad is used.                           *
 ****************************************************************************/
#define INPUT_BUFFER_SIZE	(2*8192)
#define OUTPUT_BUFFER_SIZE	1022*4 /* AVIA_GT_PCM_MAX_SAMPLES-1 */
int CMP3Dec::Decoder(FILE *InputFp,int OutputFd, State* state)
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

	/* Calc file length */
	fseek(InputFp, 0, SEEK_END);
	m_filesize=ftell(InputFp);
	rewind(InputFp);
	
	/* First the structures used by libmad must be initialized. */
	mad_stream_init(&Stream);
	mad_frame_init(&Frame);
	mad_synth_init(&Synth);
	mad_timer_reset(&Timer);

	/* Decoding options can here be set in the options field of the
	 * Stream structure.
	 */

	/* This is the decoding loop. */
	do
	{
		if(*state==PAUSE)
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
		if( (*state!=FF && 
			  *state!=REV) || 
			 FrameCount % FRAMES_TO_SKIP < FRAMES_TO_PLAY )
			ret=mad_frame_decode(&Frame,&Stream);
		else if(*state==FF) // in FF mode just decode the header, this sets bufferptr to next frame and also gives stats about the frame for totals
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
				*state=PLAY;
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
				CAudioPlayer::getInstance()->setTimePlayed(Timer.seconds);
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
				if(*state!=FF && 
					*state!=REV)
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
			if (SetDSP(OutputFd, AFMT_S16_NE, Frame.header.samplerate, 2))
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

		// if played time was modified from outside, take this value...
		if(CAudioPlayer::getInstance()->getTimePlayed()!=Timer.seconds)
		{
			mad_timer_reset(&Timer);
			Timer.seconds = CAudioPlayer::getInstance()->getTimePlayed();
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
		//mad_timer_string(Timer,m_timePlayed,"%lu:%02lu",
      //                 MAD_UNITS_MINUTES,MAD_UNITS_MILLISECONDS,0);
		CAudioPlayer::getInstance()->setTimePlayed(Timer.seconds);

				
		// decode 5 frames each 75 frames in ff mode
		if( *state!=FF || FrameCount % 75 < 5)
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
	}while(*state!=STOP_REQ);

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
	   //		mad_timer_string(Timer,m_timePlayed,"%lu:%02lu",
		//				 MAD_UNITS_MINUTES,MAD_UNITS_MILLISECONDS,0);
		CAudioPlayer::getInstance()->setTimePlayed(Timer.seconds);

//		      fprintf(stderr,"%s: %lu frames decoded (%s).\n",
//				ProgName,FrameCount,Buffer);
	}

	/* That's the end of the world (in the H. G. Wells way). */
	fclose(InputFp);
	return(Status);
}

CMP3Dec* CMP3Dec::getInstance()
{
	static CMP3Dec* MP3Dec = NULL;
	if(MP3Dec == NULL)
	{
		MP3Dec = new CMP3Dec();
	}
	return MP3Dec;
}

bool CMP3Dec::GetMetaData(FILE *in, bool nice, CAudioMetaData* m)
{
	GetMP3Info(in, nice, m);
	bool fileClosed=GetID3(in, m);
	if(!fileClosed)
		fclose(in);
	return true;
}

#define BUFFER_SIZE 2100
void CMP3Dec::GetMP3Info(FILE* in, bool nice, CAudioMetaData *m)
{
	struct mad_stream	Stream;
	struct mad_header	Header;
	unsigned char		InputBuffer[BUFFER_SIZE];
	int ReadSize;
	int filesize;

	ReadSize=fread(InputBuffer,1,BUFFER_SIZE,in);

	if(nice)
		usleep(15000);
	bool foundSyncmark=true;
	// Check for sync mark (some encoder produce data befor 1st frame in mp3 stream)
	if(InputBuffer[0]!=0xff || (InputBuffer[1]&0xe0)!=0xe0)
	{
		foundSyncmark=false;
		//skip to first sync mark
		int n=0,j=0;
		while((InputBuffer[n]!=0xff || (InputBuffer[n+1]&0xe0)!=0xe0) && ReadSize > 1)
		{
			n++;
			j++;
			if(n > ReadSize-2)
			{
				j--;
				n=0;
				fseek(in, -1, SEEK_CUR);
				ReadSize=fread(InputBuffer,1,BUFFER_SIZE,in);
				if(nice)
					usleep(15000);
			}
		}
		if(ReadSize > 1)
		{
			fseek(in, j, SEEK_SET);
			ReadSize=fread(InputBuffer,1,BUFFER_SIZE,in);
			if(nice)
				usleep(15000);
			foundSyncmark=true;
		}
	}
	if(foundSyncmark)
	{
//      printf("found syncmark...\n");
		mad_stream_init(&Stream);
		mad_stream_buffer(&Stream,InputBuffer,ReadSize);
		mad_header_decode(&Header,&Stream);

		m->vbr=false;

		if(nice)
			usleep(15000);
		mad_stream_finish(&Stream);
		// filesize
		fseek(in, 0, SEEK_END);
		filesize=ftell(in);

		m->total_time = (Header.bitrate != 0) ? (filesize * 8 / Header.bitrate) : 0;
	}
	else
	{
		m->total_time=0;
	}
}


//------------------------------------------------------------------------
bool CMP3Dec::GetID3(FILE* in, CAudioMetaData* m)
{
	unsigned int i;
	struct id3_frame const *frame;
	id3_ucs4_t const *ucs4;
	id3_utf8_t *utf8;
	char const spaces[] = "          ";
	
	struct 
		{
		char const *id;
		char const *name;
	} const info[] = 
		{
			{ ID3_FRAME_TITLE,  "Title"},
			{ "TIT3",           0},	 /* Subtitle */
			{ "TCOP",           0,},  /* Copyright */
			{ "TPRO",           0,},  /* Produced */
			{ "TCOM",           "Composer"},
			{ ID3_FRAME_ARTIST, "Artist"},
			{ "TPE2",           "Orchestra"},
			{ "TPE3",           "Conductor"},
			{ "TEXT",           "Lyricist"},
			{ ID3_FRAME_ALBUM,  "Album"},
			{ ID3_FRAME_YEAR,   "Year"},
			{ ID3_FRAME_TRACK,  "Track"},
			{ "TPUB",           "Publisher"},
			{ ID3_FRAME_GENRE,  "Genre"},
			{ "TRSN",           "Station"},
			{ "TENC",           "Encoder"}
		};

	/* text information */

	struct id3_file *id3file = id3_file_fdopen(fileno(in), ID3_FILE_MODE_READONLY);
	if(id3file == 0)
		printf("error open id3 file\n");
	else
	{
		id3_tag *tag=id3_file_tag(id3file);
		if(tag)
		{
			for(i = 0; i < sizeof(info) / sizeof(info[0]); ++i)
			{
				union id3_field const *field;
				unsigned int nstrings, namelen, j;
				char const *name;

				frame = id3_tag_findframe(tag, info[i].id, 0);
				if(frame == 0)
					continue;

				field    = &frame->fields[1];
				nstrings = id3_field_getnstrings(field);

				name = info[i].name;
				namelen = name ? strlen(name) : 0;
				assert(namelen < sizeof(spaces));

				for(j = 0; j < nstrings; ++j)
				{
					ucs4 = id3_field_getstrings(field, j);
					assert(ucs4);

					if(strcmp(info[i].id, ID3_FRAME_GENRE) == 0)
						ucs4 = id3_genre_name(ucs4);

					utf8 = id3_ucs4_utf8duplicate(ucs4);
					if (utf8 == NULL)
						goto fail;

					if (j == 0 && name)
					{
						if(strcmp(name,"Title") == 0)
							m->title = (char *) utf8;
						if(strcmp(name,"Artist") == 0)
							m->artist = (char *) utf8;
						if(strcmp(name,"Year") == 0)
							m->date = (char *) utf8;
						if(strcmp(name,"Album") == 0)
							m->album = (char *) utf8;
						if(strcmp(name,"Genre") == 0)
							m->genre = (char *) utf8;
					}
					else
					{
						if(strcmp(info[i].id, "TCOP") == 0 || strcmp(info[i].id, "TPRO") == 0)
						{
							//printf("%s  %s %s\n", spaces, (info[i].id[1] == 'C') ? ("Copyright (C)") : ("Produced (P)"), latin1);
						}
						//else
						//printf("%s  %s\n", spaces, latin1);
					}

					free(utf8);
				}
			}

#ifdef INCLUDE_UNUSED_STUFF
			/* comments */

			i = 0;
			while((frame = id3_tag_findframe(tag, ID3_FRAME_COMMENT, i++)))
			{
				id3_utf8_t *ptr, *newline;
				int first = 1;

				ucs4 = id3_field_getstring(&frame->fields[2]);
				assert(ucs4);

				if(*ucs4)
					continue;

				ucs4 = id3_field_getfullstring(&frame->fields[3]);
				assert(ucs4);

				utf8 = id3_ucs4_utf8duplicate(ucs4);
				if (utf8 == 0)
					goto fail;

				ptr = utf8;
				while(*ptr)
				{
					newline = (id3_utf8_t *) strchr((char*)ptr, '\n');
					if(newline)
						*newline = 0;

					if(strlen((char *)ptr) > 66)
					{
						id3_utf8_t *linebreak;

						linebreak = ptr + 66;

						while(linebreak > ptr && *linebreak != ' ')
							--linebreak;

						if(*linebreak == ' ')
						{
							if(newline)
								*newline = '\n';

							newline = linebreak;
							*newline = 0;
						}
					}

					if(first)
					{
						char const *name;
						unsigned int namelen;

						name    = "Comment";
						namelen = strlen(name);
						assert(namelen < sizeof(spaces));
						mp3->Comment = (char *) ptr;
						//printf("%s%s: %s\n", &spaces[namelen], name, ptr);
						first = 0;
					}
					else
						//printf("%s  %s\n", spaces, ptr);

						ptr += strlen((char *) ptr) + (newline ? 1 : 0);
				}

				free(utf8);
				break;
			}
#endif
			id3_tag_delete(tag);
		}
		else
			printf("error open id3 tag\n");

		fclose(in);
		id3_finish_file(id3file);
		return true;
	}
	return false;
	if(0)
	{
	fail:
		printf("id3: not enough memory to display tag");
		return false;
	}
}

// this is a copy of static libid3tag function "finish_file"
// which cannot be called from outside
void id3_finish_file(struct id3_file* file)
{
	unsigned int i;
	
	if (file->path)
		free(file->path);

	if (file->primary) {
		id3_tag_delref(file->primary);
		id3_tag_delete(file->primary);
	}
	
	for (i = 0; i < file->ntags; ++i) {
		struct id3_tag *tag;
		
		tag = file->tags[i].tag;
		if (tag) {
			id3_tag_delref(tag);
			id3_tag_delete(tag);
		}
	}
	
	if (file->tags)
		free(file->tags);
	
	free(file);
}	

