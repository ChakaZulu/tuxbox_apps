#ifndef __lib_movie_player_h
#define __lib_movie_player_h

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
		eString mrl;
		enum
		{
			start,
			stop
		};
		Message(int type = 0, eString mrl = ""): type(type), mrl(mrl)
		{}
	};
	eServiceReference suspendedServiceReference;
	eFixedMessagePump<Message> messages;
	static eMoviePlayer *instance;
	int serverPort;
	eString serverIP;
	int transcodeAudio;
	int transcodeVideo;
	void gotMessage(const Message &message);
	void thread();
	int waitUntilVLCStartsTalking();
	eString sout(eString mrl);
	int sendRequest2VLC(eString command);
	void playStream();
public:
	eMoviePlayer();
	~eMoviePlayer();
	void start(eString mrl);
	void stop();
	static eMoviePlayer *getInstance() {return (instance) ? instance : instance = new eMoviePlayer();}
};

#endif
