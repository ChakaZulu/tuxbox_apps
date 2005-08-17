#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <lib/base/buffer.h>
#include <lib/system/econfig.h>
#include <lib/dvb/decoder.h>
#include <lib/movieplayer/movieplayer.h>
#include <src/enigma_main.h>
#include <src/enigma_dyn_utils.h>

#if HAVE_DVB_API_VERSION < 3
#define PVRDEV "/dev/pvr"
#define BLOCKSIZE 65424*4
#define INITIALBUFFER BLOCKSIZE*10
#else
#define PVRDEV "/dev/dvb/adapter0/dvr0"
#define BLOCKSIZE 65424
#define INITIALBUFFER BLOCKSIZE*48
#endif

eIOBuffer tsBuffer(BLOCKSIZE*4);
static pthread_mutex_t mutex = PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP;

extern void find_avpids(eIOBuffer *tsBuffer, int *vpid, int *apid, int *ac3);
extern int tcpOpen(eString, int);
extern int tcpRequest(int, char *, int);
extern bool playService(const eServiceReference &ref);

pthread_t pvr = 0;
void *pvrThread(void *);
pthread_t receiver = 0;
void *receiverThread(void *);

static int fd = -1;
static int pvrfd = -1;

eMoviePlayer *eMoviePlayer::instance;

void createThreads()
{
	// create receiver thread
	if (!receiver)
	{
		pthread_create(&receiver, 0, receiverThread, (void *)&fd);
		eDebug("[MOVIEPLAYER] createThreads: receiver thread created.");
	}
	// create pvr thread
	if (!pvr)
	{
		pvrfd = open(PVRDEV, O_RDWR);
		pthread_create(&pvr, 0, pvrThread, (void *)&pvrfd);
		eDebug("[MOVIEPLAYER] createThreads: pvr thread created.");
	}
}

void killPVRThread()
{
	if (pvr)
	{	
		pthread_cancel(pvr);
		eDebug("[MOVIEPLAYER] killThreads: waiting for pvr thread to join.");
		pthread_join(pvr, 0);
		pvr = 0;
		eDebug("[MOVIEPLAYER] killThreads: pvr thread cancelled.");
	}
}

void killReceiverThread()
{
	if (receiver)
	{
		pthread_cancel(receiver);
		eDebug("[MOVIEPLAYER] killThreads: waiting for receiver thread to join.");
		pthread_join(receiver, 0);
		receiver = 0;
		eDebug("[MOVIEPLAYER] killThreads: receiver thread cancelled.");
	}
}

void killThreads()
{
	killReceiverThread();
	killPVRThread();
}

eMoviePlayer::eMoviePlayer(): messages(this,1)
{
	if (!instance)
		instance = this;
		
	CONNECT(messages.recv_msg, eMoviePlayer::gotMessage);
	eDebug("[MOVIEPLAYER] Version 1.6 starting...");
	run();
}

eMoviePlayer::~eMoviePlayer()
{
	killThreads();
	messages.send(Message::quit);
	if (thread_running())
		kill();
	if (instance == this)
		instance = 0;
	status = 0;
}

void eMoviePlayer::thread()
{
	nice(1);
	exec();
}

void eMoviePlayer::leaveStreamingClient()
{
	eMoviePlayer::getInstance()->sendRequest2VLC("?control=stop");
	tsBuffer.clear();
	Decoder::Flush();
	// restore suspended dvb service
	playService(suspendedServiceReference);
	status = 0;
}

void eMoviePlayer::control(const char *command, const char *filename)
{
	if (thread_running())
		eDebug("[MOVIEPLAYER] main thread is running.");
	else
		eDebug("[MOVIEPLAYER] main thread is NOT running.");
		
	eString cmd = eString(command);
	
	eDebug("[MOVIEPLAYER] control: command = %s", command);
	if (cmd == "start")
		messages.send(Message(Message::start, filename ? strdup(filename) : 0));
	else
	if (cmd == "stop" || cmd == "terminate")
		messages.send(Message(Message::stop, 0));
	else
	if (cmd == "play")
		messages.send(Message(Message::play, 0));
	else 
	if (cmd == "pause")
		messages.send(Message(Message::pause, 0));
	else
	if (cmd == "rewind")
		messages.send(Message(Message::rewind, 0));
	else
	if (cmd == "forward")
		messages.send(Message(Message::forward, 0));
}

int eMoviePlayer::sendRequest2VLC(eString command)
{
	char ioBuffer[512];
	int rc = -1;
	int fd = tcpOpen(serverIP, 8080);
	if (fd > 0)
	{
		eString url = "GET /" + command + " HTTP/1.0\r\n\r\n";
		strcpy(ioBuffer, url.c_str());
		eDebug("[MOVIEPLAYER] sendRequest2VLC : %d, %s", fd, ioBuffer);
		rc = tcpRequest(fd, ioBuffer, sizeof(ioBuffer) - 1);
		if (rc == 0)
		{
			if (strstr(ioBuffer, "HTTP/1.0 200 OK") == 0)
			{
				eDebug("[MOVIEPLAYER] 200 OK NOT received...");
				rc = -2;
			}
			else 
			{
				eDebug("[MOVIEPLAYER] 200 OK...");
			}
		}
		else 
			rc = -3;
		close(fd);
	}
	
	return rc;
}

int bufferStream(int fd, int bufferSize)
{
	int len = 0;
	char tempBuffer[BLOCKSIZE];
	int errors = 0;
	
	eDebug("[MOVIEPLAYER] buffering stream...");
	
	// fill buffer and temp file
	while (tsBuffer.size() < bufferSize && errors < 100)
	{
		len = recv(fd, tempBuffer, BLOCKSIZE, 0);
		if (len > 0)
		{
			eDebug("[MOVIEPLAYER] writing %d bytes to buffer, total: %d", len, tsBuffer.size());
			tsBuffer.write(tempBuffer, len);
			errors = 0;
		}
		else
		{
			errors++;
			usleep(10000);
		}
	}
	
	return tsBuffer.size();
}

int eMoviePlayer::requestStream()
{
	char ioBuffer[512];
	
	eDebug("[MOVIEPLAYER] requesting VLC stream...");
	
	int rc = -1;
	int retry = 10;
	while (--retry > 0 && rc != 0)
	{
		eDebug("[MOVIEPLAYER] retry: %d", retry);
		fd = tcpOpen(serverIP, serverPort);
		if (fd < 0)
		{
			eDebug("[MOVIEPLAYER] tcpOpen failed.");
			continue;
		}
		strcpy(ioBuffer, "GET /dboxstream HTTP/1.0\r\n\r\n");
		if (tcpRequest(fd, ioBuffer, sizeof(ioBuffer) - 1) < 0)
		{
			eDebug("[MOVIEPLAYER] get stream request failed.");
			close(fd);
			continue;
		}
	
		if (strstr(ioBuffer, "HTTP/1.0 200 OK") == 0)
		{
			eDebug("[MOVIEPLAYER] 200 OK not received...");
			close(fd);
			continue;
		}
		else
		{
			eDebug("[MOVIEPLAYER] 200 OK...");
			char *p = strstr(ioBuffer, "\r\n\r\n");
			// buffer first stream bytes after \r\n\r\n
			if ((unsigned int)(p + 4 - ioBuffer) < strlen(ioBuffer))
				tsBuffer.write(p + 4, strlen(ioBuffer) - (p + 4 - ioBuffer));
			rc = 0;
		}
	}
	return rc;
}


int eMoviePlayer::playStream(eString mrl)
{
	int apid = -1, vpid = -1, ac3 = -1;
	
	// receive video stream from VLC on PC
	eDebug("[MOVIEPLAYER] start playing stream...");
	
	tsBuffer.clear();
	
	if (requestStream() < 0)
	{
		eDebug("[MOVIEPLAYER] requesting stream failed...");
		close(fd);
		return -1;
	}
	
	if (bufferStream(fd, INITIALBUFFER) == 0)
	{
		eDebug("[MOVIEPLAYER] buffer is empty.");
		close(fd);
		return -4;
	}
	
	find_avpids(&tsBuffer, &vpid, &apid, &ac3);
	if (vpid == -1 || apid == -1)
	{
		eDebug("[MOVIEPLAYER] no AV pids found.");
		close(fd);
		return -5;
	}
	
	// save current dvb service for later
	eDVBServiceController *sapi;
	if (sapi = eDVB::getInstance()->getServiceAPI())
		suspendedServiceReference = sapi->service;
	// stop dvb service
	eServiceInterface::getInstance()->stop();
	
	// set pids
	Decoder::parms.vpid = vpid;
	Decoder::parms.apid = apid;
	Decoder::parms.audio_type = DECODE_AUDIO_MPEG;
	
	int ac3default = 0;
	eConfig::getInstance()->getKey("/elitedvb/audio/ac3default", ac3default);
	if (ac3 && ac3default)
	{
		if (mrl.right(3) == "vob" || mrl.left(3) == "dvd")
			Decoder::parms.audio_type = DECODE_AUDIO_AC3_VOB;
		else
			Decoder::parms.audio_type = DECODE_AUDIO_AC3;
	}

	eZapMain::getInstance()->hideInfobar();
	usleep(100000);
	Decoder::Set();

#ifndef DISABLE_LCD
	eZapLCD::getInstance()->lcdMain->ServiceName->setText(mrl);
#endif

	createThreads();
	
	return 0;
}

void eMoviePlayer::gotMessage(const Message &msg )
{
	eString mrl;
	eDebug("[MOVIEPLAYER] message %d coming in...", msg.type);
	switch (msg.type)
	{
		case Message::start:
		{
			status = 1;
			killThreads();
			if (msg.filename)
			{
				mrl = eString(msg.filename);
				eDebug("[MOVIEPLAYER] mrl = %s", mrl.c_str());
				free((char*)msg.filename);
				
				serverPort = 9090;
				eConfig::getInstance()->getKey("/movieplayer/serverport", serverPort);
				char *serverip;
				if (eConfig::getInstance()->getKey("/movieplayer/serverip", serverip))
					serverip = strdup("");
				serverIP = eString(serverip);
				free(serverip);
				eDebug("[MOVIEPLAYER] Server IP: %s", serverIP.c_str());
				eDebug("[MOVIEPLAYER] Server Port: %d", serverPort);
				
				int retry = 5;
				while (--retry > 0)
				{
					eDebug("[MOVIEPLAYER] trying to get vlc going... retry = %d", retry);
					// receive and play ts stream
					if (playStream(mrl) < 0)
						continue;
					else
						retry = 0;
				}
			}
			break;
		}
		case Message::pause:
			break;
		case Message::play:
			break;
		case Message::forward:
			break;
		case Message::stop:
		{
			killThreads();
			status = 0;
			break;
		}
		case Message::quit:
		{
			quit(0);
			break;
		}
		default:
			eDebug("unhandled thread message");
	}
	eDebug("[MOVIEPLAYER] message %d handled.", msg.type);
}

eString eMoviePlayer::sout(eString mrl)
{
	eString soutURL = "#";
	eString serverIP, DVDDrive;
	int serverPort;
	int settingVideoRate, settingResolution, settingTranscodeVideoCodec, settingTranscodeVideo, settingAudioRate, settingTranscodeAudio;
	
	readStreamingServerSettings(serverIP, serverPort, DVDDrive, settingVideoRate, settingResolution, settingTranscodeVideoCodec, settingTranscodeVideo, settingAudioRate, settingTranscodeAudio);
	
	eDebug("[MOVIEPLAYER] determine ?sout for mrl: %s", mrl.c_str());
	eDebug("[MOVIEPLAYER] transcoding audio: %d, video: %d", settingTranscodeAudio, settingTranscodeVideo);

	// add sout (URL encoded)
	// example (with transcode to mpeg1):
	//  ?sout=#transcode{vcodec=mpgv,vb=2000,acodec=mpga,ab=192,channels=2}:duplicate{dst=std{access=http,mux=ts,url=:8080/dboxstream}}
	// example (without transcode to mpeg1): 
	// ?sout=#duplicate{dst=std{access=http,mux=ts,url=:8080/dboxstream}}

	eString res_horiz;
	eString res_vert;
	switch (settingResolution)
	{
		case 1:
			res_horiz = "352";
			res_vert = "576";
			break;
		case 2:
			res_horiz = "480";
			res_vert = "576";
			break;
		case 3:
			res_horiz = "704";
			res_vert = "576";
			break;
		default:
			res_horiz = "352";
			res_vert = "288";
	}
	
	if (settingTranscodeVideo || settingTranscodeAudio)
	{
		soutURL += "transcode{";
		if (settingTranscodeVideo)
		{
			eString videoCodec = (settingTranscodeVideoCodec == 1) ? "mpgv" : "mp2v";
			soutURL += "vcodec=" + videoCodec;
			soutURL += ",vb=" + eString().sprintf("%d", settingVideoRate);
			soutURL += ",width=" + res_horiz;
			soutURL += ",height=" + res_vert;
		}
		if (settingTranscodeAudio)
		{
			if (settingTranscodeVideo)
				soutURL += ",";
			soutURL += "acodec=mpga,ab=" + eString().sprintf("%d", settingAudioRate) + ",channels=2";
		}
		soutURL += "}:";
	}
	
	soutURL += "duplicate{dst=std{access=http,mux=ts,url=:" + eString().sprintf("%d", serverPort) + "/dboxstream}}";
	eDebug("[MOVIEPLAYER] sout = %s", soutURL.c_str());
	return soutURL;
}

void eMoviePlayer::readStreamingServerSettings(eString& serverIP, int& serverPort, eString& 
DVDDrive, int& settingVideoRate, int& settingResolution, int& settingTranscodeVideoCodec, int& settingTranscodeVideo, int& settingAudioRate, int& settingTranscodeAudio)
{
	char *serverip;
	if (eConfig::getInstance()->getKey("/movieplayer/serverip", serverip))
		serverip = strdup("");
	serverIP = eString(serverip);
	free(serverip);
	serverPort = 9090;
	eConfig::getInstance()->getKey("/movieplayer/serverport", serverPort);
	char *dvddrive;
	if (eConfig::getInstance()->getKey("/movieplayer/dvddrive", dvddrive))
		dvddrive = strdup("D");
	DVDDrive = eString(dvddrive);
	free(dvddrive);
	settingResolution = 3;
	eConfig::getInstance()->getKey("/movieplayer/resolution", settingResolution);
	settingAudioRate = 192;
	eConfig::getInstance()->getKey("/movieplayer/audiorate", settingAudioRate);
	settingVideoRate = 2048;
	eConfig::getInstance()->getKey("/movieplayer/videorate", settingVideoRate);
	settingTranscodeVideoCodec = 2;
	eConfig::getInstance()->getKey("/movieplayer/transcodevideocodec", settingTranscodeVideoCodec);
	settingTranscodeVideo = 0;
	eConfig::getInstance()->getKey("/movieplayer/transcodevideo", settingTranscodeVideo);
	settingTranscodeAudio = 0;
	eConfig::getInstance()->getKey("/movieplayer/transcodeaudio", settingTranscodeAudio);
}

void eMoviePlayer::writeStreamingServerSettings(eString serverIP, int serverPort, eString DVDDrive, int settingVideoRate, int settingResolution, int settingTranscodeVideoCodec, int settingTranscodeVideo, int settingAudioRate, int settingTranscodeAudio)
{
	eConfig::getInstance()->setKey("/movieplayer/serverip", serverIP.c_str());
	eConfig::getInstance()->setKey("/movieplayer/serverport", serverPort);
	eConfig::getInstance()->setKey("/movieplayer/dvddrive", DVDDrive.c_str());
	eConfig::getInstance()->setKey("/movieplayer/resolution", settingResolution);
	eConfig::getInstance()->setKey("/movieplayer/audiorate", settingAudioRate);
	eConfig::getInstance()->setKey("/movieplayer/videorate", settingVideoRate);
	eConfig::getInstance()->setKey("/movieplayer/transcodevideocodec", settingTranscodeVideoCodec);
	eConfig::getInstance()->setKey("/movieplayer/transcodevideo", settingTranscodeVideo);
	eConfig::getInstance()->setKey("/movieplayer/transcodeaudio", settingTranscodeAudio);
}

void pvrThreadCleanup(void *pvrfd)
{
	close(*(int *)pvrfd);
	eDebug("[MOVIEPLAYER] pvrThreadCleanup done.");
}

void *pvrThread(void *pvrfd)
{
	char tempBuffer[BLOCKSIZE];
	int rd = 0;
	int tsBufferSize = 0;
	eDebug("[MOVIEPLAYER] pvrThread starting: pvrfd = %d", *(int *)pvrfd);
	pthread_cleanup_push(pvrThreadCleanup, (void *)pvrfd);
	nice(-1);
	while (true)
	{
		pthread_testcancel();
		pthread_mutex_lock(&mutex);
		rd = tsBuffer.read(tempBuffer, BLOCKSIZE);
		tsBufferSize = tsBuffer.size();
		pthread_mutex_unlock(&mutex);
		if (rd > 0)
		{
			write(*(int *)pvrfd, tempBuffer, rd);
			eDebug("[MOVIEPLAYER] %d >>> writing %d bytes to pvr...", tsBufferSize, rd);
		}
	}
	pthread_cleanup_pop(1);
}

void receiverThreadCleanup(void *fd)
{
	close(*(int *)fd);
	killPVRThread();
	eMoviePlayer::getInstance()->leaveStreamingClient();
	eDebug("[MOVIEPLAYER] receiverThreadCleanup done.");
}

void *receiverThread(void *fd)
{
	char tempBuffer[BLOCKSIZE];
	int len = 0;
	int tsBufferSize = 0;
	int errors = 0;

	eDebug("[MOVIEPLAYER] receiverThread starting: fd = %d", *(int *)fd);
	pthread_cleanup_push(receiverThreadCleanup, (void *)fd);
	nice(-1);
	// fill buffer
	while (true)
	{
		pthread_testcancel();
		pthread_mutex_lock(&mutex);
		tsBufferSize = tsBuffer.size();
		pthread_mutex_unlock(&mutex);
		errors = (tsBufferSize == 0) ? errors + 1 : 0;
		if (errors > 100000)
			break;
		if (tsBufferSize < INITIALBUFFER)
		{
			len = recv(*(int *)fd, tempBuffer, BLOCKSIZE, 0);
//			eDebug("[MOVIEPLAYER] %d <<< writing %d bytes to buffer...", tsBufferSize, len);
			if (len > 0)
			{
				pthread_mutex_lock(&mutex);
				tsBuffer.write(tempBuffer, len);
				pthread_mutex_unlock(&mutex);
			}
		}
	}
	pthread_exit(NULL);
	pthread_cleanup_pop(1);
}



