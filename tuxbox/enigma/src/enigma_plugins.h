#ifndef __enigma_plugins_h
#define __enigma_plugins_h

#include "ewidget.h"
#include "elbwindow.h"

class eListboxEntry;

class ePlugin: eListboxEntry
{
public:
	int version;
	QString name, desc;
	QString depend, sopath, pluginname;
	bool needfb, needrc, needlcd, needvtxtpid, needoffsets, showpig;
	int posx, posy, sizex, sizey;
	int isback;
	ePlugin(eListbox *parent, const char *cfgfile);
	QString getText(int t) const;
};

class eZapPlugins: public QObject
{
	Q_OBJECT
	eLBWindow *window;

private slots:
	void selected(eListboxEntry *);
public:
	eZapPlugins(eWidget* lcdTitle=0, eWidget* lcdElement=0);
	~eZapPlugins();
	int exec();
};

#endif /* __enigma_plugins_h */
