#ifndef __lib_movieplayer_h
#define __lib_movieplayer_h

#include <lib/base/estring.h>
#include <lib/base/thread.h>
#include <lib/base/message.h>
#include <lib/dvb/dvb.h>
#include <lib/dvb/service.h>
#include <lib/dvb/dvbservice.h>

class eMoviePlayer: public eMainloop, private eThread, public Object
{
	struct Message
	{
		int type;
		const char *filename;
		enum
		{
			start,
			stop,
			play,
			pause,
			forward,
			rewind,
			quit
		};
		Message(int type = 0, const char *filename = 0)
			:type(type), filename(filename)
		{}
	};
	eFixedMessagePump<Message> messages;
	static eMoviePlayer *instance;
	int serverPort;
	eString serverIP;
	int status;
	eServiceReference suspendedServiceReference;
	void gotMessage(const Message &message);
	void thread();
	eString sout(eString mrl);
	int requestStream();
	int playStream(eString mrl);
public:
	eMoviePlayer();
	~eMoviePlayer();
	int sendRequest2VLC(eString command);
	void control(const char *command, const char *filename);
	int getStatus() { return status; }
	void readStreamingServerSettings(eString& ip, int& port, eString& dvddrive, int& videodatarate, int& resolution, int& mpegcodec, int& transcodevideo, int& audiodatarate, int& transcodeaudio);
	void writeStreamingServerSettings(eString ip, int port, eString dvddrive, int videodatarate, int resolution, int mpegcodec, int transcodevideo, int audiodatarate, int transcodeaudio);
	static eMoviePlayer *getInstance() { return instance; }
};

#endif
