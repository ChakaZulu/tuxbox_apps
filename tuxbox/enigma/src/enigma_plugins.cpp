#include <dlfcn.h>
#include <dirent.h>
#include "enigma_plugins.h"
#include "rc.h"
#include "elistbox.h"
#include "ewindow.h"
#include "edvb.h"
#include "enigma.h"
#include "emessage.h"

typedef int     (*PluginInfoProc)( struct SPluginInfo *info );
typedef int     (*PluginExecProc)( int fd_fb, int fd_rc, int fd_lcd, char *cfgfile );

static QString getInfo(const char *file, const char *info)
{
	FILE *f=fopen(file, "rt");
	if (!f)
		return 0;
	QString result(0);
	while (1)
	{
		char buffer[128];
		if (!fgets(buffer, 128, f))
			break;
		if (strlen(buffer))
			buffer[strlen(buffer)-1]=0;
		if ((!strncmp(buffer, info, strlen(info)) && (buffer[strlen(info)]=='=')))
		{
			result=QString(buffer).mid(strlen(info)+1);
			break;
		}
	}	
	fclose(f);
	return result;
}

ePlugin::ePlugin(eListbox *parent, const char *cfgfile): eListboxEntry(parent)
{
	if (!cfgfile)
	{
		isback=1;
		return;
	}
	qDebug(cfgfile);
	name=getInfo(cfgfile, "name");
	if (!name)
		name="(" + QString(cfgfile) + " is invalid)";
	desc=getInfo(cfgfile, "desc");
	if (!desc)
		desc="";
	const char *aneedfb=getInfo(cfgfile, "needfb"), 
			*aneedlcd=getInfo(cfgfile, "needlcd"),
			*aneedrc=getInfo(cfgfile, "needrc");

	needfb=atoi(aneedfb?aneedfb:0);
	needlcd=atoi(aneedlcd?aneedlcd:0);
	needrc=atoi(aneedrc?aneedrc:0);
	isback=0;
	sopath=QString(cfgfile).left(strlen(cfgfile)-4)+".so";	// uarg
	pluginname=QString(cfgfile).mid(QString(cfgfile).findRev('/')+1);
	pluginname=pluginname.left(pluginname.length()-4);
	depend=getInfo(cfgfile, "depend");
}

QString ePlugin::getText(int t) const
{
	if (t)
		return 0;
	if (isback)
		return "Zurück";
	return name + " - " + desc;
}

eZapPlugins::eZapPlugins()
{
	window=new eLBWindow("Plugins", eListbox::tBorder, 10, eZap::FontSize, 400);
	window->move(QPoint(150, 136));
	new ePlugin(window->list, 0);
  connect(window->list, SIGNAL(selected(eListboxEntry*)), SLOT(selected(eListboxEntry*)));
}

int eZapPlugins::exec()
{
//	const QString PluginPath = "/usr/lib/neutrino/games/";
	const QString PluginPath = "/usr/games/";
#warning FIXME to DATADIR
	struct dirent **namelist;

	int n = scandir(PluginPath, &namelist, 0, alphasort);

	if (n < 0)
	{
		qDebug("Error Read Plugin Directory\n");
		eMessageBox msg("Error Read Plugin Directory", "Error");
		msg.show();
		msg.exec();
		msg.hide();
		return -1;
	}

	int nPlugins = 0; //Anzahl Plugins die gefunden wurden (CFG Dateien)

	for(int count=0; count<n; count++)
	{       	
		QString	FileName = namelist[count]->d_name;
		if (FileName.contains(".cfg"))
			nPlugins++;
	}

	if (nPlugins > 1)
	{
		for(int count=0;count<n;count++)
		{
			QString	FileName = namelist[count]->d_name;
			if (FileName.contains(".cfg"))
				new ePlugin(window->list, PluginPath+FileName);		

			free(namelist[count]);
	  }
	}
	free(namelist);

	window->show();
	int res=window->exec();
	window->hide();
	return res;
}

eZapPlugins::~eZapPlugins()
{
	delete window;
}

void eZapPlugins::selected(eListboxEntry *lbe)
{
	ePlugin *plugin=(ePlugin*)lbe;
	
	if (!plugin || plugin->isback)
	{
		window->close(0);
		return;
	}
	
	window->hide();
	
	void *libhandle[20];
	int argc=0;
	QString argv[20];
	
	
	if (plugin->depend)
	{
		char	depstring[129];
		char	*p;
		char	*np;

		strcpy(depstring,(const char*)plugin->depend);

		p=depstring;
		while(p)
		{
			np=strchr(p,',');
			if ( np )
				*np=0;
			argv[ argc++ ] = (*p == '/') ?
				QString(p) : "/lib/"+QString(p);
			p=np?np+1:0;
		}
	}

	argv[argc++]=plugin->sopath;

	int i;
		
	qDebug("pluginname is %s", (const char*)plugin->pluginname);

	for (i=0; i<argc; i++)
	{
		qDebug("loading %s" ,(const char*)argv[i]);
		libhandle[i]=dlopen(argv[i], RTLD_NOW|RTLD_GLOBAL);
		if (!libhandle[i])
		{
			qDebug(dlerror());
			eMessageBox msg(dlerror(), "plugin loading failed");
			msg.show();
			msg.exec();
			msg.hide();
			break;
		}
	}
	
	if (i==argc)
	{
		qDebug("would exec plugin %s", (const char*)plugin->sopath);
		PluginExecProc execPlugin=(PluginExecProc)dlsym(libhandle[i-1], plugin->pluginname + "_exec");
		
		if (!execPlugin)
		{
			eMessageBox msg("The symbol " + plugin->pluginname + "_exec" + " was not found. sorry.", "plugin executing failed");
			msg.show();
			msg.exec();
			msg.hide();
		} else
		{
			int fb_fd=fbClass::getInstance()->lock();
			int rc_fd=eRCInput::getInstance()->lock();
			execPlugin(fb_fd, rc_fd, -1,0 /*cfgfile*/);
			fbClass::getInstance()->unlock();
			eRCInput::getInstance()->unlock();
		}
	}
	
	while (i--)
	{
		dlclose(libhandle[i]);
	}

	window->show();
}
