#include "enigma_plugins.h"

#include "config.h"
#include <plugin.h>
#include <dbox/avia_gt_vbi.h>

#include <dlfcn.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>

#include "enigma.h"

#include <core/base/eerror.h>
#include <core/gdi/lcd.h>
#include <core/driver/rc.h>
#include <core/dvb/edvb.h>
#include <core/dvb/decoder.h>
#include <core/gui/elistbox.h>
#include <core/gui/ewindow.h>
#include <core/gui/emessage.h>
#include <core/gui/eskin.h>

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

ePlugin::ePlugin(eListbox *parent, const char *cfgfile): eListboxEntry(parent)
{
	if (!cfgfile)
	{
		isback=1;
		return;
	}
	isback=0;

	eDebug(cfgfile);
	name=getInfo(cfgfile, "name");

	if (name.isNull())
		name="(" + eString(cfgfile) + " is invalid)";
		
	desc=getInfo(cfgfile, "desc");

	if (desc.isNull())
		desc="";

	depend=getInfo(cfgfile, "depend");

	eString atype=getInfo(cfgfile, "type"),
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

	sopath=eString(cfgfile).left(strlen(cfgfile)-4)+".so";	// uarg

	pluginname=eString(cfgfile).mid(eString(cfgfile).rfind('/')+1);

	pluginname=pluginname.left(pluginname.length()-4);
}

eString ePlugin::getText(int t) const
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
	window->move(ePoint(150, 136));
	window->setLCD(lcdTitle, lcdElement);
	new ePlugin(window->list, 0);
	CONNECT(window->list->selected, eZapPlugins::selected);
}

int eZapPlugins::exec()
{
	const eString PluginPath = PLUGINDIR "/";
	struct dirent **namelist;

	int n = scandir(PluginPath, &namelist, 0, alphasort);

	if (n < 0)
	{
		eDebug("Error Read Plugin Directory\n");
		eMessageBox msg("Error Read Plugin Directory", "Error");
		msg.show();
		msg.exec();
		msg.hide();
		return -1;
	}

/*	int nPlugins = 0;

	for(int count=0; count<n; count++)
	{       	
		eString	FileName = namelist[count]->d_name;
		if ( FileName.find(".cfg") != -1 )
			nPlugins++;
	}
	if (nPlugins > 1)
	{*/
		for(int count=0;count<n;count++)
		{
			eString	FileName = namelist[count]->d_name;

			if ( FileName.find(".cfg") != -1 )
				new ePlugin(window->list, (PluginPath+FileName).c_str());		

			free(namelist[count]);
	  }
//	}
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

void eZapPlugins::execPluginByName(const char* name)
{
	eString PluginPath = PLUGINDIR "/";
	PluginPath+=name;
	ePlugin p(0, PluginPath);
	execPlugin(&p);
}

void eZapPlugins::execPlugin(ePlugin* plugin)
{
	void *libhandle[20];
	int argc=0;
	eString argv[20];

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
				eString(p) : eString(PLUGINDIR "/" + eString(p));
			p=np?np+1:0;
		}
	}

	argv[argc++]=plugin->sopath;

	int i;
	eDebug("pluginname is %s", (const char*)plugin->pluginname);

	if (plugin->needfb)
		MakeParam(P_ID_FBUFFER, fbClass::getInstance()->lock());

	if (plugin->needrc)
		MakeParam(P_ID_RCINPUT, eRCInput::getInstance()->lock());

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
		eDebug("try to stop gtx/enx_vbi");
		MakeParam(P_ID_VTXTPID, Decoder::parms.tpid);
		int fd = open("/dev/dbox/vbi0", O_RDWR);
		if (fd > 0)
		{
			eDebug("stop gtx/enx_vbi");
			ioctl(fd, AVIA_VBI_STOP_VTXT, 0);
			close(fd);
		}
	}

/*	
	for(PluginParam *par = first; par; par=par->next )
	{
		printf ("id: %s - val: %s\n", par->id, par->val);
		printf("%d\n", par->next);
	}*/

	for (i=0; i<argc; i++)
	{
		eDebug("loading %s" ,(const char*)argv[i]);
		libhandle[i]=dlopen(argv[i], RTLD_NOW|RTLD_GLOBAL);
		if (!libhandle[i])
		{
			const char *de=dlerror();
			eDebug(de);
			eMessageBox msg(de, "plugin loading failed");
			msg.show();
			msg.exec();
			msg.hide();
			break;
		}
	}
	
	if (i==argc)
	{
		eDebug("would exec plugin %s", (const char*)plugin->sopath);

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
			eDebug("exec Plugin now...");
			execPlugin(first);
			dlclose(libhandle[i-1]);
			eDebug("exec done...");
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

	if (plugin->needlcd)
		eLCD::getPrimary()->unlock();

 	if (plugin->needvtxtpid)
 	{
		// versuche, den gtx/enx_vbi wieder zu starten
		eDebug("try to restart gtx/enx_vbi");
 		int fd = open("/dev/dbox/vbi0", O_RDWR);
		if (fd > 0)
		{
			ioctl(fd, AVIA_VBI_START_VTXT, Decoder::parms.tpid);
			close(fd);
		}
	}

}

void eZapPlugins::selected(eListboxEntry *lbe)
{
	ePlugin *plugin=(ePlugin*)lbe;
	
	if (!plugin || plugin->isback)
	{
		window->close(0);
		return;
	}

	execPlugin(plugin);

	window->hide();
	window->show();
}
