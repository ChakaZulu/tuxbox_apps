#ifndef __enigma_plugins_h
#define __enigma_plugins_h

#include "ewidget.h"
#include "elbwindow.h"

class eListboxEntry;

class ePlugin: eListboxEntry
{
public:
	QString name, desc;
	QString depend, sopath, pluginname;
	int needfb, needrc, needlcd;
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
	eZapPlugins();
	~eZapPlugins();
	int exec();
};

#endif /* __enigma_plugins_h */
