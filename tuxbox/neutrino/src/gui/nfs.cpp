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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gui/nfs.h>

#include <gui/filebrowser.h>
#include <gui/widget/menue.h>
#include <gui/widget/hintbox.h>
#include <gui/widget/stringinput.h>
#include <gui/widget/stringinput_ext.h>

#include <sys/mount.h>
#include <unistd.h>
#include <fstream>
#include <global.h>
#include <pthread.h>

#include <asm/errno.h>

#include <zapit/client/zapittools.h>

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
	bool changeNotify(const neutrino_locale_t OptionName, void *)
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
	ret=system((const char *) cmd);
	pthread_mutex_lock(&g_mut);
	g_mntstatus=ret;
	pthread_cond_broadcast(&g_cond);
	pthread_mutex_unlock(&g_mut);
	pthread_exit(NULL);
}

CNFSMountGui::CNFSMountGui()
{
	m_nfs_sup = CNFSMountGui::FS_UNPROBED;
	m_cifs_sup = CNFSMountGui::FS_UNPROBED;
}

bool in_proc_filesystems(const char * const fsname)
{
	std::string s;
	std::string t;
	std::ifstream in("/proc/filesystems", std::ifstream::in);

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
	else if (fstype == CNFSMountGui::CIFS)
		return (system("insmod cifs") == 0);
	return false;
}

bool nfs_mounted_once = false;

bool remove_modules(const CNFSMountGui::FSType fstype)
{
	if (fstype == CNFSMountGui::NFS)
	{
		return ((system("rmmod nfs") == 0) && (system("rmmod lockd") == 0) && (system("rmmod sunrpc") == 0));
	}
	else if (fstype == CNFSMountGui::CIFS)
		return (system("rmmod cifs") == 0);
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
		return CNFSMountGui::FS_READY;

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

			return CNFSMountGui::FS_NEEDS_MODULES;
		}
	}
	remove_modules(fstype);
	return CNFSMountGui::FS_UNSUPPORTED;
}

const char * nfs_entry_printf_string[2] =
{
	"NFS %s:%s -> %s auto: %4s",
	"CIFS //%s/%s -> %s auto: %4s"
};

int CNFSMountGui::exec( CMenuTarget* parent, const std::string & actionKey )
{
//	printf("exec: %s\n", actionKey.c_str());
	int returnval = menu_return::RETURN_REPAINT;
	
	if (m_nfs_sup == CNFSMountGui::FS_UNPROBED)
		m_nfs_sup = fsSupported(CNFSMountGui::NFS);

	if (m_cifs_sup == CNFSMountGui::FS_UNPROBED)
		m_cifs_sup = fsSupported(CNFSMountGui::CIFS);

	printf("SUPPORT: NFS: %d, CIFS: %d\n", m_nfs_sup, m_cifs_sup);

	if (actionKey.empty())
	{
		parent->hide();
		for(int i=0 ; i< 4; i++)
		{
			sprintf(m_entry[i],
				nfs_entry_printf_string[(g_settings.network_nfs_type[i] == (int) NFS) ? 0 : 1],
				g_settings.network_nfs_ip[i].c_str(),
#ifdef FILESYSTEM_IS_ISO8859_1_ENCODED
				ZapitTools::Latin1_to_UTF8(g_settings.network_nfs_dir[i]).c_str(),
				ZapitTools::Latin1_to_UTF8(g_settings.network_nfs_local_dir[i]).c_str(),
#else
				g_settings.network_nfs_dir[i],
				g_settings.network_nfs_local_dir[i],
#endif
				g_Locale->getText(g_settings.network_nfs_automount[i] ? LOCALE_MESSAGEBOX_YES : LOCALE_MESSAGEBOX_NO));
		}
		returnval = menu();
	}
	else if(actionKey.substr(0,10)=="mountentry")
	{
		parent->hide();
		returnval = menuEntry(actionKey[10]-'0');
		for(int i=0 ; i< 4; i++)
		{
			sprintf(m_entry[i],
				nfs_entry_printf_string[(g_settings.network_nfs_type[i] == (int) NFS) ? 0 : 1],
				g_settings.network_nfs_ip[i].c_str(),
#ifdef FILESYSTEM_IS_ISO8859_1_ENCODED
				ZapitTools::Latin1_to_UTF8(g_settings.network_nfs_dir[i]).c_str(),
				ZapitTools::Latin1_to_UTF8(g_settings.network_nfs_local_dir[i]).c_str(),
#else
				g_settings.network_nfs_dir[i],
				g_settings.network_nfs_local_dir[i],
#endif
				g_Locale->getText(g_settings.network_nfs_automount[i] ? LOCALE_MESSAGEBOX_YES : LOCALE_MESSAGEBOX_NO));
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
	CMenuWidget mountMenuW(LOCALE_NFS_MOUNT, "network.raw", 720);
	mountMenuW.addItem(GenericMenuSeparator);
	mountMenuW.addItem(GenericMenuBack);
	mountMenuW.addItem(GenericMenuSeparatorLine);
	std::string ISO_8859_1_entry[4];
	char s2[12];

	for(int i=0 ; i < 4 ; i++)
	{
		sprintf(s2,"mountentry%d",i);
		ISO_8859_1_entry[i] = ZapitTools::UTF8_to_Latin1(m_entry[i]); 
		mountMenuW.addItem(new CMenuForwarderNonLocalized("", true, ISO_8859_1_entry[i].c_str(), this, s2));
	}
	int ret=mountMenuW.exec(this,"");
	return ret;
}

#warning MESSAGEBOX_NO_YES_XXX is defined in neutrino.cpp, too!
#define MESSAGEBOX_NO_YES_OPTION_COUNT 2
const CMenuOptionChooser::keyval MESSAGEBOX_NO_YES_OPTIONS[MESSAGEBOX_NO_YES_OPTION_COUNT] =
{
	{ 0, LOCALE_MESSAGEBOX_NO  },
	{ 1, LOCALE_MESSAGEBOX_YES }
};

#define NFS_TYPE_OPTION_COUNT 2
const CMenuOptionChooser::keyval NFS_TYPE_OPTIONS[NFS_TYPE_OPTION_COUNT] =
{
	{ CNFSMountGui::NFS , LOCALE_NFS_TYPE_NFS  },
	{ CNFSMountGui::CIFS, LOCALE_NFS_TYPE_CIFS }
};

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
      if(m_cifs_sup != CNFSMountGui::FS_UNSUPPORTED && m_nfs_sup == CNFSMountGui::FS_UNSUPPORTED)
         *type = (int) CIFS;
   }
   bool typeEnabled = (m_cifs_sup != CNFSMountGui::FS_UNSUPPORTED && m_nfs_sup != CNFSMountGui::FS_UNSUPPORTED) || 
      (m_cifs_sup != CNFSMountGui::FS_UNSUPPORTED && *type != (int)CIFS) || 
      (m_nfs_sup != CNFSMountGui::FS_UNSUPPORTED && *type != (int)NFS);

	CMenuWidget mountMenuEntryW(LOCALE_NFS_MOUNT, "network.raw",720);
	mountMenuEntryW.addItem(GenericMenuSeparator);
	mountMenuEntryW.addItem(GenericMenuBack);
	mountMenuEntryW.addItem(GenericMenuSeparatorLine);
	CIPInput ipInput(LOCALE_NFS_IP, g_settings.network_nfs_ip[nr], LOCALE_IPSETUP_HINT_1, LOCALE_IPSETUP_HINT_2);
	CStringInputSMS dirInput(LOCALE_NFS_DIR, dir, 30, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE,"abcdefghijklmnopqrstuvwxyz0123456789-.,:|!?/ ");
	CMenuOptionChooser *automountInput= new CMenuOptionChooser(LOCALE_NFS_AUTOMOUNT, automount, MESSAGEBOX_NO_YES_OPTIONS, MESSAGEBOX_NO_YES_OPTION_COUNT, true);
	CStringInputSMS options1(LOCALE_NFS_MOUNT_OPTIONS, g_settings.network_nfs_mount_options[0], 30, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "abcdefghijklmnopqrstuvwxyz0123456789-=.,:|!?/ ");
	CMenuForwarder *options1_fwd = new CMenuForwarder(LOCALE_NFS_MOUNT_OPTIONS, true, g_settings.network_nfs_mount_options[0], &options1);
	CStringInputSMS options2(LOCALE_NFS_MOUNT_OPTIONS, g_settings.network_nfs_mount_options[1], 30, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "abcdefghijklmnopqrstuvwxyz0123456789-=.,:|!?/ ");
	CMenuForwarder *options2_fwd = new CMenuForwarder(LOCALE_NFS_MOUNT_OPTIONS, true, g_settings.network_nfs_mount_options[1], &options2);
	CStringInputSMS userInput(LOCALE_NFS_USERNAME, username, 30, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "abcdefghijklmnopqrstuvwxyz0123456789-.,:|!?/ ");
	CMenuForwarder *username_fwd = new CMenuForwarder(LOCALE_NFS_USERNAME, *type==CIFS, username, &userInput);
	CStringInputSMS passInput(LOCALE_NFS_PASSWORD, password, 30, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "abcdefghijklmnopqrstuvwxyz0123456789-.,:|!?/ ");
	CMenuForwarder *password_fwd = new CMenuForwarder(LOCALE_NFS_PASSWORD, *type==CIFS, NULL, &passInput);
	CNFSMountGuiNotifier notifier(username_fwd, password_fwd, type);

	mountMenuEntryW.addItem(new CMenuOptionChooser(LOCALE_NFS_TYPE, type, NFS_TYPE_OPTIONS, NFS_TYPE_OPTION_COUNT, typeEnabled, &notifier));
	mountMenuEntryW.addItem(new CMenuForwarder(LOCALE_NFS_IP       , true, g_settings.network_nfs_ip[nr], &ipInput       ));
	mountMenuEntryW.addItem(new CMenuForwarder(LOCALE_NFS_DIR      , true, dir                          , &dirInput      ));
	mountMenuEntryW.addItem(new CMenuForwarder(LOCALE_NFS_LOCALEDIR, true, local_dir                    , this     , cmd2));
	mountMenuEntryW.addItem(automountInput);
	mountMenuEntryW.addItem(options1_fwd);
	mountMenuEntryW.addItem(options2_fwd);
	mountMenuEntryW.addItem(username_fwd);
	mountMenuEntryW.addItem(password_fwd);
	mountMenuEntryW.addItem(new CMenuForwarder(LOCALE_NFS_MOUNTNOW , true, NULL                         , this     , cmd ));

	int ret = mountMenuEntryW.exec(this,"");
	return ret;
}

void CNFSMountGui::mount(const char * const ip, const char * const dir, const char * const local_dir, const FSType fstype, const char * const username, const char * const password, const bool showerror)
{
	char buffer[200+1],mountDev[100],mountOn[100],mountType[20];
	std::string cmd;
	pthread_mutex_init(&g_mut, NULL);
	pthread_cond_init(&g_cond, NULL);
	g_mntstatus=-1;

	FS_Support sup = fsSupported(fstype, true); /* keep modules if necessary */

	if (sup == CNFSMountGui::FS_UNSUPPORTED)
	{
		printf("FS type %d not supported\n", (int) fstype);
		ShowHintUTF(LOCALE_MESSAGEBOX_INFO, (std::string(g_Locale->getText(LOCALE_NFS_MOUNTERROR_NOTSUP)) + ((fstype == NFS) ? " (NFS)" : " (CIFS)")).c_str()); // UTF-8
		return;
	}

	printf("Mount(%d) %s:%s -> %s\n", (int) fstype, ip, dir, local_dir);

	std::ifstream in;
	in.open("/proc/mounts", std::ifstream::in);
	while(in.good())
	{
		mountDev[0] = 0; /* strcpy(mountDev,""); */
		mountOn[0] = 0; /* strcpy(mountOn,""); */
		mountType[0] = 0; /* strcpy(mountType,""); */
		in.getline(buffer, 200);
		sscanf(buffer,"%s %s %s ", mountDev, mountOn, mountType);
		if(strcmp(mountOn,local_dir)==0)
	  	{
			if(showerror)
				ShowHintUTF(LOCALE_MESSAGEBOX_INFO,  g_Locale->getText(LOCALE_NFS_ALREADYMOUNTED)); // UTF-8
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
		cmd = "mount -t nfs ";
		cmd += ip;
		cmd += ':';
		cmd += dir;
		cmd += ' ';
		cmd += local_dir;
		cmd += " -o ";
		cmd += g_settings.network_nfs_mount_options[0];
	}
	else
	{
		cmd = "mount -t cifs //";
		cmd += ip;
		cmd += '/';
		cmd += dir;
		cmd += ' ';
		cmd += local_dir;
		cmd += " -o username=";
		cmd += username;
		cmd += ",password=";
		cmd += password;
		cmd += ",unc=//";
		cmd += ip;
		cmd += '/';
		cmd += dir;
		cmd += ',';
		cmd += g_settings.network_nfs_mount_options[0];
	}
	if (g_settings.network_nfs_mount_options[1][0] !='\0')
	{
		cmd += ',';
		cmd += g_settings.network_nfs_mount_options[1];
	}
	
	pthread_create(&g_mnt, 0, mount_thread, (void *) cmd.c_str());
	
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
			ShowHintUTF(LOCALE_MESSAGEBOX_INFO, g_Locale->getText((retcode == ETIMEDOUT) ? LOCALE_NFS_MOUNTTIMEOUT : LOCALE_NFS_MOUNTERROR)); // UTF-8
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

int CNFSUmountGui::exec( CMenuTarget* parent, const std::string & actionKey )
{
	//	printf("ac: %s\n", actionKey.c_str());
	int returnval;

	if (actionKey.empty())
	{
		parent->hide();
		returnval = menu();
	}
	else if(actionKey.substr(0,8)=="doumount")
	{
		umount((actionKey.substr(9)).c_str());
		returnval = menu_return::RETURN_EXIT;
	}
	else
		returnval = menu_return::RETURN_REPAINT;

	return returnval;
}
int CNFSUmountGui::menu()
{
	char buffer[200+1],mountDev[100],mountOn[100],mountType[20];
	std::ifstream in("/proc/mounts", std::ifstream::in);
	int count=0;
	CMenuWidget umountMenu(LOCALE_NFS_UMOUNT, "network.raw",720);
	umountMenu.addItem(GenericMenuSeparator);
	umountMenu.addItem(GenericMenuBack);
	umountMenu.addItem(GenericMenuSeparatorLine);
	while(in.good())
	{
		mountDev[0] = 0; /* strcpy(mountDev,""); */
		mountOn[0] = 0; /* strcpy(mountOn,""); */
		mountType[0] = 0; /* strcpy(mountType,""); */
		in.getline(buffer, 200);
		sscanf(buffer,"%s %s %s ", mountDev, mountOn, mountType);
		if(strcmp(mountType,"nfs")==0 || strcmp(mountType,"cifs")==0)
		{
			count++;
			std::string s1 = mountDev;
			s1 += " -> ";
			s1 += mountOn;
			std::string s2 = "doumount ";
			s2 += mountOn;
			umountMenu.addItem(new CMenuForwarderNonLocalized(s1.c_str(), true, NULL, this, s2.c_str()));
		}
	}
	in.close();
	if(count > 0)
		return umountMenu.exec(this,"");
	else
		return menu_return::RETURN_REPAINT;
}


void CNFSUmountGui::umount(const char * const dir)
{
	if (dir != NULL)
	{
		if (umount2(dir, MNT_FORCE) != 0)
		{
			ShowHintUTF(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_NFS_UMOUNTERROR)); // UTF-8
			return;
		}
	}
	else
	{
		char buffer[200+1],mountDev[100],mountOn[100],mountType[20];
		std::ifstream in("/proc/mounts", std::ifstream::in);
		while(in.good())
		{
			mountDev[0] = 0; /* strcpy(mountDev,""); */
			mountOn[0] = 0; /* strcpy(mountOn,""); */
			mountType[0] = 0; /* strcpy(mountType,""); */
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

int CNFSSmallMenu::exec( CMenuTarget* parent, const std::string & actionKey )
{
	if (actionKey.empty())
	{
		CMenuWidget menu(LOCALE_NFSMENU_HEAD, "network.raw");
		CNFSMountGui mountGui;
		CNFSUmountGui umountGui;
		menu.addItem(GenericMenuSeparator);
		menu.addItem(GenericMenuBack);
		menu.addItem(GenericMenuSeparatorLine);
		menu.addItem(new CMenuForwarder(LOCALE_NFS_REMOUNT, true, NULL, this, "remount"));
		menu.addItem(new CMenuForwarder(LOCALE_NFS_MOUNT , true, NULL, & mountGui));
		menu.addItem(new CMenuForwarder(LOCALE_NFS_UMOUNT, true, NULL, &umountGui));
		return menu.exec(parent, actionKey);
	}
	else if(actionKey.substr(0,7) == "remount")
	{
		//umount automount dirs
		for(int i = 0; i < 4; i++)
		{
			if(g_settings.network_nfs_automount[i])
				umount2(g_settings.network_nfs_local_dir[i],MNT_FORCE);
		}
		CNFSMountGui::automount();
		return menu_return::RETURN_REPAINT;
	}
	return menu_return::RETURN_REPAINT;
}
