#ifndef __enigma_dyn_h
#define __enigma_dyn_h

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

#define ZAPMODETV 0
#define ZAPMODERADIO 1
#define ZAPMODEDATA 2
#define ZAPMODERECORDINGS 3

#define ZAPMODENAME 0
#define ZAPMODECATEGORY 1
#define ZAPSUBMODESATELLITES 2
#define ZAPSUBMODEPROVIDERS 3
#define ZAPSUBMODEBOUQUETS 4

#define NOCONTENT "<? header(\"HTTP/1.0 204 No Content\"); ?>"

class eHTTPDynPathResolver;

void ezapInitializeDyn(eHTTPDynPathResolver *dyn_resolver);

#endif /* __enigma_dyn_h */
