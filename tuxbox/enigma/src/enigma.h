#ifndef __enigma_h
#define __enigma_h

#include <lib/base/ebase.h>
#include <src/sselect.h>

class eServiceSelector;
class eServicePath;
class eZapMain;
class eService;
class eWidget;
struct gRGB;
class eInit;
class eRCKey;
class eHTTPD;
class eHTTPConnection;

class eZap: public eApplication, public Object
{
	static eZap *instance;

	eWidget *desktop_lcd, *desktop_fb;
	
	eHTTPD *httpd;
	eHTTPConnection *serialhttpd;

private:
	void keyEvent(const eRCKey &key);
	void status();

private:
	void Fade(gRGB *pal, int i, __u32 rgb1, __u32 rgb2, int level);
	void GenFade(gRGB *pal, int in, __u32 rgb1, __u32 rgb2, int num, int tr=0);
	void Set(gRGB *pal, int i, __u32 rgb);
	__u32 Fade(__u32 val, int h);
	
	eInit *init;
	eServiceSelector *serviceSelector;

	eZapMain *main;
//	eTimer statusTimer;
public:
 	enum { desktopLCD, desktopFB };
 	
 	eWidget *getDesktop(int nr)
 	{
 		switch (nr)
 		{
 		case desktopLCD:
 			return desktop_lcd;
 		case desktopFB:
 			return desktop_fb;
 		default:
 			return 0;
 		}
 	}
	static eZap *getInstance();
	eWidget *focus;
	eServiceSelector *getServiceSelector()
	{
		ASSERT(serviceSelector);
		return serviceSelector;
	}
	
	eZap(int argc, char **argv);
	~eZap();
};

#endif /* __enigma_h */
