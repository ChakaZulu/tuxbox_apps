#ifndef __enigma_plugins_h
#define __enigma_plugins_h

#include <lib/base/thread.h>
#include <lib/base/message.h>
#include <lib/base/console.h>
#include <lib/gui/listbox.h>
#include <lib/gui/emessage.h>

class ePlugin: public eListBoxEntryText
{
	friend class eZapPlugins;
	friend class eListBox<ePlugin>;
public:
	int version, type;
	eString depend, sopath, pluginname, requires, cfgname, desc, name;
	bool needfb, needrc, needlcd, needvtxtpid, needoffsets, showpig;
	int posx, posy, sizex, sizey;
	ePlugin(eListBox<ePlugin> *parent, const char *cfgfile, const char* descr=0);
	bool operator < ( const ePlugin & e ) const 
	{
		return cfgname < e.cfgname;
	}
};

class eZapPlugins: public eListBoxWindow<ePlugin>
{
private:
	eString PluginPath[3];
	void selected(ePlugin *);
	int type;
public:
	eZapPlugins(int type, eWidget* lcdTitle=0, eWidget* lcdElement=0);
	eString execPluginByName(const char* name, bool onlySearch=false);
	void execPlugin(ePlugin* plugin);
	int exec();
	int find(bool ignore_requires=false);
};

class ePluginThread: public eThread, public Object
{
	eFixedMessagePump<int> message;
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
	void finalize_plugin();
	void recv_msg(const int &);
public:
	ePluginThread( ePlugin *p, const eString PluginPath[3], eZapPlugins *wnd )
		:message(eApp,1), depend(p->depend), sopath(p->sopath), pluginname(p->pluginname),
		needfb(p->needfb), needrc(p->needrc), needlcd(p->needlcd),
		needvtxtpid(p->needvtxtpid), needoffsets(p->needoffsets),
		showpig(p->showpig), posx(p->posx), posy(p->posy), sizex(p->sizex),
		sizey(p->sizey), wasVisible(0), wnd(wnd)
	{
		CONNECT(message.recv_msg, ePluginThread::recv_msg);
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

class eScriptOutputWindow: public eWindow
{
private:
	eLabel *label;
	eWidget *visible;
	eProgress *scrollbar;
	int pageHeight;
	int total;
	int lines;
	int eventHandler(const eWidgetEvent &event);
	void updateScrollbar();
	eConsoleAppContainer *script;
	void getData( eString );
	void scriptClosed(int);
	eString scriptOutput;
public:
	eScriptOutputWindow(ePlugin *plugin);
	~eScriptOutputWindow();
};

#endif /* __enigma_plugins_h */
