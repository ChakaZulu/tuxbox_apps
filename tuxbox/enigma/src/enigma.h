#ifndef __enigma_h
#define __enigma_h

#include <qapplication.h>
#include <qtimer.h>
#include "rc.h"
#include "fb.h"
#include "font.h"
#include "ewidget.h"
#include "elbwindow.h"
#include "si.h"
#include "lcd.h"

class gFBDC;
class gLCDDC;
class eServiceSelector;
class eZapMain;
class eService;
struct gRGB;
class gRC;

class eZap: public QApplication
{
	Q_OBJECT
	static eZap *instance;
private slots:
	void keyDown(int);
	void keyUp(int);
	void status();
	
private:
	void Fade(gRGB *pal, int i, __u32 rgb1, __u32 rgb2, int level);
	void GenFade(gRGB *pal, int in, __u32 rgb1, __u32 rgb2, int num, int tr=0);
	void Set(gRGB *pal, int i, __u32 rgb);
	__u32 Fade(__u32 val, int h);
	
	eRCInput *rc;
	gFBDC *gfbdc;
	gRC *grc;
	fontRenderClass *font;

	eLCD *lcd;
	gLCDDC *glcddc;
	
	eServiceSelector *serviceSelector;
	
	eZapMain *main;
	QTimer statusTimer;
public:
	void switchFontSize();
	static eZap *getInstance();
	eWidget *focus;
	int useBigOSD;
	int useBigFonts;
	static int FontSize;
	eServiceSelector *getServiceSelector()
	{
		return serviceSelector;
	}
	
	QString getVersion();

	eZap(int argc, char **argv);
	~eZap();

signals:
	void fontSizeChanged(int NewFontSize);

};

class eListBoxEntry;

class eMainMenu: public QObject
{
	Q_OBJECT;
	eLBWindow* window;
private slots:
	void sel_close(eListboxEntry *);
	void sel_vcr(eListboxEntry *);
	void sel_scan(eListboxEntry *);
	void sel_setup(eListboxEntry *);
	void sel_streaminfo(eListboxEntry *);
	void sel_quit(eListboxEntry *);
	void sel_bnversion(eListboxEntry *);
	void sel_record(eListboxEntry *);
	void sel_plugins(eListboxEntry *);
	void sel_about(eListboxEntry *);
public:
	eMainMenu();
	~eMainMenu();
	int exec();
};

#endif /* __enigma_h */
