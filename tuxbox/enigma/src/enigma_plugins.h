#ifndef __enigma_plugins_h
#define __enigma_plugins_h

#include <lib/gui/listbox.h>

class ePlugin: public eListBoxEntryText
{
	friend class eZapPlugins;
	friend class eListBox<ePlugin>;
public:
	int version;
	eString depend, sopath, pluginname;
	bool needfb, needrc, needlcd, needvtxtpid, needoffsets, showpig;
	int posx, posy, sizex, sizey;
	ePlugin(eListBox<ePlugin> *parent, const char *cfgfile, const char* descr=0);
};

class eZapPlugins: public eListBoxWindow<ePlugin>
{
private:
	void selected(ePlugin *);
public:
	eZapPlugins(eWidget* lcdTitle=0, eWidget* lcdElement=0);
	void execPluginByName(const char* name);
	void execPlugin(ePlugin* plugin);
	int exec();
};

#endif /* __enigma_plugins_h */
