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
#include <timer.h>
#include <enigma_main.h>
#include <enigma_plugins.h>
#include <enigma_standby.h>
#include <sselect.h>
#include <upgrade.h>

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
#include <lib/gdi/fb.h>
#include <lib/gdi/glcddc.h>
#include <lib/gdi/gfbdc.h>
#include <lib/gdi/epng.h>
#include <lib/gui/emessage.h>
#include <lib/system/http_dyn.h>
#include <lib/system/econfig.h>
#include <lib/system/info.h>
#include <lib/system/dmfp.h>
#include <enigma_dyn.h>
#include <enigma_dyn_mount.h>

using namespace std;

#define TEMPLATE_DIR DATADIR+eString("/enigma/templates/")
#define HTDOCS_DIR DATADIR+eString("/enigma/htdocs/")
#define CHARSETMETA "<META http-equiv=Content-Type content=\"text/html; charset=UTF-8\">\n"

#define BLUE "#12259E"
#define RED "#CB0303"
#define GREEN "#1FCB12"
#define YELLOW "#F5FF3C"
#define LIGHTGREY "#F4F4EC"
#define DARKGREY "#D9E0E7"
#define LEFTNAVICOLOR ""
#define TOPNAVICOLOR ""
#define OCKER "#FFCC33"
#define PINK "#95077C"
#define NOCOLOR ""

#define NOCONTENT "<? header(\"HTTP/1.0 204 No Content\"); ?>"

#define WEBXFACEVERSION "1.3.1"

extern int smallScreen;

extern eString getRight(const eString&, char); // implemented in timer.cpp
extern eString getLeft(const eString&, char);  // implemented in timer.cpp
extern bool onSameTP(const eServiceReferenceDVB& ref1, const eServiceReferenceDVB &ref2); // implemented in timer.cpp
extern eString button(int width, eString buttonText, eString buttonColor, eString buttonRef); 
extern eString httpUnescape(const eString &string);
extern eString filter_string(eString string);
extern eString httpEscape(const eString &string);
extern std::map<eString, eString> getRequestOptions(eString, char);

eString addMountPoint(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	
	return "+ok";
}

eString removeMountPoint(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	
	return "+ok";
}

eString editMountPoint(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	
	return "+ok";
}

eString showMountPoints(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	
	return "+ok";
}

eString addMountPointWindow(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	
	return "+ok";
}

eString editMountPointWindow(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	
	return "+ok";
}

eString mountMountPoint(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	
	return "+ok";
}
eString unmountMountPoint(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	
	return "+ok";
}


