#include <dlfcn.h>
#include <dirent.h>
#include "enigma_plugins.h"
#include "rc.h"
#include "elistbox.h"
#include "ewindow.h"
#include "edvb.h"
#include "enigma.h"
#include "emessage.h"
#include "config.h"
#include "eskin.h"
#include "lcd.h"
#include <plugin.h>
#include <dbox/avia_vbi.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include "decoder.h"

QString getInfo(const char *file, const char *info)
{
	FILE *f=fopen(file, "rt");
	if (!f)
		return 0;

	QString result(0);

	char buffer[128];

	while (fgets(buffer, 127, f))
	{
		if (strlen(buffer))
			buffer[strlen(buffer)-1]=0;

		if (strstr(buffer, info))
		{
  		result=QString(buffer).mid(strlen(info)+1, strlen(buffer)-strlen(info+1));
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

	depend=getInfo(cfgfile, "depend");

	QString atype=getInfo(cfgfile, "type"),
					apluginVersion=getInfo(cfgfile, "pluginversion"),
					aneedfb=getInfo(cfgfile, "needfb"),
					aneedrc=getInfo(cfgfile, "needrc"),
					aneedlcd=getInfo(cfgfile, "needlcd"),
					aneedvtxtpid=getInfo(cfgfile, "needvtxtpid"),
					aneedoffsets=getInfo(cfgfile, "needoffsets"),
					apigon=getInfo(cfgfile, "pigon");

	needfb=(aneedfb.isNull()?false:atoi(aneedfb));
	needlcd=(aneedlcd.isNull()?false:atoi(aneedlcd));
	needrc=(aneedrc.isNull()?false:atoi(aneedrc));
	needvtxtpid=(aneedvtxtpid.isNull()?false:atoi(aneedvtxtpid));
	needoffsets=(aneedoffsets.isNull()?false:atoi(aneedoffsets));
	version=(apluginVersion.isNull()?0:atoi(apluginVersion));
	showpig=(apigon.isNull()?false:atoi(apigon));

	isback=0;
	sopath=QString(cfgfile).left(strlen(cfgfile)-4)+".so";	// uarg
	pluginname=QString(cfgfile).mid(QString(cfgfile).findRev('/')+1);
	pluginname=pluginname.left(pluginname.length()-4);
}

QString ePlugin::getText(int t) const
{
	if (t)
		return 0;
	if (isback)
		return "[Zurück]";
	return name + " - " + desc;
}

eZapPlugins::eZapPlugins(eWidget* lcdTitle, eWidget* lcdElement)
{
	window=new eLBWindow("Plugins", eListbox::tBorder, 10, eSkin::getActive()->queryValue("fontsize", 20), 400);
	window->move(QPoint(150, 136));
	window->setLCD(lcdTitle, lcdElement);
	new ePlugin(window->list, 0);
  connect(window->list, SIGNAL(selected(eListboxEntry*)), SLOT(selected(eListboxEntry*)));
}

int eZapPlugins::exec()
{
	const QString PluginPath = PLUGINDIR "/";
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

	int nPlugins = 0;

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
				QString(p) : PLUGINDIR "/" + QString(p);
			p=np?np+1:0;
		}
	}

	argv[argc++]=plugin->sopath;

	int i;
	qDebug("pluginname is %s", (const char*)plugin->pluginname);

	if (plugin->needfb)
		MakeParam(P_ID_FBUFFER, fbClass::getInstance()->lock());

	if (plugin->needrc)
		MakeParam(P_ID_RCINPUT, eRCInput::getInstance()->lock());
	else
	{
		qDebug("Close RC driver\n");
		eRCInput::getInstance()->close();
	}

	if (plugin->needlcd)
		MakeParam(P_ID_LCD, eLCD::getPrimary()->lock());

	if (plugin->needoffsets)
	{
		MakeParam(P_ID_OFF_X, 0);
		MakeParam(P_ID_OFF_Y, 0);
		MakeParam(P_ID_END_X, 720);
		MakeParam(P_ID_END_Y, 576);
	}

 	if (plugin->needvtxtpid)
 	{
		// versuche, den gtx/enx_vbi zu stoppen	
		qDebug("try to stop gtx/enx_vbi");
		MakeParam(P_ID_VTXTPID, Decoder::parms.tpid);
    int fd = open("/dev/dbox/vbi0", O_RDWR);
		if (fd > 0)
		{
			qDebug("stop gtx/enx_vbi");
			ioctl(fd, AVIA_VBI_STOP_VTXT, 0);
			close(fd);
		}
	}

	PluginParam *par = first;
	for( ; par; par=par->next )
	{
		 printf ("id: %s - val: %s\n", par->id, par->val);
		 printf("%d\n", par->next);
	}

	for (i=0; i<argc; i++)
	{
		qDebug("loading %s" ,(const char*)argv[i]);
		libhandle[i]=dlopen(argv[i], RTLD_NOW|RTLD_GLOBAL);
		if (!libhandle[i])
		{
			const char *de=dlerror();
			qDebug(de);
			eMessageBox msg(de, "plugin loading failed");
			msg.show();
			msg.exec();
			msg.hide();
			break;
		}
	}
	
	if (i==argc)
	{
		qDebug("would exec plugin %s", (const char*)plugin->sopath);

		PluginExec execPlugin = (PluginExec) dlsym(libhandle[i-1], "plugin_exec");
		if (!execPlugin)
		{
			eMessageBox msg("The symbol " + plugin->pluginname + "_exec" + " was not found. sorry.", "plugin executing failed");
			msg.show();
			msg.exec();
			msg.hide();
		}
		else
		{		
			printf("exec Plugin now...\n");
			execPlugin(first);
			dlclose(libhandle[i-1]);
			printf("exec done...\n");
		}

		while (i--)
			dlclose(libhandle[i]);
	}

	do  // Parameter Liste freigegeben
	{
		tmp = first->next;
		delete first;
		first = tmp;
	}
	while (first);

	if (plugin->needfb)
		fbClass::getInstance()->unlock();
	
	if (plugin->needrc)
		eRCInput::getInstance()->unlock();
	else
	{
		if (eRCInput::getInstance()->open())
			qDebug("RC driver open success\n");
	}

	if (plugin->needlcd)
		eLCD::getPrimary()->unlock();

 	if (plugin->needvtxtpid)
 	{
		// versuche, den gtx/enx_vbi wieder zu starten
		qDebug("try to restart gtx/enx_vbi");
 		int fd = open("/dev/dbox/vbi0", O_RDWR);
		if (fd > 0)
		{
			ioctl(fd, AVIA_VBI_START_VTXT, Decoder::parms.tpid);
			close(fd);
		}
  }

	window->show();
}
