#include <lib/movieplayer/movieplayer.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

eMoviePlayer *eMoviePlayer::instance;

eMoviePlayer::eMoviePlayer()
	:messages(this,1)
{
	if (!instance)
		instance = this;
	CONNECT(messages.recv_msg, eMoviePlayer::gotMessage);
	run();
}

eMoviePlayer::~eMoviePlayer()
{
	messages.send(Message::stop);
	if ( thread_running() )
		kill();
	if (instance == this)
		instance = 0;
}

void eMoviePlayer::thread()
{
	nice(5);
	exec();
}

void eMoviePlayer::start()
{
	messages.send(Message(Message::start);
}

void eMoviePlayer::stop()
{
	messages.send(Message(Message::stop);
}

void eMoviePlayer::receiveStream()
{
	// receive video stream from VLC on PC
}

void eMoviePlayer::gotMessage(const Message &msg )
{
	switch (msg.type)
	{
		case Message::start:
			receiveStream();
		case Message::stop:
			quit(0);
			break;
		default:
			eDebug("unhandled thread message");
	}
}

