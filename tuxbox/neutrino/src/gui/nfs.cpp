/*
	Neutrino-GUI  -   DBoxII-Project

	NFSMount/Umount GUI by Zwen
	
	Homepage: http://dbox.cyberphoria.org/

	Kommentar:

	Diese GUI wurde von Grund auf neu programmiert und sollte nun vom
	Aufbau und auch den Ausbaumoeglichkeiten gut aussehen. Neutrino basiert
	auf der Client-Server Idee, diese GUI ist also von der direkten DBox-
	Steuerung getrennt. Diese wird dann von Daemons uebernommen.


	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <sys/mount.h>
#include <fstream>
#include <global.h>

#include "nfs.h"
#include "widget/menue.h"
#include "widget/hintbox.h"
#include "widget/stringinput.h"
#include "widget/stringinput_ext.h"


int CNFSMountGui::exec( CMenuTarget* parent, string actionKey )
{
//	printf("exec: %s\n", actionKey.c_str());
	int returnval = menu_return::RETURN_REPAINT;

	if(actionKey=="")
	{
		parent->hide();
		for(int i=0 ; i< 4; i++)
		{
			string a;
			if(g_settings.network_nfs_automount[i])
				a=g_Locale->getText("messagebox.yes");
			else
				a=g_Locale->getText("messagebox.no");
			sprintf(m_entry[i],"%s:%s -> %s auto: %4s",g_settings.network_nfs_ip[i],g_settings.network_nfs_dir[i], 
					  g_settings.network_nfs_local_dir[i], a.c_str());
		}
      returnval = menu();
   }
	else if(actionKey.substr(0,10)=="mountentry")
	{
		parent->hide();
      returnval = menuEntry(actionKey[10]-'0');
		for(int i=0 ; i< 4; i++)
		{
			string a;
			if(g_settings.network_nfs_automount[i])
				a=g_Locale->getText("messagebox.yes");
			else
				a=g_Locale->getText("messagebox.no");
			sprintf(m_entry[i],"%s:%s -> %s auto: %s",g_settings.network_nfs_ip[i],g_settings.network_nfs_dir[i], 
					  g_settings.network_nfs_local_dir[i], a.c_str());
		}
   }
	else if(actionKey.substr(0,7)=="domount")
	{
		int nr=atoi(actionKey.substr(7,1).c_str());
		mount(g_settings.network_nfs_ip[nr], g_settings.network_nfs_dir[nr], g_settings.network_nfs_local_dir[nr],true);
		returnval = menu_return::RETURN_EXIT;
	}
	return returnval;
}

int CNFSMountGui::menu()
{
	CMenuWidget mountMenuW("nfs.mount", "network.raw", 720);
	mountMenuW.addItem(new CMenuSeparator()); 
	mountMenuW.addItem(new CMenuForwarder("menu.back")); 
	mountMenuW.addItem(new CMenuSeparator(CMenuSeparator::LINE));
	string s1;
	char s2[12];
	for(int i=0 ; i < 4 ; i++)
	{
		sprintf(s2,"mountentry%d",i);
		mountMenuW.addItem(new CMenuForwarder("", true, m_entry[i], this, s2));
	}
	int ret=mountMenuW.exec(this,"");
	return ret;
}

int CNFSMountGui::menuEntry(int nr)
{
	char *dir,*local_dir,*ip;
	int* automount;
	char cmd[9];

	ip = g_settings.network_nfs_ip[nr];
	dir = g_settings.network_nfs_dir[nr];
	local_dir = g_settings.network_nfs_local_dir[nr];
	automount=&g_settings.network_nfs_automount[nr];
	sprintf(cmd,"domount%d",nr);

	CMenuWidget mountMenuEntryW("nfs.mount", "network.raw",720);
	mountMenuEntryW.addItem(new CMenuSeparator()); 
	mountMenuEntryW.addItem(new CMenuForwarder("menu.back")); 
	mountMenuEntryW.addItem(new CMenuSeparator(CMenuSeparator::LINE));
	CIPInput  ipInput("nfs.ip", ip);
	mountMenuEntryW.addItem(new CMenuForwarder("nfs.ip", true, ip, &ipInput));
	CStringInputSMS  dirInput("nfs.dir", dir, 30,"","","abcdefghijklmnopqrstuvwxyz0123456789-.,:|!?/ "); 
	mountMenuEntryW.addItem(new CMenuForwarder("nfs.dir", true, dir, &dirInput));
	CStringInputSMS  localDirInput("nfs.localdir", local_dir, 30,"","","abcdefghijklmnopqrstuvwxyz0123456789-.,:|!?/ "); 
	mountMenuEntryW.addItem(new CMenuForwarder("nfs.localdir", true, local_dir, &localDirInput)); 
	CMenuOptionChooser *automountInput= new CMenuOptionChooser("nfs.automount", automount, true); 
	automountInput->addOption(0, "messagebox.no");
	automountInput->addOption(1, "messagebox.yes"); 
	mountMenuEntryW.addItem(automountInput);
	mountMenuEntryW.addItem(new CMenuForwarder("nfs.mountnow", true, NULL, this, cmd));

	int ret = mountMenuEntryW.exec(this,"");
	return ret;
}

void CNFSMountGui::mount(const char* ip, const char* dir, const char* local_dir, bool showerror)
{
	char buffer[200+1],mountDev[100],mountOn[100],mountType[20];
	printf("Mount %s:%s -> %s\n",ip,dir,local_dir);

	ifstream in;
	in.open("/proc/mounts",ifstream::in);
	while(in.good())
   {
		strcpy(mountDev,"");
		strcpy(mountOn,"");
		strcpy(mountType,"");
		in.getline(buffer, 200);
		sscanf(buffer,"%s %s %s ", mountDev, mountOn, mountType);
		if(strcmp(mountOn,local_dir)==0)
	  	{
			if(showerror)
				ShowHint ( "messagebox.info",  g_Locale->getText("nfs.alreadymounted"));
			printf("[neutrino]: NFS mount error %s already mounted\n", local_dir);
			in.close();
			return;
		}
	}
	in.close();

	string cmd=string("mount -t nfs ") + ip + ":" + dir + " " + local_dir + " -o ro,nolock,rsize=8192,soft,udp";
	if (system(cmd.c_str())!=0)
	{
		if(showerror)
			ShowHint ( "messagebox.info",  g_Locale->getText("nfs.mounterror"));
		printf("[neutrino]: NFS mount error: \"%s\"\n", cmd.c_str());
	}

}

void CNFSMountGui::automount()
{
	if(g_settings.networkSetOnStartup)
	{
		for(int i=0 ; i < 4 ;i++)
		{
			if(g_settings.network_nfs_automount[i])
				mount(g_settings.network_nfs_ip[i], g_settings.network_nfs_dir[i], g_settings.network_nfs_local_dir[i], false);
		}
	}
}

int CNFSUmountGui::exec( CMenuTarget* parent, string actionKey )
{
	//	printf("ac: %s\n", actionKey.c_str());
	int returnval = menu_return::RETURN_REPAINT;

	if(actionKey=="")
	{
		parent->hide();
      returnval = menu();
   }
	else if(actionKey.substr(0,8)=="doumount")
	{
      umount(actionKey.substr(9));
		returnval = menu_return::RETURN_EXIT;
	}
	return returnval;
}
int CNFSUmountGui::menu()
{
	char buffer[200+1],mountDev[100],mountOn[100],mountType[20];
	ifstream in;
	in.open("/proc/mounts",ifstream::in);
	int count=0;
	CMenuWidget umountMenu("nfs.umount", "network.raw",720);
	umountMenu.addItem(new CMenuSeparator()); 
	umountMenu.addItem(new CMenuForwarder("menu.back")); 
	umountMenu.addItem(new CMenuSeparator(CMenuSeparator::LINE));
	while(in.good())
	{
		strcpy(mountDev,"");
		strcpy(mountOn,"");
		strcpy(mountType,"");
		in.getline(buffer, 200);
		sscanf(buffer,"%s %s %s ", mountDev, mountOn, mountType);
		if(strcmp(mountType,"nfs")==0)
		{
			count++;
			string s1=string(mountDev) + " -> " + mountOn;
			string s2=string("doumount ") + mountOn;
			umountMenu.addItem(new CMenuForwarder(s1, true, NULL, this, s2));
		}
	}
	in.close();
	if(count > 0)
		return umountMenu.exec(this,"");
	else
		return menu_return::RETURN_REPAINT;
}


void CNFSUmountGui::umount(string dir)
{
	if(dir!= "")
	if(umount2(dir.c_str(),MNT_FORCE)!=0)
		ShowHint ( "messagebox.info", g_Locale->getText("nfs.umounterror") );
	else
	{
		char buffer[200+1],mountDev[100],mountOn[100],mountType[20];
		ifstream in;
		in.open("/proc/mounts",ifstream::in);
		while(in.good())
		{
			strcpy(mountDev,"");
			strcpy(mountOn,"");
			strcpy(mountType,"");
			in.getline(buffer, 200);
			sscanf(buffer,"%s %s %s ", mountDev, mountOn, mountType);
			if(strcmp(mountType,"nfs")==0 && strcmp(mountOn,"/")==0)
			{
				if(umount2(mountOn,MNT_FORCE)!=0)
					printf("[neutrino]: Error umounting %s\n",mountDev);
			}
		}
	}
}

