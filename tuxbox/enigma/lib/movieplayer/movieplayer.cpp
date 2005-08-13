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
#define INITIALBUFFER BLOCKSIZE*40
#endif

eIOBuffer tsBuffer(BLOCKSIZE*4);
static pthread_mutex_t mutex = PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP;

extern void find_avpids(eIOBuffer *tsBuffer, int *vpid, int *apid, int *ac3);
extern int tcpOpen(eString, int);
extern int tcpRequest(int, char *, int);
extern bool playService(const eServiceReference &ref);

pthread_t dvr;
void *dvrThread(void *);
pthread_t receiver;
void *receiverThread(void *);

static int fd = -1;
static int play = -1;
static int tsBufferSize = 0;

eMoviePlayer *eMoviePlayer::instance;

eMoviePlayer::eMoviePlayer(): messages(this,1)
{
	if (!instance)
		instance = this;
		
	CONNECT(messages.recv_msg, eMoviePlayer::gotMessage);
	eDebug("[MOVIEPLAYER] Version 1.4 starting...");
	run();
}

eMoviePlayer::~eMoviePlayer()
{
	play = -1; // terminate receiver and dvr thread if they are still running
	messages.send(Message::quit);
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

void eMoviePlayer::control(const char *command, const char *filename)
{
	eString cmd = eString(command);
	
	eDebug("[MOVIEPLAYER] control: command = %s", command);
	if (cmd == "start")
	{
		play = -1; // terminate threads
		messages.send(Message(Message::start, filename ? strdup(filename) : 0));
	}
	else
	if (cmd == "terminate")
	{
		play = -1; // terminate threads
	}
	if (cmd == "play")
	{
	}
	else
	if (cmd == "stop")
	{
	}
	else 
	if (cmd == "pause")
	{
	}
	else
	if (cmd == "rewind")
	{
	}
	else
	if (cmd == "forward")
	{
	}
}

int eMoviePlayer::sendRequest2VLC(eString command)
{
	char ioBuffer[512];
	int rc = -1;
	int fd = tcpOpen(serverIP, serverPort);
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
				eDebug("[MOVIEPLAYER] 200 OK not received...");
				rc = -1;
			}
		}
		else 
			rc = -2;
		close(fd);
	}
	else
		rc = -3;
	return rc;
}

int bufferStream(int fd, int bufferSize)
{
	int len = 0;
	char tempBuffer[BLOCKSIZE];
	int errors = 0;
	
	eDebug("[MOVIEPLAYER] buffering stream...");
	
	// fill buffer and temp file
	do
	{
		len = recv(fd, tempBuffer, BLOCKSIZE, 0);
		if (len > 0)
		{
//			eDebug("[MOVIEPLAYER] writing %d bytes to buffer, total: %d", len, tsBuffer.size());
			tsBuffer.write(tempBuffer, len);
		}
		else
			errors++;
	}
	while (tsBuffer.size() < bufferSize && errors < 100);
	
	tsBufferSize = tsBuffer.size();
	return tsBufferSize;
}

int eMoviePlayer::playStream(eString mrl)
{
	int apid = -1, vpid = -1, ac3 = -1;
	char ioBuffer[512];
	
	// receive video stream from VLC on PC
	eDebug("[MOVIEPLAYER] start playing stream...");
	
	int retry = 10;
	while (--retry > 0)
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
			retry = 0;
	}

	tsBuffer.clear();
	
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
	
	eDebug("[MOVIEPLAYER] found apid: 0x%04X, vpid: 0x%04X, ac3: %d", apid, vpid, ac3);
	
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

	play = 1;
			
	// create receiver thread
	pthread_create(&receiver, 0, receiverThread, (void *)&play);
	// create dvr thread
	pthread_create(&dvr, 0, dvrThread, (void *)&play);
	pthread_join(receiver, 0);
	play = -1; // request termination of dvr thread
	pthread_join(dvr, 0);

	Decoder::Flush();
	
	tsBuffer.clear();

	// restore suspended dvb service
	playService(suspendedServiceReference);
	
	return 0;
}

void eMoviePlayer::gotMessage(const Message &msg )
{
	eString mrl;
	switch (msg.type)
	{
		case Message::start:
		{
			if (msg.filename)
			{
				mrl = eString(msg.filename);
				eDebug("[MOVIEPLAYER] mrl = %s", mrl.c_str());
				free((char*)msg.filename);
				
				serverPort = 8080;
				eConfig::getInstance()->getKey("/movieplayer/serverport", serverPort);
				char *serverip;
				if (eConfig::getInstance()->getKey("/movieplayer/serverip", serverip))
					serverip = strdup("");
				serverIP = eString(serverip);
				free(serverip);
				eDebug("[MOVIEPLAYER] Server IP: %s", serverIP.c_str());
				eDebug("[MOVIEPLAYER] Server Port: %d", serverPort);
				
				int retry = 20;
				while (--retry > 0)
				{
					eDebug("[MOVIEPLAYER] trying to get vlc going... retry = %d", retry);
					// vlc: empty playlist
					if (sendRequest2VLC("?control=empty") < 0)
					{
						eDebug("[MOVIEPLAYER] couldn't communicate with vlc, streaming server ip address may be wrong in settings.");
						usleep(100000);
						continue;
					}
					// vlc: add mrl to playlist
					if (sendRequest2VLC("?control=add&mrl=" + httpEscape(mrl)) < 0)
						continue;
					// vlc: set sout...
					if (sendRequest2VLC("?sout=" + httpEscape(sout(mrl))) < 0)
						continue;
					// vlc: start playback of first item in playlist
					if (sendRequest2VLC("?control=play&item=0") < 0)
						continue;
					// receive and play ts stream
					if (playStream(mrl) < 0)
						continue;
					else
						retry = 0;
				}
			}
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
	serverPort = 8080;
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

void *dvrThread(void *ctrl)
{
	char tempBuffer[BLOCKSIZE];
	int rd = 0;
	int pvrfd = 0;
	pvrfd = open(PVRDEV, O_RDWR);
	eDebug("[MOVIEPLAYER] dvrThread starting: pvrfd = %d", pvrfd);
	nice(-1);
	while (play > 0)
	{
		pthread_mutex_lock(&mutex);
		rd = tsBuffer.read(tempBuffer, BLOCKSIZE);
		tsBufferSize = tsBuffer.size();
		pthread_mutex_unlock(&mutex);
		if (rd > 0)
		{
			write(pvrfd, tempBuffer, rd);
			eDebug("[MOVIEPLAYER] %d >>> writing %d bytes to dvr...", tsBufferSize, rd);
		}
	}
	close(pvrfd);
	eDebug("[MOVIEPLAYER] dvrThread stopping...");
	pthread_exit(NULL);
}

void *receiverThread(void *ctrl)
{
	char tempBuffer[BLOCKSIZE];
	int len = 0;
	eDebug("[MOVIEPLAYER] receiverThread starting: fd = %d", fd);
	nice(-1);
	// fill buffer
	do
	{
		if (tsBufferSize < INITIALBUFFER)
		{
			len = recv(fd, tempBuffer, BLOCKSIZE, 0);
//			eDebug("[MOVIEPLAYER] %d <<< writing %d bytes to buffer...", tsBufferSize, len);
			if (len > 0)
			{
				pthread_mutex_lock(&mutex);
				tsBuffer.write(tempBuffer, len);
				pthread_mutex_unlock(&mutex);
			}
		}
	}
	while (tsBufferSize > 0 && play == 1);
	close(fd);
	eDebug("[MOVIEPLAYER] receiverThread stopping...");
	pthread_exit(NULL);
}

