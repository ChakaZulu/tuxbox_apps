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
#include <src/enigma_dyn_utils.h>

#define BLOCKSIZE 65424
#define PVRDEV "/dev/pvr"
#define INITIALBUFFERING 5*BLOCKSIZE

static eIOBuffer *tsBuffer;
static pthread_mutex_t mutex = PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP;

extern void find_avpids(int fd, int *vpid, int *apid, int *ac3);
extern int tcpOpen(eString, int);
extern int tcpRequest(int fd, char *ioBuf, int maxLen);
extern CURLcode sendGetRequest (const eString& url, eString& response, bool useAuthorization);

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
	serverPort = 8080;
	eConfig::getInstance()->getKey("/movieplayer/serverport", serverPort);
	char *serverip;
	if (eConfig::getInstance()->getKey("/movieplayer/serverip", serverip))
		serverip = strdup("");
	serverIP = eString(serverip);
	free(serverip);
	eDebug("[MOVIEPLAYER] Server IP: %s", serverIP.c_str());
	eDebug("[MOVIEPLAYER] Server Port: %d", serverPort);
	
	run();
}

eMoviePlayer::~eMoviePlayer()
{
	eDebug("[MOVIEPLAYER] stopping...");
	messages.send(Message(Message::stop, ""));
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
	settingResolution = 0;
	eConfig::getInstance()->getKey("/movieplayer/resolution", settingResolution);
	settingAudioRate = 0;
	eConfig::getInstance()->getKey("/movieplayer/audiorate", settingAudioRate);
	settingVideoRate = 0;
	eConfig::getInstance()->getKey("/movieplayer/videorate", settingVideoRate);
	settingTranscodeVideoCodec = 0;
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

void eMoviePlayer::start(eString mrl)
{
	eDebug("[MOVIEPLAYER] issueing start...");
	messages.send(Message(Message::start, mrl));
}

void eMoviePlayer::stop()
{
	eDebug("[MOVIEPLAYER] issueing stop...");
	messages.send(Message(Message::stop, ""));
}

int eMoviePlayer::VLCStartsTalking()
{
	int skt;

	eDebug("[MOVIEPLAYER] wait for VLC talking to us...");

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
		while ((unsigned int)(bp - line) < sizeof(line))
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

bool AVPids(int skt, int *apid, int *vpid, int *ac3)
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
	find_avpids(fd, vpid, apid, ac3);
	close(fd);
	remove("/tmp/tmpts");
	
	eDebug("[MOVIEPLAYER] found apid: 0x%04X, vpid: 0x%04X, ac3: %d", *apid, *vpid, *ac3);
	
	return (*apid != -1 && *vpid != -1);
}

void eMoviePlayer::playStream(eString mrl)
{
	int apid = 0, vpid = 0, ac3 = 0;
	int len = 0;
	int play = 1;
	int skt;
	char tempBuffer[BLOCKSIZE];
	
	// receive video stream from VLC on PC
	eDebug("[MOVIEPLAYER] start playing stream...");
	
	if ((skt = VLCStartsTalking()))
	{
		eDebug("[MOVIEPLAYER] VLC is sending now... now looking for AV pids");

		if (AVPids(skt, &apid, &vpid, &ac3))
		{
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

eString eMoviePlayer::sout(eString mrl)
{
	eString soutURL = "?sout=#";
	eString serverIP, DVDDrive;
	int transcodeAudio = 0, transcodeVideo = 0;
	int serverPort;
	int settingVideoRate, settingResolution, settingTranscodeVideoCodec, settingForceTranscodeVideo, settingAudioRate, settingForceTranscodeAudio, settingForceAviRawAudio;
	
	readStreamingServerSettings(serverIP, serverPort, DVDDrive, settingVideoRate, settingResolution, settingTranscodeVideoCodec, settingForceTranscodeVideo, settingAudioRate, settingForceTranscodeAudio, settingForceAviRawAudio);
	
	eDebug("[MOVIEPLAYER] determine ?sout for mrl: %s", mrl.c_str());
	
	// determine whether VLC has to transcode or not
	if ((mrl.left(4) == "vcd:") ||
	    (mrl.right(3) == "mpg") ||
	    (mrl.right(4) == "mpeg") ||
	    (mrl.right(3) == "m2p"))
	{
		if (settingForceTranscodeVideo)
			transcodeVideo = settingTranscodeVideoCodec;
		else
			transcodeVideo = 0;
		transcodeAudio = settingForceTranscodeAudio;
	}
	else
	{
		transcodeVideo = settingTranscodeVideoCodec;
		if ((mrl.left(3) == "dvd" && settingForceTranscodeAudio) ||
		    (mrl.right(3) == "vob" && settingForceTranscodeAudio) ||
		    (mrl.right(3) == "ac3" && settingForceTranscodeAudio) ||
		    settingForceAviRawAudio)
			transcodeAudio = 0;
		else
			transcodeAudio = 1;
	}
	
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
			soutURL += "vcodec=" + (transcodeVideo == 1) ? "mpgv" : "mp2v";
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

	return soutURL;
}

int eMoviePlayer::sendRequest2VLC(eString command)
{
	CURLcode httpres;
	eString baseURL = "http://" + serverIP + ':' + eString().sprintf("%d", serverPort) + '/';

	eString response;
	httpres = sendGetRequest(baseURL + command, response, false);
	eDebug("[MOVIEPLAYER] HTTP result for vlc command %s: %d - %s", command.c_str(), httpres, response.c_str());
	
	return httpres;
}

extern bool playService(const eServiceReference &ref);

void eMoviePlayer::gotMessage(const Message &msg )
{
	eDVBServiceController *sapi;
	eDebug("[MOVIEPLAYER] received message : %d", msg.type);
	switch (msg.type)
	{
		case Message::start:
		{
			// save current dvb service for later
			if (sapi = eDVB::getInstance()->getServiceAPI())
				suspendedServiceReference = sapi->service;
			// stop dvb service
			eServiceInterface::getInstance()->stop();
			// clear VLC playlist
			sendRequest2VLC("?control=empty");
			// add mrl to VLC playlist
			sendRequest2VLC("?control=add&mrl=" + msg.mrl);
			// set VLC sout...
			sendRequest2VLC(httpEscape(sout(msg.mrl)));
			// start VLC playback of item 0 in playlist
			sendRequest2VLC("?control=play&item=0");
			// receive and play ts stream
			playStream(msg.mrl);
			// restore suspended dvb service
			playService(suspendedServiceReference);
			// shutdown vlc
			sendRequest2VLC("?control=shutdown");
			break;
		}
		case Message::stop:
		{
			// cancel dvr thread
			pthread_cancel(dvr);
			// restore suspended dvb service
			playService(suspendedServiceReference);
			// shutdown vlc
			sendRequest2VLC("?control=shutdown");
			quit(0);
			break;
		}
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
