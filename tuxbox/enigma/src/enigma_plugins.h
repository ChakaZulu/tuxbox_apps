#ifndef __enigma_plugins_h
#define __enigma_plugins_h

#include <core/gui/ewidget.h>
#include <core/gui/elbwindow.h>

class eListboxEntry;

class ePlugin: eListboxEntry
{
public:
	int version;
	eString name, desc;
	eString depend, sopath, pluginname;
	bool needfb, needrc, needlcd, needvtxtpid, needoffsets, showpig, needvidformat;
	int posx, posy, sizex, sizey;
	int isback;
	ePlugin(eListbox *parent, const char *cfgfile);
	eString getText(int t) const;
};

class eZapPlugins: public Object
{
	eLBWindow *window;

private:
	void selected(eListboxEntry *);
public:
	eZapPlugins(eWidget* lcdTitle=0, eWidget* lcdElement=0);
	void execPluginByName(const char* name);
	void execPlugin(ePlugin* plugin);
	~eZapPlugins();
	int exec();
};

#endif /* __enigma_plugins_h */
