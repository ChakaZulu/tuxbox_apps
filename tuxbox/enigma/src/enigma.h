#ifndef __enigma_h
#define __enigma_h

#include <qapplication.h>
#include <qtimer.h>
#include "rc.h"
#include "fb.h"
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
class eInit;

class eZap: public QApplication
{
	Q_OBJECT
	static eZap *instance;
private slots:
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
	QTimer statusTimer;
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
	eMainMenu(eWidget* lcdTitle=0, eWidget* lcdElement=0);
	~eMainMenu();
	int exec();
};

#endif /* __enigma_h */
