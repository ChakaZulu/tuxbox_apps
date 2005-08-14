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
#include <configfile.h>

using namespace std;

eMoviePlayer moviePlayer;

eString streamingServerSettings(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	eString ip;
	int port;
	eString dvddrive;
	int videodatarate, resolution, mpegcodec, forcetranscodevideo, audiodatarate, forcetranscodeaudio;
	eString result;
	
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	
	moviePlayer.readStreamingServerSettings(ip, port, dvddrive, videodatarate, resolution, mpegcodec, forcetranscodevideo, audiodatarate, forcetranscodeaudio);
	
	result = readFile(TEMPLATE_DIR + "streamingServerSettings.tmp");
	result.strReplace("#IP#", ip);
	result.strReplace("#PORT#", eString().sprintf("%d", port));
	result.strReplace("#DVDDRIVE#", dvddrive);
	result.strReplace("#VIDEODATARATE#", eString().sprintf("%d", videodatarate));
	result.strReplace("#RES0#", (resolution == 0) ? "selected" : "");
	result.strReplace("#RES1#", (resolution == 1) ? "selected" : "");
	result.strReplace("#RES2#", (resolution == 2) ? "selected" : "");
	result.strReplace("#RES3#", (resolution == 3) ? "selected" : "");
	result.strReplace("#CODEC1#", (mpegcodec == 1) ? "selected" : "");
	result.strReplace("#CODEC2#", (mpegcodec == 2) ? "selected" : "");
	result.strReplace("#FORCETRANSCODEVIDEO#", (forcetranscodevideo == 1) ? "checked" : "");
	result.strReplace("#AUDIODATARATE#", eString().sprintf("%d", audiodatarate));
	result.strReplace("#FORCETRANSCODEAUDIO#", (forcetranscodeaudio == 1) ? "checked" : "");
			
	return result;
}

eString setStreamingServerSettings(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	eString ip = opt["ip"];
	int port = atoi(opt["port"].c_str());
	eString dvddrive = opt["dvddrive"];
	int videodatarate = atoi(opt["videodatarate"].c_str());
	int resolution = atoi(opt["resolution"].c_str());
	int mpegcodec = atoi(opt["mpegcodec"].c_str());
	int forcetranscodevideo = (opt["forcetranscodevideo"] == "on") ? 1 : 0;
	int audiodatarate = atoi(opt["audiodatarate"].c_str());
	int forcetranscodeaudio = (opt["forcetranscodeaudio"] == "on") ? 1 : 0;
	
	moviePlayer.writeStreamingServerSettings(ip, port, dvddrive, videodatarate, resolution, mpegcodec, forcetranscodevideo, audiodatarate, forcetranscodeaudio);
	
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
		result = "dummy";
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
	dyn_resolver->addDyn("GET", "/cgi-bin/setStreamingServerSettings", setStreamingServerSettings, lockWeb);
}
