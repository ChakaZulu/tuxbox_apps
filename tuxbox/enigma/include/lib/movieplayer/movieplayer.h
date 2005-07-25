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
		enum
		{
			start,
			stop
		};
		Message(int type = 0): type(type)
		{}
	};
	eServiceReference suspendedServiceReference;
	eFixedMessagePump<Message> messages;
	static eMoviePlayer *instance;
	void gotMessage(const Message &message);
	void thread();
	void playStream();
public:
	eMoviePlayer();
	~eMoviePlayer();
	void start();
	void stop();
	static eMoviePlayer *getInstance() {return (instance) ? instance : instance = new eMoviePlayer();}
};

#endif
