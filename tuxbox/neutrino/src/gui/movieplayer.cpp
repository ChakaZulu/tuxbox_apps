/*
	Neutrino-GUI  -   DBoxII-Project

	Movieplayer (c) 2003 by gagga
	Based on code by Dirch, obi and the Metzler Bros. Thanks.

        $Id: movieplayer.cpp,v 1.34 2003/09/04 22:56:39 zwen Exp $

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

/* KNOWN ISSUES:
  - AC3 handling does not work
  - TS which are played back from CIFS drives may not work in a good quality.
*/


/* TODOs / Release Plan:
 - always: fix bugs
(currently planned order)
 - Nicer UI
 - Chapter support for DVD and (S)VCD
 - Playing from Bookmarks
 - MP3 HTTP streaming
*/

#include <config.h>
#if HAVE_DVB_API_VERSION >= 3
#include <global.h>
#include <neutrino.h>

#include <driver/fontrenderer.h>
#include <driver/rcinput.h>
#include <daemonc/remotecontrol.h>
#include <system/settings.h>
#include <algorithm>
#include <sys/time.h>
#include <fstream>

#include "eventlist.h"
#include "movieplayer.h"
#include <transform.h>
#include "color.h"
#include "infoviewer.h"
#include "nfs.h"

#include "widget/menue.h"
#include "widget/messagebox.h"
#include "widget/hintbox.h"
#include "widget/stringinput.h"

#include <fcntl.h>
#include <linux/dvb/audio.h>
#include <linux/dvb/dmx.h>
#include <linux/dvb/video.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

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

#define STOPPED		0
#define PREPARING	1
#define STREAMERROR	2
#define PLAY		3
#define PAUSE		4
#define FF		5
#define REW		6
#define SOFTRESET	99

#define STREAMTYPE_DVD	1
#define STREAMTYPE_SVCD	2
#define STREAMTYPE_FILE	3

#define ConnectLineBox_Width	15

#define RINGBUFFERSIZE 348*188*10
#define MAXREADSIZE 348*188
#define MINREADSIZE 348*188


static int playstate;
static bool isTS;
int speed = 1;
static long fileposition;
ringbuffer_t *ringbuf;
bool bufferfilled;
int streamingrunning;
unsigned short pida, pidv;
CHintBox *hintBox;
CHintBox *bufferingBox;
bool avpids_found;

//------------------------------------------------------------------------
size_t
CurlDummyWrite (void *ptr, size_t size, size_t nmemb, void *data)
{
  return size * nmemb;
}

//------------------------------------------------------------------------

CMoviePlayerGui::CMoviePlayerGui ()
{
  frameBuffer = CFrameBuffer::getInstance ();

  visible = false;
  selected = 0;

  filebrowser = new CFileBrowser ();
  filebrowser->Multi_Select = false;
  filebrowser->Dirs_Selectable = false;
  videofilefilter.addFilter ("ts");
  videofilefilter.addFilter ("ps");
  videofilefilter.addFilter ("mpg");
  videofilefilter.addFilter ("m2p");
  videofilefilter.addFilter ("avi");
  filebrowser->Filter = &videofilefilter;
  if (strlen (g_settings.network_nfs_moviedir) != 0)
    Path = g_settings.network_nfs_moviedir;
  else
    Path = "/";
}

//------------------------------------------------------------------------

CMoviePlayerGui::~CMoviePlayerGui ()
{
  delete filebrowser;
  g_Zapit->setStandby (false);
  g_Sectionsd->setPauseScanning (false);

}

//------------------------------------------------------------------------
int
CMoviePlayerGui::exec (CMenuTarget * parent, std::string actionKey)
{
  m_state = STOP;
  current = -1;
  selected = 0;

  //define screen width
  width = 710;
  if ((g_settings.screen_EndX - g_settings.screen_StartX) <
      width + ConnectLineBox_Width)
    width =
      (g_settings.screen_EndX - g_settings.screen_StartX) -
      ConnectLineBox_Width;

  //define screen height
  height = 570;
  if ((g_settings.screen_EndY - g_settings.screen_StartY) < height)
    height = (g_settings.screen_EndY - g_settings.screen_StartY);
  buttonHeight = min (25, g_Fonts->infobar_small->getHeight ());
  theight = g_Fonts->menu_title->getHeight ();
  fheight = g_Fonts->menu->getHeight ();
  sheight = g_Fonts->infobar_small->getHeight ();
  title_height = fheight * 2 + 20 + sheight + 4;
  info_height = fheight * 2;
  listmaxshow =
    (height - info_height - title_height - theight -
     2 * buttonHeight) / (fheight);
  height = theight + info_height + title_height + 2 * buttonHeight + listmaxshow * fheight;	// recalc height

  x =
    (((g_settings.screen_EndX - g_settings.screen_StartX) -
      (width + ConnectLineBox_Width)) / 2) + g_settings.screen_StartX +
    ConnectLineBox_Width;
  y =
    (((g_settings.screen_EndY - g_settings.screen_StartY) - height) / 2) +
    g_settings.screen_StartY;

  if (parent)
    {
      parent->hide ();
    }

  // set zapit in standby mode
  g_Zapit->setStandby (true);

  // tell neutrino we're in ts_mode
  CNeutrinoApp::getInstance ()->handleMsg (NeutrinoMessages::CHANGEMODE,
					   NeutrinoMessages::mode_ts);
  // remember last mode
  m_LastMode =
    (CNeutrinoApp::getInstance ()->
     getLastMode () /*| NeutrinoMessages::norezap */ );

  // Stop sectionsd
  g_Sectionsd->setPauseScanning (true);


  show ();

  //stop();
  hide ();

  g_Zapit->setStandby (false);

  // Start Sectionsd
  g_Sectionsd->setPauseScanning (false);

  // Restore last mode
  CNeutrinoApp::getInstance ()->handleMsg (NeutrinoMessages::CHANGEMODE,
					   m_LastMode);

  // always exit all
  return menu_return::RETURN_EXIT_ALL;
}

//------------------------------------------------------------------------
CURLcode sendGetRequest (std::string url) {
  CURL *curl;
  CURLcode httpres;
  httpres = (CURLcode) 1;
  std::string response = "";
  
  curl = curl_easy_init ();
  curl_easy_setopt (curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, CurlDummyWrite);
  curl_easy_setopt (curl, CURLOPT_FILE, (void *) &response);
  curl_easy_setopt (curl, CURLOPT_FAILONERROR, true);
  httpres = curl_easy_perform (curl);
  //printf ("[movieplayer.cpp] HTTP Result: %d\n", httpres);
  curl_easy_cleanup (curl);
  return httpres;
}

//------------------------------------------------------------------------
void *
ReceiveStreamThread (void *mrl)
{
  printf ("[movieplayer.cpp] ReceiveStreamThread started\n");
  int skt;
  const char *server;
  int port;
  CURLcode httpres;
  httpres = (CURLcode) 1;
  int nothingreceived=0;

  // Get Server and Port from Config
  server = g_settings.streaming_server_ip.c_str ();
  sscanf (g_settings.streaming_server_port, "%d", &port);


  std::string baseurl = "http://" + g_settings.streaming_server_ip + ":" + g_settings.streaming_server_port + "/";

  // empty playlist
  std::string emptyurl = baseurl + "?control=empty";
  httpres = sendGetRequest(emptyurl);
  printf ("[movieplayer.cpp] HTTP Result (emptyurl): %d\n", httpres);
  if (httpres != 0)
    {
      ShowMsg ("messagebox.error",
	       g_Locale->getText ("movieplayer.nostreamingserver"),
	       CMessageBox::mbrCancel, CMessageBox::mbCancel, "error.raw");
      playstate = STOPPED;
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
  httpres = sendGetRequest(addurl);
  
  // add sout (URL encoded)
  // Example(mit transcode zu mpeg1): ?sout=#transcode{vcodec=mpgv,vb=2000,acodec=mpga,ab=192,channels=2}:duplicate{dst=std{access=http,mux=ts,url=:8080/dboxstream}}
  // Example(ohne transcode zu mpeg1): ?sout=#duplicate{dst=std{access=http,mux=ts,url=:8080/dboxstream}}
  //TODO make this nicer :-)
  std::string souturl;
  if(!memcmp((char*)mrl, "vcd:", 4) || addurl.substr(addurl.length()-4) == ".mpg" || addurl.substr(addurl.length()-4) == ".m2p")
  {
	  // no transcode
	  souturl = baseurl + "?sout=%23duplicate%7Bdst%3Dstd%7Baccess%3Dhttp%2Cmux%3Dts%2Curl%3D%3A" + g_settings.streaming_server_port + "%2Fdboxstream%7D%7D";
  }
  else
  {
	  // with transcode
	  souturl = baseurl + "?sout=%23transcode%7Bvcodec%3Dmpgv%2Cvb%3D" + g_settings.streaming_videorate + "%2Cacodec%3Dmpga%2Cab%3D" + g_settings.streaming_audiorate + "%2Cchannels%3D2%7D%3Aduplicate%7Bdst%3Dstd%7Baccess%3Dhttp%2Cmux%3Dts%2Curl%3D%3A" + g_settings.streaming_server_port + "%2Fdboxstream%7D%7D";
  }
  httpres = sendGetRequest(souturl);

  // play MRL
  std::string playurl = baseurl + "?control=play&item=0";
  httpres = sendGetRequest(playurl);
  
// TODO: Better way to detect if http://<server>:8080/dboxstream is already alive. For example repetitive checking for HTTP 404.
// Unfortunately HTTP HEAD requests are not supported by VLC :(
// vlc 0.6.3 and up may support HTTP HEAD requests.

// Open HTTP connection to VLC
  bool vlc_is_sending = false;

  struct sockaddr_in servAddr;
  servAddr.sin_family = AF_INET;
  servAddr.sin_port = htons (port);
  servAddr.sin_addr.s_addr = inet_addr (server);
  int res;
  printf ("[movieplayer.cpp] Server: %s\n", server);
  printf ("[movieplayer.cpp] Port: %d\n", port);
  char buf[RINGBUFFERSIZE];
  int len;

  while (!vlc_is_sending)
    {

      //printf ("[movieplayer.cpp] Trying to call socket\n");
      skt = socket (AF_INET, SOCK_STREAM, 0);

      printf ("[movieplayer.cpp] Trying to connect socket\n");
      res = connect (skt, (struct sockaddr *) &servAddr, sizeof (servAddr));
      if (res < 0)
	{
	  perror ("SOCKET");
	  playstate = STOPPED;
	  pthread_exit (NULL);
	}
      fcntl (skt, O_NONBLOCK);
      printf ("[movieplayer.cpp] Socket OK\n");

      // Skip HTTP header
      char *msg = "GET /dboxstream HTTP/1.0\r\n\r\n";
      int msglen = strlen (msg);
      if (send (skt, msg, msglen, 0) == -1)
	{
	  perror ("send()");
	  playstate = STOPPED;
	  pthread_exit (NULL);
	}

      printf ("[movieplayer.cpp] GET Sent\n");

      // Skip HTTP Header
      int found = 0;
      char line[200];
      strcpy (line, "");
      while (found < 4)
	{
	  len = recv (skt, buf, 1, 0);
	  strncat (line, buf, 1);
	  if (strcmp (line, "HTTP/1.0 404") == 0)
	    {
	      printf ("[movieplayer.cpp] VLC still does not send. Retrying...\n");
	      close (skt);
	      break;
	    }
	  if ((found == 0) & (buf[0] == '\r'))
	    {
	      found++;
	    }
	  else if ((found == 1) & (buf[0] == '\n'))
	    {
	      found++;
	    }
	  else if ((found == 2) & (buf[0] == '\r'))
	    {
	      found++;
	    }
	  else if ((found == 3) & (buf[0] == '\n'))
	    {
	      found++;
	    }
	  else
	    {
	      (found = 0);
	    }
	}
      if (found == 4)
	{
	  vlc_is_sending = true;

	}
    }
  printf ("[movieplayer.cpp] Now VLC is sending. Read sockets created\n");
  hintBox->hide ();
  bufferingBox->paint ();
  printf ("[movieplayer.cpp] Buffering approx. 3 seconds\n");

  int done;
  int size;
  streamingrunning = 1;
  int fd = open ("/tmp/tmpts", O_CREAT | O_WRONLY);

  struct pollfd poller[0];
  poller[0].fd = skt;
  poller[0].events = POLLIN | POLLPRI;
  int pollret;

  while (streamingrunning == 1)
    {
      while ((size = ringbuffer_write_space (ringbuf)) == 0)
	{
	  if (playstate == STOPPED)
	    {
	      close(skt);
	      pthread_exit (NULL);
	    }
	  if (!avpids_found)
	    {
	      // find apid and vpid. Easiest way to do that is to write the TS to a file 
	      // and use the usual find_avpids function. This is not even overhead as the
	      // buffer needs to be prefilled anyway
	      close (fd);
	      fd = open ("/tmp/tmpts", O_RDONLY);
	      //Use global pida, pidv
	      //unsigned short pidv = 0, pida = 0;
	      find_avpids (fd, &pidv, &pida);
	      close (fd);
	      printf ("[movieplayer.cpp] ReceiveStreamThread: while streaming found pida: 0x%04X ; pidv: 0x%04X\n",
		       pida, pidv);
	      avpids_found = true;
	    }
	  if (!bufferfilled) {
	    bufferingBox->hide ();
	    //TODO reset drivers?
	    bufferfilled = true;
	  }
	}
      //printf("[movieplayer.cpp] ringbuf write space:%d\n",size);

      if (playstate == STOPPED)
	{
	  close(skt);
	  pthread_exit (NULL);
	}

      len = 0;
      pollret = poll (poller, (unsigned long) 1, -1);

      if ((pollret < 0) ||
          ((poller[0].revents & POLLHUP) == POLLHUP) ||
	  ((poller[0].revents & POLLERR) == POLLERR) ||
	  ((poller[0].revents & POLLNVAL) == POLLNVAL))
	{
	  perror ("Error while polling()");
	  playstate = STOPPED;
	  close(skt);
	  pthread_exit (NULL);
	}


      if (((poller[0].revents & POLLIN) == POLLIN) ||
          ((poller[0].revents & POLLPRI) == POLLPRI))
	{
	  len = recv (poller[0].fd, buf, size, 0);
	}

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
        if (playstate == PLAY) {
          nothingreceived++;
          if (nothingreceived > 200) {
            printf ("[movieplayer.cpp] PlayStreamthread: Didn't receive for a while. Stopping.\n");
            playstate = STOPPED;	
          }	
        }
      }
      
      while (len > 0)
	{
	  done = ringbuffer_write (ringbuf, buf, len);
	  len -= done;
	}

    }
  close(skt);
  pthread_exit (NULL);
}


//------------------------------------------------------------------------
void *
PlayStreamThread (void *mrl)
{
  char buf[348 * 188];
  bool failed = false;
  // use global pida and pidv
  pida = 0, pidv = 0;
  int done, dmxa = 0, dmxv = 0, dvr = 0, adec = 0, vdec = 0;
  struct dmx_pes_filter_params p;
  ssize_t wr;
  ringbuf = ringbuffer_create (RINGBUFFERSIZE);
  printf ("[movieplayer.cpp] ringbuffer created\n");

  bufferingBox = new CHintBox ("messagebox.info",
			  g_Locale->getText ("movieplayer.buffering"),
			  "info.raw", 450);

  CURLcode httpres;
  httpres = (CURLcode) 1;
  std::string baseurl = "http://" + g_settings.streaming_server_ip + ":" + g_settings.streaming_server_port + "/";

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

  playstate = SOFTRESET;
  printf ("[movieplayer.cpp] read starting\n");
  size_t readsize, len;
  len = 0;
  bool driverready = false;
  std::string pauseurl;
  std::string unpauseurl;
  while (playstate > STOPPED)
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
	      printf ("[movieplayer.cpp] PlayStreamthread: while streaming found pida: 0x%04X ; pidv: 0x%04X\n",
		       pida, pidv);

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
	    }

	  len = ringbuffer_read (ringbuf, buf, (readsize / 188) * 188);

	  switch (playstate)
	    {
	    case PAUSE:
	      //ioctl (dmxv, DMX_STOP);
	      ioctl (dmxa, DMX_STOP);

	      // pause VLC
	      pauseurl = baseurl + "?control=pause";
	      httpres = sendGetRequest(pauseurl);

	      while (playstate == PAUSE)
		{
		  //ioctl (dmxv, DMX_STOP);	
		  ioctl (dmxa, DMX_STOP);
		}
	      // unpause VLC
	      unpauseurl = baseurl + "?control=pause";
	      httpres = sendGetRequest(unpauseurl);

	      speed = 1;
	      break;
	    case PLAY:
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
	    case SOFTRESET:
	      ioctl (vdec, VIDEO_STOP);
	      ioctl (adec, AUDIO_STOP);
	      ioctl (dmxv, DMX_STOP);
	      ioctl (dmxa, DMX_STOP);
	      ioctl (vdec, VIDEO_PLAY);
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
	      playstate = PLAY;

	    }
	}
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
  httpres = sendGetRequest(stopurl);

  printf ("[movieplayer.cpp] Waiting for RCST to stop\n");
  pthread_join (rcst, NULL);
  printf ("[movieplayer.cpp] Seems that RCST was stopped succesfully\n");
  pthread_exit (NULL);
}

//------------------------------------------------------------------------
void *
PlayFileThread (void *filename)
{
  bool failed = false;
  unsigned char buf[384 * 188 * 2];
  unsigned short pida = 0, pidv = 0;
  int done, fd = 0, dmxa = 0, dmxv = 0, dvr = 0, adec = 0, vdec = 0;
  struct dmx_pes_filter_params p;
  ssize_t wr = 0;
  ssize_t cache = sizeof (buf);
  size_t r = 0;
  if ((char *) filename == NULL)
    {
      playstate = STOPPED;
      pthread_exit (NULL);
    }

  if ((fd = open ((char *) filename, O_RDONLY | O_LARGEFILE)) < 0)
    {
      playstate = STOPPED;
      pthread_exit (NULL);
    }

  // todo: check if file is valid ts or pes
  if (isTS)
    {
      find_avpids (fd, &pidv, &pida);
      printf ("[movieplayer.cpp] found pida: 0x%04X ; pidv: 0x%04X\n",
	       pida, pidv);
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
  fileposition = 0;
  if (isTS && !failed)
    {
      while ((r = read (fd, buf, cache)) > 0 && playstate >= PLAY)
	{
	  done = 0;
	  wr = 0;
	  fileposition += r;
	  switch (playstate)
	    {
	    case PAUSE:
	      while (playstate == PAUSE)
		{
		  ioctl (dmxa, DMX_STOP);
		}
	      break;
	    case FF:
	    case REW:
	      ioctl (dmxa, DMX_STOP);
	      lseek (fd, cache * speed, SEEK_CUR);
	      fileposition += cache * speed;
	      break;
	    case SOFTRESET:
	      ioctl (vdec, VIDEO_STOP);
	      ioctl (adec, AUDIO_STOP);
	      ioctl (dmxv, DMX_STOP);
	      ioctl (dmxa, DMX_STOP);
	      ioctl (vdec, VIDEO_PLAY);
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
	      playstate = PLAY;
	    }

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
    }
  else if (!failed)
    {
      ioctl (vdec, VIDEO_PLAY);
      ioctl (adec, AUDIO_PLAY);
      ioctl (dmxv, DMX_START);
      ioctl (dmxa, DMX_START);
      pes_to_ts2 (fd, dvr, pida, pidv, &playstate);	// VERY bad performance!!!
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
  if (playstate != STOPPED)
    {
      playstate = STOPPED;
      g_RCInput->postMsg (CRCInput::RC_red, 0);	// for faster exit in PlayStream(); do NOT remove!
    }

  pthread_exit (NULL);
}

//------------------------------------------------------------------------
void
CMoviePlayerGui::PlayStream (int streamtype)
{
  uint msg, data;
  string sel_filename;
  bool update_info = true, start_play = false, exit =
    false, open_filebrowser = true;
  char mrl[200];
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

  playstate = STOPPED;
  /* playstate == STOPPED         : stopped
   * playstate == PREPARING       : preparing stream from server
   * playstate == ERROR           : error setting up server
   * playstate == PLAY            : playing
   * playstate == PAUSE           : pause-mode
   * playstate == FF              : fast-forward
   * playstate == REW             : rewind
   * playstate == SOFTRESET       : softreset without clearing buffer (playstate toggle to 1)
   */
  do
    {
      if (exit)
	{
	  exit = false;
	  if (playstate >= PLAY)
	    {
	      playstate = STOPPED;
	      break;
	    }
	}

      if (open_filebrowser)
	{
	  open_filebrowser = false;
	  filename = NULL;
	  char startDir[40 + 6];
	  strcpy (startDir, "vlc://");
	  strcat (startDir, g_settings.streaming_server_startdir);
	  printf ("[movieplayer.cpp] Startdir: %s\n", startDir);
	  if (filebrowser->exec (startDir))
	    {
	      Path = filebrowser->getCurrentDir ();
	      if ((filename =
		   filebrowser->getSelectedFile ()->Name.c_str ()) != NULL)
		{
		  sel_filename =
		    filebrowser->getSelectedFile ()->getFileName ();
		  //printf ("[movieplayer.cpp] sel_filename: %s\n", filename);
		  int namepos =
		    filebrowser->getSelectedFile ()->Name.rfind ("vlc://");
		  string mrl_str =
		    filebrowser->getSelectedFile ()->Name.substr (namepos +
								  6);
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
	      if (playstate == STOPPED)
		break;
	    }

	  CLCD::getInstance ()->setMode (CLCD::MODE_TVRADIO);
	}

      if (update_info)
	{
	  update_info = false;
	  char tmp[20];
	  string lcd;
	  switch (playstate)
	    {
	    case PAUSE:
	      lcd = "|| (" + sel_filename + ")";
	      break;
	    case REW:
	      sprintf (tmp, "%dx<< ", speed);
	      lcd = tmp + sel_filename;
	      break;
	    case FF:
	      sprintf (tmp, "%dx>> ", speed);
	      lcd = tmp + sel_filename;
	      break;
	    default:
	      lcd = "> " + sel_filename;
	      break;
	    }

	  CLCD::getInstance ()->showServicename (lcd);
	}

      if (start_play)
	{
	  start_play = false;
	  bufferfilled = false;
	  avpids_found=false;
	  
	  if (playstate >= PLAY)
	    {
	      playstate = STOPPED;
	      pthread_join (rct, NULL);
	    }
	  //TODO: Add Dialog (Remove Dialog later)
	  hintBox =
	    new CHintBox ("messagebox.info",
			  g_Locale->getText ("movieplayer.pleasewait"),
			  "info.raw", 450);
	  hintBox->paint ();
	  if (pthread_create (&rct, 0, PlayStreamThread, (void *) mrl) != 0)
	    {
	      break;
	    }
	  playstate = SOFTRESET;
	}

      g_RCInput->getMsg (&msg, &data, 100);	// 10 secs..
      if (msg == CRCInput::RC_red || msg == CRCInput::RC_home)
	{
	  //exit play
	  exit = true;
	}
      else if (msg == CRCInput::RC_yellow)
	{
	  if (playstate != PAUSE)
	    {
	      update_info = true;
	      playstate = PAUSE;
	    }
	  else
	    {
	      // resume play
	      update_info = true;
	      playstate = SOFTRESET;
	    }
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
	if (CNeutrinoApp::getInstance ()->
	    handleMsg (msg, data) & messages_return::cancel_all)
	{
	  exit = true;
	}
    }
  while (playstate >= PLAY);
  pthread_join (rct, NULL);
}

void
CMoviePlayerGui::PlayFile (void)
{
  uint msg, data;
  string sel_filename;
  bool update_lcd = true, open_filebrowser =
    true, start_play = false, exit = false;
  playstate = STOPPED;
  /* playstate == STOPPED         : stopped
   * playstate == PLAY            : playing
   * playstate == PAUSE           : pause-mode
   * playstate == FF              : fast-forward
   * playstate == REW             : rewind
   * playstate == SOFTRESET       : softreset without clearing buffer (playstate toggle to 1)
   */
  do
    {
      if (exit)
	{
	  exit = false;
	  if (playstate >= PLAY)
	    {
	      playstate = STOPPED;
	      break;
	    }
	}

      if (open_filebrowser)
	{
	  open_filebrowser = false;
	  filename = NULL;
	  if (filebrowser->exec (g_settings.network_nfs_moviedir))
	    {
	      Path = filebrowser->getCurrentDir ();
	      if ((filename =
		   filebrowser->getSelectedFile ()->Name.c_str ()) != NULL)
		{
		  update_lcd = true;
		  start_play = true;
		  sel_filename =
		    filebrowser->getSelectedFile ()->getFileName ();
		}
	    }
	  else
	    {
	      if (playstate == STOPPED)
		break;
	    }

	  CLCD::getInstance ()->setMode (CLCD::MODE_TVRADIO);
	}

      if (update_lcd)
	{
	  update_lcd = false;
	  char tmp[20];
	  string lcd;
	  switch (playstate)
	    {
	    case PAUSE:
	      lcd = "|| (" + sel_filename + ")";
	      break;
	    case REW:
	      sprintf (tmp, "%dx<< ", speed);
	      lcd = tmp + sel_filename;
	      break;
	    case FF:
	      sprintf (tmp, "%dx>> ", speed);
	      lcd = tmp + sel_filename;
	      break;
	    default:
	      lcd = "> " + sel_filename;
	      break;
	    }

	  CLCD::getInstance ()->showServicename (lcd);
	}

      if (start_play)
	{
	  start_play = false;
	  if (playstate >= PLAY)
	    {
	      playstate = STOPPED;
	      pthread_join (rct, NULL);
	    }

	  if (pthread_create
	      (&rct, 0, PlayFileThread, (void *) filename) != 0)
	    {
	      break;
	    }
	  playstate = SOFTRESET;
	}

      g_RCInput->getMsg (&msg, &data, 100);	// 10 secs..
      if (msg == CRCInput::RC_red || msg == CRCInput::RC_home)
	{
	  //exit play
	  exit = true;
	}
      else if (msg == CRCInput::RC_yellow)
	{
	  if (playstate != PAUSE)
	    {
	      update_lcd = true;
	      playstate = PAUSE;
	    }
	  else
	    {
	      // resume play
	      update_lcd = true;
	      playstate = SOFTRESET;
	    }
	}
      else if (msg == CRCInput::RC_blue)
	{
	  FILE *bookmarkfile;
	  char bookmarkfilename[] =
	    "/var/tuxbox/config/movieplayer.bookmarks";
	  bookmarkfile = fopen (bookmarkfilename, "a");
	  fprintf (bookmarkfile, "%s\n", filename);
	  fprintf (bookmarkfile, "%ld\n", fileposition);
	  fclose (bookmarkfile);
	}
      else if (msg == CRCInput::RC_left)
	{
	  // rewind
	  if (speed > 1)
	    speed = 1;
	  speed *= -2;
	  speed *= (speed > 1 ? -1 : 1);
	  playstate = REW;
	  update_lcd = true;
	}
      else if (msg == CRCInput::RC_right)
	{
	  // fast-forward
	  if (speed < 1)
	    speed = 1;
	  speed *= 2;
	  playstate = FF;
	  update_lcd = true;
	}
      else if (msg == CRCInput::RC_up || msg == CRCInput::RC_down)
	{
	  // todo: next/prev file
	}
      else if (msg == CRCInput::RC_help)
	{
	  // todo: infobar
	}
      else if (msg == CRCInput::RC_ok)
	{
	  if (playstate > PLAY)
	    {
	      update_lcd = true;
	      playstate = SOFTRESET;
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
	if (CNeutrinoApp::getInstance ()->
	    handleMsg (msg, data) & messages_return::cancel_all)
	{
	  isTS = true;		// also exit in PES Mode
	  exit = true;
	}
    }
  while (playstate >= PLAY);
  pthread_join (rct, NULL);
}

int
CMoviePlayerGui::show ()
{
  int res = -1;
  uint msg, data;
  bool loop = true, update = true;
  key_level = 0;
  while (loop)
    {
      if (CNeutrinoApp::getInstance ()->
	  getMode () != NeutrinoMessages::mode_ts)
	{
	  // stop if mode was changed in another thread
	  loop = false;
	}

      if (update)
	{
	  hide ();
	  update = false;
	  paint ();
	}

      // Check Remote Control

      g_RCInput->getMsg (&msg, &data, 10);	// 1 sec timeout to update play/stop state display
      if (msg == CRCInput::RC_home)
	{			//Exit after cancel key
	  loop = false;
	}
      else if (msg == CRCInput::RC_timeout)
	{
	  // do nothing
	}
//------------ RED --------------------
      else if (msg == CRCInput::RC_red)
	{
	  hide ();
	  PlayStream (STREAMTYPE_FILE);
	  paint ();
	}
//------------ GREEN --------------------
      else if (msg == CRCInput::RC_green)
	{
	  hide ();
	  isTS = true;
	  PlayFile ();
	  paint ();
	}
/*//------------ YELLOW --------------------
      else if (msg == CRCInput::RC_yellow)
	{
	  hide ();
	  isTS = false;
	  PlayFile ();
	  paint ();
	}
	*/
//------------ YELLOW --------------------
      else if (msg == CRCInput::RC_yellow)
	{
	  hide ();
	  PlayStream (STREAMTYPE_DVD);
	  paint ();
	}
//------------ BLUE --------------------
      else if (msg == CRCInput::RC_blue)
	{
	  hide ();
	  PlayStream (STREAMTYPE_SVCD);
	  paint ();
	}
      else if (msg == NeutrinoMessages::CHANGEMODE)
	{
	  if ((data & NeutrinoMessages::
	       mode_mask) != NeutrinoMessages::mode_ts)
	    {
	      loop = false;
	      m_LastMode = data;
	    }
	}
      else
	if (msg == NeutrinoMessages::RECORD_START
	    || msg == NeutrinoMessages::ZAPTO
	    || msg == NeutrinoMessages::STANDBY_ON
	    || msg == NeutrinoMessages::SHUTDOWN
	    || msg == NeutrinoMessages::SLEEPTIMER)
	{
	  // Exit for Record/Zapto Timers
	  // Add bookmark
	  loop = false;
	  g_RCInput->postMsg (msg, data);
	}
      else
	{
	  if (CNeutrinoApp::getInstance ()->
	      handleMsg (msg, data) & messages_return::cancel_all)
	    {
	      loop = false;
	    }
	  // update mute icon
	  paintHead ();
	}
    }
  hide ();
  if (m_state != STOP)
    {
      //stop();
    }

  return (res);
}

//------------------------------------------------------------------------

void
CMoviePlayerGui::hide ()
{
  if (visible)
    {
      frameBuffer->paintBackgroundBoxRel (x -
					  ConnectLineBox_Width
					  - 1,
					  y +
					  title_height
					  - 1,
					  width
					  +
					  ConnectLineBox_Width
					  + 2, height + 2 - title_height);
      frameBuffer->paintBackgroundBoxRel (x, y, width, title_height);
      frameBuffer->ClearFrameBuffer ();
      visible = false;
    }
}

//------------------------------------------------------------------------

void
CMoviePlayerGui::paintHead ()
{
//      printf("[movieplayer.cpp] paintHead{\n");
  std::string strCaption = g_Locale->getText ("movieplayer.head");
  frameBuffer->paintBoxRel (x, y + title_height, width, theight,
			    COL_MENUHEAD);
  frameBuffer->paintIcon ("movie.raw", x + 7, y + title_height + 10);
  g_Fonts->menu_title->RenderString (x + 35, y + theight + title_height + 0, width - 45, strCaption.c_str (), COL_MENUHEAD, 0, true);	// UTF-8
  int ypos = y + title_height;
  if (theight > 26)
    ypos = (theight - 26) / 2 + y + title_height;
  frameBuffer->paintIcon ("dbox.raw", x + width - 30, ypos);
  if (CNeutrinoApp::getInstance ()->isMuted ())
    {
      int xpos = x + width - 75;
      ypos = y + title_height;
      if (theight > 32)
	ypos = (theight - 32) / 2 + y + title_height;
      frameBuffer->paintIcon ("mute.raw", xpos, ypos);
    }
  visible = true;
}

//------------------------------------------------------------------------

void
CMoviePlayerGui::paintImg ()
{
  // TODO: find better image
  frameBuffer->paintBoxRel (x,
			    y +
			    title_height +
			    theight, width,
			    height -
			    info_height -
			    2 *
			    buttonHeight -
			    title_height - theight, COL_BACKGROUND);
  frameBuffer->paintIcon ("movieplayer.raw",
			  x + 25, y + 15 + title_height + theight);
}

//------------------------------------------------------------------------
void
CMoviePlayerGui::paintFoot ()
{
  if (m_state == STOP)		// insurance
    key_level = 0;
  int ButtonWidth = (width - 20) / 4;
  frameBuffer->paintBoxRel (x,
			    y + (height -
				 info_height
				 -
				 2 *
				 buttonHeight),
			    width, 2 * buttonHeight, COL_MENUHEAD);
  frameBuffer->paintHLine (x, x + width - x,
			   y + (height -
				info_height
				- 2 * buttonHeight), COL_INFOBAR_SHADOW);
/*  frameBuffer->paintIcon ("rot.raw", x + 0 * ButtonWidth + 10,
			  y + (height - info_height - 2 * buttonHeight) + 4);
  g_Fonts->infobar_small->RenderString (x + 0 * ButtonWidth + 30, y + (height - info_height - 2 * buttonHeight) + 24 - 1, ButtonWidth - 20, g_Locale->getText ("movieplayer.bookmark").c_str (), COL_INFOBAR, 0, true);	// UTF-8
*/
  frameBuffer->paintIcon ("rot.raw", x + 0 * ButtonWidth + 10,
			  y + (height - info_height - 2 * buttonHeight) + 4);
  g_Fonts->infobar_small->RenderString (x + 0 * ButtonWidth + 30, y + (height - info_height - 2 * buttonHeight) + 24 - 1, ButtonWidth - 20, g_Locale->getText ("movieplayer.choosestreamfile").c_str (), COL_INFOBAR, 0, true);	// UTF-8

  frameBuffer->paintIcon ("gruen.raw", x + 1 * ButtonWidth + 10,
			  y + (height - info_height - 2 * buttonHeight) + 4);
  g_Fonts->infobar_small->RenderString (x + 1 * ButtonWidth + 30, y + (height - info_height - 2 * buttonHeight) + 24 - 1, ButtonWidth - 20, g_Locale->getText ("movieplayer.choosets").c_str (), COL_INFOBAR, 0, true);	// UTF-8
  frameBuffer->paintIcon ("gelb.raw", x + 2 * ButtonWidth + 10,
			  y + (height - info_height - 2 * buttonHeight) + 4);
  g_Fonts->infobar_small->RenderString (x + 2 * ButtonWidth + 30, y + (height - info_height - 2 * buttonHeight) + 24 - 1, ButtonWidth - 20, g_Locale->getText ("movieplayer.choosestreamdvd").c_str (), COL_INFOBAR, 0, true);	// UTF-8
  frameBuffer->paintIcon ("blau.raw", x + 3 * ButtonWidth + 10,
			  y + (height - info_height - 2 * buttonHeight) + 4);
  g_Fonts->infobar_small->RenderString (x + 3 * ButtonWidth + 30, y + (height - info_height - 2 * buttonHeight) + 24 - 1, ButtonWidth - 20, g_Locale->getText ("movieplayer.choosestreamsvcd").c_str (), COL_INFOBAR, 0, true);	// UTF-8
}

void
CMoviePlayerGui::paint ()
{
  CLCD::getInstance ()->setMode (CLCD::MODE_TVRADIO);
  CLCD::getInstance ()->showServicename ("Movieplayer");
  frameBuffer->loadPal ("radiomode.pal", 18, COL_MAXFREE);
  frameBuffer->loadBackground ("radiomode.raw");
  frameBuffer->useBackground (true);
  frameBuffer->paintBackground ();
  paintHead ();
  paintImg ();
  paintFoot ();
  visible = true;
}

#endif
