#include <map>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/reboot.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <lib/system/info.h>
#include <lib/system/http_dyn.h>
#include <enigma_dyn_utils.h>
#include <enigma_dyn_dreamflash.h>

using namespace std;

int installImage(eString image, eString name, eString target)
{
	return 0;
}

eString dreamflash(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	eString image = opt["image"];
	eString target = opt["target"];
	eString name = opt["name"];
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	
	installImage(image, name, target);
	
	eString result = "not supported yet";
	return result;
}

void ezapDreamflashInitializeDyn(eHTTPDynPathResolver *dyn_resolver, bool lockWeb)
{
	dyn_resolver->addDyn("GET", "/cgi-bin/dreamflash", dreamflash, lockWeb);
}
