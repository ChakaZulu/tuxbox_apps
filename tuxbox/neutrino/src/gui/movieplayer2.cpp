/*
  Neutrino-GUI  -   DBoxII-Project 

  Movieplayer "v2"
  (C) 2008 Novell, Inc. Author: Stefan Seyfried

  Based on the old movieplayer code (c) 2003, 2004 by gagga
  which was based on code by Dirch, obi and the Metzler Bros. Thanks.

  The remultiplexer code was inspired by the vdrviewer plugin and the
  enigma1 demultiplexer.

  $Id: movieplayer2.cpp,v 1.4 2008/12/31 17:49:04 seife Exp $

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

////////////////////////////////////////////////////////////
/*
  THIS IS AN ALPHA VERSION, NOT EVERYTHING WILL WORK.

  This code plays (tested):
  * TS which were recorded with neutrino on dbox2
  * VDR 1.4.7 recordings (dual PES)
  * mpeg files created from VDR recordings with dvbcut,
    "target DVD(libavformat)"
  It does not yet play correctly:
  * MPEG1 like e.g. the "Warriors Of The Net" movie from
    http://ftp.sunet.se/pub/tv+movies/warriors/warriors-700-VBR.mpg

  The VLC code should still work, but is not tested at all.

  TODO:
  * the whole g_playstate state machine is too complicated and probably
    horribly broken - clean it up.
  * more error checking (end of file, anyone?)
  * get rid of all that moviebrowser stuff, it's not used anyway
  * clean up #includes
  * bookmarks? what bookmarks?
  * parental code
  * MPEG1 parser
  * AC3
  * the hintBox creation and deletion is fishy.
  * GUI improvements (infobar...)
  * check if the CLCD->setMode(MODE_MOVIE) are all correct (and needed)
  * ...lots more... ;)

  To build it, just copy it over to movieplayer.cpp and build.
  Enjoy.
 */

#define INFO(fmt, args...) fprintf(stderr, "[mp:%s:%d] " fmt, __FUNCTION__, __LINE__, ##args)
#if 0	// change for verbose debug output
#define DBG INFO
#else
#define DBG(args...)
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gui/movieplayer.h>

#include <global.h>
#include <neutrino.h>

#include <driver/fontrenderer.h>
#include <driver/rcinput.h>
#include <daemonc/remotecontrol.h>
#include <system/settings.h>
#include <system/helper.h>
#include <system/xmlinterface.h>
#include <gui/plugins.h>

#include <gui/eventlist.h>
#include <gui/color.h>
#include <gui/infoviewer.h>
#include <gui/nfs.h>
#include <gui/bookmarkmanager.h>
#include <gui/timeosd.h>
#include <gui/movieviewer.h>
#include <gui/imageinfo.h>

#include <gui/widget/buttons.h>
#include <gui/widget/icons.h>
#include <gui/widget/messagebox.h>
#include <gui/widget/hintbox.h>
#include <gui/widget/helpbox.h>
#include <gui/widget/stringinput.h>
#include <gui/widget/stringinput_ext.h>

#if HAVE_DVB_API_VERSION >= 3
#include <linux/dvb/audio.h>
#include <linux/dvb/dmx.h>
#include <linux/dvb/video.h>
#else
#include <ost/audio.h>
#include <ost/dmx.h>
#include <ost/video.h>
#define dmx_pes_filter_params	dmxPesFilterParams
#define pes_type		pesType
#endif

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

#ifdef MOVIEBROWSER
#include <sys/timeb.h>
#define MOVIE_HINT_BOX_TIMER 5 // time to show bookmark hints in seconds
#endif /* MOVIEBROWSER */

#if HAVE_DVB_API_VERSION < 3
#define ADAP	"/dev/dvb/card0"
#define DVR	"/dev/pvr"
#else
#define ADAP	"/dev/dvb/adapter0"
#define DVR	ADAP "/dvr0"
#endif
#define ADEC	ADAP "/audio0"
#define VDEC	ADAP "/video0"
#define DMX	ADAP "/demux0"

#define STREAMTYPE_DVD		1
#define STREAMTYPE_SVCD		2
#define STREAMTYPE_FILE		3
#define STREAMTYPE_LOCAL	4

#define MOVIEPLAYER_ConnectLineBox_Width	15

#define RINGBUFFERSIZE (5577 * 188) // almost 1 MB
#define MAXREADSIZE (348 * 188)
#define MINREADSIZE (5 * 188)

#define MOVIEPLAYER_START_SCRIPT CONFIGDIR "/movieplayer.start" 
#define MOVIEPLAYER_END_SCRIPT CONFIGDIR "/movieplayer.end"

bool g_ZapitsetStandbyState = false;

static bool isTS, isPES, isBookmark;

#ifdef MOVIEBROWSER
static bool isMovieBrowser = false;
static bool movieBrowserDelOnExit = false;
#endif /* MOVIEBROWSER */

#ifndef __USE_FILE_OFFSET64
#error not using 64 bit file offsets
#endif /* __USE_FILE__OFFSET64 */
ringbuffer_t *ringbuf;
bool bufferfilled;
bool bufferreset = false;
int fPercent = 0;	// percentage of the file position

unsigned short pida, pidv, pidt;
CHintBox *hintBox;
CHintBox *bufferingBox;
bool avpids_found;
std::string startfilename;
char skipvalue[20];
int skipseconds;
int buffer_time = 0;
int dmxa = -1 , dmxv = -1, dvr = -1, adec = -1, vdec = -1;

// global variables shared by playthread and PlayFile
static CMoviePlayerGui::state g_playstate;
//static bool g_EOF;

static off_t g_startposition = 0L;

uint16_t g_apids[10];
unsigned short g_ac3flags[10];
uint16_t g_numpida=0;
unsigned short g_currentapid = 0; // pida is for the decoder, most of the time the same as g_currentapid
unsigned int   g_currentac3  = 0;
bool           g_apidchanged = false;
unsigned int   g_has_ac3 = 0;
unsigned short g_prozent=0;

time_t g_pts = 0;
time_t g_startpts = -1;

#if HAVE_DVB_API_VERSION >=3
video_size_t   g_size;
#endif // HAVE_DVB_API_VERSION >=3

bool  g_showaudioselectdialog = false;

// Function prototypes for helper functions
static void checkAspectRatio (int vdec, bool init);
static off_t mp_seekSync(int fd, off_t pos);
static inline void skip(int seconds, bool remote, bool absolute);
static inline int get_filetime(void);
static inline int get_pts(char *p, bool pes);
std::string url_escape(const char *url);
size_t curl_dummywrite (void *ptr, size_t size, size_t nmemb, void *data);
static void close_devices(const char *function);
//------------------------------------------------------------------------

int CAPIDSelectExec::exec(CMenuTarget* /*parent*/, const std::string & actionKey)
{
	g_apidchanged = false;
	unsigned int sel= atoi(actionKey.c_str());
	if (g_currentapid != g_apids[sel-1])
	{
		g_currentapid = g_apids[sel-1];
		g_currentac3 = g_ac3flags[sel-1];
		g_apidchanged = true;
		printf("[movieplayer.cpp] apid changed to %d\n",g_currentapid);
	}
	return menu_return::RETURN_EXIT;
}

//------------------------------------------------------------------------

CMoviePlayerGui::CMoviePlayerGui()
{
	frameBuffer = CFrameBuffer::getInstance();
	bookmarkmanager=0;

	if (strlen(g_settings.network_nfs_moviedir) != 0)
		Path_local = g_settings.network_nfs_moviedir;
	else
		Path_local = "/";
	Path_vlc = "vlc://";
	Path_vlc += g_settings.streaming_server_startdir;
	Path_vlc_settings = g_settings.streaming_server_startdir;

	if (g_settings.filebrowser_denydirectoryleave)
		filebrowser = new CFileBrowser(Path_local.c_str());	// with filebrowser patch
	else
		filebrowser = new CFileBrowser();

	filebrowser->Dirs_Selectable = false;

#ifdef MOVIEBROWSER
	moviebrowser = NULL;
#endif /* MOVIEBROWSER */

	tsfilefilter.addFilter("ts");
	tsfilefilter.addFilter("mpg");
	tsfilefilter.addFilter("mpeg");
	tsfilefilter.addFilter("vdr");
	tsfilefilter.addFilter("m2p");	// untested
	tsfilefilter.addFilter("vob");	// untested
	tsfilefilter.addFilter("m2v");	// untested

	vlcfilefilter.addFilter("mpg");
	vlcfilefilter.addFilter("mpeg");
	vlcfilefilter.addFilter("m2p");
	vlcfilefilter.addFilter("avi");
	vlcfilefilter.addFilter("vob");
	vlcfilefilter.addFilter("wmv");
	vlcfilefilter.addFilter("m2v");
	vlcfilefilter.addFilter("mp4");

	filebrowser->Filter = &tsfilefilter;
}

//------------------------------------------------------------------------

CMoviePlayerGui::~CMoviePlayerGui ()
{
	delete filebrowser;
#ifdef MOVIEBROWSER
	if (moviebrowser != NULL)
	{	
		delete moviebrowser;
	}
#endif /* MOVIEBROWSER */
	if (bookmarkmanager)
		delete bookmarkmanager;

	if (g_playstate != CMoviePlayerGui::STOPPED)
		INFO("g_playstate != STOPPED: %d!\n", g_playstate);
	// just to make sure...
	g_playstate = CMoviePlayerGui::STOPPED;
	while (dmxa != -1 || dmxv != -1 || adec != -1 || vdec != -1 || dvr != -1)
	{
		INFO("want zapit wakeup, but dmxa = %d, dmxv = %d adec = %d vdec = %d dvr = %d\n",
		      dmxa, dmxv, adec, vdec, dvr);
		sleep(1);
	}

	CLCD::getInstance()->setMode(CLCD::MODE_TVRADIO);
	g_Zapit->setStandby(false);
	g_Sectionsd->setPauseScanning(false);
}

//------------------------------------------------------------------------
int
CMoviePlayerGui::exec(CMenuTarget *parent, const std::string &actionKey)
{
	printf("[movieplayer.cpp] %s actionKey=%s\n", __PRETTY_FUNCTION__, actionKey.c_str());

	if (Path_vlc_settings != g_settings.streaming_server_startdir)
	{
		Path_vlc = "vlc://";
		Path_vlc += g_settings.streaming_server_startdir;
		Path_vlc_settings = g_settings.streaming_server_startdir;
	}
	if (!bookmarkmanager)
		bookmarkmanager = new CBookmarkManager();

	if (parent)
		parent->hide();

	bool usedBackground = frameBuffer->getuseBackground();
	if (usedBackground)
	{
		frameBuffer->saveBackgroundImage();
		frameBuffer->ClearFrameBuffer();
	}

	const CBookmark *theBookmark = NULL;
	if (actionKey=="bookmarkplayback")
	{
		isBookmark = true;
		theBookmark = bookmarkmanager->getBookmark(NULL);
		if (theBookmark == NULL)
		{
			bookmarkmanager->flush();
			return menu_return::RETURN_REPAINT;
		}
	}
	
	filebrowser->Multi_Select = !!g_settings.streaming_allow_multiselect;

	g_ZapitsetStandbyState = false; // 'Init State

	// if filebrowser or moviebrowser playback we check if we should disable the tv (other modes might be added later)
	if (g_settings.streaming_show_tv_in_browser == false ||
	    (actionKey != "tsmoviebrowser" &&
	     actionKey != "tsplayback"     &&
	     actionKey != "fileplayback"   &&
	     actionKey != "tsplayback_pc"))
	{
		// set zapit in standby mode
		g_ZapitsetStandbyState = true;
		g_Zapit->setStandby(true);
	}

	CHintBox *startBox = new CHintBox(LOCALE_MESSAGEBOX_INFO, "Starte Movieplayer...");
	startBox->paint();
	INFO("executing %s\n", MOVIEPLAYER_START_SCRIPT);
	system(MOVIEPLAYER_START_SCRIPT);
	startBox->hide();
	delete startBox;

	// tell neutrino we're in ts_mode
	CNeutrinoApp::getInstance()->handleMsg(NeutrinoMessages::CHANGEMODE, NeutrinoMessages::mode_ts);
	// remember last mode
	m_LastMode = (CNeutrinoApp::getInstance()->getLastMode() | NeutrinoMessages::norezap);

	// Stop sectionsd
	g_Sectionsd->setPauseScanning(true);

	isBookmark=false;
	startfilename = "";
	g_startposition = 0;
	isTS=false;
	isPES=false;

#ifdef MOVIEBROWSER
	isMovieBrowser = false;
#endif /* MOVIEBROWSER */

	if (actionKey == "fileplayback")
		PlayStream(STREAMTYPE_FILE);
	else if (actionKey == "dvdplayback")
		PlayStream(STREAMTYPE_DVD);
	else if (actionKey == "vcdplayback")
		PlayStream(STREAMTYPE_SVCD);
	else if (actionKey == "tsplayback")
	{
		PlayFile();
	}
#ifdef MOVIEBROWSER
	else if (actionKey == "tsmoviebrowser")
	{
		if (moviebrowser == NULL)
		{
			TRACE("[mp] new MovieBrowser");
			moviebrowser= new CMovieBrowser();
		}
		if (moviebrowser != NULL)
		{
			isMovieBrowser = true;
			PlayFile();
		}
		else
		{
			TRACE("[mp] error: cannot create MovieBrowser");
		}
	}
#endif /* MOVIEBROWSER */
	else if (actionKey=="tsplayback_pc")
	{
		//isPES=true;
		ParentalEntrance();
	}
	else if (actionKey=="bookmarkplayback")
	{
		isBookmark = true;
		if (theBookmark != NULL)
		{
			startfilename = theBookmark->getUrl();
			sscanf(theBookmark->getTime(), "%lld", &g_startposition);
			int vlcpos = startfilename.rfind("vlc://");
			CLCD::getInstance()->setMode(CLCD::MODE_MOVIE);
			if (vlcpos == 0)
				PlayStream(STREAMTYPE_FILE);
			else
			{
				// TODO check if file is a TS. Not required right now as writing bookmarks is disabled for PES anyway
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
	if (g_ZapitsetStandbyState)
	{
		while (dmxa != -1 || dmxv != -1 || adec != -1 || vdec != -1 || dvr != -1)
		{
			INFO("want zapit wakeup, but dmxa = %d, dmxv = %d adec = %d vdec = %d dvr = %d\n",
			      dmxa, dmxv, adec, vdec, dvr);
			sleep(1);
		}
		g_Zapit->setStandby(false);
	}

	INFO("executing %s\n", MOVIEPLAYER_END_SCRIPT);
	system(MOVIEPLAYER_END_SCRIPT);

	// Start Sectionsd
	g_Sectionsd->setPauseScanning(false);

	// Restore last mode
	CNeutrinoApp::getInstance()->handleMsg(NeutrinoMessages::CHANGEMODE, m_LastMode);
	g_RCInput->postMsg(NeutrinoMessages::SHOW_INFOBAR, 0);

	CLCD::getInstance()->setMode(CLCD::MODE_TVRADIO);
	// always exit all
	if (bookmarkmanager)
	{
		delete bookmarkmanager;
		bookmarkmanager = NULL;
	}

	if (moviebrowser != NULL && movieBrowserDelOnExit == true)
	{
		//moviebrowser->fileInfoStale();
		TRACE("[mp] delete MovieBrowser");
		delete moviebrowser;
		moviebrowser = NULL;
	}

	return menu_return::RETURN_REPAINT;
}

//------------------------------------------------------------------------
CURLcode sendGetRequest (const std::string & url, std::string & response)
{
	CURL *curl;
	CURLcode httpres;

	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_dummywrite);
	curl_easy_setopt(curl, CURLOPT_FILE, (void *)&response);
	curl_easy_setopt(curl, CURLOPT_FAILONERROR, true);
	httpres = curl_easy_perform(curl);
	//printf ("[movieplayer.cpp] HTTP Result: %d\n", httpres);
	curl_easy_cleanup(curl);
	return httpres;
}

#define TRANSCODE_VIDEO_OFF 0
#define TRANSCODE_VIDEO_MPEG1 1
#define TRANSCODE_VIDEO_MPEG2 2
//------------------------------------------------------------------------
bool VlcRequestStream(char* mrl, int  transcodeVideo, int transcodeAudio)
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
	if (transcodeVideo != TRANSCODE_VIDEO_OFF || transcodeAudio != 0)
	{
		souturl += "transcode{";
		if (transcodeVideo != TRANSCODE_VIDEO_OFF)
		{
			souturl += "vcodec=";
			souturl += (transcodeVideo == TRANSCODE_VIDEO_MPEG1) ? "mpgv" : "mp2v";
			souturl += ",vb=";
			souturl += g_settings.streaming_videorate;
			souturl += ",width=";
			souturl += res_horiz;
			souturl += ",height=";
			souturl += res_vert;
			souturl += ",fps=25";
		}
		if (transcodeAudio != 0)
		{
			if (transcodeVideo != TRANSCODE_VIDEO_OFF)
				souturl += ",";
			souturl += "acodec=mpga,ab=";
			souturl += g_settings.streaming_audiorate;
			souturl += ",channels=2";
		}
		souturl += "}:";
	}
	souturl += "std{access=http,mux=ts,dst=:";
	souturl += g_settings.streaming_server_port;
	souturl += "/dboxstream}";
	
	std::string url = baseurl;
	url += "requests/status.xml?command=in_play&input=";
	url += mrl;	
	url += "%20%3Asout%3D";
	url += url_escape(souturl.c_str());
	printf("[movieplayer.cpp] URL(enc) : %s\n", url.c_str());
	std::string response;
	httpres = sendGetRequest(url, response);

	return true; // TODO error checking
}

//------------------------------------------------------------------------
int VlcGetStreamTime()
{
	// TODO: check if answer_parser leaks memory
	// TODO calculate REAL position as position returned by VLC does not take the ringbuffer into consideration
	std::string positionurl = "http://";
	positionurl += g_settings.streaming_server_ip;
	positionurl += ':';
	positionurl += g_settings.streaming_server_port;
	positionurl += "/requests/status.xml";
	//printf("[movieplayer.cpp] positionurl=%s\n",positionurl.c_str());
	std::string response = "";
	CURLcode httpres = sendGetRequest(positionurl, response);
	//printf("[movieplayer.cpp] httpres=%d, response.length()=%d, stream_length = %s\n",httpres,response.length(),response.c_str());
	if (httpres == 0 && response.length() > 0)
	{
		xmlDocPtr answer_parser = parseXml(response.c_str());
		if (answer_parser != NULL)
		{
			xmlNodePtr element = xmlDocGetRootElement(answer_parser);
			element = element->xmlChildrenNode;
			while (element) {
				char* tmp = xmlGetName(element);
				if (strcmp(tmp, "time") == 0)
					return atoi(xmlGetData(element));
				element = element->xmlNextNode;
			}
		}
		return -1;
	}
	else
		return -1;
}

//------------------------------------------------------------------------
int VlcGetStreamLength()
{
	// TODO: check if answer_parser leaks memory
	// TODO calculate REAL position as position returned by VLC does not take the ringbuffer into consideration
	std::string positionurl = "http://";
	positionurl += g_settings.streaming_server_ip;
	positionurl += ':';
	positionurl += g_settings.streaming_server_port;
	positionurl += "/requests/status.xml";
	//printf("[movieplayer.cpp] positionurl=%s\n",positionurl.c_str());
	std::string response = "";
	CURLcode httpres = sendGetRequest(positionurl, response);
	//printf("[movieplayer.cpp] httpres=%d, response.length()=%d, stream_length = %s\n",httpres,response.length(),response.c_str());
	if(httpres == 0 && response.length() > 0)
	{
		xmlDocPtr answer_parser = parseXml(response.c_str());
		if (answer_parser) {
			xmlNodePtr element = xmlDocGetRootElement(answer_parser);
			element = element->xmlChildrenNode;
			while (element)
			{
				char* tmp = xmlGetName(element);
				if (strcmp(tmp, "length") == 0)
					return atoi(xmlGetData(element));
				element = element->xmlNextNode;
			}
		}
		return -1;
	}
	else
		return -1;
}

//------------------------------------------------------------------------
void *
ReceiveStreamThread (void *mrl)
{
	INFO("started\n", __FUNCTION__);
	int skt;

	int nothingreceived = 0;

	// Get Server and Port from Config	
	std::string response;
	std::string baseurl = "http://";
	baseurl += g_settings.streaming_server_ip;
	baseurl += ':';
	baseurl += g_settings.streaming_server_port;
	baseurl += '/';
	baseurl += "requests/status.xml";
	CURLcode httpres = sendGetRequest(baseurl, response);
	if (httpres != 0)
	{
		DisplayErrorMessage(g_Locale->getText(LOCALE_MOVIEPLAYER_NOSTREAMINGSERVER));	// UTF-8
		g_playstate = CMoviePlayerGui::STOPPED;
		pthread_exit(NULL);
		// Assume safely that all succeeding HTTP requests are successful
	}

	int transcodeVideo, transcodeAudio;
	std::string sMRL = (char*)mrl;
	//Menu Option Force Transcode: Transcode all Files, including mpegs.
	if (!memcmp((char*)mrl, "vcd:", 4) ||
	    !strcasecmp(sMRL.substr(sMRL.length()-3).c_str(), "mpg") ||
	    !strcasecmp(sMRL.substr(sMRL.length()-4).c_str(), "mpeg") ||
	    !strcasecmp(sMRL.substr(sMRL.length()-3).c_str(), "m2p"))
	{
		if (g_settings.streaming_force_transcode_video)
			transcodeVideo = g_settings.streaming_transcode_video_codec + 1;
		else
			transcodeVideo = 0;
		transcodeAudio = g_settings.streaming_transcode_audio;
	}
	else
	{
		transcodeVideo = g_settings.streaming_transcode_video_codec + 1;
		if ((!memcmp((char*)mrl, "dvd", 3) && !g_settings.streaming_transcode_audio) ||
		    (!strcasecmp(sMRL.substr(sMRL.length()-3).c_str(), "vob") && !g_settings.streaming_transcode_audio) ||
		    (!strcasecmp(sMRL.substr(sMRL.length()-3).c_str(), "ac3") && !g_settings.streaming_transcode_audio) ||
		    g_settings.streaming_force_avi_rawaudio)
			transcodeAudio = 0;
		else
			transcodeAudio = 1;
	}
	VlcRequestStream((char*)mrl, transcodeVideo, transcodeAudio);

// TODO: Better way to detect if http://<server>:8080/dboxstream is already alive. For example repetitive checking for HTTP 404.
// Unfortunately HTTP HEAD requests are not supported by VLC :(
// vlc 0.6.3 and up may support HTTP HEAD requests.

// Open HTTP connection to VLC

	const char *server = g_settings.streaming_server_ip.c_str();
	int len;
	int port;
	sscanf(g_settings.streaming_server_port, "%d", &port);

	struct sockaddr_in servAddr;
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(port);
	servAddr.sin_addr.s_addr = inet_addr(server);

	INFO("Server: %s\n", server);
	INFO("Port: %d\n", port);

	while (true)
	{
		//printf ("[movieplayer.cpp] Trying to call socket\n");
		skt = socket (AF_INET, SOCK_STREAM, 0);

		INFO("Trying to connect socket\n");
		if (connect(skt, (struct sockaddr *) &servAddr, sizeof (servAddr)) < 0)
		{
			perror("SOCKET");
			g_playstate = CMoviePlayerGui::STOPPED;
			pthread_exit(NULL);
		}
		fcntl(skt, O_NONBLOCK);
		INFO("Socket OK\n");

		// Skip HTTP header
		const char * msg = "GET /dboxstream HTTP/1.0\r\n\r\n";
		int msglen = strlen(msg);
		if (send (skt, msg, msglen, 0) == -1)
		{
			perror("send()");
			g_playstate = CMoviePlayerGui::STOPPED;
			pthread_exit(NULL);
		}

		printf("[movieplayer.cpp] GET Sent\n");

		// Skip HTTP Header
		int found = 0;
		char buf[2];
		char line[200];
		buf[0] = buf[1] = '\0';
		strcpy(line, "");
		while (true)
		{
			len = recv(skt, buf, 1, 0);
			strncat(line, buf, 1);
			if (strcmp(line, "HTTP/1.0 404") == 0)
			{
				printf("[movieplayer.cpp] VLC still does not send. Retrying...\n");
				close(skt);
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
		if (g_playstate == CMoviePlayerGui::STOPPED)
		{
			close(skt);
			pthread_exit(NULL);
		}
	}
 vlc_is_sending:
	INFO("Now VLC is sending. Read sockets created\n");
	hintBox->hide();
	bufferingBox->paint();
	INFO("Buffering approx. 3 seconds\n");

	int size;
	int fd = open("/tmp/tmpts", O_CREAT | O_WRONLY);

	struct pollfd poller[1];
	poller[0].fd = skt;
	poller[0].events = POLLIN | POLLPRI;
	int pollret;
	ringbuffer_data_t vec[2];

	while (true)
	{
		if (g_playstate == CMoviePlayerGui::STOPPED)
		{
			close(skt);
			pthread_exit(NULL);
		}

		ringbuffer_get_write_vector(ringbuf, &(vec[0]));
		/* vec[0].len is not the total empty size of the buffer! */
		/* but vec[0].len = 0 if and only if the buffer is full! */
		if ((size = vec[0].len) == 0)
		{
			if (avpids_found)
			{
				if (bufferfilled)
				{
					/* do not waste cpu cycles if there is nothing to do */
					usleep(1000);
				}
			}
			else
			{
				printf("[movieplayer.cpp] Searching for vpid and apid\n");
				// find apid and vpid. Easiest way to do that is to write the TS to a file
				// and use the usual find_avpids function. This is not even overhead as the
				// buffer needs to be prefilled anyway
				close(fd);
				fd = open("/tmp/tmpts", O_RDONLY);
				//Use global pida, pidv
				//unsigned short pidv = 0, pida = 0;
				find_avpids(fd, &pidv, &pida);
				g_currentapid = pida;
				lseek(fd, 0, SEEK_SET);
				g_currentac3 = (is_audio_ac3(fd) > 0);
				close(fd);
				INFO("ReceiveStreamThread: found pida: 0x%04X pidv: 0x%04X ac3: %d\n",
				      pida, pidv, g_currentac3);
				avpids_found = true;
			}
			if (!bufferfilled)
			{
				bufferingBox->hide();
				//TODO reset drivers?
				bufferfilled = true;
			}
		}
		else
		{
			//printf("[movieplayer.cpp] ringbuf write space:%d\n",size);

			pollret = poll(poller, 1UL, -1);

			if (pollret < 0 ||
			    (poller[0].revents & (POLLHUP | POLLERR | POLLNVAL)) != 0)
			{
				perror("Error while polling()");
				g_playstate = CMoviePlayerGui::STOPPED;
				close(skt);
				pthread_exit(NULL);
			}

			if ((poller[0].revents & (POLLIN | POLLPRI)) != 0)
				len = recv(poller[0].fd, vec[0].buf, size, 0);
			else
				len = 0;

			if (len > 0)
			{
				ringbuffer_write_advance(ringbuf, len);

				nothingreceived = 0;
				//printf ("[movieplayer.cpp] bytes received:%d\n", len);
				if (!avpids_found)
					write (fd, vec[0].buf, len);
			}
			else
			{
				if (g_playstate == CMoviePlayerGui::PLAY)
				{
					nothingreceived++;
					if (nothingreceived > (buffer_time + 3) * 100) // wait at least buffer time secs +3 to play buffer when stream ends
					{
						printf("[movieplayer.cpp] ReceiveStreamthread: Didn't receive for a while. Stopping.\n");
						g_playstate = CMoviePlayerGui::STOPPED;
					}
					usleep(10000);	//sleep 10 ms
				}
			}
		}
	}
	close(skt);
	INFO("ends now.\n");
	pthread_exit(NULL);
} // ReceiveStreamThread

void *
ReadTSFileThread(void *parm)
{
	/* reads a TS file into *ringbuf */
	char *fn = (char *)parm;
	int fd = open(fn, O_RDONLY);
	INFO("start, filename = '%s', fd = %d\n", fn, fd);
	ssize_t len;
	size_t readsize;
	off_t bytes_per_second = 500000;
	off_t filesize, filepos;
	unsigned int lastpts = 0, count = 0;
	off_t lastpos = 0, ptspos = 0;
	time_t last = 0;
	int i;
	ringbuffer_data_t vec[2];
	
	fPercent = 0;
	hintBox->hide(); // the "connecting to streaming server" hintbox

	filesize = lseek(fd, 0, SEEK_END);
	filepos = mp_seekSync(fd, 0);
	if (filepos < 0)
		perror("ReadTSFileThread lseek");
	INFO("file starts at %lld\n", filepos);

	pidv = 0;
	memset(&g_apids, 0, sizeof(g_apids));
	find_all_avpids(fd, &pidv, g_apids, g_ac3flags, &g_numpida);
	pida = g_apids[0];
	g_currentac3 = g_ac3flags[0];
	INFO("found pida: 0x%04X pidv: 0x%04X ac3: %d numpida: %d\n", pida, pidv, g_currentac3, g_numpida);
	if (g_numpida > 1)
	{
		printf(" => additional apids:");
		for (i = 1; i < g_numpida; i++)
			printf(" 0x%04X", g_apids[i]);
		printf("\n");
	}
	g_currentapid = pida;

	bufferingBox->paint();
	INFO("Buffering...\n");

	bool failed = false;

	lseek(fd, filepos, SEEK_SET);
	ringbuffer_reset(ringbuf);
	ringbuffer_get_write_vector(ringbuf, &(vec[0]));
	readsize = vec[0].len / 188 * 188;
	len = read(fd, vec[0].buf, readsize); // enough?
	if (len < 0)
	{
		INFO("first read failed (%m)\n");
		failed = true;
	}
	else
	{
		char *ts = vec[0].buf;
		i = 0;
		while (i + 188 < len)
		{
			g_startpts = get_pts(ts + i, false);
			if (g_startpts != -1)
				break;
			i  += 188;
		}
		if (g_startpts == -1)
			INFO("could not determine PTS at file start\n");
		else
			INFO("PTS at file start: %d\n", g_startpts);

		lastpts = g_startpts;
		if (len % 188)
		{
			ringbuffer_reset(ringbuf); // not aligned anymore, so reset...
			lseek(fd, filepos, SEEK_SET);
		}
		else
			filepos += len;
	}

	while (g_playstate != CMoviePlayerGui::STOPPED && !failed)
	{
		switch(g_playstate)
		{
		case CMoviePlayerGui::SKIP:
			INFO("lseek from %lld, seconds %d\n", filepos, skipseconds);
			filesize = lseek(fd, 0, SEEK_END);
			filepos += (bytes_per_second * skipseconds) / 188 * 188;
			if (filepos >= filesize)
				filepos -= (bytes_per_second * skipseconds) / 188 * 188;
			//	g_playstate = CMoviePlayerGui::PLAY;
			if (filepos < 0)
				filepos = 0;
			count = 0;
			last = 0;
			bufferingBox->paint();
			INFO("lseek to %lld, size %lld\n", filepos, filesize);
			if (mp_seekSync(fd, filepos) < 0)
				perror("ReadTSFileThread lseek");
			while (!bufferreset && g_playstate != CMoviePlayerGui::STOPPED)
				usleep(100000);
			bufferfilled = false;
			ringbuffer_reset(ringbuf);
			bufferreset = false;
			g_playstate = CMoviePlayerGui::PLAY;
			break;
		case CMoviePlayerGui::PLAY:
			if (last == 0)
			{
				last = time(NULL);
				lastpts = g_pts;
				lastpos = ptspos;
				break;
			}
			time_t now = time(NULL);
			if ((now - last) > 4) // update bytes_per_second every 5 seconds
			{
				int diff_pts = g_pts - lastpts;
				off_t diff_pos = ptspos - lastpos;
				off_t diff_bps = diff_pos * 1000 / diff_pts;
				lastpos = ptspos;
				lastpts = g_pts;
				printf("update bytes_per_second. old: %lld", bytes_per_second);
				if ((diff_bps > 0 && diff_pos > 0)) // discontinuity, startup...
				{
					if (count < 8)
						count++;
					bytes_per_second = (count * bytes_per_second + diff_bps) / (count + 1);
				} else
					printf(" not updated");
				printf(" new: %lld, diff PTS:%5d diff_pos %lld, filepos: %d%%\n",
					bytes_per_second, diff_pts, diff_pos, fPercent);
				last = now;
				// should not happen...
				if (g_startpts == -1)
					g_startpts = g_pts;
			}
			break;
		default:
			break;
		}

		readsize = ringbuffer_write_space(ringbuf);
		if (readsize % 188)
			readsize = readsize / 188ULL * 188ULL; // read in TS packet sized hunks
		
		if (readsize < 188) // the output buffer is full.
		{
			if (!bufferfilled)
			{
				bufferingBox->hide();
				//TODO reset drivers?
				bufferfilled = true;
			}
			/* do not waste cpu cycles if there is nothing to do */
			usleep(250000);
			continue;
		}

		//printf("[movieplayer.cpp] ringbuf write space:%d\n",size);
		/* this is pretty complicated, but it makes sure that multiples of
		   188 bytes are read in, so that the TS stays in sync.
		   This, in turn, makes it easier to parse the PTS... */
		size_t todo = readsize;
		while (todo > 0)
		{
			size_t done = 0;
			ringbuffer_get_write_vector(ringbuf, &(vec[0]));
			if (vec[0].len >= todo) // everything fits into vec[0].buf...
			{
				len = read(fd, vec[0].buf, todo);
				if (todo == readsize) // only first read of the while() loop, so we are still TS-aligned
				{
					char *ts = vec[0].buf;
					while (ts - vec[0].buf < (int)todo)
					{
						int pts = get_pts(ts, false);
						if (pts != -1)
						{
							g_pts = pts;
							ptspos = filepos + (ts - vec[0].buf);
							break;
						}
						ts += 188;
					}
				}
			}
			else if (vec[1].len != 0)
			{
				/* this is the "ringbuffer wraparound" case */
				DBG("v[0].l: %ld v[1].l: %ld, todo: %ld\n", vec[0].len, vec[1].len, todo);
				len = read(fd, vec[0].buf, vec[0].len);
				if (len < 0)
				{
					perror("ReadTSFileThread read");
					failed = true;
					break;
				}
				done += len;
				todo -= len;
				len = read(fd, vec[1].buf, todo);
			}
			else
			{
				INFO("something is wrong. vec[0].len: %ld vec[1].len: %ld, todo: %ld\n",
				      vec[0].len, vec[1].len, todo);
				failed = true;
				break;
			}

			if (len <= 0) // len < 0 => error, len == 0 => EOF
			{
				if (len < 0)
					perror("ReadTSFileThread read");
				INFO("error or EOF => exiting\n");
				failed = true;
				break;
			}
			done += len;
			ringbuffer_write_advance(ringbuf, done);
			filepos += done;
			todo -= len;
			//if (todo) DBG(stderr, "todo: %ld\n", todo); // in reality, this never triggers.
		}

		if (filesize)
			fPercent = filepos * 100 / filesize;	// overflow? not with 64 bits...
	}
	close(fd);
	INFO("ends now.\n");
	pthread_exit(NULL);
} // ReadTSFileThread

void *
ReadMPEGFileThread(void *parm)
{
	/* reads a mpeg or dual-pes (VDR) file into the *buf_in ringbuffer,
	   then remultiplexes it as a TS into *ringbuf.
	   TODO: get rid of the input ringbuffer if possible
	 */
	char *fn = (char *)parm;
	int fd = open(fn, O_RDONLY);
	INFO("start, filename = '%s', fd = %d\n", fn, fd);
	int len, size;
	size_t rd;
	off_t bytes_per_second = 500000;
	off_t filesize;
	off_t filepos = 0;
	fPercent = 0;
	char *ppes;
	char ts[188];
	unsigned char acc = 0;		// continutiy counter for audio packets
	unsigned char vcc = 0;		// continutiy counter for video packets
	unsigned char *cc = &vcc;	// either cc=&vcc; or cc=&acc;
	unsigned short pid = 0;
	unsigned short skip;
	unsigned int pesPacketLen;
	int tsPacksCount;
	unsigned char rest;

	hintBox->hide(); // the "connecting to streaming server" hintbox

	filesize = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);

	pidv = 100;
	pida = 101;

	bufferingBox->paint();
	INFO("Buffering...\n");

	ringbuffer_t *buf_in = ringbuffer_create(65535);
	ringbuffer_data_t vec_in[2];
	time_t last = 0;
	unsigned int lastpts = 0, count = 0;
	off_t lastpos = 0, ptspos = 0;
	int type = 0; // TODO: use wisely...
	int i = 0; // loop counter for PTS check

	unsigned char found_aid[0xff]; // lame hash;
	memset(&found_aid, 0, sizeof(found_aid));
	g_currentapid = 0;
	g_currentac3 = 0;
	g_numpida = 0;
	g_startpts = -1;
	bool input_empty = true;
	bool failed = false;

	while (g_playstate != CMoviePlayerGui::STOPPED && !failed)
	{
		switch(g_playstate)
		{
		case CMoviePlayerGui::SKIP:
			if (skipseconds)
			{
				INFO("lseek from %lld, seconds %d\n", filepos, skipseconds);
				filesize = lseek(fd, 0, SEEK_END);
				filepos += bytes_per_second * skipseconds;
				if (filepos >= filesize)
					filepos -= bytes_per_second * skipseconds;
				if (filepos < 0)
					filepos = 0;
				count = 0;
				last = 0;
				skipseconds = 0;
//				bufferingBox->paint();
				INFO("lseek to %lld, size %lld\n", filepos, filesize);
				if (lseek(fd, filepos, SEEK_SET) < 0)
					perror("ReadMPEGFileThread lseek");
			}
			if (!bufferreset)
				break;
			INFO("BUFFERRESET!\n");
			ringbuffer_reset(ringbuf);
			ringbuffer_reset(buf_in);
			bufferreset = false;
			input_empty = true;
			g_playstate = CMoviePlayerGui::PLAY;
			break;
		case CMoviePlayerGui::PLAY:
			if (last == 0)
			{
				last = time(NULL);
				lastpts = g_pts;
				lastpos = ptspos;
				break;
			}
			time_t now = time(NULL);
			if ((now - last) > 9)
			{
				int diff_pts = g_pts - lastpts;
				off_t diff_pos = ptspos - lastpos;
				off_t diff_bps = diff_pos * 1000 / diff_pts;
				lastpos = ptspos;
				lastpts = g_pts;
				printf("update bytes_per_second. old: %lld", bytes_per_second);
				if ((diff_bps > 0 && diff_pos > 0)) // discontinuity, startup...
				{
					if (count < 8)
						count++;
					bytes_per_second = (count * bytes_per_second + diff_bps) / (count + 1);
				} else
					printf(" not updated");
				printf(" new: %lld, diff PTS:%5d diff_pos %lld, count %d %d%%\n", bytes_per_second, diff_pts, diff_pos,count, fPercent);
				last = now;
			}
			break;
		default:
			break;
		}

#if 0 // the more sophisticated, but also much more complex version
		size = ringbuffer_write_space(buf_in);
		if (size < 2048)
			input_empty = false;

		if (size > 32768) // 32k chunks...
			size = 32768;

		size_t todo = size;
		while (todo > 0)
		{
			size_t done = 0;
			ringbuffer_get_write_vector(buf_in, &(vec_in[0]));
			if (vec_in[0].len >= todo)
			{
				len = read(fd, vec_in[0].buf, todo);
			}
			else if (vec_in[1].len != 0)
			{
				/* this is the "ringbuffer wraparound" case */
				DBG("v[0].l: %ld v[1].l: %ld, todo: %ld\n", vec_in[0].len, vec_in[1].len, todo);
				len = read(fd, vec_in[0].buf, vec_in[0].len);
				if (len < 0)
				{
					perror("ReadMPEGFileThread read");
					failed = true;
					break;
				}
				done += len;
				todo -= len;
				len = read(fd, vec_in[1].buf, todo);
			}
			else
			{
				INFO("something is wrong. vec[0].len: %ld vec[1].len: %ld, todo: %ld\n",
				      vec_in[0].len, vec_in[1].len, todo);
				failed = true;
				break;
			}

			if (len <= 0) // len < 0 => error, len == 0 => EOF
			{
				if (len < 0)
					perror("ReadMPEGFileThread read");
				INFO("error or EOF => exiting\n");
				failed = true;
				break;
			}
			done += len;
			ringbuffer_write_advance(buf_in, done);
			filepos += done;
			todo -= len;
			//if (todo) DBG(stderr, "todo: %ld\n", todo); // in reality, this never triggers.
		}
		if (input_empty)
			continue;
#else
		ringbuffer_get_write_vector(buf_in, &vec_in[0]);
		if ((size = vec_in[0].len) != 0)
		{
			len = read(fd, vec_in[0].buf, size);
			if (len > 0)
				ringbuffer_write_advance(buf_in, len);
			else if (len < 0)
			{
				perror("ReadMPEGFileThread read");
				break;
			}
			DBG("read %d bytes, size %d\n", len, size);
			filepos += len;
			if (len && input_empty)
				continue;
		}
		input_empty = false;
#endif

		if (ringbuffer_write_space(ringbuf) < 2256)
		{
			DBG("ringbuf write space vec.len:%d write_buf:%d\n", size, ringbuffer_write_space(ringbuf));
			if (!bufferfilled)
			{
				bufferingBox->hide();
				//TODO reset drivers?
				bufferfilled = true;
			}
			/* do not waste cpu cycles if there is nothing to do */
			usleep(10000);
			continue;
		}

 again:
		rd = ringbuffer_get_readpointer(buf_in, &ppes, 6);
		if (rd < 6)
		{
			INFO("rd:%d\n", rd);
			usleep(300000);
			continue;
		}

		bool resync = false;	// TODO: improve
		if ((ppes[0] != 0x00) || (ppes[1] != 0x00) || (ppes[2] != 0x01))
		{
			//INFO("async, not 000001: %02x%02x%02x ", ppes[0], ppes[1], ppes[2]);
			int deleted = 0;
			do {
				ringbuffer_read_advance(buf_in, 1); // remove 1 Byte
				rd = ringbuffer_get_readpointer(buf_in, &ppes, 3);
				deleted++;
				//fprintf(stderr, "%d", rd);
				if ((ppes[0] == 0x00) || (ppes[1] == 0x00) || (ppes[2] == 0x01))
				{
					deleted = 0;
					break;
				}
			}
			while (rd == 3);
			//fprintf(stderr, "\n");
			if (deleted > 0)
			{
				INFO("No valid PES signature found. %d Bytes deleted.\n", deleted);
				continue;
			}
			resync = true;
		}

		int av = 0; // 1 = video, 2 = audio
		switch(ppes[3])
		{
			case 0xba:
				// fprintf(stderr, "pack start code, 0x%02x\n", ppes[4]);
				if ((ppes[4] & 0x3) == 1) // ??
				{
					type = 1; // mpeg1
					skip = 12;
				}
				else
					skip = 14;
				ringbuffer_read_advance(buf_in, skip);
				continue;
				break;
			case 0xbb:
			case 0xbd: // TODO: AC3
			case 0xbe:
			case 0xbf:
			case 0xf0 ... 0xf3:
			case 0xff:
				skip = (ppes[4] << 8 | ppes[5]) + 6;
				DBG("0x%02x header, skip = %d\n", ppes[3], skip);
				break;
			case 0xc0 ... 0xcf:
			case 0xd0 ... 0xdf:
				// fprintf(stderr, "audio stream 0x%02x\n", ppes[3]);
				if (!g_currentapid)
				{
					g_currentapid = ppes[3];
					INFO("found aid: %02x\n", g_currentapid);
					if (g_numpida < 10);
						g_apids[g_numpida++] = ppes[3];
					found_aid[g_currentapid] = 1;
				}
				else if (g_currentapid != ppes[3])
				{
					if (!found_aid[(int)ppes[3]] && !resync) //only if we are in sync...
					{
						if (g_numpida < 10);
							g_apids[g_numpida++] = ppes[3];
						INFO("additional aid: %02x\n", ppes[3]);
						found_aid[(int)ppes[3]] = 1;
					}
					av = 0; // skip over this stream;
					break;
				}
				pid = 101;
				cc = &acc;
				av = 2;
				break;
			case 0xe0 ... 0xef:
				// fprintf(stderr, "video stream 0x%02x, %02x %02x \n", ppes[3], ppes[4], ppes[5]);
				pid = 100;
				cc = &vcc;
				av = 1;
				break;
			case 0xb9:
			case 0xbc:
				INFO("%s\n", (ppes[3] == 0xb9) ? "program_end_code" : "program_stream_map");
				resync = true;
				// fallthrough. TODO: implement properly.
			default:
				if (! resync)
					INFO("Unknown stream id: 0x%X.\n", ppes[3]);
				ringbuffer_read_advance(buf_in, 1); // remove 1 Byte
				goto again;
				break;
		}

		pesPacketLen = ((ppes[4] << 8) | ppes[5]) + 6;
		if (ringbuffer_read_space(buf_in) < pesPacketLen)
		{
			INFO("ringbuffer: %ld, pesPacketLen: %ld :-(\n", ringbuffer_read_space(buf_in), pesPacketLen);
			continue;
		}

		if (av)
		{
			rd = ringbuffer_get_readpointer(buf_in, &ppes, pesPacketLen);
			// we already checked for enough space above...

			if (av == 1 && (i++ %10) == 0) // only for every tenth video packet.
			{
				int pts = get_pts(ppes, true);
				if (pts != -1)
				{
					g_pts = pts;
					ptspos = filepos; // not exact: disregards the bytes in the buffer!
					if (g_startpts == -1) // only works if we are starting from the start of the file
					{
						g_startpts = pts;
						INFO("startpts = %d\n", g_startpts);
					}
				}
			}

			tsPacksCount = pesPacketLen / 184;
			rest = pesPacketLen % 184;

			// divide PES packet into small TS packets
			bool first = true;
			int i;
			for (i = 0; i < tsPacksCount; i++) 
			{
				ts[0] = 0x47;			//SYNC Byte
				if (first)
					ts[1] = 0x40;		// Set PUSI or
				else
					ts[1] = 0x00;		// clear PUSI
				ts[2] = pid;			// PID (low)
				ts[3] = 0x10 | ((*cc) & 0x0F);	// No adaptation field, payload only, continuity counter
				++(*cc);
				ringbuffer_write(ringbuf, ts, 4);
				ringbuffer_write(ringbuf, ppes + i * 184, 184);
				first = false;
			}

			if (rest > 0)
			{
				ts[0] = 0x47;			//SYNC Byte
				if (first)
					ts[1] = 0x40;		// Set PUSI or
				else
					ts[1] = 0x00;		// clear PUSI
				ts[2] = pid;			// PID (low)
				ts[3] = 0x30 | ((*cc) & 0x0F);	// adaptation field, payload, continuity counter 
				++(*cc);
				ts[4] = 183 - rest;
				if (ts[4] > 0)
				{
					ts[5] = 0x00;
					memset(ts + 6, 0xFF, ts[4] - 1);
				}
				ringbuffer_write(ringbuf, ts, 188 - rest);
				ringbuffer_write(ringbuf, ppes + i * 184, rest);
			}
		} //if (av)

		ringbuffer_read_advance(buf_in, pesPacketLen);

		if (filesize)
			fPercent = filepos * 100 / filesize;	// overflow? not with 64 bits...
	}
	close(fd);
	ringbuffer_free(buf_in);
	INFO("ends now.\n");
	pthread_exit(NULL);
} // ReadMPEGFileThread()



//------------------------------------------------------------------------
void *
PlayStreamThread (void *mrl)
{
	//-- lcd stuff --
	int cPercent = 0;
	int lPercent = -1;
	CURLcode httpres;
	struct dmx_pes_filter_params p;
	ssize_t wr;
	char buf[MAXREADSIZE];
	bool failed = false;
	bool remote = true;
	// use global pida and pidv
	pida = 0, pidv = 0, g_currentac3 = 0;
	int ret, done;
	/* paranoia checks, should never trigger */
	close_devices(__FUNCTION__);

	if (((char *)mrl)[0] == '/')
		remote = false;	// we are playing a "local" file (hdd or NFS)

	ringbuf = ringbuffer_create(RINGBUFFERSIZE);
	if (ringbuf)
		INFO("ringbuffer created\n");
	else
	{
		INFO("ringbuffer_create failed!\n");
		failed = true;
	}

	bufferingBox = new CHintBox(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_MOVIEPLAYER_BUFFERING));	// UTF-8

	std::string baseurl = "http://";
	baseurl += g_settings.streaming_server_ip;
	baseurl += ':';
	baseurl += g_settings.streaming_server_port;
	baseurl += '/';

	printf("[movieplayer.cpp] mrl:%s\n", (char *) mrl);

	pthread_t rcvt;	// the input / "receive" thread
	if (remote)
		ret = pthread_create(&rcvt, NULL, ReceiveStreamThread, mrl);
	else
	{
		std::string tmp = (char *)mrl;
		if (tmp.rfind(".ts") == tmp.size()-3)
		{
			INFO("found TS file\n");
			isPES = false;
			isTS = true;
			ret = pthread_create(&rcvt, NULL, ReadTSFileThread, mrl);
		}
		else
		{
			INFO("found non-TS file, hoping for MPEG\n");
			isTS = false;
			isPES = true;
			ret = pthread_create(&rcvt, NULL, ReadMPEGFileThread, mrl);
		}
	}

	if (ret)
	{
		INFO("pthread_create failed with error %d (%s)\n", ret, strerror(ret));
		failed = true;
	}

	if((dmxa = open(DMX, O_RDWR | O_NONBLOCK)) < 0 ||
	   (dmxv = open(DMX, O_RDWR | O_NONBLOCK)) < 0 ||
	   (dvr  = open(DVR, O_WRONLY /* | O_NONBLOCK */)) < 0 ||
	   (adec = open(ADEC, O_RDWR | O_NONBLOCK)) < 0 ||
	   (vdec = open(VDEC, O_RDWR | O_NONBLOCK)) < 0)
	{
		failed = true;
	}

	g_playstate = CMoviePlayerGui::SOFTRESET;
	printf("[movieplayer.cpp] read starting\n");
	size_t readsize, len;
	len = 0;
	bool driverready = false;
	std::string pauseurl = baseurl;
	pauseurl += "requests/status.xml?command=pl_pause";
	std::string unpauseurl = pauseurl;
	std::string skipurl;
	std::string response;

	bool init = true;
	checkAspectRatio(vdec, true);
	int length = 0;
	int counter = 120;

	while (g_playstate > CMoviePlayerGui::STOPPED && !failed)
	{
		if (! bufferfilled)
		{
			// fprintf(stderr, "!");
			usleep(10000);	// non busy wait
			continue;
		}

		readsize = ringbuffer_read_space(ringbuf);
		//fprintf(stderr, "XX readsize = %d MAX %d\n", readsize, MAXREADSIZE);
		if (readsize > MAXREADSIZE)
			readsize = MAXREADSIZE;
		else if (readsize % 188)
			readsize = (readsize / 188) * 188;

		//printf("[movieplayer.cpp] readsize=%d\n",readsize);
		if (!driverready)
		{
			driverready = true;
			// pida and pidv should have been set by ReceiveStreamThread now
			INFO("while streaming found pida: 0x%04X ; pidv: 0x%04X ac3: %d\n",
			      pida, pidv, g_currentac3);

			g_playstate= CMoviePlayerGui::SOFTRESET;
			// Calculate diffrence between vlc time and play time
			// movieplayer is about to start playback so ask vlc for his position
			if (remote && (buffer_time = VlcGetStreamTime()) < 0)
				buffer_time=0;
		}

		if (g_startposition > 0)
		{
			printf("[movieplayer.cpp] Was Bookmark. Skipping to startposition\n");
			sprintf(skipvalue, "%lld", g_startposition);
			skipseconds = 0;
			g_startposition = 0;
			g_playstate = CMoviePlayerGui::SKIP;
		}

		if (g_apidchanged && !isPES)
		{
			INFO("APID changed from 0x%04x to 0x%04x\n", pida, g_currentapid);
			pida = g_currentapid;
			g_apidchanged = false;
			g_playstate= CMoviePlayerGui::SOFTRESET;
		}

		switch (g_playstate)
		{
			case CMoviePlayerGui::PAUSE:
				ioctl(dmxa, DMX_STOP);
				// pause VLC
				if (remote)
					httpres = sendGetRequest(pauseurl, response);

				while (g_playstate == CMoviePlayerGui::PAUSE)
					usleep(100000); // no busy wait

				// unpause VLC
				if (remote)
					httpres = sendGetRequest(unpauseurl, response);
				break;
			case CMoviePlayerGui::SKIP:
				if (remote)
				{
					counter = 0;
					skipurl = baseurl;
					skipurl += "requests/status.xml?command=seek&val=";
					skipurl += url_escape(skipvalue);
					printf("[movieplayer.cpp] skipping URL(enc) : %s\n",skipurl.c_str());
					int bytes = (ringbuffer_read_space(ringbuf) / 188) * 188;
					ringbuffer_read_advance(ringbuf, bytes);
//					bufferingBox->paint();
//					bufferfilled = false;
					httpres = sendGetRequest(skipurl, response);
				}
				else
				{
					printf("[mp] requesting buffer reset\n");
					bufferreset = true;
					while (bufferreset && g_playstate != CMoviePlayerGui::STOPPED)
					{
						DBG("WAITING FOR BUFFERRESET\n");
						usleep(250000);
					}
				}
				g_playstate = CMoviePlayerGui::SOFTRESET;
				printf("[mp] skipping end\n");
				break;
			case CMoviePlayerGui::RESYNC:
				INFO("Resyncing\n");
				ioctl(dmxa, DMX_STOP);
				INFO("Buffering approx. 3 seconds\n");
				/*
				 * always call bufferingBox->paint() before setting bufferfilled to false
				 * to ensure that it is painted completely before bufferingBox->hide()
				 * might be called by ReceiveStreamThread (otherwise the hintbox's window
				 * variable is deleted while being used)
				 */
				bufferingBox->paint();
				bufferfilled = false;
				while (!bufferfilled && g_playstate != CMoviePlayerGui::STOPPED)
				{
					DBG("WAITING FOR BUFFERFILLED\n");
					usleep(100000);
				}
				ioctl(dmxa, DMX_START);
				g_playstate = CMoviePlayerGui::PLAY;
				break;
			case CMoviePlayerGui::PLAY:
				len = ringbuffer_read(ringbuf, buf, readsize); // readsize is n*188
				if (len < MINREADSIZE)
				{
					ioctl(dmxa, DMX_STOP);
					INFO("len: %d, buffering...\n", len);
					/*
					 * always call bufferingBox->paint() before setting bufferfilled to false
					 * to ensure that it is painted completely before bufferingBox->hide()
					 * might be called by ReceiveStreamThread (otherwise the hintbox's window
					 * variable is deleted while being used)
					 */
					bufferingBox->paint();
					bufferfilled = false;
					ioctl(dmxa, DMX_START);
				}
				//printf ("[movieplayer.cpp] [%d bytes read from ringbuf]\n", len);

				done = 0;
				while (len > 0)
				{
					// TODO: dvr is opened blocking => handle _really_ blocking write...
					wr = write(dvr, &buf[done], len);
					if (wr < 0)
					{
						if (errno == EAGAIN && g_playstate != CMoviePlayerGui::STOPPED)
						{
							DBG("write EAGAIN\n");
							usleep(10000);
							continue;
						}
						perror("[movieplayer.cpp] PlayStreamThread write");
						g_playstate = CMoviePlayerGui::STOPPED;
						break;
					}
					len -= wr;
					done += wr;
				}
				//fprintf(stderr, "V");
				checkAspectRatio(vdec, init);
				init = false;

				//-- lcd progress bar --
				if (remote)
				{
					if (length == 0)
						length = VlcGetStreamLength();
					if (length != 0)
					{
						if (counter == 0 || counter == 120)
						{
							cPercent = (VlcGetStreamTime() * 100) / length;
							if (lPercent != cPercent)
							{
								lPercent = cPercent;
								CLCD::getInstance()->showPercentOver(cPercent, true, CLCD::MODE_MOVIE);
							}
							counter = 119;
						}
						else
							counter--;
					}
				}
				else if (lPercent != fPercent)
				{
					lPercent = fPercent;
					CLCD::getInstance()->showPercentOver(fPercent, true, CLCD::MODE_MOVIE);
				}
				break;
			case CMoviePlayerGui::SOFTRESET:
				INFO("CMoviePlayerGui::SOFTRESET\n");
				ioctl(vdec, VIDEO_STOP);
				ioctl(adec, AUDIO_STOP);
				ioctl(dmxv, DMX_STOP);
				ioctl(dmxa, DMX_STOP);
				ioctl(vdec, VIDEO_PLAY);
				if (g_currentac3 == 1)
					ioctl(adec, AUDIO_SET_BYPASS_MODE, 0UL);
				else
					ioctl(adec, AUDIO_SET_BYPASS_MODE, 1UL);
				ioctl(adec, AUDIO_PLAY);
				p.pid = pida;
				p.pes_type = DMX_PES_AUDIO;
				ioctl(dmxa, DMX_SET_PES_FILTER, &p);
				p.pid = pidv;
				p.pes_type = DMX_PES_VIDEO;
				ioctl(dmxv, DMX_SET_PES_FILTER, &p);
				ioctl(dmxv, DMX_START);
				ioctl(dmxa, DMX_START);
				// on the dbox2, the dmx must be started for SET_AV_SYNC
				if (ioctl(adec, AUDIO_SET_AV_SYNC, 1UL))
					perror("AUDIO_SET_AV_SYNC");
				g_playstate = CMoviePlayerGui::PLAY;
				break;
			case CMoviePlayerGui::STOPPED:
			case CMoviePlayerGui::PREPARING:
			case CMoviePlayerGui::STREAMERROR:
			case CMoviePlayerGui::FF:
			case CMoviePlayerGui::REW:
			case CMoviePlayerGui::JF:
			case CMoviePlayerGui::JB:
			case CMoviePlayerGui::AUDIOSELECT:
			case CMoviePlayerGui::ITEMSELECT:
			default:
				break;
		}
	}

	ioctl(vdec, VIDEO_STOP);
	ioctl(adec, AUDIO_STOP);
	ioctl(dmxv, DMX_STOP);
	ioctl(dmxa, DMX_STOP);
	close_devices(__FUNCTION__);

	// stop VLC
	std::string stopurl = baseurl;
	stopurl += "requests/status.xml?command=pl_stop";
	if (remote)
		httpres = sendGetRequest(stopurl, response);
	// request reader termination
	g_playstate = CMoviePlayerGui::STOPPED;

	INFO("Waiting for input thread to stop\n");
	pthread_join(rcvt, NULL);
	INFO("Seems that input thread was stopped succesfully\n");

	// Some memory clean up
	ringbuffer_free(ringbuf);
	delete bufferingBox;
	delete hintBox;	// TODO is this allowed here?

	pthread_exit(NULL);
} // PlayStreamThread

//== updateLcd ==
//===============
void updateLcd(const std::string & sel_filename)
{
	static int  l_playstate = -1;
	std::string lcd;

	if (l_playstate == g_playstate)
		return;

	switch (g_playstate)
	{
	case CMoviePlayerGui::PAUSE:
		lcd = "|| (";
		lcd += sel_filename;
		lcd += ')';
		break;
	default:
		lcd = "> ";
		lcd += sel_filename;
		break;
	}
	StrSearchReplace(lcd,"_", " ");
	CLCD::getInstance()->setMovieInfo("", lcd);
}

//== seek to pos with sync to next proper TS packet ==
//== returns offset to start of TS packet or actual ==
//== pos on failure.                                ==
//====================================================
#define SIZE_PROBE  (100 * 188)

static off_t mp_seekSync(int fd, off_t pos)
{
	off_t npos = pos;
	off_t ret;
	uint8_t pkt[188];

	ret = lseek(fd, npos, SEEK_SET);
	if (ret < 0)
		INFO("lseek ret = %d (%m)\n", ret);

	while (read(fd, pkt, 1) > 0)
	{
		//-- check every byte until sync word reached --
		npos++;
		if (*pkt == 0x47)
		{
			//-- if found double check for next sync word --
			if (read(fd, pkt, 188) == 188)
			{
				if(pkt[188-1] == 0x47)
				{
					ret = lseek(fd, npos-1, SEEK_SET); // assume sync ok
					if (ret < 0)
						INFO("lseek ret = %d (%m)\n", ret);
					return ret;
				}
				else
				{
					ret = lseek(fd, npos, SEEK_SET); // oops, next pkt doesn't start with sync
					if (ret < 0)
						INFO("lseek ret = %d (%m)\n", ret);
				}
			}
		}

		//-- check probe limits --
		if (npos > (pos + SIZE_PROBE))
			break;
	}

	//-- on error stay on actual position --
	return lseek(fd, pos, SEEK_SET);
}

//=======================================
//== CMoviePlayerGui::ParentalEntrance ==
//=======================================
void CMoviePlayerGui::ParentalEntrance(void)
{
	CZapProtection pin(g_settings.parentallock_pincode, 18);
	if(pin.check())
	{
		PlayFile(1);
	}
}
//=======================================
//== CMoviePlayerGui::showMovieViewer      ==
//=======================================
void CMoviePlayerGui::showMovieViewer(void)
{
	uint aspect = 0;
	CMovieViewer mv;
	g_has_ac3 = 0;
	for (unsigned int count = 0; count < g_numpida; count++)
		if (g_ac3flags[count] == 1)
			g_has_ac3 = 1;

#if HAVE_DVB_API_VERSION >=3
	aspect = (g_size.aspect_ratio == VIDEO_FORMAT_4_3)? 0:1;
#endif // HAVE_DVB_API_VERSION >=3

	mv.setData(aspect, g_playstate, g_currentac3, g_has_ac3, g_numpida, g_prozent, filename);
	mv.exec();
}

//===============================
//== CMoviePlayerGui::PlayFile ==
//===============================
void
CMoviePlayerGui::PlayFile(int parental)
{
	//TODO: parental...
	PlayStream(STREAMTYPE_LOCAL);
	return;
}

//=================================
//== CMoviePlayerGui::PlayStream ==
//=================================
void
CMoviePlayerGui::PlayStream(int streamtype)
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	std::string Path = Path_vlc;
	std::string sel_filename;
	bool update_info = true, start_play = false, exit = false;
	bool open_filebrowser = true, cdDvd = false, aborted = false;
	bool stream = true;
	char mrl[200];
	unsigned int selected = 0;
	CTimeOSD StreamTime;
	CFileList filelist;

	if (streamtype == STREAMTYPE_DVD)
	{
		strcpy(mrl, "dvdsimple:");
		strcat(mrl, g_settings.streaming_server_cddrive);
		INFO("Generated MRL: %s\n", mrl);
		sel_filename = "DVD";
		open_filebrowser = false;
		start_play = true;
		cdDvd = true;
	}
	else if (streamtype == STREAMTYPE_SVCD)
	{
		strcpy(mrl, "vcd:");
		strcat(mrl, g_settings.streaming_server_cddrive);
		INFO("Generated MRL: %s\n", mrl);
		sel_filename = "(S)VCD";
		open_filebrowser = false;
		start_play = true;
		cdDvd = true;
	}
	else if (streamtype == STREAMTYPE_LOCAL)
	{
		INFO("STREAMTYPE_LOCAL\n");
		Path = Path_local;
		stream = false;
	}

	g_apidchanged = false;
	g_playstate = CMoviePlayerGui::STOPPED;
	/* playstate == CMoviePlayerGui::STOPPED	: stopped
	 * playstate == CMoviePlayerGui::PREPARING	: preparing stream from server	####
	 * playstate == CMoviePlayerGui::ERROR		: error setting up server	####
	 * playstate == CMoviePlayerGui::PLAY		: playing
	 * playstate == CMoviePlayerGui::PAUSE		: pause-mode
	 * playstate == CMoviePlayerGui::FF		: fast-forward	####
	 * playstate == CMoviePlayerGui::REW		: rewind	####
	 * playstate == CMoviePlayerGui::JF		: jump forward x minutes	####
	 * playstate == CMoviePlayerGui::JB		: jump backward x minutes	####
	 * playstate == CMoviePlayerGui::SOFTRESET	: softreset without clearing buffer (playstate toggle to 1)
	 * #### == not implemented / used
	 */
	do
	{
		if (g_playstate == CMoviePlayerGui::STOPPED && !cdDvd)
		{
			if (selected + 1 < filelist.size() && !aborted)
			{
				selected++;
				filename = filelist[selected].Name.c_str();
				sel_filename = filelist[selected].getFileName();
				//printf ("[movieplayer.cpp] sel_filename: %s\n", filename);
				int namepos = filelist[selected].Name.rfind("vlc://");
				std::string mrl_str = filelist[selected].Name.substr(namepos + 6);
				char *tmp = curl_escape(mrl_str.c_str(), 0);
				strncpy(mrl, tmp, sizeof(mrl) - 1);
				curl_free(tmp);
				printf ("[movieplayer.cpp] Generated FILE MRL: %s\n", mrl);

				update_info = true;
				start_play = true;
			}
			else
			{
				open_filebrowser = true;
				aborted = false;
			}
		}

		if (exit)
		{
			exit = false;
			cdDvd = false;
			if (g_playstate >= CMoviePlayerGui::PLAY)
			{
				g_playstate = CMoviePlayerGui::STOPPED;
				break;
			}
		}

		if (isBookmark)
		{
			open_filebrowser = false;
			isBookmark = false;
			filename = startfilename.c_str();
			int namepos = startfilename.rfind("vlc://");
			std::string mrl_str = startfilename.substr(namepos + 6);
			char *tmp = curl_escape(mrl_str.c_str(), 0);
			strncpy(mrl, tmp, sizeof(mrl) - 1);
			curl_free(tmp);
			printf("[movieplayer.cpp] Generated Bookmark FILE MRL: %s\n", mrl);
			namepos = startfilename.rfind("/");
			sel_filename = startfilename.substr(namepos + 1);
			update_info = true;
			start_play = true;
		}

		if (open_filebrowser && !cdDvd)
		{
			g_playstate = CMoviePlayerGui::STOPPED;
			if (g_settings.streaming_show_tv_in_browser == true &&
			    g_ZapitsetStandbyState == true)
			{
				while (dmxa != -1 || dmxv != -1 || adec != -1 || vdec != -1 || dvr != -1)
				{
					INFO("want zapit wakeup, but dmxa = %d, "
					     "dmxv = %d adec = %d vdec = %d dvr = %d\n",
					      dmxa, dmxv, adec, vdec, dvr);
					usleep(250000);
				}
				g_Zapit->setStandby(false);
				g_ZapitsetStandbyState = false;
			}
			open_filebrowser = false;
			filename = NULL;
			if (stream)
				filebrowser->Filter = &vlcfilefilter;
			else
				filebrowser->Filter = &tsfilefilter;
			if (filebrowser->exec(Path.c_str()))
			{
				Path = filebrowser->getCurrentDir();
				INFO("Path: '%s'\n", Path.c_str());
				if (g_settings.streaming_allow_multiselect)
					filelist = filebrowser->getSelectedFiles();
				else
				{
					CFile *file = filebrowser->getSelectedFile();
					filelist.clear();
					filelist.push_back(*file);
				}

				if (!filelist.empty())
				{
					filename = filelist[0].Name.c_str();
					sel_filename = filelist[0].getFileName();
					INFO("sel_filename: %s\n", filename);
					if (stream)
					{
						int namepos = filelist[0].Name.rfind("vlc://");
						std::string mrl_str = filelist[0].Name.substr(namepos + 6);
						char *tmp = curl_escape(mrl_str.c_str(), 0);
						strncpy(mrl, tmp, sizeof(mrl) - 1);
						curl_free(tmp);
					}
					else
						strncpy(mrl, filename, sizeof(mrl) - 1);
					INFO("Generated FILE MRL: %s\n", mrl);

					update_info = true;
					start_play = true;
					selected = 0;
				}
			}
			else
			{
				if (g_playstate == CMoviePlayerGui::STOPPED)
					break;
			}

			CLCD::getInstance()->setMode(CLCD::MODE_MOVIE);
		}

		if (update_info)
		{
			update_info = false;
			updateLcd(sel_filename);
		}

		if (start_play)
		{
			if (g_settings.streaming_show_tv_in_browser == true &&
			    g_ZapitsetStandbyState == false)
			{
				g_Zapit->setStandby(true);
				g_ZapitsetStandbyState = true;
			}
			start_play = false;
			bufferfilled = false;
			avpids_found = false;

			if (g_playstate >= CMoviePlayerGui::PLAY)
			{
				g_playstate = CMoviePlayerGui::STOPPED;
				INFO("waiting for rct...\n");
				pthread_join(rct, NULL);
				INFO("done\n");
			}
			//TODO: Add Dialog (Remove Dialog later)
			hintBox = new CHintBox(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_MOVIEPLAYER_PLEASEWAIT)); // UTF-8
			hintBox->paint();
			buffer_time=0;

			if (pthread_create(&rct, 0, PlayStreamThread, (void *)mrl) != 0)
				break;

			g_playstate = CMoviePlayerGui::SOFTRESET;
		}

//###########################################################################
		if (g_showaudioselectdialog)
		{
			CMenuWidget APIDSelector(LOCALE_APIDSELECTOR_HEAD, "audio.raw", 300);
			APIDSelector.addItem(GenericMenuSeparator);
			g_apidchanged = false;
			pidt = 0;
			CAPIDSelectExec *APIDChanger = new CAPIDSelectExec;
			char apidnumber[3], show_pid_number[5];

			// show the normal audio pids first
			for (unsigned int count = 0; count < g_numpida; count++)
			{
				if (g_ac3flags[count] != 0) // AC3 or Teletext
					continue;
				sprintf(apidnumber, "%d", count+1);
				sprintf(show_pid_number, "%u", g_apids[count]);

				std::string apidtitle = "Stream ";
				apidtitle.append(show_pid_number);
				if (g_apids[count] == g_currentapid)
					apidtitle.append(" *"); // current stream.

				// get_movie_info_apid_name(g_apids[count],p_movie_info,&apidtitle);
				APIDSelector.addItem(
					new CMenuForwarderNonLocalized(apidtitle.c_str(), true,
						NULL, APIDChanger, apidnumber,
						CRCInput::convertDigitToKey(count+1)),
					(g_apids[count] == g_currentapid)); // select current stream
			}

			// then show the other audio pids (AC3/teletex)
			for (unsigned int count = 0; count < g_numpida; count++)
			{
				if (g_ac3flags[count] == 0) // already handled...
					continue;
				sprintf(apidnumber, "%d", count+1);
				sprintf(show_pid_number, "%u", g_apids[count]);

				std::string apidtitle = "Stream ";
				apidtitle.append(show_pid_number);
				if (g_apids[count] == g_currentapid)
					apidtitle.append(" *"); // current stream.

				if (g_ac3flags[count] == 2)
				{
					apidtitle.append(" (Teletext)");
					pidt = g_apids[count];
				}
				if (g_ac3flags[count] == 1)
				{
					// get_movie_info_apid_name(g_apids[count], p_movie_info, &apidtitle);
					// if ((int)apidtitle.find("AC3") < 0) //std::nopos)
					apidtitle.append(" (AC3)");
				}
				APIDSelector.addItem(
					new CMenuForwarderNonLocalized(apidtitle.c_str(), true,
						NULL, APIDChanger, apidnumber,
						CRCInput::convertDigitToKey(count+1)),
					(g_apids[count] == g_currentapid));
			}
			APIDSelector.exec(NULL, ""); // otherwise use Dialog
			delete APIDChanger;
			g_showaudioselectdialog = false;
		}

		g_RCInput->getMsg(&msg, &data, 10);	// 1 secs..

		if (StreamTime.IsVisible())
		{
			if (stream)
				StreamTime.update();
			else
				StreamTime.show(get_filetime());
		}

		if (msg == CRCInput::RC_red)
			g_showaudioselectdialog = true;
		else
		if (msg == CRCInput::RC_home || msg == CRCInput::RC_red)
		{
			if (g_playstate >= CMoviePlayerGui::PLAY)
			{
				StreamTime.hide();
				g_playstate = CMoviePlayerGui::STOPPED;
				aborted = true;
				if(cdDvd) {
					cdDvd = false;
					break;
				}
				else
					open_filebrowser = true;
			}
			else
				break;
		}
		else if (msg == CRCInput::RC_yellow)
		{
			update_info = true;
			g_playstate = (g_playstate == CMoviePlayerGui::PAUSE) ? CMoviePlayerGui::SOFTRESET : CMoviePlayerGui::PAUSE;
			if (stream) // stream time is only counting seconds...
				StreamTime.hide();
		}
		else if (msg == CRCInput::RC_green)
		{
			if (g_playstate == CMoviePlayerGui::PLAY)
				g_playstate = CMoviePlayerGui::RESYNC;
			if (stream)
				StreamTime.hide();
		}
		else if (msg == CRCInput::RC_blue)
		{
			if (bookmarkmanager->getBookmarkCount() < bookmarkmanager->getMaxBookmarkCount())
			{
				int stream_time;
				if ((stream_time = VlcGetStreamTime()) >= 0)
				{
					std::stringstream stream_time_ss;
					stream_time_ss << (stream_time - buffer_time);
					bookmarkmanager->createBookmark(filename, stream_time_ss.str());
				}
				else
					DisplayErrorMessage(g_Locale->getText(LOCALE_MOVIEPLAYER_WRONGVLCVERSION)); // UTF-8
			}
			else
			{
				//popup error message
				printf("too many bookmarks\n");
				DisplayErrorMessage(g_Locale->getText(LOCALE_MOVIEPLAYER_TOOMANYBOOKMARKS)); // UTF-8
			}
		}
		else if (msg == CRCInput::RC_1)
		{
			skip(-60, stream, false);
		}
		else if (msg == CRCInput::RC_3)
		{
			skip(60, stream, false);
		}
		else if (msg == CRCInput::RC_4)
		{
			skip(-300, stream, false);
		}
		else if (msg == CRCInput::RC_6)
		{
			skip(300, stream, false);
		}
		else if (msg == CRCInput::RC_7)
		{
			skip(-600, stream, false);
		}
		else if (msg == CRCInput::RC_9)
		{
			skip(600, stream, false);
		}
		else if (msg == CRCInput::RC_down)
		{
			char tmp[10+1];
			bool cancel;

			CTimeInput ti(LOCALE_MOVIEPLAYER_GOTO, tmp, LOCALE_MOVIEPLAYER_GOTO_H1, LOCALE_MOVIEPLAYER_GOTO_H2, NULL, &cancel);
			ti.exec(NULL, "");
			if (!cancel)	// no cancel
			{
				if (tmp[0]== '=')
					strcpy(skipvalue, tmp+1);
				else
					strcpy(skipvalue, tmp);
				g_playstate = CMoviePlayerGui::SKIP;
			}
		}
		else if (msg == CRCInput::RC_setup)
		{
			if (StreamTime.IsVisible())
			{
				if (stream && (StreamTime.GetMode() == CTimeOSD::MODE_ASC))
				{
					int stream_length = VlcGetStreamLength();
					int stream_time = VlcGetStreamTime();
					if (stream_time >= 0 && stream_length >= 0)
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
				// TODO: local files
				if (stream)
				{
					if ((stream_time = VlcGetStreamTime()) >= 0)
					{
						StreamTime.SetMode(CTimeOSD::MODE_ASC);
						StreamTime.show(stream_time - buffer_time);
					}
				}
				else if (g_startpts >= 0)
				{
					StreamTime.SetMode(CTimeOSD::MODE_ASC);
					StreamTime.show(get_filetime());
				}
			}
		}
		else if (msg == CRCInput::RC_help)
			showHelpVLC();
		else if (msg == CRCInput::RC_ok)
		{
			if (stream) // TODO: local files
				showFileInfoVLC();
		}
		else if (msg == CRCInput::RC_left)
		{
			if (!filelist.empty() && selected > 0 && g_playstate == CMoviePlayerGui::PLAY)
			{
				selected--;
				filename = filelist[selected].Name.c_str();
				sel_filename = filelist[selected].getFileName();
				//printf ("[movieplayer.cpp] sel_filename: %s\n", filename);
				int namepos = filelist[selected].Name.rfind("vlc://");
				std::string mrl_str = filelist[selected].Name.substr(namepos + 6);
				char *tmp = curl_escape(mrl_str.c_str(), 0);
				strncpy(mrl, tmp, sizeof(mrl) - 1);
				curl_free (tmp);
				printf ("[movieplayer.cpp] Generated FILE MRL: %s\n", mrl);
				update_info = true;
				start_play = true;
			}
		}
		else if (msg == CRCInput::RC_right)
		{
			if (!filelist.empty() && selected + 1 < filelist.size() && g_playstate == CMoviePlayerGui::PLAY)
			{
				selected++;
				filename = filelist[selected].Name.c_str();
				sel_filename = filelist[selected].getFileName();
				//printf ("[movieplayer.cpp] sel_filename: %s\n", filename);
				int namepos = filelist[selected].Name.rfind("vlc://");
				std::string mrl_str = filelist[selected].Name.substr(namepos + 6);
				char *tmp = curl_escape (mrl_str.c_str (), 0);
				strncpy (mrl, tmp, sizeof (mrl) - 1);
				curl_free (tmp);
				printf ("[movieplayer.cpp] Generated FILE MRL: %s\n", mrl);
				update_info = true;
				start_play = true;
			}
		}
		else if (msg == NeutrinoMessages::RECORD_START ||
				msg == NeutrinoMessages::ZAPTO ||
				msg == NeutrinoMessages::STANDBY_ON ||
				msg == NeutrinoMessages::SHUTDOWN ||
				msg == NeutrinoMessages::SLEEPTIMER)
		{
			// Exit for Record/Zapto Timers
			exit = true;
			g_RCInput->postMsg(msg, data);
		}
		else if (CNeutrinoApp::getInstance()->handleMsg(msg, data) & messages_return::cancel_all)
			exit = true;

		if (g_playstate == CMoviePlayerGui::SKIP && stream)
			StreamTime.hide();
	}
	while (true);
	remove("/tmp/tmpts");
	INFO("waiting for rct\n");
	pthread_join(rct, NULL);
	INFO("ends here\n");
}

#if HAVE_DVB_API_VERSION >= 3
// checks if AR has changed an sets cropping mode accordingly
static void checkAspectRatio (int vdec, bool init)
{
	static time_t last_check = 0;

	// only necessary for auto mode, check each 5 sec. max
	if (!init && time(NULL) <= last_check + 5)
		return;

	if (g_settings.video_Format == 1 && g_currentac3) // Display format 16:9
	{
		// Workaround for 16:9/AC3/PAUSE/PLAY problem
		// AVIA does reset on Jump/pause with Ac3 and 16:9 Display.It loose the display format information, which is set to default (4:3)
		// We set the display format to the correct value cyclic
		// This issue might be better fixed in the AVIA itself ... sometime   ... if somebody knows how ...
		ioctl(vdec, VIDEO_SET_DISPLAY_FORMAT, VIDEO_CENTER_CUT_OUT);
		if (ioctl(vdec, VIDEO_GET_SIZE, &g_size) < 0)
			perror("[movieplayer.cpp] VIDEO_GET_SIZE");
		last_check = time(NULL);
	}
	else if (g_settings.video_Format == 0) //Display format auto
	{
		if(init)
		{
			if (ioctl(vdec, VIDEO_GET_SIZE, &g_size) < 0)
				perror("[movieplayer.cpp] VIDEO_GET_SIZE");
			last_check = 0;
		}
		else
		{
			video_size_t new_size;
			if (ioctl(vdec, VIDEO_GET_SIZE, &new_size) < 0)
				perror("[movieplayer.cpp] VIDEO_GET_SIZE");
			if (g_size.aspect_ratio != new_size.aspect_ratio)
			{
				printf("[movieplayer.cpp] AR change detected in auto mode, adjusting display format\n");
				video_displayformat_t vdt;
				if (new_size.aspect_ratio == VIDEO_FORMAT_4_3)
					vdt = VIDEO_LETTER_BOX;
				else
					vdt = VIDEO_CENTER_CUT_OUT;
				if (ioctl(vdec, VIDEO_SET_DISPLAY_FORMAT, vdt))
					perror("[movieplayer.cpp] VIDEO_SET_DISPLAY_FORMAT");
				memcpy(&g_size, &new_size, sizeof(g_size));
			}
			last_check = time(NULL);
		}
	}
	else
	{
		if (ioctl(vdec, VIDEO_GET_SIZE, &g_size) < 0)
			perror("[movieplayer.cpp] VIDEO_GET_SIZE");
		last_check = time(NULL);;
	}
}
#else
static void checkAspectRatio (int /*vdec*/, bool /*init*/)
{
}
#endif

/************************************************************************/
std::string CMoviePlayerGui::getMoviePlayerVersion(void)
{
	static CImageInfo imageinfo;
	return imageinfo.getModulVersion("","$Revision: 1.4 $");
}

void CMoviePlayerGui::showHelpVLC()
{
	std::string version = "Movieplayer2 Version: " + getMoviePlayerVersion();
	Helpbox helpbox;
	helpbox.addLine(NEUTRINO_ICON_BUTTON_RED, g_Locale->getText(LOCALE_MOVIEPLAYER_VLCHELP1));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_GREEN, g_Locale->getText(LOCALE_MOVIEPLAYER_VLCHELP2));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_YELLOW, g_Locale->getText(LOCALE_MOVIEPLAYER_VLCHELP3));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_BLUE, g_Locale->getText(LOCALE_MOVIEPLAYER_VLCHELP4));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_DBOX, g_Locale->getText(LOCALE_MOVIEPLAYER_VLCHELP5));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_1, g_Locale->getText(LOCALE_MOVIEPLAYER_VLCHELP6));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_3, g_Locale->getText(LOCALE_MOVIEPLAYER_VLCHELP7));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_4, g_Locale->getText(LOCALE_MOVIEPLAYER_VLCHELP8));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_6, g_Locale->getText(LOCALE_MOVIEPLAYER_VLCHELP9));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_7, g_Locale->getText(LOCALE_MOVIEPLAYER_VLCHELP10));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_9, g_Locale->getText(LOCALE_MOVIEPLAYER_VLCHELP11));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_DOWN, g_Locale->getText(LOCALE_MOVIEPLAYER_VLCHELP13));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_RIGHT, g_Locale->getText(LOCALE_MOVIEPLAYER_VLCHELP15));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_LEFT, g_Locale->getText(LOCALE_MOVIEPLAYER_VLCHELP16));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_OKAY, g_Locale->getText(LOCALE_MOVIEPLAYER_VLCHELP14));
// 	helpbox.addLine(g_Locale->getText(LOCALE_MOVIEPLAYER_VLCHELP12));
	helpbox.addLine(version);
	hide();
	helpbox.show(LOCALE_MESSAGEBOX_INFO);
}

void CMoviePlayerGui::showFileInfoVLC()
{
	Helpbox helpbox;
	std::string url = "http://";
	url += g_settings.streaming_server_ip;
	url += ':';
	url += g_settings.streaming_server_port;
	url += "/requests/status.xml";
	std::string response = "";
	CURLcode httpres = sendGetRequest(url, response);
	
	if (httpres == 0 && response.length() > 0)
	{
		xmlDocPtr answer_parser = parseXml(response.c_str());
		if (answer_parser != NULL)
		{
			xmlNodePtr element = xmlDocGetRootElement(answer_parser);
			element = element->xmlChildrenNode;
			while (element)
			{
				if (strcmp(xmlGetName(element), "information") == 0)
				{
					element = element->xmlChildrenNode;
					break;
				}
				element = element->xmlNextNode;
			}
			while (element)
			{
				helpbox.addLine(NEUTRINO_ICON_BUTTON_RED, xmlGetAttribute(element, "name"));
				xmlNodePtr element1 = element->xmlChildrenNode;
				while (element1)
				{
					char tmp[50] = "-- ";
					strcat(tmp, xmlGetAttribute(element1, "name"));
					strcat(tmp, " : ");
					char* data = xmlGetData(element1);
					if (data != NULL)
						strcat(tmp, data);
					helpbox.addLine(tmp);
					element1 = element1->xmlNextNode;
				}
				element = element->xmlNextNode;
			}
			xmlFreeDoc(answer_parser);
			hide();
			helpbox.show(LOCALE_MESSAGEBOX_INFO);
		}
	}
}

static inline void skip(int seconds, bool remote, bool absolute)
{
	char *t = skipvalue;
	time_t s = abs(seconds);
	if (remote)
	{
		if (! absolute)
		{
			if (seconds < 0)
				t[0] = '-';
			else
				t[0] = '+';
			t++;
		}
		strftime(t, 9, "%T", gmtime(&s));
	}
	else
	{
		// TODO: absolute jump in local mode
		skipseconds = seconds;
	}
	g_playstate = CMoviePlayerGui::SKIP;
}

static inline int get_filetime(void)
{
	int filetime = g_pts - g_startpts;
	if (filetime < 0)
		filetime += (int)(0x1ffffffffLL / 90);
	filetime /= 1000;
	return filetime;
}

/* get the pts value from a TS or PES packet
   pes == true selects PES mode.
   Returns the pts value in ms, thus a signed int is sufficient */
static inline int get_pts(char *p, bool pes)
{
	if (!pes)
	{
		if (p[0] != 0x47)
			return -1;

		int pid = ((p[1] & 0x1f) << 8) | p[2];
		if (pid != pidv)
			return -1;

		if (!(p[1] & 0x40))
			return -1;

		if (!(p[3] & 0x10))
			return -1;

		const char *end = p + 188;

		if (p[3] & 0x20)
			p += p[4] + 4 + 1;
		else
			p += 4;

		if (p + 13 > end)
			return -1;

		if (p[0] || p[1] || (p[2] != 1))
			return -1;
	}

	if (!(p[7] & 0x80))
		return -1;

	unsigned long long pts =
		((p[ 9] & 0x0EULL) << 29) |
		((p[10] & 0xFFULL) << 22) |
		((p[11] & 0xFEULL) << 14) |
		((p[12] & 0xFFULL) << 7) |
		((p[13] & 0xFEULL) >> 1);

	//int msec = pts / 90;
	//INFO("time: %02d:%02d:%02d\n", msec / 3600000, (msec / 60000) % 60, (msec / 1000) % 60);
	return pts / 90;
}

std::string url_escape(const char *url)
{
	std::string escaped;
	char * tmp = curl_escape(url, 0);
	escaped = (std::string)tmp;
	curl_free(tmp);
	return escaped;
}

size_t curl_dummywrite (void * /*ptr*/, size_t size, size_t nmemb, void * /*data*/)
{
	return size * nmemb;
}

static void close_devices(const char *function)
{
	if (dmxa != -1)
	{
		printf("[mp:%s] dmxa != -1\n", function);
		close(dmxa);
	}
	if (dmxv != -1)
	{
		printf("[mp:%s] dmxv != -1\n", function);
		close(dmxv);
	}
	if (dvr != -1)
	{
		printf("[mp:%s] dvr != -1\n", function);
		close(dvr);
	}
	if (adec != -1)
	{
		printf("[mp:%s] adec != -1\n", function);
		close(adec);
	}
	if (vdec != -1)
	{
		printf("[mp:%s] vdec != -1\n", function);
		close(vdec);
	}
	dmxa = -1; dmxv = -1; dvr = -1; adec = -1; vdec = -1;
}
