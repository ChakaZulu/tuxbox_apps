#ifndef __enigma_dyn_utils_h
#define __enigma_dyn_utils_h

#include <lib/dvb/dvb.h>
#include <lib/dvb/edvb.h>
#include <lib/dvb/servicestructure.h>
#include <lib/dvb/decoder.h>
#include <lib/dvb/dvbservice.h>
#include <lib/dvb/service.h>
#include <lib/dvb/serviceplaylist.h>
#include <lib/system/http_dyn.h>

#define TEMPLATE_DIR DATADIR+eString("/enigma/templates/")
#define HTDOCS_DIR DATADIR+eString("/enigma/htdocs/")

#define CHARSETMETA "<META http-equiv=Content-Type content=\"text/html; charset=UTF-8\">\n"
#define NOCONTENT "<? header(\"HTTP/1.0 204 No Content\"); ?>"
#define WINDOWCLOSE "<html><head><META http-equiv=Content-Type content=\"text/html; charset=UTF-8\"></head><body><script  language=\"javascript\">close();</script></body></html>"
#define WINDOWBACK "<html><head><META http-equiv=Content-Type content=\"text/html; charset=UTF-8\"></head><body><script  language=\"javascript\">back();</script></body></html>"

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

eString getAttribute(eString filename, eString attribute);
eString readFile(eString filename);
eString button(int width, eString buttonText, eString buttonColor, eString buttonRef);
eString getTitle(eString title);
int getHex(int c);
eString httpUnescape(const eString &string);
eString filter_string(eString string);
eString httpEscape(const eString &string);
std::map<eString, eString> getRequestOptions(eString opt, char delimiter);
eString ref2string(const eServiceReference &r);
eServiceReference string2ref(const eString &service);
eString closeWindow(eHTTPConnection *content, eString msg, int wait);

extern eString getRight(const eString&, char); // implemented in timer.cpp
extern eString getLeft(const eString&, char);  // implemented in timer.cpp

#endif /* __enigma_dyn_utils_h */
