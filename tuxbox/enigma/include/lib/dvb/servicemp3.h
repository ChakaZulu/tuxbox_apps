#ifndef __core_dvb_servicemp3_h
#define __core_dvb_servicemp3_h

#include <core/dvb/service.h>
#include <core/base/buffer.h>

#include <mad.h>
#include <core/base/ebase.h>
#include <core/base/thread.h>
#include <core/base/message.h>

class eMP3Decoder: public eThread, public eMainloop, public Object
{
	enum { INPUT_BUFFER_SIZE=8192 };
	unsigned char input_buffer[INPUT_BUFFER_SIZE];
	eIOBuffer input;
	eIOBuffer output;
	enum
	{
		stateInit, stateError, stateBuffering, stateBufferFull, statePlaying, stateFileEnd
	};
	int state;
	int dspfd;
	int sourcefd;
	eSocketNotifier *inputsn, *outputsn;
	void decodeMore(int what);
	void outputReady(int what);
	int maxOutputBufferSize;
public:
	mad_stream stream;
	mad_frame frame;
	mad_synth synth;
	mad_timer_t timer;
	
	struct eMP3DecoderMessage
	{
		enum
		{
			start, exit
		};
		int type;
		eMP3DecoderMessage() { }
		eMP3DecoderMessage(int type): type(type) { }
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
	
	eMP3Decoder(const char *filename);
	~eMP3Decoder();
	
	void thread();
};

class eServiceHandlerMP3: public eServiceHandler
{
	eService *createService(const eServiceReference &service);
	void addFile(void *node, const eString &filename);
	
	int state;
	eMP3Decoder *decoder;
public:
	int getID() const;

	eServiceHandlerMP3();
	~eServiceHandlerMP3();
	eService *lookupService(const eServiceReference &service);

	int play(const eServiceReference &service);

	int getFlags();
	int getState();
	int getErrorInfo();

	int stop();
};

#endif
