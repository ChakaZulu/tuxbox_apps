#include <lib/movieplayer/movieplayer.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

pthread_t dvr;
void *dvrThread(void *);

eMoviePlayer *eMoviePlayer::instance;

eMoviePlayer::eMoviePlayer() :messages(this,1)
{
	if (!instance)
		instance = this;
	CONNECT(messages.recv_msg, eMoviePlayer::gotMessage);
	eDebug("[MOVIEPLAYER] starting...");
	run();
}

eMoviePlayer::~eMoviePlayer()
{
	eDebug("[MOVIEPLAYER] stopping...");
	messages.send(Message::stop);
	if ( thread_running() )
		kill();
	if (instance == this)
		instance = 0;
}

void eMoviePlayer::thread()
{
	eDebug("[MOVIEPLAYER] receiver thread starting...");
	nice(5);
	exec();
}

void eMoviePlayer::start()
{
	eDebug("[MOVIEPLAYER] issuing start...");
	messages.send(Message(Message::start));
}

void eMoviePlayer::stop()
{
	eDebug("[MOVIEPLAYER] issuing stop...");
	messages.send(Message(Message::stop));
}

void eMoviePlayer::playStream()
{
	// receive video stream from VLC on PC
	eDebug("[MOVIEPLAYER] start playing stream...");
	
	// create dvr thread
	pthread_create(&dvr, 0, dvrThread, (void *)buffer.c_str());
	
	// filling buffer
	while (1)
	{
		if (buffer == "flip")
			buffer = "flop";
		sleep(2);
	}
	
	quit(0);
}

void eMoviePlayer::gotMessage(const Message &msg )
{
	eDebug("[MOVIEPLAYER] received message : %d", msg.type);
	switch (msg.type)
	{
		case Message::start:
			playStream();
			break;
		case Message::stop:
			quit(0);
			break;
		default:
			eDebug("[MOVIEPLAYER] received unknown message");
	}
}

void *dvrThread(void *buffer)
{
	eDebug("[MOVIEPLAYER] dvrThread starting...");
	while (strlen((char *)buffer) > 0)
	{
		sleep(1);
		eDebug("[MOVIEPLAYER] writing to dvr: %s", (char *)buffer);
	}
	pthread_exit(NULL);
}
