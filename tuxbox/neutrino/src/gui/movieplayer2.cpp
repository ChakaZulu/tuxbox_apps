/*
  Neutrino-GUI  -   DBoxII-Project 

  Movieplayer "v2"
  (C) 2008, 2009 Novell, Inc. Author: Stefan Seyfried
  (C) 2009 Stefan Seyfried

  Based on the old movieplayer code (c) 2003, 2004 by gagga
  which was based on code by Dirch, obi and the Metzler Bros. Thanks.

  The remultiplexer code was inspired by the vdrviewer plugin and the
  enigma1 demultiplexer.

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
  Foundation, 51 Franklin Street, Fifth Floor Boston, MA 02110-1301, USA.
*/

/*
  This code plays (tested):
  * TS which were recorded with neutrino
  * VDR 1.4.7 recordings (dual PES)
  * VDR 1.6.0 recordings (dual PES)
  * mpeg files created from VDR recordings with dvbcut,
    "target DVD(libavformat)"
  It does not yet play correctly:
  * MPEG1 like e.g. the "Warriors Of The Net" movie from
    http://ftp.sunet.se/pub/tv+movies/warriors/warriors-700-VBR.mpg

  The VLC code allows playback of all files that are supported by VLC
  via transcoding into MPEG1 or MPEG2 and multiplexing into TS.

  Split TS recordings (Neutrino) and multi-file VDR recordings are played
  back seamlessly without any interruption including seeking across file
  boundaries etc. VLC file sets and split TS recordings are automatically
  put together as an "auto-playlist" if the first file is selected for
  playback.

  VDR's info.vdr and neutrino's XML files are used to display information
  about the recording.

  MPEG timestamps are parsed and used to determine the total and elapsed
  time. This is also used for exact seeking.

  TODO:
  * bookmarks? what bookmarks?
  * use index.vdr for precise seeking in VDR files.
  * Test and fix AC3 (I now have hardware, thanks Ray! ;)
  * the whole g_playstate state machine is too complicated and probably
    broken in subtle ways - clean it up.
  * MPEG1 parser
  * check if the CLCD->setMode(MODE_MOVIE) are all correct (and needed)
  * ...lots more... ;)

  To build it, configure with "--enable-movieplayer2".
  Enjoy.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <sstream>

#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>

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

#include <transform.h>

#include <global.h>
#include <neutrino.h>
extern "C" {
#include <driver/ringbuffer.h>
}

#include <system/helper.h>
#include <xmltree/xmlinterface.h>

#include <gui/movieplayer.h>
#include <gui/timeosd.h>
#include <gui/movieinfo.h>
#include <gui/imageinfo.h>

#include <gui/widget/icons.h>
#include <gui/widget/messagebox.h>
#include <gui/widget/hintbox.h>
#include <gui/widget/helpbox.h>
#include <gui/widget/stringinput_ext.h>

#define INFO(fmt, args...) fprintf(stderr, "[mp:%s:%d] " fmt, __FUNCTION__, __LINE__, ##args)
#if 0	// change for verbose debug output
#define DBG INFO
#else
#define DBG(args...)
#endif

// if there is more than one audio stream, should it be autoselected?
// define, if you don't that
//#define AUDIO_STREAM_AUTOSELECT

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

static bool isPES, isBookmark;

#ifndef __USE_FILE_OFFSET64
#error not using 64 bit file offsets
#endif /* __USE_FILE__OFFSET64 */
ringbuffer_t *ringbuf;
bool bufferfilled;
bool bufferreset = false;
int g_percent = 0;	// percentage of the file position

unsigned short pida, pidv, pidt;
CHintBox *hintBox;
CHintBox *bufferingBox;
std::string startfilename;

// globals for skipping / jumping. Ugly...
int skipseconds = 0;
bool skipabsolute = false;

int buffer_time = 0;

// globals for multi-file handling.
// this should be implemented as a class...
CFileList *g_f = NULL;
const char *g_fn = NULL;
int g_fd = -1;
int g_numfiles = 0;
int g_fileno = -1;

// global variables shared by playthread and PlayFile
static CMoviePlayerGui::state g_playstate;
// the input thread requests skipping (e.g. for retrying)
static bool g_skiprequest = false;
// input thread signals end-of-file
static bool g_EOF;
// input thread failed.
static bool g_input_failed;
// output thread is running <= TODO: fix this ugly kludge
static bool g_output_thread = false;

// start position in seconds for bookmarks...
static int g_startposition = 0;
// 32 MPEG audio streams + 8 AC3 streams, theoretically.
#define MAX_APIDS 40
uint16_t g_apids[MAX_APIDS];
unsigned short g_ac3flags[MAX_APIDS];
uint16_t g_numpida=0;
int g_currentapid = -1; // pida is for the decoder, most of the time the same as g_currentapid
unsigned int   g_currentac3  = 0;
bool           g_apidchanged = false;
bool           g_ac3changed = false;
unsigned int   g_has_ac3 = 0;
unsigned short g_prozent=0;

time_t g_pts = 0;
time_t g_startpts = -1;
time_t g_endpts = -1;

#if HAVE_DVB_API_VERSION >=3
video_size_t   g_size;
#endif // HAVE_DVB_API_VERSION >=3

bool  g_showaudioselectdialog = false;

// Function prototypes for helper functions
static void checkAspectRatio (int vdec, bool init);
static off_t mp_seekSync(off_t pos);
static inline void skip(int seconds, bool remote, bool absolute);
static inline int get_filetime(bool remaining = false);
static inline int get_pts(char *p, bool pes);
static int mp_syncPES(ringbuffer_t *buf, char **pes);
static int get_PES_PTS(ringbuffer_t *buf, off_t position, bool until_eof = false);
std::string url_escape(const char *url);
size_t curl_dummywrite (void *ptr, size_t size, size_t nmemb, void *data);
static bool filelist_auto_add(CFileList &filelist);

static int mf_open(int fileno);
static int mf_close(void);
static off_t mf_lseek(off_t pos);
static off_t mf_getsize(void);

//------------------------------------------------------------------------

int CAPIDSelectExec::exec(CMenuTarget* /*parent*/, const std::string & actionKey)
{
	g_apidchanged = false;
	g_ac3changed = false;
	unsigned int sel= atoi(actionKey.c_str());
	if (g_currentapid != g_apids[sel-1])
	{
		g_currentapid = g_apids[sel-1];
		if (g_currentac3 != g_ac3flags[sel-1])
			g_ac3changed = true;
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

	if (g_settings.streaming_moviedir.length() != 0)
		Path_local = g_settings.streaming_moviedir;
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

	tsfilefilter.addFilter("ts");
	tsfilefilter.addFilter("mpg");
	tsfilefilter.addFilter("mpeg");
	tsfilefilter.addFilter("vdr");
	tsfilefilter.addFilter("m2p");	// untested
	tsfilefilter.addFilter("vob");	// untested
	tsfilefilter.addFilter("m2v");	// untested
	tsfilefilter.addFilter("dbox");

	vlcfilefilter.addFilter("mpg");
	vlcfilefilter.addFilter("mpeg");
	vlcfilefilter.addFilter("m2p");
	vlcfilefilter.addFilter("avi");
	vlcfilefilter.addFilter("vob");
	vlcfilefilter.addFilter("wmv");
	vlcfilefilter.addFilter("m2v");
	vlcfilefilter.addFilter("mp4");

	filebrowser->Filter = &tsfilefilter;
	INFO("Initialized!\n");
}

//------------------------------------------------------------------------

CMoviePlayerGui::~CMoviePlayerGui ()
{
	delete filebrowser;

	if (bookmarkmanager)
		delete bookmarkmanager;

	if (g_playstate != CMoviePlayerGui::STOPPED)
		INFO("g_playstate != STOPPED: %d!\n", g_playstate);
	// just to make sure...
	g_playstate = CMoviePlayerGui::STOPPED;
	while (g_output_thread)
	{
		INFO("waiting for output thread to terminate...\n");
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
	printf("[%s] CMoviePlayerGui::exec actionKey='%s'\n", __FILE__, actionKey.c_str());

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

	// tell neutrino we're in ts_mode
	CNeutrinoApp::getInstance()->handleMsg(NeutrinoMessages::CHANGEMODE, NeutrinoMessages::mode_ts);

	/* remember last mode,
	   needs to be done while zapit is still not paused */
	CZapitClient::responseGetLastChannel firstchannel;
	g_Zapit->getLastChannel(firstchannel.channelNumber, firstchannel.mode);
	if ((firstchannel.mode == 'r') ?
	    (CNeutrinoApp::getInstance()->zapto_radio_on_init_done) :
	    (CNeutrinoApp::getInstance()->zapto_tv_on_init_done))
		m_LastMode = (CNeutrinoApp::getInstance()->getLastMode() | NeutrinoMessages::norezap);
	else
		m_LastMode = (CNeutrinoApp::getInstance()->getLastMode());

	g_ZapitsetStandbyState = false; // 'Init State

	// if filebrowser playback we check if we should disable the tv (other modes might be added later)
	if (g_settings.streaming_show_tv_in_browser == false ||
	    (actionKey != "tsplayback"     &&
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

	// Stop sectionsd
	g_Sectionsd->setPauseScanning(true);

	isBookmark=false;
	startfilename = "";
	g_startposition = 0;
	isPES=false;

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
	else if (actionKey=="tsplayback_pc")
	{
		ParentalEntrance();
	}
	else if (actionKey=="bookmarkplayback")
	{
		isBookmark = true;
		if (theBookmark != NULL)
		{
			startfilename = theBookmark->getUrl();
			sscanf(theBookmark->getTime(), "%d", &g_startposition);
			int vlcpos = startfilename.rfind("vlc://");
			CLCD::getInstance()->setMode(CLCD::MODE_MOVIE);
			if (vlcpos == 0)
				PlayStream(STREAMTYPE_FILE);
			else
			{
				// TODO check if file is a TS. Not required right now as writing bookmarks is disabled for PES anyway
				startfilename = "";
				PlayFile();
			}
		}
	}
	else if (actionKey.find("file://") == 0)
	{
		std::string::size_type spos = actionKey.rfind("?startpos=");
		std::string fn;
		if (spos != std::string::npos)
		{
			const char *tmp = actionKey.substr(spos + 10).c_str();
			g_startposition = atoll(tmp);
		}
		else
		{
			g_startposition = 0;
			spos = actionKey.size();
		}

		startfilename = actionKey.substr(7, spos - 7);
		CLCD::getInstance()->setMode(CLCD::MODE_MOVIE);
		PlayFile();
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
		while (g_output_thread)
		{
			INFO("waiting for output thread to terminate...\n");
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

	return menu_return::RETURN_REPAINT;
}

//------------------------------------------------------------------------
CURLcode sendGetRequest (const std::string & url, std::string & response)
{
	CURL *curl;
	CURLcode httpres;

	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1); // no signals, please.
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30); // nothing should take longer than 30 seconds. hopefully
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15); // "15 seconds should be enough for everyone"
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_dummywrite);
	curl_easy_setopt(curl, CURLOPT_FILE, (void *)&response);
	curl_easy_setopt(curl, CURLOPT_FAILONERROR, true);
	httpres = curl_easy_perform(curl);
	//INFO("HTTP url: '%s' Result: %d\n", url.c_str(), httpres);
	curl_easy_cleanup(curl);
	return httpres;
}

/* gets a filename with a stream2stream playlist file,
   returns a file descriptor to the stream or -1 */
int box2box_request_stream(const char *fn)
{
	char tmpbuf[512];
	char id_s[33]; // long long hex == 32 char's + \0
	int fd = -1;
	char *ip;
	char *name;
	int port, vp, ap, ret;
	long long id;
	char *p1, *p2;
	std::string url, response;
	CURLcode httpres;
	FILE *fp = fopen(fn, "r");
	if (!fp)
		return -1;

	if (!fgets(tmpbuf, 1024, fp))
		goto nofile;
	DBG("tmpbuf: %s", tmpbuf);
	if (strcmp(tmpbuf, "#DBOXSTREAM\n") != 0)
	{
		INFO("invalid file signature: no #DBOXSTREAM found\n");
		goto nofile;
	}
	DBG("found #DBOXSTREAM\n");
	if (!fgets(tmpbuf, 1024, fp))
		goto nofile;
	if (tmpbuf[strlen(tmpbuf) - 1] == '\n')
		tmpbuf[strlen(tmpbuf) - 1] = 0; // remove \n, cosmetic
	DBG("got nextline: '%s'\n", tmpbuf);
	p1 = tmpbuf;
	p2 = strchr(tmpbuf, '=');
	if (!p2)
		goto nofile;
	*p2 = 0;
	name = p1;
	INFO("name: '%s'\n", name); // just info for now...
	p1 = p2 + 1;
	p2 = strchr(p1, ';');
	if (!p2)
		goto nofile;
	*p2 = 0;
	ip = p1;;
	DBG("ip: '%s'\n", ip);
	p1 = p2 + 1;
	ret = sscanf(p1, "%d;0x%x;0x%x;0x%llx", &port, &vp, &ap, &id);
	DBG("sscanf result: %d. port: %d vpid: 0x%x, apid: 0x%x, zapid: 0x%llx\n", ret, port, vp, ap, id);
	if (ret != 4)
		goto nofile;

	url = "http://";
	url += ip;
	url += "/control/zapto?0x";
	snprintf(id_s, 33, "%llx", id);
	url += id_s;
	INFO("get zapto: '%s'\n", url.c_str());
	httpres = sendGetRequest(url, response);
	if (httpres == 0)
	{
		INFO("zapto response: %s\n", response.c_str());
		// usleep(250000); should not be necessary, since the zapto is (or should be) blocking
		struct sockaddr_in ads;
		socklen_t ads_len = sizeof(sockaddr_in);
		memset((char *)&ads, 0, ads_len);
		ads.sin_family = AF_INET;
		ads.sin_addr.s_addr = inet_addr(ip);
		ads.sin_port = htons(port);
		fd = socket(AF_INET, SOCK_STREAM, 0);
		if (fd == -1)
			goto nostream;
		if (connect(fd, (struct sockaddr *)&ads, ads_len) == -1)
		{
			INFO("connect failed (%m)\n");
			goto nostream;
		}
		// do not use "ip" or "name" below here, since the tmpbuf gets reused!
		sprintf(tmpbuf, "GET /0x%x,0x%x HTTP/1.0\r\n", vp, ap);
		INFO("get request: %s", tmpbuf);
		write(fd, &tmpbuf, strlen(tmpbuf));
		ret = read(fd, &tmpbuf, 17); // HTTP/1.1 200 OK\r\n
		if (ret < 0)
		{
			INFO("read failed (%m)\n");
			goto nostream;
		}
		tmpbuf[ret] = 0; // terminate
		if (!strstr(tmpbuf, "200 OK"))
		{
			INFO("did not receive '200 OK'\n");
			goto nostream;
		}
		int found = 0;
		for (int i = 0; i < 512 && found < 2; i++)
		{
			if (read(fd, &tmpbuf, 1) != 1)
				break;
			if (tmpbuf[0] == '\n')
				found++;
		}
		if (found < 2)
		{
			INFO("incorrect fileheader\n");
			goto nostream;
		}
		g_numpida = 1;
		pida = ap;
		pidv = vp;
		g_currentapid = pida;
		DBG("isstream == true, numpida: %d pida: %d, pidv: %d\n", g_numpida, pida, pidv);
	}
	return fd;

 nostream:
	close(fd);
 nofile:
	INFO("no file. Sorry.\n");
	fclose(fp);
	return -1;
}

#define TRANSCODE_VIDEO_OFF 0
#define TRANSCODE_VIDEO_MPEG1 1
#define TRANSCODE_VIDEO_MPEG2 2
//------------------------------------------------------------------------
bool VlcRequestStream(std::string mrl, int transcodeVideo, int transcodeAudio)
{
	CURLcode httpres;
	std::string response;
	std::string baseurl = "http://";
	baseurl += g_settings.streaming_server_ip;
	baseurl += ':';
	baseurl += g_settings.streaming_server_port;
	baseurl += '/';

	std::string reseturl = baseurl;
	reseturl += "requests/status.xml?command=pl_empty";
	httpres = sendGetRequest(reseturl, response);

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
	url += url_escape(mrl.c_str());
	url += "%20%3Asout%3D";
	url += url_escape(souturl.c_str());
	printf("[movieplayer.cpp] URL(enc) : %s\n", url.c_str());
	httpres = sendGetRequest(url, response);

	return true; // TODO error checking
}

//------------------------------------------------------------------------
int VlcGetStatus(const char *attribute)
{
	int ret = -1;
	std::string positionurl = "http://";
	positionurl += g_settings.streaming_server_ip;
	positionurl += ':';
	positionurl += g_settings.streaming_server_port;
	positionurl += "/requests/status.xml";
	//printf("[movieplayer.cpp] positionurl=%s\n",positionurl.c_str());
	std::string response = "";
	CURLcode httpres = sendGetRequest(positionurl, response);
	//printf("[movieplayer.cpp] httpres=%d, response.length()=%d, resp='%s'\n",httpres,response.length(),response.c_str());
	if (httpres == 0 && response.length() > 0)
	{
		xmlDocPtr answer_parser = parseXml(response.c_str());
		if (answer_parser != NULL)
		{
			xmlNodePtr element = xmlDocGetRootElement(answer_parser);
			element = element->xmlChildrenNode;
			while (element)
			{
				char* tmp = xmlGetName(element);
				if (strcmp(tmp, attribute) == 0)
				{
					ret = atoi(xmlGetData(element));
					break;
				}
				element = element->xmlNextNode;
			}
			xmlFreeDoc(answer_parser);
		}
	}

	return ret;
}

inline int VlcGetStreamTime()
{
	return VlcGetStatus("time");
}

inline int VlcGetStreamLength()
{
	return VlcGetStatus("length");
}

//------------------------------------------------------------------------
void *
ReceiveStreamThread(void *arg)
{
	INFO("started\n");
	int skt;
	int len;

	int nothingreceived = 0;
	std::string sMRL = (char*)arg;
	bool is_box2box = false;
	bool avpids_found = false;
	g_input_failed = false;

	// Get Server and Port from Config	
	std::string response;
	std::string baseurl = "http://";
	baseurl += g_settings.streaming_server_ip;
	baseurl += ':';
	baseurl += g_settings.streaming_server_port;
	baseurl += '/';

	std::string statusurl = baseurl;
	statusurl += "requests/status.xml";

	std::string pauseurl = baseurl;
	pauseurl += "requests/status.xml?command=vlc_pause";
	std::string unpauseurl = baseurl;
	unpauseurl += "requests/status.xml?command=vlc_play";

	CURLcode httpres;

	if (sMRL.rfind(".dbox") == sMRL.length() - 5)
	{
		// dbox2dbox playlist file
		skt = box2box_request_stream(sMRL.c_str());
		if (skt < 0)
		{
			INFO("box2box playlist open failed\n");
			g_input_failed = true;
			pthread_exit(NULL);
		}
		is_box2box = true;
		avpids_found = true;
	}
	else
	{
		httpres = sendGetRequest(statusurl, response);
		if (httpres != 0)
		{
			hintBox->hide();
			DisplayErrorMessage(g_Locale->getText(LOCALE_MOVIEPLAYER_NOSTREAMINGSERVER));	// UTF-8
			g_input_failed = true;
			pthread_exit(NULL);
			// Assume safely that all succeeding HTTP requests are successful
		}

		int transcodeVideo, transcodeAudio;
		//Menu Option Force Transcode: Transcode all Files, including mpegs.
		if (!strncmp(sMRL.c_str(), "vcd:", 4) ||
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
			if ((!strncmp(sMRL.c_str(), "dvd", 3) && !g_settings.streaming_transcode_audio) ||
			    (!strcasecmp(sMRL.substr(sMRL.length()-3).c_str(), "vob") && !g_settings.streaming_transcode_audio) ||
			    (!strcasecmp(sMRL.substr(sMRL.length()-3).c_str(), "ac3") && !g_settings.streaming_transcode_audio) ||
			    g_settings.streaming_force_avi_rawaudio)
				transcodeAudio = 0;
			else
				transcodeAudio = 1;
		}
		if (sMRL.find("vlc://") == 0)
			sMRL = sMRL.substr(6);
		VlcRequestStream(sMRL, transcodeVideo, transcodeAudio);

		// TODO: Better way to detect if http://<server>:8080/dboxstream is already alive. For example repetitive checking for HTTP 404.
		// Unfortunately HTTP HEAD requests are not supported by VLC :(
		// vlc 0.6.3 and up may support HTTP HEAD requests.

		// Open HTTP connection to VLC

		const char *server = g_settings.streaming_server_ip.c_str();
		int port;
		sscanf(g_settings.streaming_server_port, "%d", &port);

		struct sockaddr_in servAddr;
		servAddr.sin_family = AF_INET;
		servAddr.sin_port = htons(port);
		servAddr.sin_addr.s_addr = inet_addr(server);

		INFO("Server: %s Port: %d\n", server, port);
		time_t start = time(NULL);

		while (true)
		{
			//printf ("[movieplayer.cpp] Trying to call socket\n");
			skt = socket (AF_INET, SOCK_STREAM, 0);

			if (connect(skt, (struct sockaddr *) &servAddr, sizeof (servAddr)) < 0)
			{
				INFO("Socket connect failed: %m\n");
				g_input_failed = true;
				pthread_exit(NULL);
			}
			fcntl(skt, O_NONBLOCK);
			DBG("Connect OK\n");

			// Skip HTTP header
			const char * msg = "GET /dboxstream HTTP/1.0\r\n\r\n";
			int msglen = strlen(msg);
			if (send (skt, msg, msglen, 0) == -1)
			{
				INFO("Socket send failed: %m\n");
				g_input_failed = true;
				pthread_exit(NULL);
			}

			DBG("GET Sent\n");

			// Skip HTTP Header
			int found = 0;
			char buf[2];
			char line[200];
			buf[0] = buf[1] = '\0';
			strcpy(line, "");
			time_t now = time(NULL);
			while (g_playstate != CMoviePlayerGui::STOPPED && (now - start < 30))
			{
				len = recv(skt, buf, 1, 0);
				strncat(line, buf, 1);
				if (strcmp(line, "HTTP/1.0 404") == 0)
				{
					INFO("VLC still does not send. Retrying...\n");
					close(skt);
					sleep(1);
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
				now = time(NULL);
			}
			if (g_playstate == CMoviePlayerGui::STOPPED || (now - start >= 30))
			{
				close(skt);
				pthread_exit(NULL);
			}
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


	int length = 0;
	int counter = 0;
	if (!is_box2box)
		length = VlcGetStreamLength();
	INFO("VLC Stream length: %d\n", length);

	while (g_playstate != CMoviePlayerGui::STOPPED)
	{
		switch (g_playstate)
		{
		case CMoviePlayerGui::SKIP:
		{
			if (is_box2box)
			{	// ugly, to avoid special b2b case in output thread
				DBG("fake BUFFERRESET!\n");
				while (!bufferreset && g_playstate != CMoviePlayerGui::STOPPED)
					usleep(100000);
				bufferfilled = false;
				bufferreset = false;
				/* OutputThread() sets g_playstate to SOFTRESET */
				while (g_playstate == CMoviePlayerGui::SKIP)
					usleep(100000);
				break;
			}
			std::string skipurl;
			char skipvalue[20];
			time_t s = abs(skipseconds);
			int o = 0;
			if (!skipabsolute)
			{
				if (skipseconds < 0)
					skipvalue[0] = '-';
				else
					skipvalue[0] = '+';
				o = 1;
			}
			strftime(&skipvalue[o], 9, "%T", gmtime(&s));
			counter = 0;
			skipurl = baseurl;
			skipurl += "requests/status.xml?command=seek&val=";
			skipurl += url_escape(skipvalue);
			INFO("skipping URL(enc) : %s\n",skipurl.c_str());
			DBG("BUFFERRESET!\n");
			while (!bufferreset && g_playstate != CMoviePlayerGui::STOPPED)
				usleep(100000);
			bufferfilled = false;
//			int bytes = (ringbuffer_read_space(ringbuf) / 188) * 188;
//			ringbuffer_read_advance(ringbuf, bytes);
			ringbuffer_reset(ringbuf);
			bufferreset = false;
			INFO("BUFFERRESET done.\n");
			httpres = sendGetRequest(skipurl, response);
			/* OutputThread() sets g_playstate to SOFTRESET */
			while (g_playstate == CMoviePlayerGui::SKIP)
				usleep(100000);
			break;
		}
		case CMoviePlayerGui::PAUSE:
			// pause VLC
			if (is_box2box)
			{
				usleep(100000);
				continue;
			}
			httpres = sendGetRequest(pauseurl, response);
			while (g_playstate == CMoviePlayerGui::PAUSE)
				usleep(100000); // no busy wait
			// unpause VLC
			httpres = sendGetRequest(unpauseurl, response);
			break;
		case CMoviePlayerGui::PLAY:
			if (is_box2box)
				break;
			if (length == 0)
				length = VlcGetStreamLength();
			if (length != 0)
			{
				if (--counter < 0) // initialized to 0;
				{
					g_percent = (VlcGetStreamTime() * 100) / length;
					counter = 119;
				}
			}
			break;
		default:
			break;
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
				INFO("Searching for vpid and apid\n");
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
				INFO("found pida: 0x%04X pidv: 0x%04X ac3: %d\n",
				      pida, pidv, g_currentac3);
				avpids_found = true;
				// Calculate diffrence between vlc time and play time
				// The buffer is filled for the first time, ask vlc for his position
				buffer_time = VlcGetStreamTime();
				INFO("VLC buffer time: %d\n", buffer_time);
				if (buffer_time < 0)
					buffer_time = 0;
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
			pollret = poll(poller, 1UL, 10000); // ten second timeout, don't block.
			if (pollret == 0) // timeout;
			{
				INFO("nothing received for 10 seconds... => get out!\n");
				g_input_failed = true;
				break;
			}

			if (pollret < 0 ||
			    (poller[0].revents & (POLLHUP | POLLERR | POLLNVAL)) != 0)
			{
				perror("Error while polling()");
				g_input_failed = true;
				break;
			}

			if ((poller[0].revents & (POLLIN | POLLPRI)) != 0)
				len = recv(poller[0].fd, vec[0].buf, size, 0);
			else
				len = 0;

			if (len > 0)
			{
				ringbuffer_write_advance(ringbuf, len);

				nothingreceived = 0;
				DBG("bytes received:%d\n", len);
				if (!avpids_found)
					write (fd, vec[0].buf, len);
			}
			else
			{
				if (g_playstate == CMoviePlayerGui::PLAY)
				{
					nothingreceived++;
fprintf(stderr,".");
					if (nothingreceived > (buffer_time + 4) * 100) // wait at least buffer time secs +3 to play buffer when stream ends
					{
						INFO("Didn't receive for a while. Stopping.\n");
						g_input_failed = true;
						break;
					}
					usleep(10000);	//sleep 10 ms
				}
			}
		}
	}
	// stop VLC
	std::string stopurl = baseurl;
	stopurl += "requests/status.xml?command=pl_stop";
	if (!is_box2box)
		httpres = sendGetRequest(stopurl, response);
	close(skt);

	remove("/tmp/tmpts");
	INFO("ends now. input_failed: %s\n", g_input_failed ? "true" : "false");
	/* for testing thread interaction
	sleep(10);
	INFO("ends really\n");
	*/
	pthread_exit(NULL);
} // ReceiveStreamThread

void *
ReadTSFileThread(void *parm)
{
	/* reads a TS file into *ringbuf */
	g_f = (CFileList *)parm;
	g_fd = mf_open(0);
	g_EOF = false;
	INFO("start, filename = '%s', fd = %d, f.size = %d\n", (*g_f)[0].Name.c_str(), g_fd, (*g_f).size());
	ssize_t len;
	size_t readsize;
	off_t bytes_per_second = 500000;
	off_t filesize = 0;
	off_t filepos = 0;
	unsigned int lastpts = 0, smooth = 0;
	off_t lastpos = 0, ptspos = 0;
	time_t last = 0;
	int skipretry = 0;
	int i;
	char *ts;
	ringbuffer_data_t vec[2];

	g_percent = 0;
	hintBox->hide(); // the "connecting to streaming server" hintbox
	bufferingBox->paint();
	INFO("Buffering...\n");

	g_input_failed = false;

	filesize = mf_getsize();
	INFO("Number of files: %d overall size: %lld\n", g_numfiles, filesize);

	filepos = mp_seekSync(0);
	if (filepos < 0)
		perror("ReadTSFileThread lseek");
	INFO("file starts at %lld\n", filepos);

	pidv = 0;
	memset(&g_apids, 0, sizeof(g_apids));
	memset(&g_ac3flags, 0, sizeof(g_ac3flags));
	find_all_avpids(g_fd, &pidv, g_apids, g_ac3flags, &g_numpida);
	pida = g_apids[0];
	g_currentac3 = g_ac3flags[0];
	g_currentapid = -1;
	INFO("found pida: 0x%04X pidv: 0x%04X ac3: %d numpida: %d\n", pida, pidv, g_currentac3, g_numpida);
	if (g_numpida > 1)
	{
		printf(" => additional apids:");
		for (i = 1; i < g_numpida; i++)
			printf(" 0x%04X", g_apids[i]);
		printf("\n");
	}
#ifndef AUDIO_STREAM_AUTOSELECT
	else
		g_currentapid = pida;
#else
	g_currentapid = pida;
#endif

	mf_lseek(filepos);
	ringbuffer_reset(ringbuf);
	ringbuffer_get_write_vector(ringbuf, &(vec[0]));
	readsize = vec[0].len / 188 * 188;
	len = read(g_fd, vec[0].buf, readsize); // enough?
	if (len < 0)
	{
		INFO("first read failed (%m)\n");
		mf_close();
		INFO("ends now.\n");
		g_input_failed = true;
		pthread_exit(NULL);
	}

	ts = vec[0].buf;
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
		INFO("PTS at file start: %ld\n", g_startpts);

	off_t endpos = filepos + (filesize - filepos - 1024*1024) / 188 * 188;
	g_endpts = -1;
	if (endpos > filepos)
	{
		off_t syncpos = mp_seekSync(endpos);
		if (syncpos != endpos)
			INFO("seeking to end of file: out of sync (wanted %lld got %lld)\n", endpos, syncpos);
		ringbuffer_reset(ringbuf);
		ringbuffer_get_write_vector(ringbuf, &(vec[0]));
		readsize = vec[0].len / 188 * 188;
		len = read(g_fd, vec[0].buf, readsize); // enough?
		if (len < 0)
			INFO("last read failed??? (%m)\n");
		else
		{
			ts = vec[0].buf;
			i = (len / 188 * 188) - 188;
			/* go backwards from the end of the buffer */
			while (i >= 0)
			{
				g_endpts = get_pts(ts + i, false);
				if (g_endpts != -1)
					break;
				i -= 188;
			}
			if (g_endpts == -1)
				INFO("could not determine PTS at file end\n");
			else
			{
				if (g_endpts < g_startpts)
					g_endpts += 95443717; // (0x200000000 / 90);
				time_t tmp = (g_endpts - g_startpts) / 1000;
				if (g_endpts != g_startpts)
					bytes_per_second = (syncpos - filepos) / tmp;
				INFO("PTS at file pos %lld: %ld filelen: %ld, bps: %lld\n", syncpos + i, g_endpts, tmp, bytes_per_second);
			}
		}
	}
	else
		INFO("file too short for determining PTS at file end\n");

	lastpts = g_startpts;
	g_pts = g_startpts;

	ringbuffer_reset(ringbuf); // not aligned anymore, so reset...
	mf_lseek(filepos);

	skipabsolute = false;
	skipseconds = 0;

	while (g_playstate != CMoviePlayerGui::STOPPED && !g_EOF && !g_input_failed)
	{
		time_t now;
		switch(g_playstate)
		{
		case CMoviePlayerGui::SKIP:
			INFO("lseek from %lld, seconds %d\n", filepos, skipseconds);
			if (!(g_startpts == -1 && skipabsolute)) // only jump absolute if startpts is known
			{
				int s = skipseconds;
				if (skipabsolute)
					s = skipseconds - get_filetime();
				filepos += (bytes_per_second * s) / 188 * 188;
				// smooth = 0;
				last = 0;
				bufferingBox->paint();

				if (filepos >= filesize)
					filepos -= (filepos - filesize + 30 * bytes_per_second) / 188 * 188;
				if (filepos < 0)
					filepos = 0;
				if (mp_seekSync(filepos) < 0)
					INFO("ReadTSFileThread mp_seekSync < 0!\n");
			}
			DBG("BUFFERRESET!\n");
			while (!bufferreset && g_playstate != CMoviePlayerGui::STOPPED)
				usleep(100000);
			bufferfilled = false;
			ringbuffer_reset(ringbuf);
			bufferreset = false;
			DBG("BUFFERRESET done.\n");
			/* OutputThread() sets g_playstate to SOFTRESET */
			while (g_playstate == CMoviePlayerGui::SKIP)
				usleep(100000);
			INFO("skip ends\n");
			break;
		case CMoviePlayerGui::PLAY:
			if (g_startpts != -1 && g_endpts != -1)
				break;	// no need to update bytes_per_second
			if (last == 0)
			{
				last = time(NULL);
				lastpts = g_pts;
				lastpos = ptspos;
				break;
			}
			now = time(NULL);
			if ((now - last) > 4) // update bytes_per_second every 5 seconds
			{
				int diff_pts = g_pts - lastpts;
				off_t diff_pos = ptspos - lastpos;
				off_t diff_bps = diff_pos * 1000 / diff_pts;
				lastpos = ptspos;
				lastpts = g_pts;
				// printf("bytes p/s old: %lld", bytes_per_second);
				if (diff_bps > 0 && diff_pos > 0) // discontinuity, startup...
				{
					if (smooth < 8)
						smooth++;
					bytes_per_second = (smooth * bytes_per_second + diff_bps) / (smooth + 1);
				}
				// else printf(" not updated");
				// printf(" new: %lld, diff PTS:%5d diff_pos %lld, filepos: %d%%\n",
				// 	bytes_per_second, diff_pts, diff_pos, g_percent);
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
				len = read(g_fd, vec[0].buf, todo);
				if (todo == readsize) // only first read of the while() loop, so we are still TS-aligned
				{
					ts = vec[0].buf;
					while (ts - vec[0].buf < (int)todo)
					{
						int pts = get_pts(ts, false);
						if (pts != -1)
						{
							g_pts = pts;
							ptspos = filepos + (ts - vec[0].buf);
							if (skipabsolute && !g_skiprequest)
							{
								time_t timediff = skipseconds - get_filetime();
								if (skipretry++ < 10 && abs(timediff) > 10)
								{
									INFO("offset > 10 seconds (%ld), retry skipping...(%d)\n", timediff, skipretry);
									g_skiprequest = true;
								}
								else
								{
									// more than 10 retries -> get out
									skipabsolute = false;
									skipretry = 0;
								}
							}
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
				len = read(g_fd, vec[0].buf, vec[0].len);
				if (len < 0)
				{
					perror("ReadTSFileThread read");
					g_input_failed = true;
					break;
				}
				done += len;
				todo -= len;
				len = read(g_fd, vec[1].buf, todo);
			}
			else
			{
				INFO("something is wrong. vec[0].len: %ld vec[1].len: %ld, todo: %ld\n",
				      vec[0].len, vec[1].len, todo);
				g_input_failed = true;
				break;
			}

			if (len <= 0 && !skipabsolute) // len < 0 => error, len == 0 => EOF
			{
				if (len < 0)
				{
					perror("ReadTSFileThread read");
					g_input_failed = true;
				}
				else if (g_fileno + 1 < g_numfiles)
				{
					if (mf_lseek(filepos) >= 0) // filepos points at byte 0 of next file
						continue;
					g_input_failed = true;
				}
				INFO("error or EOF => exiting\n");
				g_EOF = true; // does not matter if EOF or read error...
				break;
			}
			done += len;
			ringbuffer_write_advance(ringbuf, done);
			filepos += done;
			todo -= len;
			//if (todo) DBG(stderr, "todo: %ld\n", todo); // in reality, this never triggers.
		}

		if (filesize)
			g_percent = filepos * 100 / filesize;	// overflow? not with 64 bits...
	}
	mf_close();
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
	g_f = (CFileList *)parm;
	mf_open(0);
	g_EOF = false;
	INFO("start, filename = '%s', fd = %d, f.size = %d\n", (*g_f)[0].Name.c_str(), g_fd, (*g_f).size());
	int len, size;
	size_t rd;
	off_t bytes_per_second = 500000;
	off_t filesize = 0;
	off_t filepos = 0;
	g_percent = 0;
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

	filesize = mf_getsize();
	INFO("Number of files: %d overall size: %lld\n", g_numfiles, filesize);
	pidv = 100;
	pida = 101;

	bufferingBox->paint();
	INFO("Buffering...\n");

	ringbuffer_t *buf_in = ringbuffer_create(0x1FFFF);
	INFO("input ringbuffer created, size: 0x%lx\n", ringbuffer_write_space(buf_in));
	ringbuffer_data_t vec_in[2];
	time_t last = 0;
	unsigned int lastpts = 0, smooth = 0;
	off_t lastpos = 0, ptspos = 0;
	int skipretry = 0;
	int type = 0; // TODO: use wisely...
	int i = 0; // loop counter for PTS check

	unsigned char found_aid[MAX_APIDS]; // lame hash;
	memset(&found_aid, 0, sizeof(found_aid));
	memset(&g_apids, 0, sizeof(g_apids));
	memset(&g_ac3flags, 0, sizeof(g_ac3flags));
	g_currentapid = -1;
	g_currentac3 = 0;
	g_numpida = 0;
	g_startpts = -1;
	g_endpts = -1;
	bool input_empty = true;
	g_input_failed = false;
	skipabsolute = false;
	skipseconds = 0;

	g_startpts = get_PES_PTS(buf_in, 0);
	INFO("PTS at file start: %ld\n", g_startpts);
	if (filesize > 1024*1024)
	{
		g_endpts = get_PES_PTS(buf_in, filesize - 1024*1024, true); // until EOF
		if (g_endpts >= 0 && g_endpts < g_startpts)
			g_endpts += 95443717; // (0x200000000 / 90);
		time_t tmp = (g_endpts - g_startpts) / 1000;
		if (g_startpts != -1 && g_startpts != g_endpts)
			bytes_per_second = filesize / tmp;
		INFO("PTS at file end:   %ld, file len: %ld, bps: %lld\n", g_endpts, tmp, bytes_per_second);
	}
	else
		INFO("file is too short to determine PTS at file end\n");

	ringbuffer_reset(buf_in);
	mf_lseek(0);

	while (g_playstate != CMoviePlayerGui::STOPPED)
	{
#ifdef AUDIO_STREAM_AUTOSELECT
		/* Try to find the lowest numbered audio stream. The "bufferfilled" check
		   is to make sure that there is already some data parsed. */
		if (bufferfilled && g_currentapid == -1)
			for (int j = 0; j < MAX_APIDS; j++)
				if (found_aid[j])
				{
					INFO("g_currentapid set to: %d\n", j);
					g_currentapid = j;
					break;
				}
#endif
		time_t now;

		switch(g_playstate)
		{
		case CMoviePlayerGui::SKIP:
			INFO("lseek from %lld, seconds %d\n", filepos, skipseconds);
			if (!(g_startpts == -1 && skipabsolute)) // only jump absolute if startpts is known
			{
				int s = skipseconds;
				if (skipabsolute)
					s = skipseconds - get_filetime();
				filepos += (bytes_per_second * s);
				// smooth = 0; // reset the bitrate smoothing counter after seek?
				last = 0;
				bufferingBox->paint();

				if (filepos >= filesize)
				{
					filepos = filesize - 30 * bytes_per_second;
					skipabsolute = false;
				}
				if (filepos < 0)
					filepos = 0;
				if (mf_lseek(filepos) < 0)
					perror("ReadMPEGFileThread lseek");
			}
			DBG("BUFFERRESET!\n");
			while (!bufferreset && g_playstate != CMoviePlayerGui::STOPPED)
				usleep(100000);
			bufferfilled = false;
			ringbuffer_reset(ringbuf);
			ringbuffer_reset(buf_in);
			bufferreset = false;
			DBG("BUFFERRESET done.\n");
			/* OutputThread() sets g_playstate to SOFTRESET */
			while (g_playstate == CMoviePlayerGui::SKIP)
				usleep(100000);
			DBG("skip ends\n");
			break;
		case CMoviePlayerGui::PLAY:
			if (g_startpts != -1 && g_endpts != -1)
				break;	// we already have pretty correct bytes_per_second
			if (last == 0)
			{
				last = time(NULL);
				lastpts = g_pts;
				lastpos = ptspos;
				break;
			}
			now = time(NULL);
			if ((now - last) > 4)
			{
				int diff_pts = g_pts - lastpts;
				off_t diff_pos = ptspos - lastpos;
				off_t diff_bps = diff_pos * 1000 / diff_pts;
				lastpos = ptspos;
				lastpts = g_pts;
				// printf("bytes p/s old: %lld", bytes_per_second);
				if (diff_bps > 0 && diff_pos > 0) // discontinuity, startup...
				{
					if (smooth < 8)
						smooth++;
					bytes_per_second = (smooth * bytes_per_second + diff_bps) / (smooth + 1);
				}
				// else printf(" not updated");
				// printf(" new: %lld, diff PTS:%5d diff_pos %lld, smooth %d %d%%\n",
				//	 bytes_per_second, diff_pts, diff_pos, smooth, g_percent);
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
					g_input_failed = true;
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
				g_input_failed = true;
				break;
			}

			if (len <= 0) // len < 0 => error, len == 0 => EOF
			{
				if (len < 0)
					perror("ReadMPEGFileThread read");
				INFO("error or EOF => exiting\n");
				g_input_failed = true;
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
		if ((size = vec_in[0].len) != 0 && !g_EOF)
		{
			len = read(g_fd, vec_in[0].buf, size);
			if (len > 0)
				ringbuffer_write_advance(buf_in, len);
			else if (len < 0)
			{
				perror("ReadMPEGFileThread read");
				g_input_failed = true;
				break;
			}
			DBG("read %d bytes, size %d\n", len, size);
			filepos += len;
			if (len && input_empty)
				continue;
			if (!len)
			{
				if (mf_lseek(filepos) >=0 )
					continue;
				g_EOF = true;
			}
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
		rd = ringbuffer_get_readpointer(buf_in, &ppes, 10); // we need 10 bytes for AC3
		if (rd < 10)
		{
			INFO("rd:%ld, EOF: %s\n", rd, g_EOF ? "true" : "false");
			usleep(300000);
			if (g_EOF)
				break;
			continue;
		}

		int r = mp_syncPES(buf_in, &ppes);
		if (r < 0)
			continue;
		bool resync = (r > 0);
		// if (resync) INFO("after mp_syncPES, r=%d resync=%d\n",r, resync);

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
			case 0xbd: // AC3
			{
				//skip = (ppes[4] << 8 | ppes[5]) + 6;
				unsigned int off = ppes[8] + 8 + 1; // ppes[8] is often 0
				if (off >= 10) // we did only make sure for 10 bytes
				{
					rd = ringbuffer_get_readpointer(buf_in, &ppes, off);
					if (rd < off) // not enough space in ringbuf;
						continue;
				}
				int subid = ppes[off];
				// if (offset == 0x24 && subid == 0x10 ) // TTX?
				if (subid < 0x80 || subid > 0x87)
					break;
				DBG("AC3: ofs 0x%02x subid 0x%02x\n", off, subid);
				subid -= 0x60; // normalize to 32...39 (hex 0x20..0x27)
#if 0
This is commented out so we never start up with the AC3 audio selected.
TODO: OTOH, if we only have an AC3 stream there will be no sound.
				if (!g_currentapid)
				{
					g_currentapid = ppes[3];
					INFO("found AC3 aid: %02x\n", g_currentapid);
					if (g_numpida < 10)
					{
						g_ac3flags[g_numpida] = 1;
						g_apids[g_numpida++] = ppes[3];
					}
					found_aid[g_currentapid] = 1;
				}
				else
#endif
				if (g_currentapid != subid)
				{
					if (!found_aid[subid] && !resync) //only if we are in sync...
					{
						if (g_numpida < MAX_APIDS)
						{
							g_ac3flags[g_numpida] = 1;
							g_apids[g_numpida++] = subid;
						}
						INFO("found AC3 aid:  %02x\n", subid);
						found_aid[subid] = 1;
					}
					av = 0; // skip over this stream;
					break;
				}
				pid = 101;
				cc = &acc;
				//g_currentac3 = 1;
				av = 2;
				break;
			}
			case 0xbb:
			case 0xbe:
			case 0xbf:
			case 0xf0 ... 0xf3:
			case 0xff:
				//skip = (ppes[4] << 8 | ppes[5]) + 6;
				//DBG("0x%02x header, skip = %d\n", ppes[3], skip);
				break;
			case 0xc0 ... 0xcf:
			case 0xd0 ... 0xdf:
			{
				// fprintf(stderr, "audio stream 0x%02x\n", ppes[3]);
				int id = ppes[3] - 0xC0; // normalize to 0...31 (hex 0x0...0x1f)
#if 0
				if (g_currentapid == -1)
				{
					g_currentapid = id;
					INFO("found aid: %02x\n", g_currentapid);
					if (g_numpida < MAX_APIDS);
						g_apids[g_numpida++] = id;
					found_aid[g_currentapid] = 1;
				}
				else
#endif
				if (g_currentapid != id)
				{
					if (!found_aid[id] && !resync) //only if we are in sync...
					{
						if (g_numpida < MAX_APIDS)
							g_apids[g_numpida++] = id;
						INFO("found aid: %02x\n", id);
						found_aid[id] = 1;
					}
					av = 0; // skip over this stream;
					break;
				}
				pid = 101;
				cc = &acc;
				av = 2;
				break;
			}
			case 0xe0 ... 0xef:
				// fprintf(stderr, "video stream 0x%02x, %02x %02x \n", ppes[3], ppes[4], ppes[5]);
				pid = 100;
				cc = &vcc;
				av = 1;
				break;
			case 0xb9:
			case 0xbc:
				DBG("%s\n", (ppes[3] == 0xb9) ? "program_end_code" : "program_stream_map");
				resync = true;
				// fallthrough. TODO: implement properly.
			default:
				if (! resync)
					DBG("Unknown stream id: 0x%X.\n", ppes[3]);
				ringbuffer_read_advance(buf_in, 1); // remove 1 Byte
				goto again;
				break;
		}

		pesPacketLen = ((ppes[4] << 8) | ppes[5]) + 6;
		if (ringbuffer_read_space(buf_in) < pesPacketLen)
		{
			INFO("ringbuffer: %ld, pesPacketLen: %d :-(\n", ringbuffer_read_space(buf_in), pesPacketLen);
			continue;
		}

		if (av)
		{
			static time_t gotpts = 0;
			rd = ringbuffer_get_readpointer(buf_in, &ppes, pesPacketLen);
			// we already checked for enough space above...

			if (av == 1 && ((i++ %10) == 0 || time(NULL) > gotpts)) // only for every tenth video packet or once per second
			{
				int pts = get_pts(ppes, true);
				if (pts != -1)
				{
					gotpts = time(NULL);
					g_pts = pts;
					ptspos = filepos; // not exact: disregards the bytes in the buffer!
					if (g_startpts == -1) // only works if we are starting from the start of the file
					{
						g_startpts = pts;
						INFO("startpts = %ld\n", g_startpts);
					}
					if (skipabsolute && !g_skiprequest)
					{
						time_t timediff = skipseconds - get_filetime();
						if (skipretry++ < 10 && abs(timediff) > 10)
						{
							INFO("offset > 10 seconds (%ld), retry skipping...(%d)\n", timediff, skipretry);
							g_skiprequest = true;
						}
						else
						{
							// more than 10 retries -> get out
							skipabsolute = false;
							skipretry = 0;
						}
					}
				}
			}

			tsPacksCount = pesPacketLen / 184;
			rest = pesPacketLen % 184;

			// divide PES packet into small TS packets
			bool first = true;
			int j;
			for (j = 0; j < tsPacksCount; j++) 
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
				ringbuffer_write(ringbuf, ppes + j * 184, 184);
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
				ringbuffer_write(ringbuf, ppes + j * 184, rest);
			}
		} //if (av)

		ringbuffer_read_advance(buf_in, pesPacketLen);

		if (filesize)
			g_percent = filepos * 100 / filesize;	// overflow? not with 64 bits...
	}
	mf_close();
	ringbuffer_free(buf_in);
	INFO("ends now.\n");
	pthread_exit(NULL);
} // ReadMPEGFileThread()



//------------------------------------------------------------------------
void *
OutputThread(void *arg)
{
	g_output_thread = true;
	//-- lcd stuff --
	int last_percent = -1;
	struct dmx_pes_filter_params p;
	ssize_t wr;
	char buf[MAXREADSIZE];
	bool failed = false;
	bool remote = true;
	// use global pida and pidv
	pida = 0, pidv = 0, g_currentac3 = 0;
	int ret, done;
	CFileList *f = (CFileList *)arg;
	const char *fn = (*f)[0].Name.c_str();
	int dmxa = -1 , dmxv = -1, dvr = -1, adec = -1, vdec = -1;
	if (fn[0] == '/')
		remote = false;	// we are playing a "local" file (hdd or NFS)

	if (strlen(fn) > 5 &&
	    strcmp(fn + strlen(fn) - 5, ".dbox") == 0)
	{
		INFO("dbox2dbox playlist file found\n");
		remote = true;
	}

	ringbuf = ringbuffer_create(RINGBUFFERSIZE);
	if (ringbuf)
		INFO("ringbuffer (size %ld) created\n", ringbuffer_write_space(ringbuf));
	else
	{
		INFO("ringbuffer_create failed!\n");
		failed = true;
	}

	bufferingBox = new CHintBox(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_MOVIEPLAYER_BUFFERING));	// UTF-8

	INFO("mrl:%s\n", fn);

	g_input_failed = false; // there is no input thread running now... hopefully.
	pthread_t rcvt;	// the input / "receive" thread
	if (remote)
		ret = pthread_create(&rcvt, NULL, ReceiveStreamThread, (void *)fn);
	else
	{
		std::string tmp = fn;
		if (tmp.rfind(".ts") == tmp.size()-3)
		{
			INFO("found TS file\n");
			isPES = false;
			ret = pthread_create(&rcvt, NULL, ReadTSFileThread, arg);
		}
		else
		{
			INFO("found non-TS file, hoping for MPEG\n");
			isPES = true;
			ret = pthread_create(&rcvt, NULL, ReadMPEGFileThread, arg);
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
	bool driverready = false;
	size_t readsize, len;
	bool init = true;
	checkAspectRatio(vdec, true);
	int empty_counter = 0;

	while (g_playstate > CMoviePlayerGui::STOPPED && !failed)
	{
		if (!driverready && bufferfilled && g_currentapid != -1)
		{
			driverready = true;
			// pida and pidv should have been set by ReceiveStreamThread now
			INFO("while streaming found pida: 0x%04X ; pidv: 0x%04X ac3: %d\n",
			      pida, pidv, g_currentac3);
			CLCD::getInstance()->setMovieAudio(g_currentac3);

			g_playstate = CMoviePlayerGui::SOFTRESET;
		}

		if (g_startposition > 0 && g_startpts != - 1)
		{
			printf("[%s] Was Bookmark. Skipping to startposition %d %d\n", __FILE__, g_startposition, remote);
			skip(g_startposition, remote, true);
			g_startposition = 0;
		}

		if (g_apidchanged)
		{
			if (!isPES)
			{
				// TS: need to reset dmx...
				INFO("APID changed from 0x%04x to 0x%04x\n", pida, g_currentapid);
				pida = g_currentapid;
				g_playstate = CMoviePlayerGui::SOFTRESET;
			}
			else if (g_ac3changed)
			{
				// PES/PS: need to enable/disable AC3 passthrough...
				g_playstate = CMoviePlayerGui::SOFTRESET;
			}
			g_ac3changed = false;
			g_apidchanged = false;
		}

		if (g_input_failed)
		{
			INFO("g_input_failed == true!\n");
			g_playstate = CMoviePlayerGui::STOPPED;

			if (!bufferfilled)
				bufferingBox->hide();
		}

		switch (g_playstate)
		{
			case CMoviePlayerGui::PAUSE:
				ioctl(vdec, VIDEO_FREEZE);
#if HAVE_DVB_API_VERSION >= 3
				ioctl(adec, AUDIO_STOP);
				ioctl(dmxa, DMX_STOP);
#else
				if (ioctl(adec, AUDIO_SET_AV_SYNC, 0UL))
					perror("AUDIO_SET_AV_SYNC");
				if (ioctl(adec, AUDIO_SET_MUTE, 1))
					perror("AUDIO_SET_MUTE");
#endif
				while (g_playstate == CMoviePlayerGui::PAUSE)
					usleep(100000); // no busy wait
#if HAVE_DVB_API_VERSION >= 3
				ioctl(vdec, VIDEO_STOP);
				ioctl(dmxv, DMX_STOP);
				ioctl(vdec, VIDEO_PLAY);
				ioctl(adec, AUDIO_PLAY);
				ioctl(dmxv, DMX_START);
				ioctl(dmxa, DMX_START);
#else
				ioctl(vdec, VIDEO_CONTINUE);
				if (ioctl(adec, AUDIO_SET_AV_SYNC, 1UL))
					perror("AUDIO_SET_AV_SYNC");
				if (ioctl(adec, AUDIO_SET_MUTE, 0))
					perror("AUDIO_SET_MUTE");
#endif
				break;
			case CMoviePlayerGui::SKIP:
				DBG("requesting buffer reset\n");
				bufferreset = true;
				while (bufferreset && g_playstate != CMoviePlayerGui::STOPPED)
				{
					DBG("WAITING FOR BUFFERRESET\n");
					usleep(250000);
				}
				if (g_playstate == CMoviePlayerGui::SKIP) // might be STOPPED now
					g_playstate = CMoviePlayerGui::SOFTRESET;
				DBG("skipping end\n");
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
				if (g_playstate == CMoviePlayerGui::RESYNC) // might be STOPPED now
					g_playstate = CMoviePlayerGui::PLAY;
				break;
			case CMoviePlayerGui::PLAY:
			{
				if (!bufferfilled || !driverready)
				{
					usleep(10000);	// non busy wait
					continue;
				}

				len = 0;
				readsize = ringbuffer_read_space(ringbuf);
				//fprintf(stderr, "XX readsize = %d MAX %d\n", readsize, MAXREADSIZE);
				if (readsize > MAXREADSIZE)
					readsize = MAXREADSIZE;
				else if (readsize % 188)
					readsize = (readsize / 188) * 188;

				len = ringbuffer_read(ringbuf, buf, readsize); // readsize is n*188
				if (len < MINREADSIZE)
				{
					//INFO("len (%d/%d) < MINREAD, empty_counter = %d\n",len, readsize, empty_counter);
					if (g_EOF) // end of file and buffer empty...
					{
						INFO("BUFFER EMPTY AND EOF!\n");
						g_playstate = CMoviePlayerGui::STOPPED;
					}
					if (empty_counter++ < 5)
					{
						/* The ringbuffer might be empty, but video still playing fine,
						   so we start "buffering..." only after a few consecutive "ringbuffer
						   empty" events. The value might need to be adjusted to the hardware,
						   "5" works fine on the dm500 */
						usleep(100000);
						break;
					}
					ioctl(dmxa, DMX_STOP);
					INFO("len: %ld, buffering...\n", len);
					/*
					 * always call bufferingBox->paint() before setting bufferfilled to false
					 * to ensure that it is painted completely before bufferingBox->hide()
					 * might be called by ReceiveStreamThread (otherwise the hintbox's window
					 * variable is deleted while being used)
					 */
					bufferingBox->paint();
					bufferfilled = false;
					while (g_playstate == CMoviePlayerGui::PLAY && !bufferfilled && !g_input_failed)
						usleep(125000);
					INFO("...buffering ends\n");
					ioctl(dmxa, DMX_START);
				}
				else
					empty_counter = 0;
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
						perror("[movieplayer.cpp] OutputThread write");
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
				if (last_percent != g_percent)
				{
					last_percent = g_percent;
					CLCD::getInstance()->showPercentOver(last_percent, true, CLCD::MODE_MOVIE);
				}
				break;
			}
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
				if (g_playstate != CMoviePlayerGui::STOPPED) // might be changed now
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

		if (g_skiprequest && g_playstate != CMoviePlayerGui::STOPPED)
		{
			DBG("g_skiprequest == true!\n");
			g_playstate = CMoviePlayerGui::SKIP;
			g_skiprequest = false;
		}
	}

	ioctl(vdec, VIDEO_STOP);
	ioctl(adec, AUDIO_STOP);
	ioctl(dmxv, DMX_STOP);
	ioctl(dmxa, DMX_STOP);
	if (dmxa > -1)
		close(dmxa);
	if (dmxv > -1)
		close(dmxv);
	if (dvr > -1)
		close(dvr);
	if (adec > -1)
		close(adec);
	if (vdec > -1)
		close(vdec);

	// request reader termination
	g_playstate = CMoviePlayerGui::STOPPED;

	INFO("Waiting for input thread to stop\n");
	pthread_join(rcvt, NULL);
	DBG("Seems that input thread was stopped succesfully\n");

	// Some memory clean up
	ringbuffer_free(ringbuf);
	delete bufferingBox;

	INFO("ends here.\n");
	pthread_exit(NULL);
} // OutputThread

//== updateLcd ==
//===============
void updateLcd(const std::string &s)
{
	static int  l_playstate = -1;
	std::string lcd = s;
	CLCD::AUDIOMODES playmode;

	if (l_playstate == g_playstate)
		return;

	switch (g_playstate)
	{
	case CMoviePlayerGui::PAUSE:
		playmode = CLCD::AUDIO_MODE_PAUSE;
		break;
	default:
		playmode = CLCD::AUDIO_MODE_PLAY;
		break;
	}
	StrSearchReplace(lcd,"_", " ");
	CLCD::getInstance()->setMovieInfo(playmode, "", lcd, false);
}

//== seek to pos with sync to next proper TS packet ==
//== returns offset to start of TS packet or actual ==
//== pos on failure.                                ==
//====================================================
#define SIZE_PROBE  (100 * 188)

static off_t mp_seekSync(off_t pos)
{
	off_t npos = pos;
	off_t ret;
	uint8_t pkt[188];

	ret = mf_lseek(npos);
	if (ret < 0)
		INFO("lseek ret < 0 (%m)\n");

	while (read(g_fd, pkt, 1) > 0)
	{
		//-- check every byte until sync word reached --
		npos++;
		if (*pkt == 0x47)
		{
			//-- if found double check for next sync word --
			if (read(g_fd, pkt, 188) == 188)
			{
				if(pkt[188-1] == 0x47)
				{
					ret = mf_lseek(npos - 1); // assume sync ok
					if (ret < 0)
						INFO("lseek ret < 0 (%m)\n");
					return ret;
				}
				else
				{
					ret = mf_lseek(npos); // oops, next pkt doesn't start with sync
					if (ret < 0)
						INFO("lseek ret < 0 (%m)\n");
				}
			}
		}

		//-- check probe limits --
		if (npos > (pos + SIZE_PROBE))
			break;
	}

	//-- on error stay on actual position --
	return mf_lseek(pos);
}

/* returns: 0 == was already synchronous, > 0 == is now synchronous, -1 == could not sync */
static int mp_syncPES(ringbuffer_t *ring, char **pes)
{
	int rd = 10;
	int ret = 0;
	do {
		if ((*pes)[0] == 0x00 && (*pes)[1] == 0x00 && (*pes)[2] == 0x01)
			return ret;
		//INFO("%02x %02x %02x\n", (*pes)[0], (*pes)[1], (*pes)[2]);
		ringbuffer_read_advance(ring, 1); // remove 1 Byte
		rd = ringbuffer_get_readpointer(ring, &(*pes), 10);
		ret++;
	}
	while (rd == 10);

	INFO("No valid PES signature found. %d Bytes deleted.\n", ret);
	return -1;
}

/* the mf_* functions are wrappers for multiple-file I/O */
int mf_open(int fileno)
{
	if (g_f == NULL)
		return -1;

	mf_close();

	g_fd = open((*g_f)[fileno].Name.c_str(), O_RDONLY);
	if (g_fd != -1)
		g_fileno = fileno;

	return g_fd;
}

int mf_close(void)
{
	int ret = 0;

	if (g_fd != -1)
		ret = close(g_fd);
	g_fd = g_fileno = -1;

	return ret;
}

off_t mf_getsize(void)
{
	off_t ret = 0;

	g_numfiles = (*g_f).size();

	for (int i = 0; i < g_numfiles; i++)
		ret += (*g_f)[i].Size;

	return ret;
}

off_t mf_lseek(off_t pos)
{
	off_t offset = 0, lpos = pos, ret;
	int fileno;
	for (fileno = 0; fileno < (int)(*g_f).size(); fileno++)
	{
		if (lpos < (*g_f)[fileno].Size)
			break;
		offset += (*g_f)[fileno].Size;
		lpos   -= (*g_f)[fileno].Size;
	}
	if (fileno == (int)(*g_f).size())
		return -2;	// EOF

	if (fileno != g_fileno)
	{
		INFO("old fileno: %d new fileno: %d, offset: %lld\n", g_fileno, fileno, lpos);
		g_fd = mf_open(fileno);
		if (g_fd < 0)
		{
			INFO("cannot open file %d:%s (%m)\n", fileno, (*g_f)[fileno].Name.c_str());
			return -1;
		}
	}

	ret = lseek(g_fd, lpos, SEEK_SET);
	if (ret < 0)
		return ret;

	return offset + ret;
}

//=======================================
//== CMoviePlayerGui::ParentalEntrance ==
//=======================================
void CMoviePlayerGui::ParentalEntrance(void)
{
	CZapProtection pin(g_settings.parentallock_pincode, 18);
	if(pin.check())
	{
		startfilename = "";
		PlayFile(1);
	}
}

//===============================
//== CMoviePlayerGui::PlayFile ==
//===============================
void
CMoviePlayerGui::PlayFile(int parental)
{
	/* there's a lastParental variable in the header file, but i
	   apparently can't use it since it's "private static" */
	static int last_parental = -1;
	if(parental != last_parental)
	{
		INFO("setting parental to (%d)\n", parental);
		last_parental = parental;

		std::string hlpstr = "/var/bin/parental.sh";
		if (parental == 1)
			hlpstr += " 1";
		else
			hlpstr += " 0";
		system(hlpstr.c_str());
	}

	PlayStream(STREAMTYPE_LOCAL);
	return;
}

/*
 CMoviePlayerGui::PlayStream()
 This is actually the "Control" or "GUI"-Thread
*/
void
CMoviePlayerGui::PlayStream(int streamtype)
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	static std::string Path = Path_vlc;
	std::string sel_filename, title;
	bool update_info = true, start_play = false, exit = false;
	bool open_filebrowser = true, cdDvd = false, aborted = false;
	bool stream = true, from_mb = false;
	std::string mrl_str;
	int selected = 0;
	CTimeOSD StreamTime;
	CFileList filelist;
	CFileList tmpfilelist;
	MI_MOVIE_INFO movieinfo;
	bool movieinfo_valid = false;
	bool autoplaylist = false; // auto-playlist of foo.001.ts, foo.002.ts,... or 001.vdr, 002.vdr,...
	int last_apid = -1; // for auto-playlists...
	hintBox = new CHintBox(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_MOVIEPLAYER_PLEASEWAIT)); // UTF-8

	if (streamtype == STREAMTYPE_DVD)
	{
		mrl_str = "dvd://";
		mrl_str += g_settings.streaming_server_cddrive;
		mrl_str += "@1";
		INFO("Generated MRL: %s\n", mrl_str.c_str());
		CFile file;
		file.Name = mrl_str;
		filelist.push_back(file);
		sel_filename = "DVD";
		open_filebrowser = false;
		start_play = true;
		cdDvd = true;
	}
	else if (streamtype == STREAMTYPE_SVCD)
	{
		mrl_str = "vcd:";
		mrl_str += g_settings.streaming_server_cddrive;
		INFO("Generated MRL: %s\n", mrl_str.c_str());
		CFile file;
		file.Name = mrl_str;
		filelist.push_back(file);
		sel_filename = "(S)VCD";
		open_filebrowser = false;
		start_play = true;
		cdDvd = true;
	}
	else if (streamtype == STREAMTYPE_FILE) // "file via VLC"
	{
		if (Path.find("vlc://") != 0)
		{
			INFO("old path was not vlc, setting to vlc\n");
			Path = Path_vlc;
		}
	}
	else if (streamtype == STREAMTYPE_LOCAL)
	{
		INFO("STREAMTYPE_LOCAL '%s'\n", startfilename.c_str());
		if (startfilename != "")
		{
			CFile file;
			struct stat s;

			filename = startfilename.c_str();
			if (stat(filename, &s)) // file not exist
			{
				INFO("file not found: %s\n", filename);
				exit = true;
			}
			else
			{
				file.Name = startfilename;
				file.Size = s.st_size;
				filelist.clear();
				filelist.push_back(file);
				autoplaylist = filelist_auto_add(filelist);
				sel_filename = startfilename;
				start_play = true;
				update_info = true;
				from_mb = true;
				open_filebrowser = false;
			}
		}
		else if (Path.find("vlc://") == 0)
		{
			INFO("old path was vlc, setting to local\n");
			Path = Path_local;
		}
		stream = false;
	}
	else
		INFO("strange streamtype? %d\n", streamtype);

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
		if (g_playstate == CMoviePlayerGui::STOPPED && !cdDvd && !from_mb)
		{
			if (selected + 1 < (int)filelist.size() && !aborted && !autoplaylist)
			{
				last_apid = g_currentapid;
				selected++;
				filename = filelist[selected].Name.c_str();
				sel_filename = filelist[selected].getFileName();
				//printf ("[movieplayer.cpp] sel_filename: %s\n", filename);
				if (stream)
				{
					int namepos = filelist[selected].Name.rfind("vlc://");
					mrl_str = filelist[selected].Name.substr(namepos + 6);
					mrl_str = url_escape(mrl_str.c_str());
				}
				else
					mrl_str = filelist[selected].Name;
				INFO ("Generated FILE MRL: %s\n", mrl_str.c_str());

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
			mrl_str = startfilename.substr(namepos + 6);
			mrl_str = url_escape(mrl_str.c_str());
			INFO("Generated Bookmark FILE MRL: %s\n", mrl_str.c_str());
			namepos = startfilename.rfind("/");
			sel_filename = startfilename.substr(namepos + 1);
			update_info = true;
			start_play = true;
		}

		if (open_filebrowser && !cdDvd && !from_mb)
		{
			g_playstate = CMoviePlayerGui::STOPPED;
			if (g_settings.streaming_show_tv_in_browser == true &&
			    g_ZapitsetStandbyState == true)
			{
				if (g_output_thread) // the output thread is using the devices
				{
					INFO("waiting for output thread to terminate...\n");
					pthread_join(rct, NULL);
					g_output_thread = false;
					INFO("done\n");
				}
				g_Zapit->setStandby(false);
				g_ZapitsetStandbyState = false;
			}
			open_filebrowser = false;
			filename = NULL;
			if (stream)
				filebrowser->Filter = &vlcfilefilter;
			else
			{
				filebrowser->Filter = &tsfilefilter;
				// TODO: this is probably better fixed in the filebrowser class
				if (access(Path.c_str(), R_OK)) // Path does not (no longer?) exist
					Path = Path_local;
			}

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
					/* ugly hack */
					if (filelist.size() == 1)
					{
						if (filelist[0].getFileName() == "info.vdr" ||
						    filelist[0].getFileName() == "index.vdr")
						{
							std::string fn = filelist[0].getPath() + "001.vdr";
							struct stat s;
							if (!stat(fn.c_str(), &s)) // file does exist
							{
								filelist[0].Name = fn;
								filelist[0].Size = s.st_size;
							}
						}
					}
					filename = filelist[0].Name.c_str();
					sel_filename = filelist[0].getFileName();
					INFO("sel_filename: %s\n", filename);
					if (stream)
					{
						int namepos = filelist[0].Name.rfind("vlc://");
						mrl_str = filelist[0].Name.substr(namepos + 6);
						mrl_str = url_escape(mrl_str.c_str());
					}
					else
						mrl_str = filelist[0].Name;
					INFO("Generated FILE MRL: %s\n", mrl_str.c_str());

					if (stream)
						autoplaylist = false;
					else
						autoplaylist = filelist_auto_add(filelist);

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
			CMovieInfo mi;
			mi.clearMovieInfo(&movieinfo);
			title = sel_filename;
			if (!cdDvd)
			{
				movieinfo.file.Name = filename;
				movieinfo_valid = mi.loadMovieInfo(&movieinfo);
				if (movieinfo_valid)
					title = movieinfo.epgTitle;
			}
			update_info = false;
			updateLcd(title);
		}

		if (start_play)
		{
			CFileList *outArg;
			if (g_settings.streaming_show_tv_in_browser == true &&
			    g_ZapitsetStandbyState == false)
			{
				g_Zapit->setStandby(true);
				g_ZapitsetStandbyState = true;
			}
			start_play = false;
			bufferfilled = false;

			if (g_output_thread) // there is an output thread still running...
			{
				g_playstate = CMoviePlayerGui::STOPPED;
				INFO("waiting for output thread...\n");
				pthread_join(rct, NULL);
				g_output_thread = false;
				INFO("done\n");
			}
			//TODO: Add Dialog (Remove Dialog later)
			hintBox->paint();
			buffer_time=0;
			if (autoplaylist || cdDvd)
				outArg = &filelist;
			else
			{
				tmpfilelist.clear();
				tmpfilelist.push_back(filelist[selected]);
				outArg = &tmpfilelist;
			}
			if (pthread_create(&rct, 0, OutputThread, (void *)outArg) != 0)
				break;
			g_playstate = CMoviePlayerGui::SOFTRESET;
		}

#ifndef AUDIO_STREAM_AUTOSELECT
		if (bufferfilled && g_currentapid == -1)
		{
			if (g_numpida == 1)	// if there is only one audio stream...
			{			// ...we can as well just use that.
				g_currentapid = g_apids[0];
				g_currentac3 = g_ac3flags[0];
				g_apidchanged = true;
				CLCD::getInstance()->setMovieAudio(g_currentac3);
			}
			else if (last_apid != -1)
			{
				// check if the last apid is still present...
				for (int i = 0; i < g_numpida; i++)
				{
					if (g_apids[i] == last_apid)
					{
						g_currentapid = g_apids[i];
						g_currentac3 = g_ac3flags[i];
						g_apidchanged = true;
						CLCD::getInstance()->setMovieAudio(g_currentac3);
						break;
					}
				}
			}
			if (g_currentapid == -1) // not automatically determined -> ask user
				g_showaudioselectdialog = true;
		}
#endif

		if (g_showaudioselectdialog)
		{
			CMenuWidget APIDSelector(LOCALE_APIDSELECTOR_HEAD, "audio.raw", 400);
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

				std::string apidtitle = "";
				bool mi_found = false, current = false;
				sprintf(apidnumber, "%d", count+1);
				sprintf(show_pid_number, "%u", g_apids[count]);

				if (movieinfo_valid)
				{
					for (unsigned int i = 0; i < movieinfo.audioPids.size(); i++)
					{
						if (movieinfo.audioPids[i].epgAudioPid == g_apids[count])
						{
							apidtitle.append(movieinfo.audioPids[i].epgAudioPidName);
							mi_found = true;
							break;
						}
					}
				}
				if (!mi_found)
					apidtitle = "Stream ";

				apidtitle.append(" [");
				apidtitle.append(show_pid_number);
				apidtitle.append("]");

				if (g_currentapid == -1 && g_apids[count] == 0)
					current = true;
				if (g_apids[count] == g_currentapid)
				{
					current = true;
					apidtitle.append(" *"); // current stream.
				}
				APIDSelector.addItem(
					new CMenuForwarderNonLocalized(apidtitle.c_str(), true,
						NULL, APIDChanger, apidnumber,
						CRCInput::convertDigitToKey(count+1)),
					current); // select current stream
			}

			// then show the other audio pids (AC3/teletex)
			for (unsigned int count = 0; count < g_numpida; count++)
			{
				if (g_ac3flags[count] == 0) // already handled...
					continue;
				std::string apidtitle = "";
				bool mi_found = false, current = false;
				sprintf(apidnumber, "%d", count+1);
				sprintf(show_pid_number, "%u", g_apids[count]);

				if (movieinfo_valid)
				{
					for (unsigned int i = 0; i < movieinfo.audioPids.size(); i++)
					{
						if (movieinfo.audioPids[i].epgAudioPid == g_apids[count])
						{
							apidtitle.append(movieinfo.audioPids[i].epgAudioPidName);
							mi_found = true;
							break;
						}
					}
				}
				if (!mi_found)
					apidtitle = "Stream ";

				apidtitle.append(" [");
				apidtitle.append(show_pid_number);
				apidtitle.append("]");

				if (g_ac3flags[count] == 1)
					apidtitle.append(" (AC3)");
				if (g_ac3flags[count] == 2)
				{
					apidtitle.append(" (Teletext)");
					pidt = g_apids[count];
				}

				if (g_currentapid == -1 && g_apids[count] == 0)
					current = true;
				if (g_apids[count] == g_currentapid)
				{
					current = true;
					apidtitle.append(" *"); // current stream.
				}
				APIDSelector.addItem(
					new CMenuForwarderNonLocalized(apidtitle.c_str(), true,
						NULL, APIDChanger, apidnumber,
						CRCInput::convertDigitToKey(count+1)),
					current); // select current stream
			}
			APIDSelector.exec(NULL, ""); // otherwise use Dialog
			delete APIDChanger;
			if (g_currentapid == -1) // exit if inital pid is not selected
				g_playstate = CMoviePlayerGui::STOPPED;
			g_showaudioselectdialog = false;
			CLCD::getInstance()->setMovieAudio(g_currentac3);
			updateLcd(title);
		}

		g_RCInput->getMsg(&msg, &data, 10);	// 1 secs..
		DBG("msg: 0x%08x\n", msg);

		if (StreamTime.IsVisible())
		{
			if (stream)
				StreamTime.update();
			else
				StreamTime.show(get_filetime(StreamTime.GetMode() == CTimeOSD::MODE_DESC));
		}

		if (msg == CRCInput::RC_green)
			g_showaudioselectdialog = true;
		else if (msg == CRCInput::RC_home)
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
			g_playstate = (g_playstate == CMoviePlayerGui::PAUSE) ? CMoviePlayerGui::PLAY : CMoviePlayerGui::PAUSE;
			if (stream) // stream time is only counting seconds...
				StreamTime.hide();
			g_RCInput->postMsg(NeutrinoMessages::SHOW_INFOBAR, data);
		}
		else if (msg == CRCInput::RC_red)
		{
			if (g_playstate == CMoviePlayerGui::PLAY)
				g_playstate = CMoviePlayerGui::SOFTRESET;
				// g_playstate = CMoviePlayerGui::RESYNC;
			if (stream)
				StreamTime.hide();
		}
		else if (msg == CRCInput::RC_blue)
		{
			if (bookmarkmanager->getBookmarkCount() < bookmarkmanager->getMaxBookmarkCount())
			{
				if (stream)
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
			char tmp[10 + 1];
			bool cancel;

			CTimeInput ti(LOCALE_MOVIEPLAYER_GOTO, tmp, LOCALE_MOVIEPLAYER_GOTO_H1, LOCALE_MOVIEPLAYER_GOTO_H2, NULL, &cancel);
			ti.exec(NULL, "");
			if (!cancel)	// no cancel
			{
				struct tm t;
				bool absolute = (tmp[0] == '=');

				if (strptime(tmp + 1, "%T", &t) != NULL)
				{
					time_t seconds = t.tm_sec + 60 * t.tm_min + 3600 * t.tm_hour;
					if (tmp[0] == '-')
						seconds = -seconds;
					skip(seconds, stream, absolute);
				}
			}
		}
		else if (msg == CRCInput::RC_setup)
		{
			if (StreamTime.IsVisible())
			{
				if (StreamTime.GetMode() == CTimeOSD::MODE_ASC)
				{
					if (stream)
					{
						int stream_length = VlcGetStreamLength();
						int stream_time = VlcGetStreamTime();
						if (stream_time >= 0 && stream_length >= 0)
						{
							StreamTime.SetMode(CTimeOSD::MODE_DESC);
							StreamTime.show(stream_length - stream_time + buffer_time);
						}
					}
					else if (g_endpts != -1)
					{
						StreamTime.SetMode(CTimeOSD::MODE_DESC);
						StreamTime.show(get_filetime(true));
					}
					else
						StreamTime.hide();
				}
				else
					StreamTime.hide();
			}
			else
			{
				if (stream)
				{
					int stream_time;
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
		else if (msg == CRCInput::RC_help || msg == NeutrinoMessages::SHOW_INFOBAR)
		{
			std::string sub_title = "";
			if (movieinfo_valid)
			{
				sub_title = movieinfo.epgInfo1;
				if (sub_title.empty())
					sub_title = sel_filename;
			}

			int elapsed_time, remaining_time;
			if (stream)
			{
				elapsed_time = VlcGetStreamTime() - buffer_time;
				remaining_time = VlcGetStreamLength() - elapsed_time;
			}
			else
			{
				elapsed_time = get_filetime();
				remaining_time = get_filetime(true);
			}

			int ac3state = CInfoViewer::NO_AC3;
			if (g_currentac3)
				ac3state = CInfoViewer::AC3_ACTIVE;
			else
			{
				for (int i = 0; i < g_numpida; i++)
					if (g_ac3flags[i] == 1) // AC3
					{
						ac3state = CInfoViewer::AC3_AVAILABLE;
						break;
					}
			}

			g_InfoViewer->showMovieTitle(g_playstate, title, sub_title,
					g_percent, elapsed_time, remaining_time,
					ac3state, g_numpida > 1,
					g_Locale->getText(LOCALE_INFOVIEWER_LANGUAGES),
					g_Locale->getText(LOCALE_MOVIEPLAYER_TSHELP16));
		}
		else if (msg == CRCInput::RC_ok)
		{
			if (bufferfilled)
				showHelpVLC();
		}
		else if (msg == CRCInput::RC_left || msg == CRCInput::RC_right)
		{
			if (!autoplaylist)
			{
				if (msg == CRCInput::RC_left)
					selected--;
				else
					selected++;

				if (selected < 0)
					selected = 0;
				else if (selected >= (int)filelist.size())
					selected = filelist.size() - 1;
				else if (!filelist.empty() && g_playstate == CMoviePlayerGui::PLAY)
				{
					filename = filelist[selected].Name.c_str();
					sel_filename = filelist[selected].getFileName();
					//printf ("[movieplayer.cpp] sel_filename: %s\n", filename);
					if (stream)
					{
						int namepos = filelist[selected].Name.rfind("vlc://");
						mrl_str = filelist[selected].Name.substr(namepos + 6);
						mrl_str = url_escape(mrl_str.c_str());
					}
					else
						mrl_str = filelist[selected].Name;
					INFO("Generated FILE MRL: %s\n", mrl_str.c_str());
					update_info = true;
					start_play = true;
				}
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
		else if (msg == NeutrinoMessages::SHOW_EPG)
		{
			if (stream)
				showFileInfoVLC();
			else if (movieinfo_valid)
			{
				CMovieInfo mi;
				mi.showMovieInfo(movieinfo);
			}
		}
		else if (CNeutrinoApp::getInstance()->handleMsg(msg, data) & messages_return::cancel_all)
			exit = true;

		if (g_playstate == CMoviePlayerGui::SKIP && stream)
			StreamTime.hide();

		if (from_mb && g_playstate == CMoviePlayerGui::STOPPED)
			break;
	}
	while (true);
	INFO("waiting for output thread\n");
	pthread_join(rct, NULL);
	g_output_thread = false;

	delete hintBox;
	INFO("ends here\n");
} // PlayStream()

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
	return imageinfo.getModulVersion("Movieplayer2 ","$Revision: 1.58 $");
}

void CMoviePlayerGui::showHelpVLC()
{
	std::string version = "Version: " + getMoviePlayerVersion();
	Helpbox helpbox;
	helpbox.addLine(NEUTRINO_ICON_BUTTON_HOME, g_Locale->getText(LOCALE_MOVIEPLAYER_VLCHELP1));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_RED, g_Locale->getText(LOCALE_MOVIEPLAYER_VLCHELP2));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_GREEN, g_Locale->getText(LOCALE_INFOVIEWER_LANGUAGES));
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
	helpbox.addLine(NEUTRINO_ICON_BUTTON_DBOX, g_Locale->getText(LOCALE_MOVIEPLAYER_VLCHELP14));
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
				char *data = xmlGetAttribute(element, "name");
				if (data)
					helpbox.addLine(NEUTRINO_ICON_BUTTON_RED, data);
				xmlNodePtr element1 = element->xmlChildrenNode;
				while (element1)
				{
					char tmp[50] = "-- ";
					data = xmlGetAttribute(element1, "name");
					if (data)
					{
						strcat(tmp, data);
						strcat(tmp, " : ");
						data = xmlGetData(element1);
						if (data)
							strcat(tmp, data);
						helpbox.addLine(tmp);
					}
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

static inline void skip(int seconds, bool /*remote*/, bool absolute)
{
	skipseconds = seconds;
	skipabsolute = absolute;
	if (!g_EOF)
		g_playstate = CMoviePlayerGui::SKIP;
}

static inline int get_filetime(bool remaining)
{
	int filetime;
	if (remaining)
		filetime = g_endpts - g_pts;
	else
		filetime = g_pts - g_startpts;

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

	if ((p[7] & 0x80) == 0) // packets with both pts, don't care for dts
	// if ((p[7] & 0xC0) != 0x80) // packets with only pts
	// if ((p[7] & 0xC0) != 0xC0) // packets with pts and dts
		return -1;
	if (p[8] < 5)
		return -1;
	if (!(p[9] & 0x20))
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

/* gets the PTS at a specific file position from a PES
   ATTENTION! resets buf!  */
int get_PES_PTS(ringbuffer_t *buf, off_t position, bool until_eof)
{
	int pts = -1;
	char *ppes;
	int rd, eof = 0;
	off_t startpos = position;
	ringbuffer_data_t vec_in;
	ringbuffer_reset(buf);

	if (mf_lseek(position) < 0)
	{
		INFO("could not mf_lseek to %lld\n", position);
		return -1;
	}

	while (pts == -1 || until_eof)
	{
		if (position - startpos > 1024*1024*10)
		{
			INFO("could not determine PTS in 10MB... bail out.\n");
			break;
		}
		if (g_playstate == CMoviePlayerGui::STOPPED)
		{
			pts = -1;
			break;
		}
		ringbuffer_get_write_vector(buf, &vec_in);
		if (vec_in.len != 0 && eof < 2);
		{
			rd = read(g_fd, vec_in.buf, vec_in.len);
			if (rd > 0)
			{
				ringbuffer_write_advance(buf, rd);
				position += rd;
				eof = 0;
			}
			if (rd == 0)
			{
				eof++;
				if (mf_lseek(position) < 0)
					eof++;
			}

			rd = ringbuffer_get_readpointer(buf, &ppes, 14); // ppes[13] contains last pts bits
			if (rd != 14) // EOF?.
				break;
		}

		int r = mp_syncPES(buf, &ppes);
		if (r < 0)
			break;

		rd = ((ppes[4] << 8) | ppes[5]) + 6;

		switch(ppes[3])
		{
			int tmppts;
			case 0xe0 ... 0xef:	// video!
				tmppts = get_pts(ppes, true);
				if (tmppts >= 0)
					pts = tmppts;
				break;
			case 0xbb:
			case 0xbe:
			case 0xbf:
			case 0xf0 ... 0xf3:
			case 0xff:
			case 0xc0 ... 0xcf:
			case 0xd0 ... 0xdf:
				break;
			case 0xb9:
			case 0xba:
			case 0xbc:
			default:
				rd = 1;
				break;
		}
		if ((int)ringbuffer_read_space(buf) < rd)
		{
			INFO("ringbuffer_read_space %ld < rd %d\n", ringbuffer_read_space(buf), rd);
			continue;
		}
		ringbuffer_read_advance(buf, rd);
	}
	ringbuffer_reset(buf);
	return pts;
}


std::string url_escape(const char *url)
{
	std::string escaped;
	char * tmp = curl_escape(url, 0);
	escaped = (std::string)tmp;
	curl_free(tmp);
	return escaped;
}

size_t curl_dummywrite (void *ptr, size_t size, size_t nmemb, void *data)
{
	std::string* pStr = (std::string*) data;
	*pStr += (char*) ptr;
	return size * nmemb;
}

static bool filelist_auto_add(CFileList &filelist)
{
	if (filelist.size() != 1)
		return false;

	const char *filename = filelist[0].Name.c_str();
	char *ext;
	ext = strrchr(filename, '.');	// FOO-xxx-2007-12-31.001.ts <- the dot before "ts"
					// 001.vdr <- the dot before "vdr"
	// check if there is something to do...
	if (! ext)
		return false;
	if (!((ext - 7 >= filename && !strcmp(ext, ".ts") && *(ext - 4) == '.') ||
	      (ext - 4 >= filename && !strcmp(ext, ".vdr"))))
		return false;

	int num = 0;
	struct stat s;
	size_t numpos = strlen(filename) - strlen(ext) - 3;
	CFile *file = new CFile;
	sscanf(filename + numpos, "%d", &num);
	do {
		num++;
		char nextfile[strlen(filename) + 1];
		memcpy(nextfile, filename, numpos);
		sprintf(nextfile + numpos, "%03d%s", num, ext);
		if (stat(nextfile, &s))
			break; // file does not exist
		file->Name = nextfile;
		file->Size = s.st_size;
		INFO("auto-adding '%s' to playlist\n", nextfile);
		filelist.push_back(*file);
	} while (true && num < 999);
	delete file;

	return (filelist.size() > 1);
}
