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

#include <config.h>
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
	CNFSMountGuiNotifier( CMenuForwarder* a3, CMenuForwarder* a4 , int* type)
	{
		m_user = a3;
		m_pass = a4;
		m_type = type;
	}
	bool changeNotify(std::string OptionName, void* dummy)
	{
		if(*m_type == (int)CNFSMountGui::NFS)
		{
			m_user->setActive (false);
			m_pass->setActive (false);
		}
		else
		{
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
	m_nfs_sup = fsSupported(CNFSMountGui::NFS);
	m_cifs_sup = fsSupported(CNFSMountGui::CIFS);
	printf("SUPPORT: NFS: %d, CIFS: %d\n", m_nfs_sup, m_cifs_sup);
}

bool in_proc_filesystems(const char * const fsname)
{
	std::string s;
	std::string t;
	std::ifstream in("/proc/filesystems", ifstream::in);

	t = fsname;
	
	while (in >> s)
	{
		if (s == t)
	  	{
			in.close();
			return true;
		}
	}
	in.close();
	return false;
}

bool insert_modules(const CNFSMountGui::FSType fstype)
{
	if (fstype == CNFSMountGui::NFS)
	{
#ifdef HAVE_MODPROBE
		return (system("modprobe nfs") == 0);
#else
		return ((system("insmod sunrpc") == 0) && (system("insmod lockd") == 0) && (system("insmod nfs") == 0));
#endif
	}
	return false;
}

static bool nfs_mounted_once = false;

bool remove_modules(const CNFSMountGui::FSType fstype)
{
	if (fstype == CNFSMountGui::NFS)
	{
		return ((system("rmmod nfs") == 0) && (system("rmmod lockd") == 0) && (system("rmmod sunrpc") == 0));
	}
	return false;
}

CNFSMountGui::FS_Support CNFSMountGui::fsSupported(const CNFSMountGui::FSType fstype, const bool keep_modules)
{
	const char * fsname;

	if (fstype == CNFSMountGui::NFS)
		fsname = "nfs";
	else /* if (fstype == CNFSMountGui::CIFS) */
		fsname = "cifs";

	if (in_proc_filesystems(fsname))
		return FS_READY;

	if (insert_modules(fstype))
	{
		if (in_proc_filesystems(fsname))
		{
			if (keep_modules)
			{
				if (fstype == CNFSMountGui::NFS)
					nfs_mounted_once = true;
			}
			else
			{
				remove_modules(fstype);
			}

			return FS_NEEDS_MODULES;
		}
	}
	remove_modules(fstype);
	return FS_UNSUPPORTED;
}

int CNFSMountGui::exec( CMenuTarget* parent, std::string actionKey )
{
//	printf("exec: %s\n", actionKey.c_str());
	int returnval = menu_return::RETURN_REPAINT;

	if (actionKey.empty())
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
		std::string startdir=g_settings.network_nfs_local_dir[nr];
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
	std::string s1;
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
      if(m_cifs_sup != FS_UNSUPPORTED && m_nfs_sup==FS_UNSUPPORTED)
         *type = (int) CIFS;
   }
   bool typeEnabled = (m_cifs_sup != FS_UNSUPPORTED && m_nfs_sup != FS_UNSUPPORTED) || 
      (m_cifs_sup != FS_UNSUPPORTED && *type != (int)CIFS) || 
      (m_nfs_sup != FS_UNSUPPORTED && *type != (int)NFS);

	CMenuWidget mountMenuEntryW("nfs.mount", "network.raw",720);
	mountMenuEntryW.addItem(new CMenuSeparator());
	mountMenuEntryW.addItem(new CMenuForwarder("menu.back"));
	mountMenuEntryW.addItem(new CMenuSeparator(CMenuSeparator::LINE));
	CIPInput ipInput("nfs.ip", g_settings.network_nfs_ip[nr]);
	CStringInputSMS dirInput("nfs.dir", dir, 30, NULL, NULL,"abcdefghijklmnopqrstuvwxyz0123456789-.,:|!?/ ");
	CMenuOptionChooser *automountInput= new CMenuOptionChooser("nfs.automount", automount, true);
	automountInput->addOption(0, "messagebox.no");
	automountInput->addOption(1, "messagebox.yes");
	CStringInputSMS options1("nfs.mount_options", g_settings.network_nfs_mount_options[0], 30, NULL, NULL, "abcdefghijklmnopqrstuvwxyz0123456789-=.,:|!?/ ");
	CMenuForwarder *options1_fwd = new CMenuForwarder("nfs.mount_options", true, g_settings.network_nfs_mount_options[0], &options1);
	CStringInputSMS options2("nfs.mount_options", g_settings.network_nfs_mount_options[1], 30, NULL, NULL, "abcdefghijklmnopqrstuvwxyz0123456789-=.,:|!?/ ");
	CMenuForwarder *options2_fwd = new CMenuForwarder("nfs.mount_options", true, g_settings.network_nfs_mount_options[1], &options2);
	CStringInputSMS userInput("nfs.username", username, 30, NULL, NULL,"abcdefghijklmnopqrstuvwxyz0123456789-.,:|!?/ ");
	CMenuForwarder *username_fwd = new CMenuForwarder("nfs.username", *type==CIFS, username, &userInput);
	CStringInputSMS passInput("nfs.password", password, 30, NULL, NULL,"abcdefghijklmnopqrstuvwxyz0123456789-.,:|!?/ ");
	CMenuForwarder *password_fwd = new CMenuForwarder("nfs.password", *type==CIFS, "", &passInput);
	CNFSMountGuiNotifier notifier(username_fwd, password_fwd, type);
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

void CNFSMountGui::mount(const char * const ip, const char * const dir, const char* local_dir, const FSType fstype, const char * const username, const char * const password, const bool showerror)
{
	char buffer[200+1],mountDev[100],mountOn[100],mountType[20];
	std::string cmd;
	pthread_mutex_init(&g_mut, NULL);
	pthread_cond_init(&g_cond, NULL);
	g_mntstatus=-1;

	FS_Support sup = fsSupported(fstype, true); /* keep modules if necessary */

	if (sup == FS_UNSUPPORTED)
	{
		printf("FS type %d not supported\n", (int) fstype);
		ShowHintUTF("messagebox.info", (std::string(g_Locale->getText("nfs.mounterror_notsup")) + ((fstype == NFS) ? " (NFS)" : " (CIFS)")).c_str()); // UTF-8
		return;
	}

	printf("Mount(%d) %s:%s -> %s\n", (int) fstype, ip, dir, local_dir);

	std::ifstream in;
	in.open("/proc/mounts", std::ifstream::in);
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

   if(g_settings.network_nfs_mount_options[0][0] == '\0')
   {
      strcpy(g_settings.network_nfs_mount_options[0],g_settings.network_nfs_mount_options[1]);
      g_settings.network_nfs_mount_options[1][0] = '\0';
   }

   if((g_settings.network_nfs_mount_options[0][0] == '\0') && (g_settings.network_nfs_mount_options[1][0] == '\0'))
   {
      if(fstype == NFS)
      {
         strcpy(g_settings.network_nfs_mount_options[0],"ro,soft,udp");
         strcpy(g_settings.network_nfs_mount_options[1],"nolock,rsize=8192,wsize=8192");
      }
      else if(fstype == CIFS)
      {
         strcpy(g_settings.network_nfs_mount_options[0],"ro");
         strcpy(g_settings.network_nfs_mount_options[1],"");
      }
   }
	
   if(fstype == NFS)
	{
      cmd = std::string("mount -t nfs ") + ip + ":" + dir + " " + local_dir + " -o " + 
         g_settings.network_nfs_mount_options[0];
	}
	else
	{
		cmd = std::string("mount -t cifs //") + ip + "/" + dir + " " + local_dir + 
         " -o username=" +  username + ",password=" + password + 
         ",unc=//" + ip + "/" + dir + "," + g_settings.network_nfs_mount_options[0];
	}
   if(g_settings.network_nfs_mount_options[1][0] !='\0')
      cmd = cmd + "," + g_settings.network_nfs_mount_options[1];

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
		if (showerror)
			ShowHintUTF("messagebox.info", g_Locale->getText((retcode == ETIMEDOUT) ? "nfs.mounttimeout" : "nfs.mounterror")); // UTF-8
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

int CNFSUmountGui::exec( CMenuTarget* parent, std::string actionKey )
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
			std::string s1=std::string(mountDev) + " -> " + mountOn;
			std::string s2=std::string("doumount ") + mountOn;
			umountMenu.addItem(new CMenuForwarder(s1.c_str(), true, NULL, this, s2));
		}
	}
	in.close();
	if(count > 0)
		return umountMenu.exec(this,"");
	else
		return menu_return::RETURN_REPAINT;
}


void CNFSUmountGui::umount(const std::string dir)
{
	if (dir != "")
	{
		if (umount2(dir.c_str(), MNT_FORCE) != 0)
		{
			ShowHintUTF("messagebox.info", g_Locale->getText("nfs.umounterror")); // UTF-8
			return;
		}
	}
	else
	{
		char buffer[200+1],mountDev[100],mountOn[100],mountType[20];
		std::ifstream in;
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
				if (umount2(mountOn,MNT_FORCE) != 0)
					printf("[neutrino]: Error umounting %s\n",mountDev);
			}
		}
	}
	if (nfs_mounted_once)
		remove_modules(CNFSMountGui::NFS);
}

int CNFSSmallMenu::exec( CMenuTarget* parent, std::string actionKey )
{
	CMenuWidget menu("nfsmenu.head", "network.raw");
	CNFSMountGui mountGui;
	CNFSUmountGui umountGui;
	menu.addItem(new CMenuSeparator());
	menu.addItem(new CMenuForwarder("menu.back"));
	menu.addItem(new CMenuSeparator(CMenuSeparator::LINE));
	menu.addItem( new CMenuForwarder("nfs.mount", true, "", &mountGui));
	menu.addItem( new CMenuForwarder("nfs.umount", true, "", &umountGui));
	return menu.exec(parent, actionKey);
}

