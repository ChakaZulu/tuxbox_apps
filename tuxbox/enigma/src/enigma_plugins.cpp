#include <enigma_plugins.h>

#include <config.h>
#include <plugin.h>

#include <dlfcn.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

#include <enigma.h>
#include <enigma_lcd.h>
#include <lib/base/eerror.h>
#include <lib/gdi/lcd.h>
#include <lib/gdi/font.h>
#include <lib/gdi/grc.h>
#include <lib/driver/rc.h>
#include <lib/driver/streamwd.h>
#include <lib/dvb/edvb.h>
#include <lib/dvb/decoder.h>
#include <lib/gui/emessage.h>
#include <lib/gui/eskin.h>
#include <lib/system/info.h>

ePluginThread *ePluginThread::instance=0;

eString getInfo(const char *file, const char *info)
{
	FILE *f=fopen(file, "rt");
	if (!f)
		return 0;

	eString result;

	char buffer[128];

	while (fgets(buffer, 127, f))
	{
		if (strlen(buffer))
			buffer[strlen(buffer)-1]=0;

		if (strstr(buffer, info))
		{
  		result=eString(buffer).mid(strlen(info)+1, strlen(buffer)-strlen(info+1));
			break;
		}
	}	
	fclose(f);
	return result;
}

PluginParam *first=0, *tmp=0;

void MakeParam(char* id, int val)
{
	PluginParam* p = new PluginParam;

	if (tmp)
		tmp->next = p;

	p->id = new char[strlen(id)+1];
	strcpy(p->id, id);
	char buf[10];
	sprintf(buf, "%i", val);
	p->val = new char[strlen(buf)+1];
	strcpy(p->val, buf);

	if (!first)
		first = p;

	p->next=0;
	tmp = p;
}

ePlugin::ePlugin(eListBox<ePlugin> *parent, const char *cfgfile, const char* descr)
	:eListBoxEntryText((eListBox<eListBoxEntryText>*)parent)
{
	eDebug(cfgfile);
	text=getInfo(cfgfile, "name");

	if (text.isNull())
		text="(" + eString(cfgfile) + " is invalid)";
		
	eString desc=getInfo(cfgfile, "desc");

	if (desc)
	{
		text+=" - "+desc;
	}

	depend=getInfo(cfgfile, "depend");

	eString atype=getInfo(cfgfile, "type"),
					apluginVersion=getInfo(cfgfile, "pluginversion"),
					aneedfb=getInfo(cfgfile, "needfb"),
					aneedrc=getInfo(cfgfile, "needrc"),
					aneedlcd=getInfo(cfgfile, "needlcd"),
					aneedvtxtpid=getInfo(cfgfile, "needvtxtpid"),
					aneedoffsets=getInfo(cfgfile, "needoffsets"),
					apigon=getInfo(cfgfile, "pigon");

	needfb=(aneedfb.isNull()?false:atoi(aneedfb.c_str()));
	needlcd=(aneedlcd.isNull()?false:atoi(aneedlcd.c_str()));
	needrc=(aneedrc.isNull()?false:atoi(aneedrc.c_str()));
	needvtxtpid=(aneedvtxtpid.isNull()?false:atoi(aneedvtxtpid.c_str()));
	needoffsets=(aneedoffsets.isNull()?false:atoi(aneedoffsets.c_str()));
	version=(apluginVersion.isNull()?0:atoi(apluginVersion.c_str()));
	showpig=(apigon.isNull()?false:atoi(apigon.c_str()));

	sopath=eString(cfgfile).left(strlen(cfgfile)-4)+".so";	// uarg

	pluginname=eString(cfgfile).mid(eString(cfgfile).rfind('/')+1);

	pluginname=pluginname.left(pluginname.length()-4);
}

eZapPlugins::eZapPlugins(int type, eWidget* lcdTitle, eWidget* lcdElement)
	:eListBoxWindow<ePlugin>(type==2?_("Plugins"):_("Games"), 8, 400), type(type)
{
	PluginPath[0] = "/var/tuxbox/plugins/";
	PluginPath[1] = PLUGINDIR "/";
	PluginPath[2] = "";
	setHelpText(_("select plugin and press ok"));
	move(ePoint(150, 100));
#ifndef DISABLE_LCD
	setLCD(lcdTitle, lcdElement);
#endif
	CONNECT(list.selected, eZapPlugins::selected);
}

int eZapPlugins::exec()
{
	int cnt=0;
	ePlugin *plg=0;
	std::set<eString> exist;
	for ( int i = 0; i < 2; i++ )
	{
		DIR *d=opendir(PluginPath[i].c_str());
		if (!d)
		{
			eString err;
			err.sprintf(_("Couldn't read plugin directory %s"), PluginPath[i].c_str() );
			eDebug(err.c_str());
			if ( i )
			{
				eMessageBox msg(err, _("Error"), eMessageBox::iconError|eMessageBox::btOK );
				msg.show();
				msg.exec();
				msg.hide();
				return -1;
			}
			continue;
		}
		int connType=0;
		eConfig::getInstance()->getKey("/elitedvb/network/connectionType", connType);
		while (struct dirent *e=readdir(d))
		{
			eString FileName = e->d_name;
			if ( FileName.find(".cfg") != eString::npos )
			{
				eString cfgname=(PluginPath[i]+FileName).c_str();
				int ttype=atoi(getInfo(cfgname.c_str(), "type").c_str());
				if ((type == -1) || (type == ttype))
				{
					// do not add existing plugins twice
					if ( exist.find(FileName) != exist.end() )
						continue;
					exist.insert(FileName);
					// EVIL HACK
					if ( !connType && cfgname.find("dsl") != eString::npos &&
						cfgname.find("connect") != eString::npos )
						continue;
					////////////
					// EVIL HACK
					if ( !eSystemInfo::getInstance()->hasNetwork() && cfgname.find("Ngrab") != eString::npos )
						continue;
					////////////
					plg = new ePlugin(&list, cfgname.c_str());
					++cnt;
				}
			}
		}
		closedir(d);
	}
	int res=0;
	if ((type == 2) && (cnt == 1))
	{
		selected(plg);
	} else
	{
		show();
		res=eListBoxWindow<ePlugin>::exec();
		hide();
	}
	return res;
}

eString eZapPlugins::execPluginByName(const char* name)
{
	if ( name )
	{
		eString Path;
		for ( int i = 0; i < 3; i++ )
		{
			Path=PluginPath[i];
			Path+=name;
			FILE *fp=fopen(Path.c_str(), "rb");
			if ( fp )
			{
				fclose(fp);
				ePlugin p(0, Path.c_str());
				if (ePluginThread::getInstance())
				{
					eDebug("currently one plugin is running.. dont start another one!!");
					return _("E: currently another plugin is running...");
				}
				execPlugin(&p);
				return "OK";
			}
			else if ( i == 2)
				return eString().sprintf(_("plugin '%s' not found"), name );
		}
	}
	return _("E: no name given");
}

void eZapPlugins::execPlugin(ePlugin* plugin)
{
	ePluginThread *p = new ePluginThread(plugin, PluginPath, in_loop?this:0);
	p->start();
}

void eZapPlugins::selected(ePlugin *plugin)
{
	if (!plugin || !plugin->pluginname )
	{
		close(0);
		return;
	}
	execPlugin(plugin);
}

void ePluginThread::start()
{
	wasVisible = wnd ? wnd->isVisible() : 0;

	if (!thread_running())
	{
		argc=0;
		eString argv[20];

		if (depend)
		{
			char depstring[129];
			char *p;
			char *np;

			strcpy(depstring, depend.c_str());

			p=depstring;

			while(p)
			{
				np=strchr(p,',');
				if ( np )
					*np=0;

				for ( int i=0; i < 3; i++ )
				{
					eString str;
					if (np)
						str.assign( p, np-p );
					else
						str.assign( p );

					FILE *fp=fopen((PluginPath[i]+str).c_str(), "rb");
					if ( fp )
					{
						fclose(fp);
						argv[argc++] = PluginPath[i]+str;
						break;
					}
				}
				p=np?np+1:0;
			}
		}

		argv[argc++]=sopath;

		int i;
		eDebug("pluginname is %s %d", pluginname.c_str(), wasVisible);

		for (i=0; i<argc; i++)
		{
			eDebug("loading %s" , argv[i].c_str());
			libhandle[i]=dlopen(argv[i].c_str(), RTLD_GLOBAL|RTLD_NOW);
			if (!libhandle[i])
			{
				const char *de=dlerror();
				eDebug(de);
				eMessageBox msg(de, "plugin loading failed", eMessageBox::btOK, eMessageBox::btOK, 5 );
				msg.show();
				msg.exec();
				msg.hide();
				break;
			}
		}
		if (i<argc)  // loading of one dependencie failed... close the other
		{
			while(i)
				dlclose(libhandle[--i]);
			if (wasVisible)
				wnd->show();
		}
		else
		{
// this is ugly code.. but i have no other idea to detect enigma plugins..
			bool isEnigmaPlugin=false;
			int fd = open(sopath.c_str(), O_RDONLY);
			if ( fd >= 0 )
			{
				char buf[8192];
				while(!isEnigmaPlugin)
				{
					int rd = ::read(fd, buf, 8192);
					for (int i=0; i < rd-15; ++i )
					{
						if (!strcmp(buf+i, "_ZN7eWidgetD0Ev"))
							isEnigmaPlugin=true;
					}
					if ( rd < 8192 )
						break;
				}
				close(fd);
			}

			eDebug("would exec (%s) plugin %s", 
				isEnigmaPlugin ? "ENIGMA" : "NORMAL",
				sopath.c_str());

			PluginExec execPlugin = (PluginExec) dlsym(libhandle[i-1], "plugin_exec");
			if (!execPlugin)
				// show messagebox.. and close after 5 seconds...
			{
				eMessageBox msg("The symbol plugin_exec was not found. sorry.", "plugin executing failed", eMessageBox::btOK, eMessageBox::btOK, 5 );
				msg.show();
				msg.exec();
				msg.hide();
			}
			else
			{
				if (needrc)
					MakeParam(P_ID_RCINPUT, eRCInput::getInstance()->lock());

				if ( wasVisible )
					wnd->hide();

				while(gRC::getInstance().queuelock.getDiff())
					usleep(1000);

				if (needfb)
					MakeParam(P_ID_FBUFFER, fbClass::getInstance()->lock());

#ifndef DISABLE_LCD
				if (needlcd)
					MakeParam(P_ID_LCD, eDBoxLCD::getInstance()->lock() );
#endif

				if (needvtxtpid)
				{
					if(Decoder::current.tpid==-1)
						MakeParam(P_ID_VTXTPID, 0);
					else
						MakeParam(P_ID_VTXTPID, Decoder::current.tpid);
			// stop vtxt reinsertion
					tpid = Decoder::current.tpid;
					if (tpid != -1)
					{
						eDebug("stop vtxt reinsertion");
						Decoder::parms.tpid=-1;
						Decoder::Set();
					}
				}

				if (needoffsets)
				{
					int left=20, top=20, right=699, bottom=555;
					eConfig::getInstance()->getKey("/enigma/plugins/needoffsets/left", left);
					eConfig::getInstance()->getKey("/enigma/plugins/needoffsets/top", top);
					eConfig::getInstance()->getKey("/enigma/plugins/needoffsets/right", right);
					eConfig::getInstance()->getKey("/enigma/plugins/needoffsets/bottom", bottom);
					MakeParam(P_ID_OFF_X, left);
					MakeParam(P_ID_OFF_Y, top);
					MakeParam(P_ID_END_X, right);
					MakeParam(P_ID_END_Y, bottom);
				}

/*				for(PluginParam *par = first; par; par=par->next )
				{
					printf ("id: %s - val: %s\n", par->id, par->val);
					printf("%p\n", par->next);
				}*/

				if ( isEnigmaPlugin )
				{
					eDebug("start plugin in current thread");
					thread();
					thread_finished();
				}
				else
				{
					eDebug("start plugin thread...");
					run();  // start thread
				}
			}
		}
	}
	else
		eDebug("don't start plugin.. another one is running");
}

void ePluginThread::thread()
{
	if ( thread_running() )
		eDebug("plugin thread running.. execute plugin now");
	else
		eDebug("execute plugin now");
	PluginExec execPlugin = (PluginExec) dlsym(libhandle[argc-1], "plugin_exec");
	execPlugin(first);
	eDebug("execute plugin finished");
}

void ePluginThread::thread_finished()
{
	while (argc)
		dlclose(libhandle[--argc]);

	while (first)  // Parameter Liste freigegeben
	{
		tmp = first->next;
		delete first;
		first = tmp;
	}

	if (needfb)
		fbClass::getInstance()->unlock();

#ifndef DISABLE_LCD
	if (needlcd)
	{
		eDBoxLCD::getInstance()->unlock();
		eZapLCD::getInstance()->invalidate();
	}
#endif

	if ( wasVisible )
		wnd->show();

	if (needrc)
		eRCInput::getInstance()->unlock();

	if (needvtxtpid)
	{
		// start vtxt reinsertion
		if (tpid != -1 && Decoder::current.tpid == -1)
		{
			eDebug("restart vtxt reinsertion");
			Decoder::parms.tpid = tpid;
			Decoder::Set();
		}
	}

	delete this;
}
