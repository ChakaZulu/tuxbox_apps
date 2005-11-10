/*
 * $Id: enigma_dyn_movieplayer.cpp,v 1.16 2005/11/10 21:54:51 digi_casi Exp $
 *
 * (C) 2005 by digi_casi <digi_casi@tuxbox.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
 
#ifdef ENABLE_EXPERT_WEBIF

#include <map>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <iomanip>
#include <iostream>
#include <fstream>

#include <enigma.h>
#include <enigma_main.h>
#include <enigma_standby.h>
#include <timer.h>
#include <lib/driver/eavswitch.h>
#include <lib/dvb/dvb.h>
#include <lib/dvb/edvb.h>
#include <lib/dvb/epgcache.h>
#include <lib/dvb/servicestructure.h>
#include <lib/dvb/decoder.h>
#include <lib/dvb/dvbservice.h>
#include <lib/dvb/service.h>
#include <lib/dvb/record.h>
#include <lib/dvb/serviceplaylist.h>

#include <lib/system/info.h>
#include <lib/system/http_dyn.h>
#include <lib/system/econfig.h>
#include <enigma_dyn.h>
#include <enigma_dyn_utils.h>
#include <enigma_dyn_movieplayer.h>
#include <lib/movieplayer/movieplayer.h>
#include <lib/movieplayer/mpconfig.h>
#include <configfile.h>

using namespace std;

eMoviePlayer moviePlayer;

eString streamingServerSettings(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=iso-8859-1";
	eMoviePlayer::getInstance()->mpconfig.load();
	return readFile(TEMPLATE_DIR + "movieplayer.xml");
}


eString XSLMPSettings(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=iso-8859-1";
	eString result = readFile(TEMPLATE_DIR + "XSLMPSettings.xsl");
	result.strReplace("#SERVEREDITBUTTON#", button(100, "Edit", NOCOLOR, "javascript:mpServerConfig()", "#000000"));
	result.strReplace("#VLCEDITBUTTON#", button(100, "Edit", NOCOLOR, "javascript:mpVLCConfig()", "#000000"));
	return result;
}


eString setStreamingServerSettings(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=iso-8859-1";
	
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	
	return closeWindow(content, "", 500);
}

eString setStreamingServerVideoSettings(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=iso-8859-1";
	
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	
	return closeWindow(content, "", 500);
}


eString setStreamingServerVLCSettings(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=iso-8859-1";
	
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	
	return closeWindow(content, "", 500);
}

eString editStreamingServerSettings(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=iso-8859-1";
	
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	
	eString result = readFile(TEMPLATE_DIR + "editStreamingServerSettings.tmp");
	
	return closeWindow(content, "", 500);
}

eString editStreamingServerVideoSettings(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=iso-8859-1";
	
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	
	eString result = readFile(TEMPLATE_DIR + "editStreamingServerVideoSettings.tmp");
	
	return closeWindow(content, "", 500);
}


eString editStreamingServerVLCSettings(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=iso-8859-1";
	
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	
	eString result = readFile(TEMPLATE_DIR + "editStreamingServerVLCSettings.tmp");
	
	return closeWindow(content, "", 500);
}

eString getStreamingServer()
{
	eString result = readFile(TEMPLATE_DIR + "streamingServer.tmp");
	char *drive;
	if (eConfig::getInstance()->getKey("/movieplayer/dvddrive", drive))
		drive = strdup("D");
	result.strReplace("#DRIVE#", eString(drive));
	free(drive);
	result.strReplace("#FILEBUTTON#", button(100, "File", NOCOLOR, "javascript:playFile()", "#000000"));
	result.strReplace("#DVDBUTTON#", button(100, "DVD", NOCOLOR, "javascript:playDVD()", "#000000"));
	result.strReplace("#VCDBUTTON#", button(100, "(S)VCD", NOCOLOR, "javascript:playVCD()", "#000000"));
	result.strReplace("#SETTINGSBUTTON#", button(100, "Settings", NOCOLOR, "javascript:settings()", "#000000"));
	eString tmp = button(100, "Terminate", RED, "javascript:terminateStreaming()", "#FFFFFF");
	result.strReplace("#TERMINATEBUTTON#", tmp);
	return result;
}

eString streamingServer(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	return getStreamingServer();
}

eString movieplayerm3u(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	eString command = opt["command"];
	eString mrl = httpUnescape(opt["mrl"]);
	eString result;
	
	eDebug("[MOVIEPLAYERPLS] command = %s, mrl = %s", command.c_str(), mrl.c_str());
	moviePlayer.control(command.c_str(), mrl.c_str());
	
	if (command == "start")
	{
		content->local_header["Content-Type"] = "video/mpegfile";
		content->local_header["Cache-Control"] = "no-cache";
		result = "#EXTM3U\n";
		result += "#EXTVLCOPT:sout=" + moviePlayer.sout(mrl) + "\n";
		result += mrl;
	}
	else
	{
		content->local_header["Content-Type"] = "text/html; charset=utf-8";
		content->local_header["Cache-Control"] = "no-cache";
		result = closeWindow(content, "", 500);
	}
	
	return result;
}

void ezapMoviePlayerInitializeDyn(eHTTPDynPathResolver *dyn_resolver, bool lockWeb)
{
	dyn_resolver->addDyn("GET", "/cgi-bin/movieplayer.m3u", movieplayerm3u, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/streamingServer", streamingServer, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/streamingServerSettings", streamingServerSettings, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/editStreamingServerSettings", editStreamingServerSettings, lockWeb);
	dyn_resolver->addDyn("GET", "/XSLMPSettings.xsl", XSLMPSettings, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/setStreamingServerSettings", setStreamingServerSettings, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/editStreamingServerVideoSettings", editStreamingServerVideoSettings, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/setStreamingServerVideoSettings", setStreamingServerVideoSettings, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/setStreamingServerVLCSettings", setStreamingServerVLCSettings, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/editStreamingServerVLCSettings", editStreamingServerVLCSettings, lockWeb);
}
#endif

