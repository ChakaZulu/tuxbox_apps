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

eString movieplayer(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	eMoviePlayer *moviePlayer = new eMoviePlayer();
		
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	
	eString command = opt["command"];
	if (command == "start")
	{
		moviePlayer->start(opt["mrl"]);
	}
	else
	{
		moviePlayer->stop();
		delete moviePlayer;
	}
	
	return "done";
}

void ezapMoviePlayerInitializeDyn(eHTTPDynPathResolver *dyn_resolver, bool lockWeb)
{
	dyn_resolver->addDyn("GET", "/cgi-bin/movieplayer.pls", movieplayer, lockWeb);
}
