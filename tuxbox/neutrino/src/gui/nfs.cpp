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
#include <unistd.h>
#include <fstream>
#include <global.h>
#include <pthread.h>

#include "nfs.h"
#include "filebrowser.h"
#include "widget/menue.h"
#include "widget/hintbox.h"
#include "widget/stringinput.h"
#include "widget/stringinput_ext.h"

pthread_mutex_t g_mut;
pthread_cond_t g_cond;
pthread_t g_mnt;
int g_mntstatus;

class CNFSMountGuiNotifier : public CChangeObserver
{
private:
	CMenuForwarder *m_opt1,*m_opt2, *m_user, *m_pass;
	int *m_type;
public:
	CNFSMountGuiNotifier( CMenuForwarder* a1, CMenuForwarder* a2, CMenuForwarder* a3, CMenuForwarder* a4 , int* type)
	{
		m_opt1 = a1;
		m_opt2 = a2;
		m_user = a3;
		m_pass = a4;
		m_type = type;
	}
	bool changeNotify(string OptionName, void* dummy)
	{
		if(*m_type == (int)CNFSMountGui::NFS)
		{
			m_opt1->setActive (true);
			m_opt2->setActive (true);
			m_user->setActive (false);
			m_pass->setActive (false);
		}
		else
		{
			m_opt1->setActive (false);
			m_opt2->setActive (false);
			m_user->setActive (true);
			m_pass->setActive (true);
		}
		return true;
	}
};

void *mount_thread(void* cmd)
{
	int ret;
	ret=system((char*)cmd);
	pthread_mutex_lock(&g_mut);
	g_mntstatus=ret;
	pthread_cond_broadcast(&g_cond);
	pthread_mutex_unlock(&g_mut);
	pthread_exit(NULL);
}

CNFSMountGui::CNFSMountGui()
{
   m_cifs_sup = fsSupported(CIFS);
   m_nfs_sup = fsSupported(NFS);
}

bool CNFSMountGui::fsSupported(FSType fstype)
{
   char fsname[10];
   if(fstype==NFS)
   {
      strcpy(fsname,"nfs");
   }
   else if(fstype==CIFS)
   {
      strcpy(fsname,"cifs");
   }
   else
   {
      return false;
   }
	
   ifstream in;
   char buffer[100];
	in.open("/proc/filesystems",ifstream::in);
	while(in.good())
	{
		in.getline(buffer, 100);
		if(strstr(buffer,fsname)!=NULL)
	  	{
         in.close();
			return true;
		}
	}
	in.close();
   return false;
}

int CNFSMountGui::exec( CMenuTarget* parent, string actionKey )
{
//	printf("exec: %s\n", actionKey.c_str());
	int returnval = menu_return::RETURN_REPAINT;

	if(actionKey=="")
	{
		parent->hide();
		for(int i=0 ; i< 4; i++)
		{
			std::string a = CZapitClient::Utf8_to_Latin1(g_Locale->getText(g_settings.network_nfs_automount[i] ? "messagebox.yes" : "messagebox.no"));
			if(g_settings.network_nfs_type[i] == (int) NFS)
			{
				sprintf(m_entry[i],"NFS %s:%s -> %s auto: %4s",g_settings.network_nfs_ip[i].c_str(),g_settings.network_nfs_dir[i],
						  g_settings.network_nfs_local_dir[i], a.c_str());
			}
			else
			{
				sprintf(m_entry[i],"CIFS //%s/%s -> %s auto: %4s",g_settings.network_nfs_ip[i].c_str(),g_settings.network_nfs_dir[i],
						  g_settings.network_nfs_local_dir[i], a.c_str());
			}
		}
		returnval = menu();
	}
	else if(actionKey.substr(0,10)=="mountentry")
	{
		parent->hide();
		returnval = menuEntry(actionKey[10]-'0');
		for(int i=0 ; i< 4; i++)
		{
			std::string a = CZapitClient::Utf8_to_Latin1(g_Locale->getText(g_settings.network_nfs_automount[i] ? "messagebox.yes" : "messagebox.no"));
			if(g_settings.network_nfs_type[i] == (int) NFS)
			{
				sprintf(m_entry[i],"NFS %s:%s -> %s auto: %4s",g_settings.network_nfs_ip[i].c_str(),g_settings.network_nfs_dir[i],
						  g_settings.network_nfs_local_dir[i], a.c_str());
			}
			else
			{
				sprintf(m_entry[i],"CIFS //%s/%s -> %s auto: %4s",g_settings.network_nfs_ip[i].c_str(),g_settings.network_nfs_dir[i],
						  g_settings.network_nfs_local_dir[i], a.c_str());
			}
		}
	}
	else if(actionKey.substr(0,7)=="domount")
	{
		int nr=atoi(actionKey.substr(7,1).c_str());
		mount(g_settings.network_nfs_ip[nr].c_str(), g_settings.network_nfs_dir[nr], 
				g_settings.network_nfs_local_dir[nr], (FSType) g_settings.network_nfs_type[nr],
				g_settings.network_nfs_username[nr], g_settings.network_nfs_password[nr], true);
		returnval = menu_return::RETURN_EXIT;
	}
	else if(actionKey.substr(0,3)=="dir")
	{
		parent->hide();
		int nr=atoi(actionKey.substr(3,1).c_str());
		CFileBrowser b;
		b.Dir_Mode=true;
		string startdir=g_settings.network_nfs_local_dir[nr];
		if (b.exec(startdir))
			strcpy(g_settings.network_nfs_local_dir[nr], b.getSelectedFile()->Name.c_str());
		returnval = menu_return::RETURN_REPAINT;
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
	char *dir,*local_dir, *username, *password;
	int* automount;
	int* type;
	char cmd[9];
	char cmd2[9];

	dir = g_settings.network_nfs_dir[nr];
	local_dir = g_settings.network_nfs_local_dir[nr];
	username = g_settings.network_nfs_username[nr];
	password = g_settings.network_nfs_password[nr];
	automount = &g_settings.network_nfs_automount[nr];
	type = &g_settings.network_nfs_type[nr];
	
	sprintf(cmd,"domount%d",nr);
	sprintf(cmd2,"dir%d",nr);

   /* rewrite fstype in new entries */
   if(strlen(local_dir)==0)
   {
      if(m_cifs_sup && !m_nfs_sup)
         *type = (int) CIFS;
   }
   bool typeEnabled = (m_cifs_sup && m_nfs_sup) || (m_cifs_sup && *type != (int)CIFS)
                                                || (m_nfs_sup  && *type != (int)NFS);

	CMenuWidget mountMenuEntryW("nfs.mount", "network.raw",720);
	mountMenuEntryW.addItem(new CMenuSeparator());
	mountMenuEntryW.addItem(new CMenuForwarder("menu.back"));
	mountMenuEntryW.addItem(new CMenuSeparator(CMenuSeparator::LINE));
	CIPInput  ipInput("nfs.ip", g_settings.network_nfs_ip[nr]);
	CStringInputSMS  dirInput("nfs.dir", dir, 30,"","","abcdefghijklmnopqrstuvwxyz0123456789-.,:|!?/ ");
	CMenuOptionChooser *automountInput= new CMenuOptionChooser("nfs.automount", automount, true);
	automountInput->addOption(0, "messagebox.no");
	automountInput->addOption(1, "messagebox.yes");
	CStringInputSMS options1("nfs.mount_options", g_settings.network_nfs_mount_options[0], 30,"","","abcdefghijklmnopqrstuvwxyz0123456789-=.,:|!?/ ");
	CMenuForwarder *options1_fwd = new CMenuForwarder("nfs.mount_options", *type==NFS, g_settings.network_nfs_mount_options[0], &options1);
	CStringInputSMS options2("nfs.mount_options", g_settings.network_nfs_mount_options[1], 30,"","","abcdefghijklmnopqrstuvwxyz0123456789-=.,:|!?/ ");
	CMenuForwarder *options2_fwd = new CMenuForwarder("nfs.mount_options", *type==NFS, g_settings.network_nfs_mount_options[1], &options2);
	CStringInputSMS  userInput("nfs.username", username, 30,"","","abcdefghijklmnopqrstuvwxyz0123456789-.,:|!?/ ");
	CMenuForwarder *username_fwd = new CMenuForwarder("nfs.username", *type==CIFS, username, &userInput);
	CStringInputSMS  passInput("nfs.password", password, 30,"","","abcdefghijklmnopqrstuvwxyz0123456789-.,:|!?/ ");
	CMenuForwarder *password_fwd = new CMenuForwarder("nfs.password", *type==CIFS, "", &passInput);
	CNFSMountGuiNotifier notifier(options1_fwd, options2_fwd, username_fwd, password_fwd, type);
	CMenuOptionChooser *typeInput= new CMenuOptionChooser("nfs.type", type, typeEnabled, &notifier);
	typeInput->addOption((int) NFS, "nfs.type_nfs");
	typeInput->addOption((int) CIFS, "nfs.type_cifs");
	mountMenuEntryW.addItem(typeInput);
	mountMenuEntryW.addItem(new CMenuForwarder("nfs.ip", true, g_settings.network_nfs_ip[nr], &ipInput));
	mountMenuEntryW.addItem(new CMenuForwarder("nfs.dir", true, dir, &dirInput));
	mountMenuEntryW.addItem(new CMenuForwarder("nfs.localdir", true, local_dir, this, cmd2));
	mountMenuEntryW.addItem(automountInput);
	mountMenuEntryW.addItem(options1_fwd);
	mountMenuEntryW.addItem(options2_fwd);
	mountMenuEntryW.addItem(username_fwd);
	mountMenuEntryW.addItem(password_fwd);
	mountMenuEntryW.addItem(new CMenuForwarder("nfs.mountnow", true, NULL, this, cmd));

	int ret = mountMenuEntryW.exec(this,"");
	return ret;
}

void CNFSMountGui::mount(const char* ip, const char* dir, const char* local_dir, 
								 FSType fstype, const char* username, const char* password, bool showerror)
{
	char buffer[200+1],mountDev[100],mountOn[100],mountType[20];
	string cmd;
	pthread_mutex_init(&g_mut, NULL);
   pthread_cond_init(&g_cond, NULL);
	g_mntstatus=-1;

   bool cifs_sup = fsSupported(CIFS);
   bool nfs_sup = fsSupported(NFS);
	
   if(fstype==NFS)
   {
      if(!nfs_sup)
      {
         ShowHintUTF("messagebox.info", g_Locale->getText("nfs.mounterror_notsup") + " (NFS)"); // UTF-8
         printf("FS type %d not supported\n", (int) fstype);
         return;
      }
      printf("NFS-Mount %s:%s -> %s\n",ip,dir,local_dir);
   }
   else if(fstype==CIFS)
   {
      if(!cifs_sup)
      {
         ShowHintUTF("messagebox.info", g_Locale->getText("nfs.mounterror_notsup") + " (CIFS)"); // UTF-8
         printf("FS type %d not supported\n", (int) fstype);
         return;
      }
		printf("CIFS-Mount //%s/%s -> %s\n",ip,dir,local_dir);
   }
   else
   {
      printf("Unknown FS type %d\n", (int) fstype);
   }

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
				ShowHintUTF( "messagebox.info",  g_Locale->getText("nfs.alreadymounted")); // UTF-8
			printf("[neutrino]: NFS mount error %s already mounted\n", local_dir);
			in.close();
			return;
		}
	}
	in.close();

	if(fstype == NFS)
	{
		if(g_settings.network_nfs_mount_options[0][0] == '\0')
		{
			strcpy(g_settings.network_nfs_mount_options[0],g_settings.network_nfs_mount_options[1]);
			g_settings.network_nfs_mount_options[1][0] = '\0';
		}

		if((g_settings.network_nfs_mount_options[0][0] == '\0') && (g_settings.network_nfs_mount_options[1][0] == '\0'))
		{
			strcpy(g_settings.network_nfs_mount_options[0],"ro,soft,udp");
			strcpy(g_settings.network_nfs_mount_options[1],"nolock,rsize=8192,wsize=8192");
		}

		cmd = string("mount -t nfs ") + ip + ":" + dir + " " + local_dir + " -o " + g_settings.network_nfs_mount_options[0];
		if(g_settings.network_nfs_mount_options[1][0] !='\0')
			cmd = cmd + "," + g_settings.network_nfs_mount_options[1];
	}
	else
	{
		cmd = string("mount -t cifs //") + ip + "/" + dir + " " + local_dir + " -o username=" +  username +
			",password=" + password + ",unc=//" + ip + "/" + dir; 
	}

	sprintf(buffer,"%s",cmd.c_str());
	pthread_create(&g_mnt, 0, mount_thread, buffer);

	struct timespec timeout;
	int retcode;

	pthread_mutex_lock(&g_mut);
	timeout.tv_sec = time(NULL) + 5;
	timeout.tv_nsec = 0;
	retcode = pthread_cond_timedwait(&g_cond, &g_mut, &timeout);
	if (retcode == ETIMEDOUT) 
   {  // timeout occurred
		pthread_cancel(g_mnt);
	}
	pthread_mutex_unlock(&g_mut);

	if ( g_mntstatus != 0 )
	{
		if(showerror)
			if(retcode == ETIMEDOUT)
				ShowHintUTF("messagebox.info", g_Locale->getText("nfs.mounttimeout")); // UTF-8
			else
				ShowHintUTF("messagebox.info", g_Locale->getText("nfs.mounterror")); // UTF-8
		strcpy(g_settings.network_nfs_mount_options[0],"ro,soft,udp");
		strcpy(g_settings.network_nfs_mount_options[1],"nolock,rsize=8192,wsize=8192");
		printf("[neutrino]: NFS mount error: \"%s\"\n", cmd.c_str());
	}

}

void CNFSMountGui::automount()
{
	for(int i = 0; i < 4; i++)
	{
		if(g_settings.network_nfs_automount[i])
			mount(g_settings.network_nfs_ip[i].c_str(), g_settings.network_nfs_dir[i], g_settings.network_nfs_local_dir[i], 
					(FSType) g_settings.network_nfs_type[i], g_settings.network_nfs_username[i], 
					g_settings.network_nfs_password[i], false);
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
		if(strcmp(mountType,"nfs")==0 || strcmp(mountType,"cifs")==0)
		{
			count++;
			string s1=string(mountDev) + " -> " + mountOn;
			string s2=string("doumount ") + mountOn;
			umountMenu.addItem(new CMenuForwarder(s1.c_str(), true, NULL, this, s2));
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
		ShowHintUTF("messagebox.info", g_Locale->getText("nfs.umounterror")); // UTF-8
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

CNFSSmallMenu::CNFSSmallMenu()
{
	m_menu = new CMenuWidget("nfsmenu.head", "network.raw");
	m_mountGui = new CNFSMountGui();
	m_umountGui = new CNFSUmountGui();
	m_menu->addItem(new CMenuSeparator());
	m_menu->addItem(new CMenuForwarder("menu.back"));
	m_menu->addItem(new CMenuSeparator(CMenuSeparator::LINE));
	m_menu->addItem( new CMenuForwarder("nfs.mount", true, "", m_mountGui));
	m_menu->addItem( new CMenuForwarder("nfs.umount", true, "", m_umountGui));
}
CNFSSmallMenu::~CNFSSmallMenu()
{
	delete m_menu;
	delete m_mountGui;
	delete m_umountGui;
}
int CNFSSmallMenu::exec( CMenuTarget* parent, string actionKey )
{
	return m_menu->exec(parent, actionKey);
}
