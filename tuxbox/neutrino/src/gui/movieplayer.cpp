/*
  Neutrino-GUI  -   DBoxII-Project

  Movieplayer (c) 2003, 2004 by gagga
  Based on code by Dirch, obi and the Metzler Bros. Thanks.

  $Id: movieplayer.cpp,v 1.84 2004/04/05 15:05:19 thegoodguy Exp $

  Homepage: http://www.giggo.de/dbox2/movieplayer.html

  License: GPL

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if HAVE_DVB_API_VERSION >= 3

#include <gui/movieplayer.h>

#include <global.h>
#include <neutrino.h>

#include <driver/fontrenderer.h>
#include <driver/rcinput.h>
#include <daemonc/remotecontrol.h>
extern CRemoteControl * g_RemoteControl; /* neutrino.cpp */
#include <system/settings.h>

#include <gui/eventlist.h>
#include <gui/color.h>
#include <gui/infoviewer.h>
#include <gui/nfs.h>
#include <gui/bookmarkmanager.h>
#include <gui/timeosd.h>

#include <gui/widget/buttons.h>
#include <gui/widget/icons.h>
#include <gui/widget/messagebox.h>
#include <gui/widget/hintbox.h>
#include <gui/widget/stringinput.h>
#include <gui/widget/stringinput_ext.h>

#include <linux/dvb/audio.h>
#include <linux/dvb/dmx.h>
#include <linux/dvb/video.h>

#include <algorithm>
#include <fstream>
#include <sstream>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <transform.h>

#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>

#include <poll.h>

#define ADAP	"/dev/dvb/adapter0"
#define ADEC	ADAP "/audio0"
#define VDEC	ADAP "/video0"
#define DMX	ADAP "/demux0"
#define DVR	ADAP "/dvr0"

#define AVIA_AV_STREAM_TYPE_0           0x00
#define AVIA_AV_STREAM_TYPE_SPTS        0x01
#define AVIA_AV_STREAM_TYPE_PES         0x02
#define AVIA_AV_STREAM_TYPE_ES          0x03

#define STREAMTYPE_DVD	1
#define STREAMTYPE_SVCD	2
#define STREAMTYPE_FILE	3

#define MOVIEPLAYER_ConnectLineBox_Width	15

#define RINGBUFFERSIZE 348*188*10
#define MAXREADSIZE 348*188
#define MINREADSIZE 348*188

//TODO: calculate offset for jumping 1 minute forward/backwards in stream
// needs to be a multiplier of 188
// do a VERY shitty approximation here...
//long long minuteoffset = 557892632/2;
//long long minuteoffset = 8807424;
#define MINUTEOFFSET 30898176
	

static CMoviePlayerGui::state playstate;
static bool isTS, isPES, isBookmark;
int speed = 1;
static long long fileposition;
ringbuffer_t *ringbuf;
bool bufferfilled;
int streamingrunning;
unsigned short pida, pidv;
short ac3;
CHintBox *hintBox;
CHintBox *bufferingBox;
bool avpids_found;
std::string startfilename;
std::string skipvalue;

long long startposition;
int jumpminutes = 1;
int buffer_time = 0;
//------------------------------------------------------------------------
void checkAspectRatio (int vdec, bool init);

size_t
CurlDummyWrite (void *ptr, size_t size, size_t nmemb, void *data)
{
	std::string* pStr = (std::string*) data;
	*pStr += (char*) ptr;
	return size * nmemb;
}

//------------------------------------------------------------------------

CMoviePlayerGui::CMoviePlayerGui()
{
	frameBuffer = CFrameBuffer::getInstance();
	filebrowser = new CFileBrowser ();
	filebrowser->Multi_Select = false;
	filebrowser->Dirs_Selectable = false;
	tsfilefilter.addFilter ("ts");
	vlcfilefilter.addFilter ("mpg");
	vlcfilefilter.addFilter ("mpeg");
	vlcfilefilter.addFilter ("m2p");
	vlcfilefilter.addFilter ("avi");
	vlcfilefilter.addFilter ("vob");
	pesfilefilter.addFilter ("mpv");
	filebrowser->Filter = &tsfilefilter;
	if (strlen (g_settings.network_nfs_moviedir) != 0)
		Path_local = g_settings.network_nfs_moviedir;
	else
		Path_local = "/";
	Path_vlc  = "vlc://";
	Path_vlc += g_settings.streaming_server_startdir;
	Path_vlc_settings = g_settings.streaming_server_startdir;
}

//------------------------------------------------------------------------

CMoviePlayerGui::~CMoviePlayerGui ()
{
	delete filebrowser;
	delete bookmarkmanager;
	g_Zapit->setStandby (false);
	g_Sectionsd->setPauseScanning (false);

}

//------------------------------------------------------------------------
int
CMoviePlayerGui::exec (CMenuTarget * parent, const std::string & actionKey)
{
	printf("[movieplayer.cpp] actionKey=%s\n",actionKey.c_str());
	
	if(Path_vlc_settings != g_settings.streaming_server_startdir)
	{
		Path_vlc  = "vlc://";
		Path_vlc += g_settings.streaming_server_startdir;
		Path_vlc_settings = g_settings.streaming_server_startdir;
	}
	bookmarkmanager = new CBookmarkManager ();

	if (parent)
	{
		parent->hide ();
	}

	bool usedBackground = frameBuffer->getuseBackground();
	if (usedBackground)
	{
		frameBuffer->saveBackgroundImage();
		frameBuffer->ClearFrameBuffer();
	}

	const CBookmark * theBookmark = NULL;
	if (actionKey=="bookmarkplayback") {
		isBookmark = true;
		theBookmark = bookmarkmanager->getBookmark(NULL);
		if (theBookmark == NULL) {
			bookmarkmanager->flush();
			return menu_return::RETURN_REPAINT;
		}
	}
	
	// set zapit in standby mode
	g_Zapit->setStandby (true);

	// tell neutrino we're in ts_mode
	CNeutrinoApp::getInstance ()->handleMsg (NeutrinoMessages::CHANGEMODE,
						 NeutrinoMessages::mode_ts);
	// remember last mode
	m_LastMode =
		(CNeutrinoApp::getInstance ()->
		 getLastMode () | NeutrinoMessages::norezap );

	// Stop sectionsd
	//g_Sectionsd->setPauseScanning (true);

    isBookmark=false;
    startfilename = "";
    startposition = 0;
    isTS=false;
    isPES=false;
    
	if (actionKey=="fileplayback") {
        PlayStream (STREAMTYPE_FILE);	
	}
	else if (actionKey=="dvdplayback") {
        PlayStream (STREAMTYPE_DVD);
	}
	else if (actionKey=="vcdplayback") {
        PlayStream (STREAMTYPE_SVCD);
	}
	else if (actionKey=="tsplayback") {
        isTS=true;
        PlayFile();
	}
	else if (actionKey=="pesplayback") {
        isPES=true;
        PlayFile();
	}
	else if (actionKey=="bookmarkplayback") {
        isBookmark = true;
        if (theBookmark != NULL) {
            startfilename = theBookmark->getUrl();
            sscanf (theBookmark->getTime(), "%lld", &startposition);
            int vlcpos = startfilename.rfind("vlc://");
            if (vlcpos==0)
            {
                PlayStream (STREAMTYPE_FILE);	
            }
            else {
                // TODO check if file is a TS. Not required right now as writing bookmarks is disabled for PES anyway
                isTS = true;
                isPES = false;
                PlayFile();
            }
                
        }

	}

	bookmarkmanager->flush();

	// Restore previous background
	if (usedBackground)
	{
		frameBuffer->restoreBackgroundImage();
		frameBuffer->useBackground(true);
		frameBuffer->paintBackground();
	}

	// Restore last mode
	g_Zapit->setStandby (false);

	// Start Sectionsd
	g_Sectionsd->setPauseScanning (false);

	// Restore last mode
	CNeutrinoApp::getInstance ()->handleMsg (NeutrinoMessages::CHANGEMODE,
						 m_LastMode);
	g_RCInput->postMsg( NeutrinoMessages::SHOW_INFOBAR, 0 );

	CLCD::getInstance()->showServicename(g_RemoteControl->getCurrentChannelName());
	// always exit all
	return menu_return::RETURN_REPAINT;
}

//------------------------------------------------------------------------
CURLcode sendGetRequest (const std::string & url, std::string & response, bool useAuthorization) {
	CURL *curl;
	CURLcode httpres;
  
	curl = curl_easy_init ();
	curl_easy_setopt (curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, CurlDummyWrite);
	curl_easy_setopt (curl, CURLOPT_FILE, (void *)&response);
	if (useAuthorization) curl_easy_setopt (curl, CURLOPT_USERPWD, "admin:admin"); /* !!! make me customizable */
	curl_easy_setopt (curl, CURLOPT_FAILONERROR, true); 
	httpres = curl_easy_perform (curl);
	//printf ("[movieplayer.cpp] HTTP Result: %d\n", httpres);
	curl_easy_cleanup (curl);
	return httpres;
}

//------------------------------------------------------------------------
bool VlcSendPlaylist(char* mrl)
{
	CURLcode httpres;
	std::string baseurl = "http://";
	baseurl += g_settings.streaming_server_ip;
	baseurl += ':';
	baseurl += g_settings.streaming_server_port;
	baseurl += '/';
	
	// empty playlist
	std::string emptyurl = baseurl + "?control=empty";
	std::string response ="";
	httpres = sendGetRequest(emptyurl, response, false);
	printf ("[movieplayer.cpp] HTTP Result (emptyurl): %d\n", httpres);
	if (httpres != 0)
	{
		DisplayErrorMessage(g_Locale->getText("movieplayer.nostreamingserver")); // UTF-8
		playstate = CMoviePlayerGui::STOPPED;
		pthread_exit (NULL);
		// Assume safely that all succeeding HTTP requests are successful
	}

	// add MRL
	/* demo MRLs:
	   - DVD: dvdsimple:D:@1:1
	   - DemoMovie: c:\\TestMovies\\dolby.mpg
	   - SVCD: vcd:D:@1:1
	*/
	std::string addurl = baseurl + "?control=add&mrl=" + (char*) mrl;
	httpres = sendGetRequest(addurl, response, false);
	return (httpres==0);
}
#define TRANSCODE_VIDEO_OFF 0
#define TRANSCODE_VIDEO_MPEG1 1
#define TRANSCODE_VIDEO_MPEG2 2
//------------------------------------------------------------------------
bool VlcRequestStream(int  transcodeVideo, int transcodeAudio)
{
	CURLcode httpres;
	std::string baseurl = "http://";
	baseurl += g_settings.streaming_server_ip;
	baseurl += ':';
	baseurl += g_settings.streaming_server_port;
	baseurl += '/';
	
	// add sout (URL encoded)
	// Example(mit transcode zu mpeg1): ?sout=#transcode{vcodec=mpgv,vb=2000,acodec=mpga,ab=192,channels=2}:duplicate{dst=std{access=http,mux=ts,url=:8080/dboxstream}}
	// Example(ohne transcode zu mpeg1): ?sout=#duplicate{dst=std{access=http,mux=ts,url=:8080/dboxstream}}
	//TODO make this nicer :-)
	std::string souturl;

	//Resolve Resolution from Settings...
	const char * res_horiz;
	const char * res_vert;
	switch (g_settings.streaming_resolution)
	{
		case 0:
			res_horiz = "352";
			res_vert = "288";
			break;
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
	} //switch
	souturl = "#";
	if(transcodeVideo!=TRANSCODE_VIDEO_OFF || transcodeAudio!=0)
	{
		souturl += "transcode{";
		if(transcodeVideo!=TRANSCODE_VIDEO_OFF)
		{
			souturl += "vcodec=";
			souturl += (transcodeVideo == TRANSCODE_VIDEO_MPEG1) ? "mpgv" : "mp2v";
			souturl += ",vb=";
			souturl += g_settings.streaming_videorate;
			souturl += ",width=";
			souturl += res_horiz;
			souturl += ",height=";
			souturl += res_vert;
		}
		if(transcodeAudio!=0)
		{
			if(transcodeVideo!=TRANSCODE_VIDEO_OFF)
				souturl += ",";
			souturl += "acodec=mpga,ab=";
			souturl += g_settings.streaming_audiorate;
			souturl += ",channels=2";
		}
		souturl += "}:";
	}
	souturl += "duplicate{dst=std{access=http,mux=ts,url=:";
	souturl += g_settings.streaming_server_port;
	souturl += "/dboxstream}}";
	
	char *tmp = curl_escape (souturl.c_str (), 0);
	printf("[movieplayer.cpp] URL      : %s?sout=%s\n",baseurl.c_str(), souturl.c_str());
	printf("[movieplayer.cpp] URL(enc) : %s?sout=%s\n",baseurl.c_str(), tmp);
	std::string url = baseurl + "?sout=" + tmp;
	curl_free(tmp);
	std::string response ="";
	httpres = sendGetRequest(url, response, false);

	// play MRL
	std::string playurl = baseurl + "?control=play&item=0";
	httpres = sendGetRequest(playurl, response, false);

	return true; // TODO error checking
}
//------------------------------------------------------------------------
int VlcGetStreamTime()
{
	// TODO calculate REAL position as position returned by VLC does not take the ringbuffer into consideration
	std::string positionurl = "http://";
	positionurl += g_settings.streaming_server_ip;
	positionurl += ':';
	positionurl += g_settings.streaming_server_port;
	positionurl += "/admin/dboxfiles.html?stream_time=true";
	printf("[movieplayer.cpp] positionurl=%s\n",positionurl.c_str());
	std::string response = "";
	CURLcode httpres = sendGetRequest(positionurl, response, true);
	printf("[movieplayer.cpp] httpres=%d, response.length()=%d, stream_time = %s\n",httpres,response.length(),response.c_str());
	if(httpres== 0 && response.length() > 0)
	{
		return atoi(response.c_str());
	}
	else
		return -1;
}
//------------------------------------------------------------------------
int VlcGetStreamLength()
{
	// TODO calculate REAL position as position returned by VLC does not take the ringbuffer into consideration
	std::string positionurl = "http://";
	positionurl += g_settings.streaming_server_ip;
	positionurl += ':';
	positionurl += g_settings.streaming_server_port;
	positionurl += "/admin/dboxfiles.html?stream_length=true";
	printf("[movieplayer.cpp] positionurl=%s\n",positionurl.c_str());
	std::string response = "";
	CURLcode httpres = sendGetRequest(positionurl, response, true);
	printf("[movieplayer.cpp] httpres=%d, response.length()=%d, stream_length = %s\n",httpres,response.length(),response.c_str());
	if(httpres== 0 && response.length() > 0)
	{
		return atoi(response.c_str());
	}
	else
		return -1;
}
//------------------------------------------------------------------------
void *
ReceiveStreamThread (void *mrl)
{
	printf ("[movieplayer.cpp] ReceiveStreamThread started\n");
	int skt;

	int nothingreceived=0;
	
	// Get Server and Port from Config

	if (!VlcSendPlaylist((char*)mrl))
	{
		DisplayErrorMessage(g_Locale->getText("movieplayer.nostreamingserver")); // UTF-8
		playstate = CMoviePlayerGui::STOPPED;
		pthread_exit (NULL);
		// Assume safely that all succeeding HTTP requests are successful
	}
	

	int transcodeVideo, transcodeAudio;
	std::string sMRL=(char*)mrl;
	//Menu Option Force Transcode: Transcode all Files, including mpegs.
	if ((!memcmp((char*)mrl, "vcd:", 4) ||
		  !strcasecmp(sMRL.substr(sMRL.length()-3).c_str(), "mpg") || 
		  !strcasecmp(sMRL.substr(sMRL.length()-4).c_str(), "mpeg") ||
		  !strcasecmp(sMRL.substr(sMRL.length()-3).c_str(), "m2p")))
	{
		if (g_settings.streaming_force_transcode_video)
			transcodeVideo=g_settings.streaming_transcode_video_codec+1;
		else
			transcodeVideo=0;
		transcodeAudio=g_settings.streaming_transcode_audio;
	}
	else
	{
		transcodeVideo=g_settings.streaming_transcode_video_codec+1;
		if((!memcmp((char*)mrl, "dvd", 3) && !g_settings.streaming_transcode_audio) ||
			(!strcasecmp(sMRL.substr(sMRL.length()-3).c_str(), "vob") && !g_settings.streaming_transcode_audio) ||
			(!strcasecmp(sMRL.substr(sMRL.length()-3).c_str(), "ac3") && !g_settings.streaming_transcode_audio) ||
			g_settings.streaming_force_avi_rawaudio)
			transcodeAudio=0;
		else
			transcodeAudio=1;
	}
	VlcRequestStream(transcodeVideo, transcodeAudio);

// TODO: Better way to detect if http://<server>:8080/dboxstream is already alive. For example repetitive checking for HTTP 404.
// Unfortunately HTTP HEAD requests are not supported by VLC :(
// vlc 0.6.3 and up may support HTTP HEAD requests.

// Open HTTP connection to VLC

	const char *server = g_settings.streaming_server_ip.c_str ();
	int port;
	sscanf (g_settings.streaming_server_port, "%d", &port);

	struct sockaddr_in servAddr;
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons (port);
	servAddr.sin_addr.s_addr = inet_addr (server);

	printf ("[movieplayer.cpp] Server: %s\n", server);
	printf ("[movieplayer.cpp] Port: %d\n", port);
	char buf[RINGBUFFERSIZE];
	int len;

	while (true)
	{

		//printf ("[movieplayer.cpp] Trying to call socket\n");
		skt = socket (AF_INET, SOCK_STREAM, 0);

		printf ("[movieplayer.cpp] Trying to connect socket\n");
		if (connect(skt, (struct sockaddr *) &servAddr, sizeof (servAddr)) < 0)
		{
			perror ("SOCKET");
			playstate = CMoviePlayerGui::STOPPED;
			pthread_exit (NULL);
		}
		fcntl (skt, O_NONBLOCK);
		printf ("[movieplayer.cpp] Socket OK\n");

		// Skip HTTP header
		const char * msg = "GET /dboxstream HTTP/1.0\r\n\r\n";
		int msglen = strlen (msg);
		if (send (skt, msg, msglen, 0) == -1)
		{
			perror ("send()");
			playstate = CMoviePlayerGui::STOPPED;
			pthread_exit (NULL);
		}

		printf ("[movieplayer.cpp] GET Sent\n");

		// Skip HTTP Header
		int found = 0;
		char line[200];
		strcpy (line, "");
		while (true)
		{
			len = recv (skt, buf, 1, 0);
			strncat (line, buf, 1);
			if (strcmp (line, "HTTP/1.0 404") == 0)
			{
				printf ("[movieplayer.cpp] VLC still does not send. Retrying...\n");
				close (skt);
				break;
			}
			if ((((found & (~2)) == 0) && (buf[0] == '\r')) || /* found == 0 || found == 2 */
			    (((found & (~2)) == 1) && (buf[0] == '\n')))   /* found == 1 || found == 3 */
			{
				if (found == 3)
					goto vlc_is_sending;
				else
					found++;
			}
			else
			{
				found = 0;
			}
		}
	}
 vlc_is_sending:
	printf ("[movieplayer.cpp] Now VLC is sending. Read sockets created\n");
	hintBox->hide ();
	bufferingBox->paint ();
	printf ("[movieplayer.cpp] Buffering approx. 3 seconds\n");

	int size;
	streamingrunning = 1;
	int fd = open ("/tmp/tmpts", O_CREAT | O_WRONLY);

	struct pollfd poller[1];
	poller[0].fd = skt;
	poller[0].events = POLLIN | POLLPRI;
	int pollret;

	while (streamingrunning == 1)
	{
		while ((size = ringbuffer_write_space (ringbuf)) == 0)
		{
			if (playstate == CMoviePlayerGui::STOPPED)
			{
				close(skt);
				pthread_exit (NULL);
			}
			if (!avpids_found)
			{
				printf("[movieplayer.cpp] Searching for vpid and apid\n");
				// find apid and vpid. Easiest way to do that is to write the TS to a file 
				// and use the usual find_avpids function. This is not even overhead as the
				// buffer needs to be prefilled anyway
				close (fd);
				fd = open ("/tmp/tmpts", O_RDONLY);
				//Use global pida, pidv
				//unsigned short pidv = 0, pida = 0;
				find_avpids (fd, &pidv, &pida);
				lseek(fd, 0, SEEK_SET);
				ac3 = (is_audio_ac3(fd) > 0);
				close (fd);
				printf ("[movieplayer.cpp] ReceiveStreamThread: while streaming found pida: 0x%04X ; pidv: 0x%04X ; ac3: %d\n",
					pida, pidv, ac3);
				avpids_found = true;
			}
			if (!bufferfilled) {
				bufferingBox->hide ();
				//TODO reset drivers?
				bufferfilled = true;
			}
		}
		//printf("[movieplayer.cpp] ringbuf write space:%d\n",size);

		if (playstate == CMoviePlayerGui::STOPPED)
		{
			close(skt);
			pthread_exit (NULL);
		}

		pollret = poll (poller, (unsigned long) 1, -1);

		if ((pollret < 0) ||
		    ((poller[0].revents & (POLLHUP | POLLERR | POLLNVAL)) != 0))
		{
			perror ("Error while polling()");
			playstate = CMoviePlayerGui::STOPPED;
			close(skt);
			pthread_exit (NULL);
		}


		if ((poller[0].revents & (POLLIN | POLLPRI)) != 0)
			{
			len = recv (poller[0].fd, buf, size, 0);
		    }
		else
			len = 0;

		if (len > 0)
		{
			nothingreceived = 0;
			//printf ("[movieplayer.cpp] bytes received:%d\n", len);
			if (!avpids_found)
			{
				write (fd, buf, len);
			}
		}
		else {
			if (playstate == CMoviePlayerGui::PLAY) {
				nothingreceived++;
				if (nothingreceived > (buffer_time + 3)*100) // wait at least buffer time secs +3 to play buffer when stream ends
			   {
					printf ("[movieplayer.cpp] ReceiveStreamthread: Didn't receive for a while. Stopping.\n");
					playstate = CMoviePlayerGui::STOPPED;	
				}
				usleep(10000); //sleep 10 ms
			}
		}
      
		while (len > 0)
		{
			len -= ringbuffer_write (ringbuf, buf, len);
		}

	}
	close(skt);
	pthread_exit (NULL);
}


//------------------------------------------------------------------------
void *
PlayStreamThread (void *mrl)
{
	CURLcode httpres;
	struct dmx_pes_filter_params p;
	ssize_t wr;
	char buf[348 * 188];
	bool failed = false;
	// use global pida and pidv
	pida = 0, pidv = 0, ac3 = -1;
	int done, dmxa=-1 , dmxv = -1, dvr = -1, adec = -1, vdec = -1;

	ringbuf = ringbuffer_create (RINGBUFFERSIZE);
	printf ("[movieplayer.cpp] ringbuffer created\n");

	bufferingBox = new CHintBox("messagebox.info", g_Locale->getText("movieplayer.buffering")); // UTF-8

	std::string baseurl = "http://";
	baseurl += g_settings.streaming_server_ip;
	baseurl += ':';
	baseurl += g_settings.streaming_server_port;
	baseurl += '/';

	printf ("[movieplayer.cpp] mrl:%s\n", (char *) mrl);
	pthread_t rcst;
	pthread_create (&rcst, 0, ReceiveStreamThread, mrl);
	//printf ("[movieplayer.cpp] ReceiveStreamThread created\n");
	if ((dmxa =
	     open (DMX, O_RDWR | O_NONBLOCK)) < 0
	    || (dmxv =
		open (DMX,
		      O_RDWR | O_NONBLOCK)) < 0
	    || (dvr =
		open (DVR,
		      O_WRONLY | O_NONBLOCK)) < 0
	    || (adec =
		open (ADEC,
		      O_RDWR | O_NONBLOCK)) < 0
	    || (vdec = open (VDEC, O_RDWR | O_NONBLOCK)) < 0)
	{
		failed = true;
	}

	playstate = CMoviePlayerGui::SOFTRESET;
	printf ("[movieplayer.cpp] read starting\n");
	size_t readsize, len;
	len = 0;
	bool driverready = false;
	std::string pauseurl   = baseurl + "?control=pause";
	std::string unpauseurl = baseurl + "?control=pause";
	std::string skipurl;
	std::string response = "";
	
	checkAspectRatio(vdec, true);
	
	while (playstate > CMoviePlayerGui::STOPPED)
	{
		readsize = ringbuffer_read_space (ringbuf);
		if (readsize > MAXREADSIZE)
		{
			readsize = MAXREADSIZE;
		}
		//printf("[movieplayer.cpp] readsize=%d\n",readsize);
		if (bufferfilled)
		{
			if (!driverready)
			{
				driverready = true;
				// pida and pidv should have been set by ReceiveStreamThread now
				printf ("[movieplayer.cpp] PlayStreamthread: while streaming found pida: 0x%04X ; pidv: 0x%04X ac3: %d\n",
					pida, pidv, ac3);

				p.input = DMX_IN_DVR;
				p.output = DMX_OUT_DECODER;
				p.flags = DMX_IMMEDIATE_START;
				p.pid = pida;
				p.pes_type = DMX_PES_AUDIO;
				if (ioctl (dmxa, DMX_SET_PES_FILTER, &p) < 0)
					failed = true;
				p.pid = pidv;
				p.pes_type = DMX_PES_VIDEO;
				if (ioctl (dmxv, DMX_SET_PES_FILTER, &p) < 0)
					failed = true;
				if (ac3 == 1) {
					printf("Setting bypass mode\n");
					if (ioctl (adec, AUDIO_SET_BYPASS_MODE,0UL)<0)
					{
						perror("AUDIO_SET_BYPASS_MODE");
						failed=true;
					}
				}
				else
				{
					ioctl (adec, AUDIO_SET_BYPASS_MODE,1UL);
				}
				if (ioctl (adec, AUDIO_PLAY) < 0)
				{
					perror ("AUDIO_PLAY");
					failed = true;
				}

				if (ioctl (vdec, VIDEO_PLAY) < 0)
				{
					perror ("VIDEO_PLAY");
					failed = true;
				}

				ioctl (dmxv, DMX_START);
				ioctl (dmxa, DMX_START);
				printf ("[movieplayer.cpp] PlayStreamthread: Driver successfully set up\n");
				bufferingBox->hide ();
				// Calculate diffrence between vlc time and play time
				// movieplayer is about to start playback so ask vlc for his position
				if ((buffer_time = VlcGetStreamTime()) < 0)
					buffer_time=0;
			}

			len = ringbuffer_read (ringbuf, buf, (readsize / 188) * 188);

			if (startposition > 0) {
			    printf ("[movieplayer.cpp] Was Bookmark. Skipping to startposition\n");
			    char tmpbuf[30];
			    sprintf(tmpbuf,"%lld",startposition);
			    skipvalue = tmpbuf;
			    startposition = 0;
			    playstate = CMoviePlayerGui::SKIP;
			}

			switch (playstate)
			{
			case CMoviePlayerGui::PAUSE:
				//ioctl (dmxv, DMX_STOP);
				ioctl (dmxa, DMX_STOP);

				// pause VLC
	            httpres = sendGetRequest(pauseurl, response, false);

				while (playstate == CMoviePlayerGui::PAUSE)
				{
					//ioctl (dmxv, DMX_STOP);	
					//ioctl (dmxa, DMX_STOP);
					usleep(100000); // no busy wait
				}
				// unpause VLC
				httpres = sendGetRequest(unpauseurl, response, false);
				speed = 1;
				break;
			case CMoviePlayerGui::SKIP:
			{
				skipurl = baseurl;
				skipurl += "?control=seek&seek_value=";
				char * tmp = curl_escape(skipvalue.c_str(), 0);
				skipurl += tmp;
				curl_free(tmp);
				printf("[movieplayer.cpp] skipping URL(enc) : %s\n",skipurl.c_str());
				int bytes = (ringbuffer_read_space(ringbuf) / 188) * 188;
				ringbuffer_read_advance(ringbuf, bytes);
				httpres = sendGetRequest(skipurl, response, false);
//				playstate = CMoviePlayerGui::RESYNC;
				playstate = CMoviePlayerGui::PLAY;
			}
			break;
			case CMoviePlayerGui::RESYNC:
				printf ("[movieplayer.cpp] Resyncing\n");
				ioctl (dmxa, DMX_STOP);
				printf ("[movieplayer.cpp] Buffering approx. 3 seconds\n");
				bufferfilled=false;
				bufferingBox->paint ();
				ioctl (dmxa, DMX_START);
				playstate = CMoviePlayerGui::PLAY;
				break;
			case CMoviePlayerGui::PLAY:
				if (len < MINREADSIZE)
				{
					bufferingBox->paint ();
					printf ("[movieplayer.cpp] Buffering approx. 3 seconds\n");
					bufferfilled = false;
			
				}
				//printf ("[movieplayer.cpp] [%d bytes read from ringbuf]\n", len);
				
				done = 0;
				while (len > 0)
				{
					wr = write (dvr, &buf[done], len);
					//printf ("[movieplayer.cpp] [%d bytes written]\n", wr);
					len -= wr;
					done += wr;
				}
				break;
			case CMoviePlayerGui::SOFTRESET:
				ioctl (vdec, VIDEO_STOP);
				ioctl (adec, AUDIO_STOP);
				ioctl (dmxv, DMX_STOP);
				ioctl (dmxa, DMX_STOP);
				ioctl (vdec, VIDEO_PLAY);
				if (ac3 == 1) {
					ioctl (adec, AUDIO_SET_BYPASS_MODE, 0UL );
				}
				else
				{
					ioctl (adec, AUDIO_SET_BYPASS_MODE,1UL);
				}
				ioctl (adec, AUDIO_PLAY);
				p.pid = pida;
				p.pes_type = DMX_PES_AUDIO;
				ioctl (dmxa, DMX_SET_PES_FILTER, &p);
				p.pid = pidv;
				p.pes_type = DMX_PES_VIDEO;
				ioctl (dmxv, DMX_SET_PES_FILTER, &p);
				ioctl (dmxv, DMX_START);
				ioctl (dmxa, DMX_START);
				speed = 1;
				playstate = CMoviePlayerGui::PLAY;
				break;
			case CMoviePlayerGui::STOPPED:
			case CMoviePlayerGui::PREPARING:
			case CMoviePlayerGui::STREAMERROR:
			case CMoviePlayerGui::FF:
			case CMoviePlayerGui::REW:
			case CMoviePlayerGui::JF:
			case CMoviePlayerGui::JB:
				break;
			}
		}
		else
			usleep(10000); // non busy wait
		
		checkAspectRatio(vdec, false);
	}

	ioctl (vdec, VIDEO_STOP);
	ioctl (adec, AUDIO_STOP);
	ioctl (dmxv, DMX_STOP);
	ioctl (dmxa, DMX_STOP);
	close (dmxa);
	close (dmxv);
	close (dvr);
	close (adec);
	close (vdec);

	// stop VLC
	std::string stopurl = baseurl + "?control=stop";
	httpres = sendGetRequest(stopurl, response, false);

	printf ("[movieplayer.cpp] Waiting for RCST to stop\n");
	pthread_join (rcst, NULL);
	printf ("[movieplayer.cpp] Seems that RCST was stopped succesfully\n");
  
	// Some memory clean up
	ringbuffer_free(ringbuf);
	delete bufferingBox;
	delete hintBox;
	
	pthread_exit (NULL);
}

//------------------------------------------------------------------------
void *
PlayPESFileThread (void *filename)
{
	struct pollfd pfd[2];
	unsigned char abuf[188 * 188], vbuf[188 * 188];
	const char *afilename, *vfilename;
	int adec, vdec, afile, vfile;
	ssize_t awr, vwr;
	size_t ar = 0, vr = 0;
	int adone = 0, vdone = 0;
	unsigned int acaps, vcaps;

	vfilename = (const char*) filename;
	
    std::string afilenametmp = vfilename;
    afilenametmp = afilenametmp.substr(0,afilenametmp.length()-3);
    afilenametmp += "mp2";
    afilename = afilenametmp.c_str();
	//afilename = "/mnt/movies/testfilm.mp2";
	
	printf("[movieplayer.cpp] Starting PES Playback\n");
	printf("[movieplayer.cpp] vfile=%s\n",vfilename);
	printf("[movieplayer.cpp] afile=%s\n",afilename);
	
	if ((adec = open(ADEC, O_WRONLY | O_NONBLOCK)) < 0) {
		perror(ADEC);
		playstate = CMoviePlayerGui::STOPPED;
		pthread_exit (NULL);
	}

	if ((vdec = open(VDEC, O_WRONLY)) < 0) {
		perror(VDEC);
		playstate = CMoviePlayerGui::STOPPED;
		pthread_exit (NULL);
	}

	if ((afile = open(afilename, O_RDONLY)) < 0) {
		perror(afilename);
		playstate = CMoviePlayerGui::STOPPED;
		pthread_exit (NULL);
	}

	if ((vfile = open(vfilename, O_RDONLY)) < 0) {
		perror(vfilename);
		playstate = CMoviePlayerGui::STOPPED;
		pthread_exit (NULL);
	}

	if (ioctl(adec, AUDIO_GET_CAPABILITIES, &acaps) < 0) {
		perror("AUDIO_GET_CAPABILITIES");
		playstate = CMoviePlayerGui::STOPPED;
		pthread_exit (NULL);
	}

	if (ioctl(vdec, VIDEO_GET_CAPABILITIES, &vcaps) < 0) {
		perror("VIDEO_GET_CAPABILITIES");
		playstate = CMoviePlayerGui::STOPPED;
		pthread_exit (NULL);
	}

	if (!(acaps & AUDIO_CAP_MP2)) {
		fprintf(stderr, "audio decoder does not support mpeg2 pes\n");
		playstate = CMoviePlayerGui::STOPPED;
		pthread_exit (NULL);
	}

	if (!(vcaps & VIDEO_CAP_MPEG2)) {
		fprintf(stderr, "video decoder does not support mpeg2 pes\n");
		playstate = CMoviePlayerGui::STOPPED;
		pthread_exit (NULL);
	}

	if (ioctl(adec, AUDIO_SELECT_SOURCE, AUDIO_SOURCE_MEMORY) < 0) {
		perror("AUDIO_SELECT_SOURCE");
		playstate = CMoviePlayerGui::STOPPED;
		pthread_exit (NULL);
	}

	if (ioctl(vdec, VIDEO_SELECT_SOURCE, VIDEO_SOURCE_MEMORY) < 0) {
		perror("VIDEO_SELECT_SOURCE");
		playstate = CMoviePlayerGui::STOPPED;
		pthread_exit (NULL);
	}

	if (ioctl(adec, AUDIO_SET_STREAMTYPE, AUDIO_CAP_MP2) < 0) {
		perror("AUDIO_SET_STREAMTYPE");
		playstate = CMoviePlayerGui::STOPPED;
		pthread_exit (NULL);
	}

	if (ioctl(vdec, VIDEO_SET_STREAMTYPE, VIDEO_CAP_MPEG2) < 0) {
		perror("VIDEO_SET_STREAMTYPE");
		playstate = CMoviePlayerGui::STOPPED;
		pthread_exit (NULL);
	}

	if (ioctl(adec, AUDIO_PLAY) < 0) {
		perror("AUDIO_PLAY");
		playstate = CMoviePlayerGui::STOPPED;
		pthread_exit (NULL);
	}

	if (ioctl(vdec, VIDEO_PLAY) < 0) {
		perror("VIDEO_PLAY");
		playstate = CMoviePlayerGui::STOPPED;
		pthread_exit (NULL);
	}
		
	printf("[movieplayer.cpp] Starting PES Playback. All preparations done.\n");
	
	pfd[0].fd = afile;
	pfd[0].events = POLLOUT;
	pfd[1].fd = vfile;
	pfd[1].events = POLLOUT;

	checkAspectRatio(vdec, true);

	while (playstate >= CMoviePlayerGui::PLAY) {
    		switch (playstate)
			{
			case CMoviePlayerGui::PAUSE:
				while (playstate == CMoviePlayerGui::PAUSE)
				{
					usleep(100000); // no busy wait
				}
				break;
			// ignore playstates.
			// TODO: implement them
			case CMoviePlayerGui::PREPARING:
			case CMoviePlayerGui::STREAMERROR:
			case CMoviePlayerGui::RESYNC:
		    case CMoviePlayerGui::FF:
			case CMoviePlayerGui::REW:
			case CMoviePlayerGui::JF:
			case CMoviePlayerGui::JB:
			case CMoviePlayerGui::SOFTRESET:
                playstate = CMoviePlayerGui::PLAY;
				break;
			case CMoviePlayerGui::PLAY:
			case CMoviePlayerGui::STOPPED:
			case CMoviePlayerGui::SKIP:
			    break;
			}

    	
		if (ar <= 0) {
			if ((ar = read(afile, abuf, sizeof(abuf))) < 0) {
				perror("audio read");
				playstate = CMoviePlayerGui::STOPPED;
				break;
			}
			adone = 0;
			printf("[movieplayer.cpp] adone=0\n");
		}
		if (vr <= 0) {
			if ((vr = read(vfile, vbuf, sizeof(vbuf))) < 0) {
				perror("video read");
				playstate = CMoviePlayerGui::STOPPED;
				break;
			}
			vdone = 0;
			printf("[movieplayer.cpp] vdone=0\n");
		}

		if ((ar == 0) && (vr == 0)) {
            playstate = CMoviePlayerGui::STOPPED;
            break;
        }
        printf("[movieplayer.cpp] vr=%d, ar=%d\n",vr,ar);
        
		if (poll(pfd, 2, 0)) {
			if (pfd[0].revents & POLLOUT) {
				if ((awr = write(adec, &abuf[adone], ar)) < 0) {
					if (errno != EAGAIN)
						perror("audio write");
				}
				else {
					ar -= awr;
					adone += awr;
				}
			}
			if (pfd[1].revents & POLLOUT) {
				if ((vwr = write(vdec, &vbuf[vdone], vr)) < 0) {
					perror("video write");
				}
				else {
					vr -= vwr;
					vdone += vwr;
				}
			}
		}
		checkAspectRatio(vdec, false);
	}

	ioctl(vdec, VIDEO_STOP);
	ioctl(adec, AUDIO_STOP);
	close(vfile);
	close(afile);
	close(vdec);
	close(adec);
	
	printf("[movieplayer.cpp] Stopped PES Playback\n");
		
	if (playstate != CMoviePlayerGui::STOPPED)
	{
		playstate = CMoviePlayerGui::STOPPED;
		g_RCInput->postMsg (CRCInput::RC_red, 0);	// for faster exit in PlayStream(); do NOT remove!
	}

	pthread_exit (NULL);
	

}

	
//------------------------------------------------------------------------
void *
PlayFileThread (void *filename)
{
	struct dmx_pes_filter_params p;
	bool failed = false;
	unsigned char buf[384 * 188 * 2];
	unsigned short pida = 0, pidv = 0, ac3=0;
	int done, fd=-1, dmxa=-1, dmxv = -1, dvr = -1, adec = -1, vdec = -1;
	ssize_t wr = 0;
	ssize_t cache = sizeof (buf);
	size_t r = 0;
	
	if ((char *) filename == NULL)
	{
		playstate = CMoviePlayerGui::STOPPED;
		pthread_exit (NULL);
	}

	if ((fd = open ((const char *) filename, O_RDONLY | O_LARGEFILE)) < 0)
	{
		playstate = CMoviePlayerGui::STOPPED;
		pthread_exit (NULL);
	}

	// todo: check if file is valid ts or pes
	if (isTS)
	{
		find_avpids (fd, &pidv, &pida);
		lseek(fd, 0, SEEK_SET);
		ac3 = is_audio_ac3 (fd);
		printf ("[movieplayer.cpp] found pida: 0x%04X ; pidv: 0x%04X ; ac3: %d\n",
			pida, pidv, ac3);
	}
	else
	{				// Play PES
		pida = 0x900;
		pidv = 0x8ff;
	}

	lseek (fd, 0L, SEEK_SET);
	if ((dmxa = open (DMX, O_RDWR)) < 0
	    || (dmxv = open (DMX, O_RDWR)) < 0
	    || (dvr = open (DVR, O_WRONLY)) < 0
	    || (adec = open (ADEC, O_RDWR)) < 0 || (vdec = open (VDEC, O_RDWR)) < 0)
	{
		failed = true;
	}

	p.input = DMX_IN_DVR;
	p.output = DMX_OUT_DECODER;
	p.flags = DMX_IMMEDIATE_START;
	p.pid = pida;
	p.pes_type = DMX_PES_AUDIO;
	if (ioctl (dmxa, DMX_SET_PES_FILTER, &p) < 0)
		failed = true;
	p.pid = pidv;
	p.pes_type = DMX_PES_VIDEO;
	if (ioctl (dmxv, DMX_SET_PES_FILTER, &p) < 0)
		failed = true;
	
	checkAspectRatio(vdec, true);
	
	fileposition = startposition;
	lseek (fd, fileposition, SEEK_SET);
	if (isTS && !failed)
	{
		int mincache_counter = 0;
		bool skipwriting = false;
		while ((r = read (fd, buf, cache)) > 0 && playstate >= CMoviePlayerGui::PLAY)
		{
			done = 0;
			wr = 0;
			fileposition += r;
			switch (playstate)
			{
			case CMoviePlayerGui::PAUSE:
				while (playstate == CMoviePlayerGui::PAUSE)
				{
					ioctl (dmxa, DMX_STOP);
				}
				break;
			case CMoviePlayerGui::FF:
			case CMoviePlayerGui::REW:
				ioctl (dmxa, DMX_STOP);
                if (mincache_counter == 0) {
                    lseek (fd, cache * speed, SEEK_CUR);
                    fileposition += cache * speed;
                }
                mincache_counter ++;
                if (mincache_counter == 2) {
                    mincache_counter = 0;
                }
				break;
			case CMoviePlayerGui::JF:
			case CMoviePlayerGui::JB:
				ioctl (dmxa, DMX_STOP);
                lseek (fd, jumpminutes * MINUTEOFFSET, SEEK_CUR);
                fileposition += jumpminutes * MINUTEOFFSET;
                playstate = CMoviePlayerGui::SOFTRESET;
                skipwriting = true;
                break;
			case CMoviePlayerGui::SOFTRESET:
				ioctl (vdec, VIDEO_STOP);
				ioctl (adec, AUDIO_STOP);
				ioctl (dmxv, DMX_STOP);
				ioctl (dmxa, DMX_STOP);
				ioctl (vdec, VIDEO_PLAY);
				if (ac3 == 1) {
					ioctl (adec, AUDIO_SET_BYPASS_MODE,0UL);
				}
				else
				{
					ioctl (adec, AUDIO_SET_BYPASS_MODE,1UL);
				}
				ioctl (adec, AUDIO_PLAY);
				p.pid = pida;
				p.pes_type = DMX_PES_AUDIO;
				ioctl (dmxa, DMX_SET_PES_FILTER, &p);
				p.pid = pidv;
				p.pes_type = DMX_PES_VIDEO;
				ioctl (dmxv, DMX_SET_PES_FILTER, &p);
				ioctl (dmxv, DMX_START);
				ioctl (dmxa, DMX_START);
				speed = 1;
				playstate = CMoviePlayerGui::PLAY;
				break;
			case CMoviePlayerGui::STOPPED:
			case CMoviePlayerGui::PREPARING:
			case CMoviePlayerGui::STREAMERROR:
			case CMoviePlayerGui::PLAY:
			case CMoviePlayerGui::RESYNC:
			case CMoviePlayerGui::SKIP:
				break;
			}

			if (!skipwriting) {
    			do
    			{
    				wr = write (dvr, &buf[done], r);
    				if (!done)
    					cache = wr;
    				done += wr;
    				r -= wr;
    			}
    			while (r);
			}
			else skipwriting = false;
			
			checkAspectRatio(vdec, false);
		}
	}
	else if (!failed)
	{
		ioctl (vdec, VIDEO_PLAY);
		if (ac3 == 1) {
			ioctl (adec, AUDIO_SET_BYPASS_MODE,0UL);
		}
		else
		{
			ioctl (adec, AUDIO_SET_BYPASS_MODE,1UL);
		}
		ioctl (adec, AUDIO_PLAY);
		ioctl (dmxv, DMX_START);
		ioctl (dmxa, DMX_START);
		pes_to_ts2 (fd, dvr, pida, pidv, (const int *)&playstate);	// VERY bad performance!!!
	}

	ioctl (vdec, VIDEO_STOP);
	ioctl (adec, AUDIO_STOP);
	ioctl (dmxv, DMX_STOP);
	ioctl (dmxa, DMX_STOP);
	close (fd);
	close (dmxa);
	close (dmxv);
	close (dvr);
	close (adec);
	close (vdec);
	if (playstate != CMoviePlayerGui::STOPPED)
	{
		playstate = CMoviePlayerGui::STOPPED;
		g_RCInput->postMsg (CRCInput::RC_red, 0);	// for faster exit in PlayStream(); do NOT remove!
	}

	pthread_exit (NULL);
}


void updateLcd(const std::string & sel_filename)
{
	char tmp[20];
	std::string lcd;
	
	switch(playstate)
	{
	case CMoviePlayerGui::PAUSE:
		lcd = "|| (";
		lcd += sel_filename;
		lcd += ')';
		break;
	case CMoviePlayerGui::REW:
		sprintf(tmp, "%dx<< ", speed);
		lcd = tmp;
		lcd += sel_filename;
		break;
	case CMoviePlayerGui::FF:
		sprintf(tmp, "%dx>> ", speed);
		lcd = tmp;
		lcd += sel_filename;
		break;
	default:
		lcd = "> ";
		lcd += sel_filename;
		break;
	}
	
	CLCD::getInstance()->showServicename(lcd);
}

//------------------------------------------------------------------------
#define SKIPPING_DURATION 3
void
CMoviePlayerGui::PlayStream (int streamtype)
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	std::string sel_filename;
	bool update_info = true, start_play = false, exit =
		false, open_filebrowser = true;
	char mrl[200];
	CTimeOSD StreamTime;

	if (streamtype == STREAMTYPE_DVD)
	{
		strcpy (mrl, "dvdsimple:");
		strcat (mrl, g_settings.streaming_server_cddrive);
		strcat (mrl, "@1:1");
		printf ("[movieplayer.cpp] Generated MRL: %s\n", mrl);
		sel_filename = "DVD";
		open_filebrowser = false;
		start_play = true;
	}
	else if (streamtype == STREAMTYPE_SVCD)
	{
		strcpy (mrl, "vcd:");
		strcat (mrl, g_settings.streaming_server_cddrive);
		strcat (mrl, "@1:1");
		printf ("[movieplayer.cpp] Generated MRL: %s\n", mrl);
		sel_filename = "(S)VCD";
		open_filebrowser = false;
		start_play = true;

	}

	playstate = CMoviePlayerGui::STOPPED;
	/* playstate == CMoviePlayerGui::STOPPED         : stopped
	 * playstate == CMoviePlayerGui::PREPARING       : preparing stream from server
	 * playstate == CMoviePlayerGui::ERROR           : error setting up server
	 * playstate == CMoviePlayerGui::PLAY            : playing
	 * playstate == CMoviePlayerGui::PAUSE           : pause-mode
	 * playstate == CMoviePlayerGui::FF              : fast-forward
	 * playstate == CMoviePlayerGui::REW             : rewind
	 * playstate == CMoviePlayerGui::JF              : jump forward x minutes
	 * playstate == CMoviePlayerGui::JB              : jump backward x minutes
	 * playstate == CMoviePlayerGui::SOFTRESET       : softreset without clearing buffer (playstate toggle to 1)
	 */
	do
	{
		if (exit)
		{
			exit = false;
			if (playstate >= CMoviePlayerGui::PLAY)
			{
				playstate = CMoviePlayerGui::STOPPED;
				break;
			}
		}

		if (isBookmark) {
    	    open_filebrowser = false;
    	    isBookmark = false;
    	    filename = startfilename.c_str();
			int namepos = startfilename.rfind("vlc://");
			std::string mrl_str = startfilename.substr(namepos + 6);
			char *tmp = curl_escape (mrl_str.c_str (), 0);
			strncpy (mrl, tmp, sizeof (mrl) - 1);
			curl_free (tmp);
			printf ("[movieplayer.cpp] Generated Bookmark FILE MRL: %s\n", mrl);
    	    // TODO: What to use for LCD? Bookmarkname? Filename? 
    	    sel_filename = "Bookmark Playback";
    	    update_info=true;
    	    start_play=true;
		}
		
		if (open_filebrowser)
		{
			open_filebrowser = false;
			filename = NULL;
			filebrowser->Filter = &vlcfilefilter;
			if (filebrowser->exec (Path_vlc))
			{
				Path_vlc = filebrowser->getCurrentDir ();
				CFile * file;
				if ((file = filebrowser->getSelectedFile()) != NULL)
				{
					filename = file->Name.c_str();
					sel_filename = file->getFileName();
					//printf ("[movieplayer.cpp] sel_filename: %s\n", filename);
					int namepos = file->Name.rfind("vlc://");
					std::string mrl_str = file->Name.substr(namepos + 6);
					char *tmp = curl_escape (mrl_str.c_str (), 0);
					strncpy (mrl, tmp, sizeof (mrl) - 1);
					curl_free (tmp);
					printf ("[movieplayer.cpp] Generated FILE MRL: %s\n", mrl);

					update_info = true;
					start_play = true;
				}
			}
			else
			{
				if (playstate == CMoviePlayerGui::STOPPED)
					break;
			}

			CLCD::getInstance ()->setMode (CLCD::MODE_TVRADIO);
		}

		if (update_info)
		{
			update_info = false;
			updateLcd(sel_filename);
		}

		if (start_play)
		{
			start_play = false;
			bufferfilled = false;
			avpids_found=false;
	  
			if (playstate >= CMoviePlayerGui::PLAY)
			{
				playstate = CMoviePlayerGui::STOPPED;
				pthread_join (rct, NULL);
			}
			//TODO: Add Dialog (Remove Dialog later)
			hintBox = new CHintBox("messagebox.info", g_Locale->getText("movieplayer.pleasewait")); // UTF-8
			hintBox->paint();
			buffer_time=0;
			if (pthread_create (&rct, 0, PlayStreamThread, (void *) mrl) != 0)
			{
				break;
			}
			playstate = CMoviePlayerGui::SOFTRESET;
		}

		g_RCInput->getMsg (&msg, &data, 10);	// 1 secs..
		if(StreamTime.IsVisible())
		{
			StreamTime.update();
		}
		if (msg == CRCInput::RC_home || msg == CRCInput::RC_red)
		{
			//exit play
			exit = true;
		}
		else if (msg == CRCInput::RC_yellow)
		{
			update_info = true;
			playstate = (playstate == CMoviePlayerGui::PAUSE) ? CMoviePlayerGui::SOFTRESET : CMoviePlayerGui::PAUSE;
			StreamTime.hide();
		}
		else if (msg == CRCInput::RC_green)
		{
			if (playstate == CMoviePlayerGui::PLAY) playstate = CMoviePlayerGui::RESYNC;
			StreamTime.hide();
		}
		else if (msg == CRCInput::RC_blue)
		{
			if (bookmarkmanager->getBookmarkCount() < bookmarkmanager->getMaxBookmarkCount()) 
			{
				int stream_time;
    			if ((stream_time=VlcGetStreamTime()) >= 0)
				{
					std::stringstream stream_time_ss;
					stream_time_ss << (stream_time - buffer_time);
					bookmarkmanager->createBookmark(filename, stream_time_ss.str());
    			}
    			else {
        			DisplayErrorMessage(g_Locale->getText("movieplayer.wrongvlcversion")); // UTF-8
    			}
			}
			else {
    			//popup error message
    			printf("too many bookmarks\n");
    			DisplayErrorMessage(g_Locale->getText("movieplayer.toomanybookmarks")); // UTF-8
			}
		}
		
		else if (msg == CRCInput::RC_1)
		{
			skipvalue = "-00:01:00";
			playstate = CMoviePlayerGui::SKIP;
			StreamTime.hide();
		}
		else if (msg == CRCInput::RC_3)
		{
			skipvalue = "+00:01:00";
			playstate = CMoviePlayerGui::SKIP;
			StreamTime.hide();
		}
		else if (msg == CRCInput::RC_4)
		{
			skipvalue = "-00:05:00";
			playstate = CMoviePlayerGui::SKIP;
			StreamTime.hide();
		}
		else if (msg == CRCInput::RC_6)
		{
			skipvalue = "+00:05:00";
			playstate = CMoviePlayerGui::SKIP;
			StreamTime.hide();
		}
		else if (msg == CRCInput::RC_7)
		{
			skipvalue = "-00:10:00";
			playstate = CMoviePlayerGui::SKIP;
			StreamTime.hide();
		}
		else if (msg == CRCInput::RC_9)
		{
			skipvalue = "+00:10:00";
			playstate = CMoviePlayerGui::SKIP;
			StreamTime.hide();
		}
		else if (msg == CRCInput::RC_down)
		{
			char tmp[10+1];
			bool cancel;

			CTimeInput ti("movieplayer.goto", tmp, "movieplayer.goto.h1", "movieplayer.goto.h2", NULL, &cancel);
			ti.exec(NULL, "");
			if(!cancel) // no cancel
			{
				skipvalue = tmp;
				if(skipvalue[0]== '=')
					skipvalue = skipvalue.substr(1);
				playstate = CMoviePlayerGui::SKIP;
				StreamTime.hide();
			}
		}
		else if (msg == CRCInput::RC_setup)
 		{
			if(StreamTime.IsVisible())
			{
				if(StreamTime.GetMode() == CTimeOSD::MODE_ASC)
				{
					int stream_length = VlcGetStreamLength();
					int stream_time = VlcGetStreamTime();
					if (stream_time >=0 && stream_length >=0)
					{
						StreamTime.SetMode(CTimeOSD::MODE_DESC);
						StreamTime.show(stream_length - stream_time + buffer_time);
					}
					else
						StreamTime.hide();
				}
				else
					StreamTime.hide();
			}
			else
			{
				int stream_time;
				if ((stream_time = VlcGetStreamTime())>=0)
				{
					StreamTime.SetMode(CTimeOSD::MODE_ASC);
					StreamTime.show(stream_time-buffer_time);
				}
			}
 		}
		else if (msg == CRCInput::RC_help)
 		{
     		std::string helptext = g_Locale->getText("movieplayer.vlchelp");
     		std::string fullhelptext = helptext + "\nVersion: $Revision: 1.84 $\n\nMovieplayer (c) 2003, 2004 by gagga";
     		ShowMsgUTF("messagebox.info", fullhelptext.c_str(), CMessageBox::mbrBack, CMessageBox::mbBack, "info.raw"); // UTF-8
 		}
		else
			if (msg == NeutrinoMessages::RECORD_START
			    || msg == NeutrinoMessages::ZAPTO
			    || msg == NeutrinoMessages::STANDBY_ON
			    || msg == NeutrinoMessages::SHUTDOWN
			    || msg == NeutrinoMessages::SLEEPTIMER)
			{
				// Exit for Record/Zapto Timers
				exit = true;
				g_RCInput->postMsg (msg, data);
			}
			else
				if (CNeutrinoApp::getInstance()->handleMsg(msg, data) & messages_return::cancel_all)
				{
					exit = true;
				}
	}
	while (playstate >= CMoviePlayerGui::PLAY);
	pthread_join (rct, NULL);
}

void
CMoviePlayerGui::PlayFile (void)
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	std::string sel_filename;
	CTimeOSD FileTime;
	bool update_lcd = true, open_filebrowser =
		true, start_play = false, exit = false;
	playstate = CMoviePlayerGui::STOPPED;
	/* playstate == CMoviePlayerGui::STOPPED         : stopped
	 * playstate == CMoviePlayerGui::PLAY            : playing
	 * playstate == CMoviePlayerGui::PAUSE           : pause-mode
	 * playstate == CMoviePlayerGui::FF              : fast-forward
	 * playstate == CMoviePlayerGui::REW             : rewind
	 * playstate == CMoviePlayerGui::SOFTRESET       : softreset without clearing buffer (playstate toggle to 1)
	 */
	 
	do
	{
		if (exit)
		{
			exit = false;
			if (playstate >= CMoviePlayerGui::PLAY)
			{
				playstate = CMoviePlayerGui::STOPPED;
				break;
			}
		}

		if (isBookmark) 
		{
    	    open_filebrowser=false;
    	    filename = startfilename.c_str();
    	    sel_filename = startfilename;
    	    update_lcd = true;
			start_play = true;
			isBookmark = false;

    		
		}
		if (open_filebrowser)
		{
			open_filebrowser = false;
			filename = NULL;
			if (isTS)
			{
			    filebrowser->Filter = &tsfilefilter;
		    }
			else 
			{
    			if (isPES) {
        			filebrowser->Filter = &pesfilefilter;
    			}
			}
			if (filebrowser->exec(Path_local))
			{
				Path_local = filebrowser->getCurrentDir();
				CFile * file;
				if ((file = filebrowser->getSelectedFile()) != NULL)
				{
					filename = file->Name.c_str();
					update_lcd = true;
					start_play = true;
					sel_filename = filebrowser->getSelectedFile()->getFileName();
				}
			}
			else
			{
				if (playstate == CMoviePlayerGui::STOPPED)
					break;
			}

			CLCD::getInstance ()->setMode (CLCD::MODE_TVRADIO);
		}

		if (update_lcd)
		{
			update_lcd = false;
			updateLcd(sel_filename);
		}

		if (start_play)
		{
			printf("Startplay\n");
			start_play = false;
			if (playstate >= CMoviePlayerGui::PLAY)
			{
				playstate = CMoviePlayerGui::STOPPED;
				pthread_join (rct, NULL);
			}

			if (isTS) {
			    if (pthread_create
			        (&rct, 0, PlayFileThread, (void *) filename) != 0)
			    {
				    break;
			    }
			}
			    
			else {
			    if (isPES && pthread_create
			        (&rct, 0, PlayPESFileThread, (void *) filename) != 0)
			    {
				    break;
			    }
		    }
			playstate = CMoviePlayerGui::SOFTRESET;
		}

		g_RCInput->getMsg (&msg, &data, 10);	// 1 secs..
		if(FileTime.IsVisible())
		{
			FileTime.update();
		}
		if (msg == CRCInput::RC_red || msg == CRCInput::RC_home)
		{
			//exit play
			exit = true;
		}
		else if (msg == CRCInput::RC_yellow)
		{
			update_lcd = true;
			playstate = (playstate == CMoviePlayerGui::PAUSE) ? CMoviePlayerGui::SOFTRESET : CMoviePlayerGui::PAUSE;
		}
		else if (msg == CRCInput::RC_blue)
		{
			if (bookmarkmanager->getBookmarkCount() < bookmarkmanager->getMaxBookmarkCount())
			{
				char timerstring[200];
				printf("fileposition: %lld\n",fileposition);
				sprintf(timerstring, "%lld",fileposition);
				printf("timerstring: %s\n",timerstring);
				std::string bookmarktime = "";
				bookmarktime.append(timerstring);
				printf("bookmarktime: %s\n",bookmarktime.c_str());
				bookmarkmanager->createBookmark(filename, bookmarktime);
			}
			else
			{
				printf("too many bookmarks\n");
				DisplayErrorMessage(g_Locale->getText("movieplayer.toomanybookmarks")); // UTF-8
			}
		}
 		else if (msg == CRCInput::RC_help)
 		{
			std::string fullhelptext = g_Locale->getText("movieplayer.tshelp");
			fullhelptext += "\nVersion: $Revision: 1.84 $\n\nMovieplayer (c) 2003, 2004 by gagga";
			ShowMsgUTF("messagebox.info", fullhelptext.c_str(), CMessageBox::mbrBack, CMessageBox::mbBack, "info.raw"); // UTF-8
 		}
 		else if (msg == CRCInput::RC_setup)
 		{
			if(FileTime.IsVisible())
			{
				FileTime.hide();
			}
			else
			{
				FileTime.show(fileposition / (MINUTEOFFSET/60));
			}
 		}
		else if (msg == CRCInput::RC_left)
		{
			// rewind
			if (speed > 1)
				speed = 1;
			speed *= -2;
			speed *= (speed > 1 ? -1 : 1);
			playstate = CMoviePlayerGui::REW;
			update_lcd = true;
			FileTime.hide();
		}
		else if (msg == CRCInput::RC_right)
		{
			// fast-forward
			if (speed < 1)
				speed = 1;
			speed *= 2;
			playstate = CMoviePlayerGui::FF;
			update_lcd = true;
			FileTime.hide();
		}
		else if (msg == CRCInput::RC_1)
		{
			// Jump Backwards 1 minute
			jumpminutes = -1;
			playstate = CMoviePlayerGui::JB;
			update_lcd = true;
			FileTime.hide();
		}
		else if (msg == CRCInput::RC_3)
		{
			// Jump Forward 1 minute
			jumpminutes = 1;
			playstate = CMoviePlayerGui::JF;
			update_lcd = true;
			FileTime.hide();
		}
		else if (msg == CRCInput::RC_4)
		{
			// Jump Backwards 5 minutes
			jumpminutes = -5;
			playstate = CMoviePlayerGui::JB;
			update_lcd = true;
			FileTime.hide();
		}
		else if (msg == CRCInput::RC_6)
		{
			// Jump Forward 5 minutes
			jumpminutes = 5;
			playstate = CMoviePlayerGui::JF;
			update_lcd = true;
			FileTime.hide();
		}
		else if (msg == CRCInput::RC_7)
		{
			// Jump Backwards 10 minutes
			jumpminutes = -10;
			playstate = CMoviePlayerGui::JB;
			update_lcd = true;
			FileTime.hide();
		}
		else if (msg == CRCInput::RC_9)
		{
			// Jump Forward 10 minutes
			jumpminutes = 10;
			playstate = CMoviePlayerGui::JF;
			update_lcd = true;
			FileTime.hide();
		}
		else if (msg == CRCInput::RC_up || msg == CRCInput::RC_down)
		{
			// todo: next/prev file
		}
		else if (msg == CRCInput::RC_ok)
		{
			if (playstate > CMoviePlayerGui::PLAY)
			{
				update_lcd = true;
				playstate = CMoviePlayerGui::SOFTRESET;
			}
			else
				open_filebrowser = true;
		}
		else
			if (msg == NeutrinoMessages::RECORD_START
			    || msg == NeutrinoMessages::ZAPTO
			    || msg == NeutrinoMessages::STANDBY_ON
			    || msg == NeutrinoMessages::SHUTDOWN
			    || msg == NeutrinoMessages::SLEEPTIMER)
			{
				// Exit for Record/Zapto Timers
				isTS = true;		// also exit in PES Mode
				exit = true;
				g_RCInput->postMsg (msg, data);
			}
			else
				if (CNeutrinoApp::getInstance()->handleMsg(msg, data) & messages_return::cancel_all)
				{
					isTS = true;		// also exit in PES Mode
					exit = true;
				}
	}
	while (playstate >= CMoviePlayerGui::PLAY);
	pthread_join (rct, NULL);
}

// checks if AR has changed an sets cropping mode accordingly (only video mode auto)
void checkAspectRatio (int vdec, bool init)
{

	static video_size_t size; 
	static time_t last_check=0;
	
	// only necessary for auto mode, check each 5 sec. max
	if(g_settings.video_Format != 0 || (!init && time(NULL) <= last_check+5)) 
		return;

	if(init)
	{
		if (ioctl(vdec, VIDEO_GET_SIZE, &size) < 0)
			perror("[movieplayer.cpp] VIDEO_GET_SIZE");
		last_check=0;
		return;
	}
	else
	{
		video_size_t new_size; 
		if (ioctl(vdec, VIDEO_GET_SIZE, &new_size) < 0)
			perror("[movieplayer.cpp] VIDEO_GET_SIZE");
		if(size.aspect_ratio != new_size.aspect_ratio)
		{
			printf("[movieplayer.cpp] AR change detected in auto mode, adjusting display format\n");
			video_displayformat_t vdt;
			if(new_size.aspect_ratio == VIDEO_FORMAT_4_3)
				vdt = VIDEO_LETTER_BOX;
			else
				vdt = VIDEO_CENTER_CUT_OUT;
			if (ioctl(vdec, VIDEO_SET_DISPLAY_FORMAT, vdt))
				perror("[movieplayer.cpp] VIDEO_SET_DISPLAY_FORMAT");
			memcpy(&size, &new_size, sizeof(size));
		}
		last_check=time(NULL);
	}
}
#endif
