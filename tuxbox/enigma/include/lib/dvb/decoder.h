#ifndef __DECODER_H
#define __DECODER_H

		// all kind of decoding-related stuff
#define DECODE_AUDIO_MPEG	0
#define DECODE_AUDIO_AC3	1

struct decoderParameters
{
	int vpid, apid, tpid, pcrpid, ecmpid, emmpid, pmtpid, casystemid;
	int audio_type;

	int recordmode;
	int flushbuffer;
	
	__u8 descriptors[2048];
	int descriptor_length;
};

class Decoder
{
	static struct fd
	{
		static int video, audio, demux_video, demux_audio, demux_pcr, demux_vtxt;
	} fd;
	static decoderParameters current;
public:
	static int getAudioDevice()	{ return fd.audio; }
	static int getVideoDevice()	{ return fd.video; }
	static decoderParameters parms;
	static int Initialize();
	static void Close();
	static void Flush();
	static void Pause();
	static void Resume();
	static void addCADescriptor(__u8 *descriptor);
	static int Set();
	static void flushBuffer();
	static void startTrickmode();
	static void stopTrickmode();
	static void setVideoFormat( int format );
	static int  displayIFrame(const char *frame, int len);
	static int  displayIFrameFromFile(const char *filename);
};

#endif
