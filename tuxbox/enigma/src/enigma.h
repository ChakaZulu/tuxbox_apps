#ifndef __enigma_h
#define __enigma_h

#include <qapplication.h>
#include "rc.h"
#include "fb.h"
#include "ewidget.h"
#include "elbwindow.h"
#include "si.h"
#include "lcd.h"
#include <ebase.h>

class gFBDC;
class gLCDDC;
class eServiceSelector;
class eZapMain;
class eService;
struct gRGB;
class eInit;

class eZap: public eApplication
{
//	Q_OBJECT
	static eZap *instance;
private:// slots:
	void keyEvent(const eRCKey &key);
	void keyUp(int);
	void keyDown(int);
	void status();
	
private:
	void Fade(gRGB *pal, int i, __u32 rgb1, __u32 rgb2, int level);
	void GenFade(gRGB *pal, int in, __u32 rgb1, __u32 rgb2, int num, int tr=0);
	void Set(gRGB *pal, int i, __u32 rgb);
	__u32 Fade(__u32 val, int h);
	
	eInit *init;
	
	eServiceSelector *serviceSelector;

	eZapMain *main;
	eTimer statusTimer;
public:
	static eZap *getInstance();
	eWidget *focus;
	eServiceSelector *getServiceSelector()
	{
		return serviceSelector;
	}
	
	QString getVersion();

	eZap(int argc, char **argv);
	~eZap();
};

#endif /* __enigma_h */
