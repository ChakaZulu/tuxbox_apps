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
#include <src/enigma_dyn_colors.h>

#define TEMPLATE_DIR TUXBOXDATADIR+eString("/enigma/templates/")
#define TEMPLATE_DIR2 CONFIGDIR+eString("/enigma/templates/")
#define HTDOCS_DIR TUXBOXDATADIR+eString("/enigma/htdocs/")
#define HTDOCS_DIR2 CONFIGDIR+eString("/enigma/htdocs/")

#define CHARSETMETA "<META http-equiv=Content-Type content=\"text/html; charset=UTF-8\">\n"
#define WINDOWCLOSE "<html><head><META http-equiv=Content-Type content=\"text/html; charset=UTF-8\"></head><body><script  language=\"javascript\">close();</script></body></html>"
#define WINDOWBACK "<html><head><META http-equiv=Content-Type content=\"text/html; charset=UTF-8\"></head><body><script  language=\"javascript\">back();</script></body></html>"

eString getAttribute(eString filename, eString attribute);
eString readFile(eString filename);
eString button(int width, eString buttonText, eString buttonColor, eString buttonRef, eString color="");
eString getTitle(eString title);
int getHex(int c);
eString httpUnescape(const eString &string);
eString filter_string(eString string);
eString httpEscape(const eString &string);
std::map<eString, eString> getRequestOptions(eString opt, char delimiter);
eString ref2string(const eServiceReference &r);
eServiceReference string2ref(const eString &service);
eString closeWindow(eHTTPConnection *content, eString msg, int wait);
eString htmlChars(eString);
eString unHtmlChars(eString);
eString getIP(void);
off64_t getMovieSize(eString);

extern eString getRight(const eString&, char); // implemented in timer.cpp
extern eString getLeft(const eString&, char);  // implemented in timer.cpp

#endif /* __enigma_dyn_utils_h */
