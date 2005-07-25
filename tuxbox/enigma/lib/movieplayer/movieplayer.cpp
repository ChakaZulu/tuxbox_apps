#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <lib/movieplayer/movieplayer.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <lib/base/buffer.h>
#include <lib/system/econfig.h>
#include <lib/dvb/decoder.h>

#define BLOCKSIZE 65424
#define PVRDEV "/dev/pvr"
#define INITIALBUFFERING 5*BLOCKSIZE

static eIOBuffer *tsBuffer;
static pthread_mutex_t mutex = PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP;

extern int tcpOpen(eString, int);
extern void find_avpids(int fd, uint16_t *vpid, uint16_t *apid);
extern int is_audio_ac3(int);
extern int tcpRequest(int fd, char *ioBuf, int maxLen);

pthread_t dvr;
void *dvrThread(void *);

eMoviePlayer *eMoviePlayer::instance;

eMoviePlayer::eMoviePlayer():messages(this, 1)
{
	if (!instance)
		instance = this;
	tsBuffer = new eIOBuffer(BLOCKSIZE);
	CONNECT(messages.recv_msg, eMoviePlayer::gotMessage);
	eDebug("[MOVIEPLAYER] starting...");
	run();
}

eMoviePlayer::~eMoviePlayer()
{
	eDebug("[MOVIEPLAYER] stopping...");
	messages.send(Message::stop);
	if (tsBuffer)
		delete tsBuffer;
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
	eDebug("[MOVIEPLAYER] issueing stop...");
	messages.send(Message(Message::stop));
}

int waitUntilVLCStartsTalking()
{
	int skt;

	eDebug("[MOVIEPLAYER] wait for VLC talking to us...");
	// Open HTTP connection to VLC
	int serverPort = 0;
	eConfig::getInstance()->getKey("/movieplayer/serverport", serverPort);
	char *serverip;
	if (eConfig::getInstance()->getKey("/movieplayer/serverip", serverip))
		serverip = strdup("");
	eString serverIP = eString(serverip);
	free(serverip);
	
	eDebug("[MOVIEPLAYER] Server IP: %s", serverIP.c_str());
	eDebug("[MOVIEPLAYER] Server Port: %d", serverPort);

	while (true)
	{
		skt = tcpOpen(serverIP, serverPort);
		if (skt == -1)
		{
			eDebug("[MOVIEPLAYER] couldn't connect socket.");
			return  -1;
		}
		
		fcntl(skt, O_NONBLOCK);
		eDebug("[MOVIEPLAYER] socket connected.");

		if (tcpRequest(skt, "GET /dboxstream HTTP/1.0\r\n\r\n", 28) < 0)
		{
			eDebug("[MOVIEPLAYER] sending GET failed.");
			return -2;
		}

		eDebug("[MOVIEPLAYER] GET request sent.");

		// Skip HTTP Header
		char line[256];
		memset(line, '\0', sizeof(line));
		char *bp = line;
		while (bp - line < sizeof(line))
		{
			recv(skt, bp, 1, 0);
			if (strstr(line, "\r\n\r\n") == 0)
			{
				if (strstr(line, "HTTP/1.0 404") == 0)
				{
					eDebug("[MOVIEPLAYER] VLC is not sending... retrying...");
					close(skt);
					break;
				}
				else
				{
					eDebug("[MOVIEPLAYER] VLC is sending data... header received.");
					return skt;
				}
			}
			bp++;
		}
	}
	return -4;
}

int foundAVPids(int skt, unsigned short * apid, unsigned short * vpid, unsigned short * ac3)
{
	int len = 0;
	int totallen = 0;
	char tempBuffer[BLOCKSIZE];
	int fd = open ("/tmp/tmpts", O_CREAT | O_WRONLY);
	
	eDebug("[MOVIEPLAYER] buffering data...");
	
	// fill buffer and temp file
	do
	{
		len = recv(skt, tempBuffer, BLOCKSIZE, 0);
		if (len >= 0)
		{
			eDebug("[MOVIEPLAYER] writing %d bytes to buffer...", len);
//			pthread_mutex_lock(&mutex);
			tsBuffer->write(tempBuffer, len);
			write(fd, tempBuffer, len);
//			pthread_mutex_unlock(&mutex);
			totallen += len;
		}
	}
	while (totallen < INITIALBUFFERING);
	
	eDebug("[MOVIEPLAYER] searching for vpid and apid");
	close (fd);
	fd = open ("/tmp/tmpts", O_RDONLY);
	find_avpids(fd, vpid, apid);
	lseek(fd, 0, SEEK_SET);
	*ac3 = is_audio_ac3(fd);
	close(fd);
	remove("/tmp/tmpts");
	
	eDebug("[MOVIEPLAYER] found apid: 0x%04X, vpid: 0x%04X, ac3: %d", *apid, *vpid, *ac3);
	
	return (int)(*apid > 0 && *vpid > 0)
}

void eMoviePlayer::playStream()
{
	unsigned short apid = 0, vpid = 0, ac3 = 0;
	int len = 0;
	int play = 1;
	
	// receive video stream from VLC on PC
	eDebug("[MOVIEPLAYER] start playing stream...");
	
	int skt;
	char tempBuffer[BLOCKSIZE];

	if (skt = waitUntilVLCStartsTalking())
	{
		eDebug("[MOVIEPLAYER] VLC is sending now... now looking for AV pids");

		if (foundAVPids(skt, &apid, &vpid, &ac3))
		{
			// set pids
			Decoder::parms.vpid = vpid;
			Decoder::parms.apid = apid;
			Decoder::Set();	
		
			// create dvr thread
			pthread_create(&dvr, 0, dvrThread, (void *)&play);
	
			// continue to fill buffer
			do
			{
				len = recv(skt, tempBuffer, BLOCKSIZE, 0);
				if (len >= 0)
				{
					eDebug("[MOVIEPLAYER] writing %d bytes to buffer...", len);
					pthread_mutex_lock(&mutex);
					tsBuffer->write(tempBuffer, len);
					pthread_mutex_unlock(&mutex);
				}
			}
			while (len > 0);
			close(skt);
		}
		else
		{
			eDebug("[MOVIEPLAYER] could not find AV pids.");
		}
	}
	else
	{
		eDebug("[MOVIEPLAYER] VLC is not talking to us.");
	}
	
	quit(0);
}

extern bool playService(const eServiceReference &ref);

void eMoviePlayer::gotMessage(const Message &msg )
{
	eDebug("[MOVIEPLAYER] received message : %d", msg.type);
	switch (msg.type)
	{
		case Message::start:
			eDVBServiceController *sapi = eDVB::getInstance()->getServiceAPI();
			if (sapi)
				suspendedServiceReference = sapi->service;
			eServiceInterface::getInstance()->stop();
			playStream();
			playService(suspendedServiceReference);
			break;
		case Message::stop:
			pthread_cancel(dvr);
			playService(suspendedServiceReference);
			quit(0);
			break;
		default:
			eDebug("[MOVIEPLAYER] received unknown message");
	}
}

void *dvrThread(void *ctrl)
{
	char tempBuffer[BLOCKSIZE];
	int rd = 0;
	eDebug("[MOVIEPLAYER] dvrThread starting...");
	int pvrfd = 0;
	pvrfd = open(PVRDEV, O_RDWR);
	while (*((int *)ctrl) == 1)
	{
		sleep(2);
		pthread_mutex_lock(&mutex);
		rd = tsBuffer->read(tempBuffer, BLOCKSIZE);
		eDebug("[MOVIEPLAYER] writing %d bytes to dvr...", rd);
		if (rd > 0)
		{
			write(pvrfd, tempBuffer, rd);
			pthread_mutex_unlock(&mutex);
		}
		else 
		{
			pthread_mutex_unlock(&mutex);
			usleep(1000);
		}
	}
	pthread_exit(NULL);
}
