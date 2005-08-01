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
#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>
#include <lib/base/buffer.h>
#include <lib/system/econfig.h>
#include <lib/dvb/decoder.h>
#include <lib/movieplayer/movieplayer.h>

#define BLOCKSIZE 4*65424
#define PVRDEV "/dev/pvr"
#define INITIALBUFFERING BLOCKSIZE*5

static eIOBuffer *tsBuffer;
static pthread_mutex_t mutex = PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP;

extern void find_avpids(int fd, int *vpid, int *apid, int *ac3);
extern int tcpOpen(eString, int);
extern eString httpEscape(eString);
extern CURLcode sendGetRequest (const eString& url, eString& response, bool useAuthorization);
extern bool playService(const eServiceReference &ref);

pthread_t dvr;
void *dvrThread(void *);
pthread_t receiver;
void *receiverThread(void *);

int skt = -1;
int play = -1;

eMoviePlayer *eMoviePlayer::instance;

eMoviePlayer::eMoviePlayer(): messages(this,1)
{
	if (instance)
		delete instance;
	instance = this;
		
	CONNECT(messages.recv_msg, eMoviePlayer::gotMessage);
	eDebug("[MOVIEPLAYER] starting...");
	serverPort = 8080;
	eConfig::getInstance()->getKey("/movieplayer/serverport", serverPort);
	char *serverip;
	if (eConfig::getInstance()->getKey("/movieplayer/serverip", serverip))
		serverip = strdup("");
	serverIP = eString(serverip);
	free(serverip);
	eDebug("[MOVIEPLAYER] Server IP: %s", serverIP.c_str());
	eDebug("[MOVIEPLAYER] Server Port: %d", serverPort);
	tsBuffer = new eIOBuffer(BLOCKSIZE);
	run();
}

eMoviePlayer::~eMoviePlayer()
{
	play = -1; skt = -1; // terminate receiver and dvr thread if they are still running
	if (thread_running())
		kill();
	if (tsBuffer)
		delete tsBuffer;
	if (instance == this)
		instance = 0;
}

void eMoviePlayer::thread()
{
	nice(5);
	exec();
}

void eMoviePlayer::start(const char *filename)
{
	messages.send(Message(Message::start, filename ? strdup(filename) : 0));
}


int eMoviePlayer::sendRequest2VLC(eString command, bool authenticate)
{
	CURLcode httpres;
	eString baseURL = "http://" + serverIP + ':' + eString().sprintf("%d", serverPort) + '/';
	eDebug("[MOVIEPLAYER] sendRequest2VLC: %s", eString(baseURL + command).c_str());
	eString response;
	httpres = sendGetRequest(baseURL + command, response, authenticate);
	eDebug("[MOVIEPLAYER] HTTP result for vlc command %s: %d", command.c_str(), httpres);
	
	return httpres;
}

bool AVPids(int skt, int *apid, int *vpid, int *ac3)
{
	int len = 0;
	int totallen = 0;
	int error = 0;
	char tempBuffer[BLOCKSIZE];
	int fd = open ("/tmp/tmpts", O_CREAT | O_WRONLY);
	
	eDebug("[MOVIEPLAYER] buffering data...");
	
	// fill buffer and temp file
	do
	{
		len = recv(skt, tempBuffer, BLOCKSIZE, 0);
		if (len > 0)
		{
			eDebug("[MOVIEPLAYER] writing %d bytes to buffer, total: %d", len, totallen);
//			pthread_mutex_lock(&mutex);
			tsBuffer->write(tempBuffer, len);
			write(fd, tempBuffer, len);
//			pthread_mutex_unlock(&mutex);
			totallen += len;
		}
		else 
			error++;
	}
	while (totallen < INITIALBUFFERING && error < 1000);
	
	if (error == 0)
	{
		eDebug("[MOVIEPLAYER] searching for vpid and apid");
		close (fd);
		fd = open ("/tmp/tmpts", O_RDONLY);
		find_avpids(fd, vpid, apid, ac3);
		close(fd);
		remove("/tmp/tmpts");
	
		eDebug("[MOVIEPLAYER] found apid: 0x%04X, vpid: 0x%04X, ac3: %d", *apid, *vpid, *ac3);
	}
	
	return (*apid != -1 && *vpid != -1);
}

int eMoviePlayer::playStream(eString mrl)
{
	int apid = -1, vpid = -1, ac3 = -1;
	
	// receive video stream from VLC on PC
	eDebug("[MOVIEPLAYER] start playing stream...");
	
	skt = tcpOpen(serverIP, serverPort);
	if (skt == -1)
	{
		eDebug("[MOVIEPLAYER] couldn't connect socket");
		return -1;
	}
	
	eDebug("[MOVIEPLAYER] socket connected: skt = %d", skt);
	fcntl(skt, O_NONBLOCK);
	
	const char * msg = "GET /dboxstream HTTP/1.0\r\n\r\n";
	if (send(skt, msg, strlen (msg), 0) == -1)
	{
		eDebug("[MOVIEPLAYER] sending GET failed.");
		close(skt);
		return -2;
	}
		
	eDebug("[MOVIEPLAYER] GET request sent.");
		
	// Skip HTTP Header
	char line[256];
	memset(line, '\0', sizeof(line));
	char *bp = line;
	while ((unsigned int)(bp - line) < sizeof(line))
	{
//		eDebug("[MOVIEPLAYER] reading: %s", line);
		recv(skt, bp, 1, 0);
		if (strstr(line, "\r\n\r\n") != 0)
		{
			if (strstr(line, "HTTP/1.0 404") != 0)
			{
				eDebug("[MOVIEPLAYER] VLC header not received...");
				close(skt);
				return -3;
			}
			else
			{
				eDebug("[MOVIEPLAYER] VLC header received.");
				break;
			}
		}
		bp++;
	}

	if (!(AVPids(skt, &apid, &vpid, &ac3)))
	{
		eDebug("[MOVIEPLAYER] could not find AV pids.");
		close(skt);
		return -4;
	}
	
	eDebug("[MOVIEPLAYER] AV pids found.");
	
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
	if (ac3)
	{
		if (mrl.right(3) == "vob" || mrl.left(3) == "dvd")
			Decoder::parms.audio_type = DECODE_AUDIO_AC3_VOB;
		else
			Decoder::parms.audio_type = DECODE_AUDIO_AC3;
	}
	
	Decoder::Set();
			
	// create receiver thread
	pthread_create(&receiver, 0, receiverThread, (void *)&skt);
	// create dvr thread
	play = 1;
	pthread_create(&dvr, 0, dvrThread, (void *)&play);
			
	while (skt != -1)
	{
		sleep(1);
	}
	play = 0; // terminate dvr thread
	while (play != -1) 
	{
		// wait for dvr thread to terminate
		sleep(1);
	}
	
	usleep(100000); // wait for threads to terminate...
	
	// cancel dvr thread
	if (dvr)
		pthread_cancel(dvr);
	// cancel receiver thread
	if (receiver)
		pthread_cancel(receiver);
		
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
				mrl = msg.filename;
				eDebug("[MOVIEPLAYER] mrl = %s", mrl.c_str());
				free((char*)msg.filename);
				
				int retry = 10;
				do
				{
					// vlc: empty playlist
					if (sendRequest2VLC("?control=empty", false) > 0)
					{
						eDebug("[MOVIEPLAYER] couldn't communicate with vlc, streaming server ip address may be wrong in settings.");
						retry = 0;
						break;
					}
					// vlc: add mrl to playlist
					sendRequest2VLC("?control=add&mrl=" + httpEscape(mrl), false);
					// vlc: set sout...
					sendRequest2VLC("?sout=" + httpEscape(sout(mrl)), false);
					// vlc: start playback of first item in playlist
					sendRequest2VLC("?control=play&item=0", false);
					// receive and play ts stream
				} while (playStream(mrl) < 0 && retry-- > 0);

				// shutdown vlc
				sendRequest2VLC("admin/?control=shutdown", true);
			}
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
	int transcodeAudio = 0, transcodeVideo = 0;
	int serverPort;
	int settingVideoRate, settingResolution, settingTranscodeVideoCodec, settingForceTranscodeVideo, settingAudioRate, settingForceTranscodeAudio, settingForceAviRawAudio;
	
	readStreamingServerSettings(serverIP, serverPort, DVDDrive, settingVideoRate, settingResolution, settingTranscodeVideoCodec, settingForceTranscodeVideo, settingAudioRate, settingForceTranscodeAudio, settingForceAviRawAudio);
	
	eDebug("[MOVIEPLAYER] determine ?sout for mrl: %s", mrl.c_str());
	
	if (settingForceTranscodeVideo)
		transcodeVideo = settingTranscodeVideoCodec;
	transcodeAudio = settingForceTranscodeAudio;
	
	eDebug("[MOVIEPLAYER] transcoding audio: %d, video: %d", transcodeAudio, transcodeVideo);

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
	
	if (transcodeVideo || transcodeAudio)
	{
		soutURL += "transcode{";
		if (transcodeVideo)
		{
			eString videoCodec = (transcodeVideo == 1) ? "mpgv" : "mp2v";
			soutURL += "vcodec=" + videoCodec;
			soutURL += ",vb=" + eString().sprintf("%d", settingVideoRate);
			soutURL += ",width=" + res_horiz;
			soutURL += ",height=" + res_vert;
		}
		if (transcodeAudio)
		{
			if (transcodeVideo)
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
DVDDrive, int& settingVideoRate, int& settingResolution, int& settingTranscodeVideoCodec, int& settingForceTranscodeVideo, int& settingAudioRate, int& settingForceTranscodeAudio, int& settingForceAviRawAudio)
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
	settingForceTranscodeVideo = 0;
	eConfig::getInstance()->getKey("/movieplayer/forcetranscodevideo", settingForceTranscodeVideo);
	settingForceTranscodeAudio = 0;
	eConfig::getInstance()->getKey("/movieplayer/forcetranscodeaudio", settingForceTranscodeAudio);
	settingForceAviRawAudio = 0;
	eConfig::getInstance()->getKey("/movieplayer/forceavirawaudio", settingForceAviRawAudio);
}

void eMoviePlayer::writeStreamingServerSettings(eString serverIP, int serverPort, eString DVDDrive, int settingVideoRate, int settingResolution, int settingTranscodeVideoCodec, int settingForceTranscodeVideo, int settingAudioRate, int settingForceTranscodeAudio, int settingForceAviRawAudio)
{
	eConfig::getInstance()->setKey("/movieplayer/serverip", serverIP.c_str());
	eConfig::getInstance()->setKey("/movieplayer/serverport", serverPort);
	eConfig::getInstance()->setKey("/movieplayer/dvddrive", DVDDrive.c_str());
	eConfig::getInstance()->setKey("/movieplayer/resolution", settingResolution);
	eConfig::getInstance()->setKey("/movieplayer/audiorate", settingAudioRate);
	eConfig::getInstance()->setKey("/movieplayer/videorate", settingVideoRate);
	eConfig::getInstance()->setKey("/movieplayer/transcodevideocodec", settingTranscodeVideoCodec);
	eConfig::getInstance()->setKey("/movieplayer/forcetranscodevideo", settingForceTranscodeVideo);
	eConfig::getInstance()->setKey("/movieplayer/forcetranscodeaudio", settingForceTranscodeAudio);
	eConfig::getInstance()->setKey("/movieplayer/forceavirawaudio", settingForceAviRawAudio);
}

void *dvrThread(void *ctrl)
{
	char tempBuffer[BLOCKSIZE];
	int rd = 0;
	eDebug("[MOVIEPLAYER] dvrThread starting...");
	int pvrfd = 0;
	timeval t1, t2;
	pvrfd = open(PVRDEV, O_RDWR);
	eDebug("[MOVIEPLAYER] pvr device opened: %d", pvrfd);
	nice(-50);
//	while (*((int *)ctrl) == 1)
	while (*((int *)ctrl) > 0)
	{
		if (tsBuffer->size() > 0)
		{
			pthread_mutex_lock(&mutex);
			rd = tsBuffer->read(tempBuffer, BLOCKSIZE);
			pthread_mutex_unlock(&mutex);
			gettimeofday(&t1, 0);
			write(pvrfd, tempBuffer, rd);
			gettimeofday(&t2, 0);
			eDebug("%d:%d\n%d:%d [MOVIEPLAYER]     >>> writing %d bytes to dvr...", t1.tv_sec, t1.tv_usec, t2.tv_sec, t2.tv_usec, rd);
		}
		else 
			usleep(100);
	}
	close(pvrfd);
	*((int *)ctrl) = -1;
	eDebug("[MOVIEPLAYER] dvrThread stopping...");
	pthread_exit(NULL);
}

void *receiverThread(void *skt)
{
	timeval t1, t2;
	char tempBuffer[BLOCKSIZE];
	int len = 0;
	eDebug("[MOVIEPLAYER] receiverThread starting: skt = %d", *((int *)skt));
	nice(-50);
	// fill buffer
	do
	{
		gettimeofday(&t1, 0);
		len = recv(*((int *)skt), tempBuffer, BLOCKSIZE, 0);
		gettimeofday(&t2, 0);
		eDebug("%d:%d\n%d:%d [MOVIEPLAYER] <<< writing %d bytes to buffer...", t1.tv_sec, t1.tv_usec, t2.tv_sec, t2.tv_usec, len);
		if (len >= 0)
		{
			pthread_mutex_lock(&mutex);
			tsBuffer->write(tempBuffer, len);
			pthread_mutex_unlock(&mutex);
		}
	}
	while (len > 0);
	close(*((int *)skt));
	*((int *)skt) = -1;
	eDebug("[MOVIEPLAYER] receiverThread stopping...");
	pthread_exit(NULL);
}

