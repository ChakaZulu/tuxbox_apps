#ifndef __lib_dvb_servicemp3_h
#define __lib_dvb_servicemp3_h

#include <lib/dvb/service.h>
#include <lib/base/buffer.h>

#include <lib/base/ebase.h>
#include <lib/base/thread.h>
#include <lib/base/message.h>
#include <lib/system/elock.h>
#include <lib/codecs/codec.h>

#include <lib/system/httpd.h>

class eServiceHandlerMP3;

class eHTTPStream: public eHTTPDataSource
{
	eIOBuffer &buffer;
	int bytes;
	int metadatainterval, metadataleft, metadatapointer;
	__u8 metadata[16*256+1]; // maximum size
	void processMetaData();
public:
	eHTTPStream(eHTTPConnection *c, eIOBuffer &buffer);
	~eHTTPStream();
	void haveData(void *data, int len);
	Signal0<void> dataAvailable;
	Signal0<void> metaDataUpdated;
};

class eMP3Decoder: public eThread, public eMainloop, public Object
{
	eServiceHandlerMP3 *handler;
	eAudioDecoder *audiodecoder;
	eIOBuffer input;
	eIOBuffer output;
	enum
	{
		stateInit, stateError, stateBuffering, stateBufferFull, statePlaying, statePause, stateFileEnd
	};

	int state;
	int dspfd;

	int sourcefd;
	eHTTPStream *stream;
	eHTTPConnection *http;
	
	int error;
	int outputbr;
	eSocketNotifier *inputsn, *outputsn;
	void streamingDone(int err);
	void decodeMoreHTTP();
	void decodeMore(int what);
	void outputReady(int what);
	void checkFlow(int last);
	eHTTPDataSource *createStreamSink(eHTTPConnection *conn);
	
	int maxOutputBufferSize;
	
	eAudioDecoder::pcmSettings pcmsettings;
	
	int filelength;
	
	int length;
	int position;
	eLock poslock;
	
	void dspSync();
public:
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
	
	eMP3Decoder(const char *filename, eServiceHandlerMP3 *handler);
	~eMP3Decoder();
	
	int getPosition(int);
	int getLength(int);
	
	int getError() const { return (state == stateError) ? error : 0; }
	
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

class eServiceID3
{
public:
		// tags are according to ID3v2
	std::map<eString, eString> tags;
};

class eServiceMP3: public eService
{
	eServiceID3 id3tags;
public:
	eServiceMP3(const char *filename);
	eServiceMP3(const eServiceMP3 &c);
};

#endif
