#define VIDEO_DEV "/dev/dvb/adapter0/video0"
#define AUDIO_DEV "/dev/dvb/adapter0/audio0"
#define DEMUX_DEV "/dev/dvb/adapter0/demux0"
  	
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <memory.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

// #define OLD_VBI

#undef strcpy
#undef strcmp
#undef strlen
#undef strnicmp
#undef strncmp

#include <linux/dvb/dmx.h>
#include <linux/dvb/video.h>
#include <linux/dvb/audio.h>

#ifdef OLD_VBI  // oldvbi header
#include <dbox/avia_gt_vbi.h>
#endif

typedef unsigned char __u8;

#include <lib/dvb/decoder.h>
#include <lib/base/eerror.h>

decoderParameters Decoder::current;
decoderParameters Decoder::parms;
int Decoder::fd::video;
int Decoder::fd::audio;
int Decoder::fd::demux_video;
int Decoder::fd::demux_audio;
int Decoder::fd::demux_pcr;
int Decoder::fd::demux_vtxt;

static void SetECM(int vpid, int apid, int ecmpid, int emmpid, int pmtpid, int casystemid, int descriptor_length, __u8 *descriptors)
{
	static int lastpid=-1;

	if (lastpid != -1)
	{
		kill(lastpid, SIGKILL);
		waitpid(lastpid, 0, 0);
		lastpid=-1;
	}

	if (!descriptor_length)
		return;

	char buffer[6][5];
	sprintf(buffer[0], "%x", vpid);
	sprintf(buffer[1], "%x", apid);
	sprintf(buffer[2], "%x", ecmpid);
	sprintf(buffer[3], "%x", emmpid);
	sprintf(buffer[4], "%x", pmtpid);
	sprintf(buffer[5], "%x", casystemid);
	
	char descriptor[2048];
	
	for (int i=0; i<descriptor_length; i++)
		sprintf(descriptor+i*2, "%02x", descriptors[i]);
	
	switch (lastpid=fork())
	{
	case -1:
		eDebug("fork failed!");
		return;
	case 0:
	{ 
#if 0
		close(0);
		close(1);
		close(2);
#endif

		if (execlp("camd", "camd", buffer[0], buffer[1], buffer[4], descriptor, 0)<0)
			eDebug("camd");

		exit(0);
		break;
	}
	}
}

int Decoder::Initialize()
{
	parms.vpid = parms.apid = parms.tpid = parms.pcrpid = parms.ecmpid = -1;
	parms.audio_type=0;
	parms.emmpid=-2;
	parms.recordmode=0;
	parms.descriptor_length=0;
	current=parms;
	fd.video = fd.audio = fd.demux_video = fd.demux_audio =	fd.demux_pcr = fd.demux_vtxt = -1;
	return 0;
}

void Decoder::Close()
{	
	Flush();
	eDebug("fd video = %d, audio = %d, demux_video = %d, demux_audio = %d, demux_pcr = %d, demux_vtxt = %d",
					fd.video, fd.audio, fd.demux_video, fd.demux_audio, fd.demux_pcr, fd.demux_vtxt);
}

void Decoder::Flush()
{
	parms.vpid = parms.apid = parms.tpid = parms.pcrpid = parms.ecmpid = -1;
	parms.audio_type=0;
	parms.descriptor_length=0;
	parms.emmpid=-2;
	parms.recordmode=0;
	Set();
}

void Decoder::Pause()
{
	if (fd.video != -1)
	{
		::ioctl(fd.video, VIDEO_FREEZE);
		::ioctl(fd.audio, AUDIO_SET_AV_SYNC, 0);
		eDebug("video_freeze: %m");
	}
	if (fd.audio != -1)
	{
		::ioctl(fd.audio, AUDIO_STOP);
		eDebug("audio_pause: %m");
	}
}

void Decoder::Resume()
{
	if (fd.video != -1)
	{
		::ioctl(fd.video, VIDEO_CONTINUE);
		::ioctl(fd.audio, AUDIO_SET_AV_SYNC, 1);
		eDebug("video_continue %m");
	}
	if (fd.audio != -1)
	{
  ::ioctl(fd.audio, AUDIO_PLAY);
		eDebug("audio continue %m");
	}
}

void Decoder::flushBuffer()
{
	if (fd.video != -1)
		::ioctl(fd.video, VIDEO_CLEAR_BUFFER);
	if (fd.audio != -1)
		::ioctl(fd.audio, AUDIO_CLEAR_BUFFER);
//	parms.flushbuffer=1;
//	Set();
}

int Decoder::Set()
{
	int changed=0;

	dmx_pes_filter_params pes_filter;

	if (parms.vpid != current.vpid)
		changed |= 1;
	if (parms.apid != current.apid)
		changed |= 2;
	if (parms.tpid != current.tpid)
		changed |= 4;
	if (parms.pcrpid != current.pcrpid)
		changed |= 8;
	if (parms.ecmpid != current.ecmpid)
		changed |= 0x10;
	if (parms.emmpid != current.emmpid)
		changed |= 0x20;
	if (parms.pmtpid != current.pmtpid)
		changed |= 0x40;
	if (parms.casystemid != current.casystemid)
		changed |= 0x80;
	if (parms.audio_type != current.audio_type)
		changed |= 0x100;
/*	if (parms.recordmode != current.recordmode)
		changed |= 0xF;*/
		
	if (parms.flushbuffer)
		changed |= 9;

	parms.flushbuffer=0;
	
	eDebug(" ------------> changed! %x", changed);


	if (changed & 4)
	{
#ifdef OLD_VBI // for old drivers in alexW Image...
		// vtxt reinsertion (ost api)
		if ( fd.demux_vtxt == -1 )
		{
			fd.demux_vtxt=open("/dev/dbox/vbi0", O_RDWR);
			if (fd.demux_vtxt<0)
				eDebug("fd.demux_vtxt couldn't be opened");
/*			else
				eDebug("fd.demux_vtxt opened");*/
		}
		if ( current.tpid != -1 ) // we stop old vbi vtxt insertion
		{
			eDebugNoNewLine("VBI_DEV_STOP - vtxt - ");
			if (ioctl(fd.demux_vtxt, AVIA_VBI_STOP_VTXT )<0)
				perror("failed");
			else
				eDebug("ok");
		}
		if ( parms.tpid != -1 )  // we start old vbi vtxt insertion
		{
			eDebugNoNewLine("VBI_DEV_START - vtxt - ");
			if (ioctl(fd.demux_vtxt, AVIA_VBI_START_VTXT, parms.tpid)<0)
				perror("failed");
			else
				eDebug("ok");
		}
		else  // we have no tpid ... close device
		{
			close(fd.demux_vtxt);
			fd.demux_vtxt = -1;
//			eDebug("fd.demux_vtxt closed");
		}
#else
    // vtxt reinsertion (ost api)
    if ( fd.demux_vtxt == -1 )
		{
			fd.demux_vtxt=open(DEMUX_DEV, O_RDWR);
			if (fd.demux_vtxt<0)
				eDebug("fd.demux_vtxt couldn't be opened");
/*			else
				eDebug("fd.demux_vtxt opened");*/
		}
		if ( current.tpid != -1 ) // we stop dmx vtxt
		{
			eDebugNoNewLine("DEMUX_STOP - vtxt - ");
			if (ioctl(fd.demux_vtxt, DMX_STOP)<0)
				perror("failed");
			else
				eDebug("ok");
		}
		if ( parms.tpid != -1 )
		{
			pes_filter.pid=parms.tpid;
			pes_filter.input=DMX_IN_FRONTEND;
			pes_filter.output=DMX_OUT_DECODER;
			pes_filter.pes_type=DMX_PES_TELETEXT;
			pes_filter.flags=DMX_IMMEDIATE_START;
			eDebugNoNewLine("DMX_SET_PES_FILTER(0x%02x) - vtxt - ", parms.tpid);
			if (ioctl(fd.demux_vtxt, DMX_SET_PES_FILTER, &pes_filter)<0)
				perror("failed");
			else
				eDebug("ok");
			eDebugNoNewLine("DEMUX_START - vtxt - ");
			if (ioctl(fd.demux_vtxt, DMX_START)<0)
				perror("failed");
			else
				eDebug("ok");
		}
		else  // we have no tpid
		{
			close(fd.demux_vtxt);
			fd.demux_vtxt = -1;
//			eDebug("fd.demux_vtxt closed");
		}
#endif    
	}

	if (changed & 0xF7)
		SetECM(parms.vpid, parms.apid, parms.ecmpid, parms.emmpid, parms.pmtpid, parms.casystemid, parms.descriptor_length, parms.descriptors);

	if ( changed & 11 )
	{
		if (fd.demux_audio == -1)
		{
			fd.demux_audio=open(DEMUX_DEV, O_RDWR);
			if (fd.demux_audio<0)
				eDebug("fd.demux_audio couldn't be opened");
/*			else
				eDebug("fd.demux_audio opened");*/
		}

		if ( fd.audio == -1 )  // open audio dev... if not open..
		{
			fd.audio=open(AUDIO_DEV, O_RDWR);
			if (fd.audio<0)
			eDebug("fd.audio couldn't be opened");
/*			else
			eDebug("fd.audio opened");*/
		}

		// get audio status
		audio_status astatus;
		eDebugNoNewLine("AUDIO_GET_STATUS - ");
		if (ioctl(fd.audio, AUDIO_GET_STATUS, &astatus)<0)
			perror("failed");
		else
			eDebug("%s", astatus.play_state == AUDIO_STOPPED ? "stopped" : astatus.play_state == AUDIO_PAUSED ? "paused" : "playing" );

		// DEMUX STOP AUDIO
		if ( astatus.play_state != AUDIO_STOPPED /* current.apid != -1 */ )
		{
			eDebugNoNewLine("DEMUX_STOP - audio - ");
			if (ioctl(fd.demux_audio, DMX_STOP)<0)
				perror("failed");
			else
				eDebug("ok");
		}

		if ( (changed & 0x0F) != 2 )  // only apid changed
		{
			if ( fd.video == -1 ) // open video dev... if not open
			{
				fd.video=open(VIDEO_DEV, O_RDWR);
				if (fd.video<0)
					eDebug("fd.video couldn't be opened");
/*			else
					eDebug("fd.video opened");*/
			}

			// get video status
			video_status vstatus;
			eDebugNoNewLine("VIDEO_GET_STATUS - ");
			if (ioctl(fd.video, VIDEO_GET_STATUS, &vstatus)<0)
				perror("failed");
			else
				eDebug("%s", vstatus.play_state == VIDEO_STOPPED ? "stopped" : vstatus.play_state == VIDEO_PLAYING ? "playing" : "freezed" );

			// DEMUX STOP VIDEO
			if (fd.demux_video == -1)
			{
				fd.demux_video=open(DEMUX_DEV, O_RDWR);
				if (fd.demux_video<0)
					eDebug("fd.demux_video couldn't be opened");
/*			else
					eDebug("fd.demux_video opened");*/
			}
			if ( vstatus.play_state != VIDEO_STOPPED /*current.vpid != -1*/ )
			{
				eDebugNoNewLine("DEMUX_STOP - video - ");
				if (ioctl(fd.demux_video, DMX_STOP)<0)
					perror("failed");
				else
					eDebug("ok");
			}
			// DEMUX STOP PCR
			if (fd.demux_pcr == -1)
			{
				fd.demux_pcr=open(DEMUX_DEV, O_RDWR);
				if (fd.demux_pcr<0)
					eDebug("fd.demux_pcr couldn't be opened");
/*			else
					eDebug("fd.demux_pcr opened");*/
			}
			if ( current.pcrpid != -1 )
			{
				eDebugNoNewLine("DEMUX_STOP - pcr - ");
				if (ioctl(fd.demux_pcr, DMX_STOP)<0)
					perror("failed");
				else
					eDebug("ok");
			}
			usleep(100);

			if ( astatus.play_state != AUDIO_STOPPED /* current.apid != -1*/ )
			{
				eDebugNoNewLine("AUDIO_STOP - ");
				if (ioctl(fd.audio, AUDIO_STOP)<0)
					perror("failed");
				else
					eDebug("ok");
			}

			if ( parms.apid != -1 )
			{
				if ( astatus.stream_source != AUDIO_SOURCE_DEMUX )
				{
					eDebugNoNewLine("AUDIO_SELECT_SOURCE(DMX) - ");
					if (ioctl(fd.audio, AUDIO_SELECT_SOURCE, (audio_stream_source_t)AUDIO_SOURCE_DEMUX)<0)
						perror("failed");
					else
						eDebug("ok");
				}
			}

			if ( vstatus.play_state != VIDEO_STOPPED ) //*/ current.vpid != -1 )
			{
				eDebugNoNewLine("VIDEO_STOP - ");
				if (ioctl(fd.video, VIDEO_STOP)<0)
					perror("failed");
				else
					eDebug("ok");
			}

			if ( parms.vpid != -1 )
			{
				if ( vstatus.stream_source != VIDEO_SOURCE_DEMUX )
				{
					eDebugNoNewLine("VIDEO_SELECT_SOURCE(DMX) - ");
					if (ioctl(fd.video, VIDEO_SELECT_SOURCE, (video_stream_source_t)VIDEO_SOURCE_DEMUX)<0)
						perror("failed");
					else
						eDebug("ok");
				}
			}

////////////////////////// DEMUX_VIDEO SET FILTER /////////////////////////
			if ( parms.vpid != -1 )
			{
				pes_filter.pid		 = parms.vpid;
				pes_filter.input	 = DMX_IN_FRONTEND;
				pes_filter.output	 = DMX_OUT_DECODER;
				pes_filter.pes_type = DMX_PES_VIDEO;
				pes_filter.flags	 = 0;
				eDebugNoNewLine("DMX_SET_PES_FILTER(0x%02x) - video - ", parms.vpid);
				if (ioctl(fd.demux_video, DMX_SET_PES_FILTER, &pes_filter)<0)
					perror("failed");
				else
					eDebug("ok");
			}
			else  // we have no vpid
			{
				close(fd.demux_video);
				fd.demux_video=-1;
				//			eDebug("fd.demux_video closed");
			}

////////////////////////// DEMUX_PCR SET FILTER /////////////////////////
			if ( parms.pcrpid != -1 )
			{
				pes_filter.pid=parms.pcrpid;
				pes_filter.input=DMX_IN_FRONTEND;
				pes_filter.output=DMX_OUT_DECODER;
				pes_filter.pes_type=DMX_PES_PCR;
				pes_filter.flags=0;

				eDebugNoNewLine("DMX_SET_PES_FILTER(0x%02x) - pcr - ",parms.pcrpid);
				if (ioctl(fd.demux_pcr, DMX_SET_PES_FILTER, &pes_filter)<0)
					perror("failed");
				else
					eDebug("ok");
			}
			else  // we have no pcrpid
			{
				close(fd.demux_pcr);
				fd.demux_pcr=-1;
//					eDebug("fd.demux_pcr closed");
			}
		}
	}

	if (changed & 0x100 && parms.audio_type != current.audio_type )
	{
		eDebugNoNewLine("AUDIO_SET_BYPASS_MODE to %d - ", (parms.audio_type==DECODE_AUDIO_MPEG)? 1 : 0 );
		if ( ioctl( fd.audio , AUDIO_SET_BYPASS_MODE, (parms.audio_type == DECODE_AUDIO_MPEG) ? 1 : 0 ) < 0)
			perror("failed");
		else
			eDebug("ok");
	}

	if (changed & 11)
	{
////////////////////////// DEMUX AUDIO SET FILTER /////////////////////////
		if ( parms.apid != -1 )
		{
			pes_filter.pid		 = parms.apid;
			pes_filter.input	 = DMX_IN_FRONTEND;
			pes_filter.output	 = DMX_OUT_DECODER;
			pes_filter.pes_type = DMX_PES_AUDIO;
			pes_filter.flags	 = 0;
			eDebugNoNewLine("DMX_SET_PES_FILTER(0x%02x) - audio - ", parms.apid);
			if (ioctl(fd.demux_audio, DMX_SET_PES_FILTER, &pes_filter)<0)
				perror("failed");
			else
				eDebug("ok");
		}
		else  // we have no apid
		{
			close(fd.demux_audio);
			fd.demux_audio=-1;
//					eDebug("fd.demux_audio closed");
		}
		if ( (changed & 0x0F) != 2 )  //  only apid changed
		{
			if ( parms.vpid != -1 )
			{
				eDebugNoNewLine("VIDEO_PLAY - ");
				if (ioctl(fd.video, VIDEO_PLAY)<0)
					perror("failed");
				else
					eDebug("ok");
			}
			else  // no vpid
			{
//					eDebug("fd.video closed");
				close(fd.video);
				fd.video = -1;
			}
			if ( parms.apid != -1 )
			{
				eDebugNoNewLine("AUDIO_PLAY - ");
				if (ioctl(fd.audio, AUDIO_PLAY)<0)
					perror("failed");
				else
					eDebug("ok");
			}
			else  // no apid
			{
				close(fd.audio);
				fd.audio = -1;
//			eDebug("fd.audio closed");
			}

			if ( parms.pcrpid != -1 )
			{
				eDebugNoNewLine("DMX_START (pcr) - ");
				if (ioctl(fd.demux_pcr, DMX_START)<0)	
					perror("failed");
				else
					eDebug("ok");
			}
			else if ( parms.apid != -1 && parms.vpid != -1 )
			{
				eDebug("enabling av sync mode");
				::ioctl(fd.audio, AUDIO_SET_AV_SYNC, 1);
			}
			if ( parms.vpid != -1 )
			{
				eDebugNoNewLine("DMX_START (video) - ");
				if (ioctl(fd.demux_video, DMX_START)<0)
					perror("failed");
				else
						eDebug("ok");
			}
		}
    
		if ( parms.apid != -1 )
		{
			eDebugNoNewLine("DMX_START (audio) - ");

			if (ioctl(fd.demux_audio, DMX_START)<0)
				perror("failed");
			else
				eDebug("ok");
		}
	}

	current=parms;
    
	return 0;
}

void Decoder::startTrickmode()
{
	if (fd.video != -1)
	{
		::ioctl(fd.video, VIDEO_FAST_FORWARD);
		::ioctl(fd.audio, AUDIO_SET_AV_SYNC, 0);
	}
	if (fd.audio != -1)
		::ioctl(fd.audio, AUDIO_PAUSE);
}

void Decoder::stopTrickmode()
{
	if (fd.video != -1)
	{
		::ioctl(fd.video, VIDEO_CONTINUE);
		::ioctl(fd.audio, AUDIO_SET_AV_SYNC, 1);
	}
	if (fd.audio != -1)
		::ioctl(fd.audio, AUDIO_CONTINUE);
}

void Decoder::addCADescriptor(__u8 *descriptor)
{
	memcpy(parms.descriptors+parms.descriptor_length, descriptor, descriptor[1]+2);
	parms.descriptor_length+=descriptor[1]+2;
}
