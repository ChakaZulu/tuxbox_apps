#ifndef __lib_dvb_servicemp3_h
#define __lib_dvb_servicemp3_h

#include <lib/dvb/service.h>
#include <lib/base/buffer.h>

#include <mad.h>
#include <lib/base/ebase.h>
#include <lib/base/thread.h>
#include <lib/base/message.h>
#include <lib/system/elock.h>

class eServiceHandlerMP3;

class eMP3Decoder: public eThread, public eMainloop, public Object
{
	enum { INPUT_BUFFER_SIZE=8192 };
	unsigned char input_buffer[INPUT_BUFFER_SIZE];
	eServiceHandlerMP3 *handler;
	eIOBuffer input;
	eIOBuffer output;
	enum
	{
		stateInit, stateError, stateBuffering, stateBufferFull, statePlaying, statePause, stateFileEnd
	};
	int state;
	int dspfd;
	int sourcefd;
	int speed;
	int framecnt;
	int error;
	eSocketNotifier *inputsn, *outputsn;
	void decodeMore(int what);
	void outputReady(int what);
	int maxOutputBufferSize;
	
	int filelength;
	int avgbr, outputbr;
	
	int length;
	int position;
	eLock poslock;
	
	void dspSync();
public:
	mad_stream stream;
	mad_frame frame;
	mad_synth synth;
	mad_timer_t timer;
	
	struct eMP3DecoderMessage
	{
		enum
		{
			start, exit,
			skip,
			setSpeed, // 0..
			seek,	// 0..65536
			seekreal
		};
		int type;
		int parm;
		eMP3DecoderMessage() { }
		eMP3DecoderMessage(int type): type(type) { }
		eMP3DecoderMessage(int type, int parm): type(type), parm(parm) { }
	};
	eFixedMessagePump<eMP3DecoderMessage> messages;
	
	void gotMessage(const eMP3DecoderMessage &message);
	
	struct mp3pcm
	{
		unsigned int samplerate;
		unsigned int channels;
		unsigned int format;
		int reconfigure;
	} pcmsettings;
	
	eMP3Decoder(const char *filename, eServiceHandlerMP3 *handler);
	~eMP3Decoder();
	
	int getPosition(int);
	int getLength(int);
	
	int getError() const { return (state != stateError) ? error : 0; }
	
	void thread();
};

class eServiceHandlerMP3: public eServiceHandler
{
	eService *createService(const eServiceReference &service);
	void addFile(void *node, const eString &filename);
	friend class eMP3Decoder;

	struct eMP3DecoderMessage
	{
		enum
		{
			done,
			status
		};
		int type;
		int parm;
		eMP3DecoderMessage() { }
		eMP3DecoderMessage(int type): type(type) { }
		eMP3DecoderMessage(int type, int status): type(type), parm(parm) { }
	};
	eFixedMessagePump<eMP3DecoderMessage> messages;
	
	void gotMessage(const eMP3DecoderMessage &message);
	
	int state;
	eMP3Decoder *decoder;
public:
	int getID() const;

	eServiceHandlerMP3();
	~eServiceHandlerMP3();

	int play(const eServiceReference &service);
	int serviceCommand(const eServiceCommand &cmd);

	int getFlags();
	int getState();
	int getErrorInfo();

	int stop();
	
	int getPosition(int what);
	int getErrorInfo() const;

	eService *addRef(const eServiceReference &service);
	void removeRef(const eServiceReference &service);
};

#endif
