#ifndef __DECODER_H
#define __DECODER_H

		// all kind of decoding-related stuff
#define DECODE_AUDIO_MPEG	0
#define DECODE_AUDIO_AC3  1
#define DECODE_AUDIO_DTS  2
#define DECODE_AUDIO_AC3_VOB  3

#define TYPE_ES 0
#define TYPE_PES 1
#define TYPE_MPEG1 2

struct decoderParameters
{
	int vpid, apid, tpid, pcrpid, pmtpid;
	int audio_type;

	__u8 restart_camd;

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
	static bool locked;
	static int getAudioDevice()	{ return fd.audio; }
	static int getVideoDevice()	{ return fd.video; }
	static decoderParameters parms;
	static int Initialize();
	static void Close();
	static void Flush();
	static void Pause( bool disableAudio=true );
	static void Resume( bool enableAudio=true );
	static void addCADescriptor(__u8 *descriptor);
	static int Set();
	static void flushBuffer();
	static void startTrickmode();
	static void stopTrickmode();
	static void SetStreamType(int type);
	static void setVideoFormat( int format );
	static int  displayIFrame(const char *frame, int len);
	static int  displayIFrameFromFile(const char *filename);
	static void showPicture(int i=1);
};

#endif
