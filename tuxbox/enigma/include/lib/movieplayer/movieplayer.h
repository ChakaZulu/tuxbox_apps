#ifndef __lib_movie_player_h
#define __lib_movie_player_h

#include <lib/base/thread.h>
#include <lib/base/message.h>

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
		Message(int type = 0)
			:type(type)
		{}
	};
	eFixedMessagePump<Message> messages;
	static eMoviePlayer *instance;
	void gotMessage(const Message &message);
	void thread();
public:
	eMoviePlayer();
	~eMoviePlayer();
	void start();
	void stop();
	static eMoviePlayer *getInstance() { return instance; }
};

#endif
