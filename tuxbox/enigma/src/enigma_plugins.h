#ifndef __enigma_plugins_h
#define __enigma_plugins_h

#include <lib/base/thread.h>
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
	eString PluginPath[3];
	void selected(ePlugin *);
	int type;
public:
	eZapPlugins(int type, eWidget* lcdTitle=0, eWidget* lcdElement=0);
	eString execPluginByName(const char* name);
	void execPlugin(ePlugin* plugin);
	int exec();
};

class ePluginThread: public eThread
{
	void *libhandle[20];
	int argc, tpid;
	static ePluginThread *instance;
	eString depend, sopath, pluginname;
	bool needfb, needrc, needlcd, needvtxtpid, needoffsets, showpig;
	int posx, posy, sizex, sizey, wasVisible;
	eZapPlugins *wnd;
	eString PluginPath[3];
	void thread();
	void thread_finished();
public:
	ePluginThread( ePlugin *p, eString PluginPath[3], eZapPlugins *wnd )
		:depend(p->depend), sopath(p->sopath), pluginname(p->pluginname),
		needfb(p->needfb), needrc(p->needrc), needlcd(p->needlcd),
		needvtxtpid(p->needvtxtpid), needoffsets(p->needoffsets),
		showpig(p->showpig), posx(p->posx), posy(p->posy), sizex(p->sizex),
		sizey(p->sizey), wasVisible(0), wnd(wnd)
	{
		instance=this;
		this->PluginPath[0] = PluginPath[0];
		this->PluginPath[1] = PluginPath[1];
		this->PluginPath[2] = PluginPath[2];
	}
	~ePluginThread()
	{
		instance=0;
	}
	void start();
	static ePluginThread *getInstance() { return instance; }
};

#endif /* __enigma_plugins_h */
