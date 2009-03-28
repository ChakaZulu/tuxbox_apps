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

#include <fstream>

#include <global.h>

#include <errno.h>
#include <pthread.h>
#include <sys/mount.h>
#include <unistd.h>

#include <zapit/client/zapittools.h>

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
	bool changeNotify(const neutrino_locale_t, void *)
	{
		if(*m_type == (int)CFSMounter::NFS)
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

CNFSMountGui::CNFSMountGui()
{
#warning move probing from exec() to fsmounter
	m_nfs_sup = CFSMounter::FS_UNPROBED;
	m_cifs_sup = CFSMounter::FS_UNPROBED;
	m_lufs_sup = CFSMounter::FS_UNPROBED;
	m_smbfs_sup = CFSMounter::FS_UNPROBED;

}


const char * nfs_entry_printf_string[4] =
{
	"NFS %s:%s -> %s auto: %4s",
	"CIFS //%s/%s -> %s auto: %4s",
	"FTPFS %s/%s -> %s auto: %4s",
	"SMBFS //%s%s -> %s auto: %4s"
};


int CNFSMountGui::exec( CMenuTarget* parent, const std::string & actionKey )
{
	//printf("exec: %s\n", actionKey.c_str());
	int returnval = menu_return::RETURN_REPAINT;

	if (m_nfs_sup == CFSMounter::FS_UNPROBED)
		m_nfs_sup = CFSMounter::fsSupported(CFSMounter::NFS);

	if (m_cifs_sup == CFSMounter::FS_UNPROBED)
		m_cifs_sup = CFSMounter::fsSupported(CFSMounter::CIFS);

	if (m_lufs_sup == CFSMounter::FS_UNPROBED)
		m_lufs_sup = CFSMounter::fsSupported(CFSMounter::LUFS);

	if (m_smbfs_sup == CFSMounter::FS_UNPROBED)
		m_smbfs_sup = CFSMounter::fsSupported(CFSMounter::SMBFS);

	printf("SUPPORT: NFS: %d, CIFS: %d, LUFS: %d, SMBFS: %d\n", m_nfs_sup, m_cifs_sup, m_lufs_sup, m_smbfs_sup);

	if (actionKey.empty())
	{
		parent->hide();
		for(int i=0 ; i < NETWORK_NFS_NR_OF_ENTRIES; i++)
		{
			sprintf(m_entry[i],
				nfs_entry_printf_string[(g_settings.network_nfs_type[i] == (int) CFSMounter::NFS) ? 0 : ((g_settings.network_nfs_type[i] == (int) CFSMounter::CIFS) ? 1 : ((g_settings.network_nfs_type[i] == (int) CFSMounter::SMBFS) ? 3 : 2))],
				g_settings.network_nfs_ip[i].c_str(),
				FILESYSTEM_ENCODING_TO_UTF8(g_settings.network_nfs_dir[i]),
				FILESYSTEM_ENCODING_TO_UTF8(g_settings.network_nfs_local_dir[i]),
				g_Locale->getText(g_settings.network_nfs_automount[i] ? LOCALE_MESSAGEBOX_YES : LOCALE_MESSAGEBOX_NO));
		}
		returnval = menu();
	}
	else if(actionKey.substr(0,10)=="mountentry")
	{
		parent->hide();
		returnval = menuEntry(actionKey[10]-'0');
		for(int i=0 ; i < NETWORK_NFS_NR_OF_ENTRIES; i++)
		{
			sprintf(m_entry[i],
				nfs_entry_printf_string[(g_settings.network_nfs_type[i] == (int) CFSMounter::NFS) ? 0 : ((g_settings.network_nfs_type[i] == (int) CFSMounter::CIFS) ? 1 : ((g_settings.network_nfs_type[i] == (int) CFSMounter::SMBFS) ? 3 : 2))],
				g_settings.network_nfs_ip[i].c_str(),
				FILESYSTEM_ENCODING_TO_UTF8(g_settings.network_nfs_dir[i]),
				FILESYSTEM_ENCODING_TO_UTF8(g_settings.network_nfs_local_dir[i]),
				g_Locale->getText(g_settings.network_nfs_automount[i] ? LOCALE_MESSAGEBOX_YES : LOCALE_MESSAGEBOX_NO));
			sprintf(ISO_8859_1_entry[i],ZapitTools::UTF8_to_Latin1(m_entry[i]).c_str());
		}
	}
	else if(actionKey.substr(0,7)=="domount")
	{
		int nr=atoi(actionKey.substr(7,1).c_str());
		CFSMounter::mount(g_settings.network_nfs_ip[nr].c_str(), g_settings.network_nfs_dir[nr],
				  g_settings.network_nfs_local_dir[nr], (CFSMounter::FSType) g_settings.network_nfs_type[nr],
				  g_settings.network_nfs_username[nr], g_settings.network_nfs_password[nr],
				  g_settings.network_nfs_mount_options1[nr], g_settings.network_nfs_mount_options2[nr]);
		// TODO show msg in case of error
		returnval = menu_return::RETURN_EXIT;
	}
	else if(actionKey.substr(0,3)=="dir")
	{
		parent->hide();
		int nr=atoi(actionKey.substr(3,1).c_str());
		CFileBrowser b;
		b.Dir_Mode=true;

		if (b.exec(g_settings.network_nfs_local_dir[nr]))
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
	char s2[12];

	for(int i=0 ; i < NETWORK_NFS_NR_OF_ENTRIES ; i++)
	{
		sprintf(s2,"mountentry%d",i);
		sprintf(ISO_8859_1_entry[i],ZapitTools::UTF8_to_Latin1(m_entry[i]).c_str());
		CMenuForwarderNonLocalized *forwarder = new CMenuForwarderNonLocalized("", true, ISO_8859_1_entry[i], this, s2);
		if (CFSMounter::isMounted(g_settings.network_nfs_local_dir[i]))
		{
			forwarder->iconName = NEUTRINO_ICON_MOUNTED;
		} else
		{
			forwarder->iconName = NEUTRINO_ICON_NOT_MOUNTED;
		}
		mountMenuW.addItem(forwarder);
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

#define NFS_TYPE_OPTION_COUNT 4
const CMenuOptionChooser::keyval NFS_TYPE_OPTIONS[NFS_TYPE_OPTION_COUNT] =
{
	{ CFSMounter::NFS , LOCALE_NFS_TYPE_NFS  },
	{ CFSMounter::CIFS, LOCALE_NFS_TYPE_CIFS },
	{ CFSMounter::LUFS, LOCALE_NFS_TYPE_LUFS },
	{ CFSMounter::SMBFS, LOCALE_NFS_TYPE_SMBFS }
};

int CNFSMountGui::menuEntry(int nr)
{
	char *dir,*local_dir, *username, *password, *options1, *options2, *mac;
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
	options1 = g_settings.network_nfs_mount_options1[nr];
	options2 = g_settings.network_nfs_mount_options2[nr];
	mac = g_settings.network_nfs_mac[nr];

	sprintf(cmd,"domount%d",nr);
	sprintf(cmd2,"dir%d",nr);

   /* rewrite fstype in new entries */
   if(strlen(local_dir)==0)
   {
	   if(m_cifs_sup != CFSMounter::FS_UNSUPPORTED && m_nfs_sup == CFSMounter::FS_UNSUPPORTED && m_lufs_sup == CFSMounter::FS_UNSUPPORTED && m_smbfs_sup == CFSMounter::FS_UNSUPPORTED)
		   *type = (int) CFSMounter::CIFS;

	   else if(m_lufs_sup != CFSMounter::FS_UNSUPPORTED && m_cifs_sup == CFSMounter::FS_UNSUPPORTED && m_nfs_sup == CFSMounter::FS_UNSUPPORTED && m_smbfs_sup == CFSMounter::FS_UNSUPPORTED)
		   *type = (int) CFSMounter::LUFS;

	   else if(m_smbfs_sup != CFSMounter::FS_UNSUPPORTED && m_cifs_sup == CFSMounter::FS_UNSUPPORTED && m_nfs_sup == CFSMounter::FS_UNSUPPORTED && m_lufs_sup == CFSMounter::FS_UNSUPPORTED)
		   *type = (int) CFSMounter::SMBFS;
   }
   bool typeEnabled = (m_cifs_sup != CFSMounter::FS_UNSUPPORTED && m_nfs_sup != CFSMounter::FS_UNSUPPORTED && m_lufs_sup != CFSMounter::FS_UNSUPPORTED && m_smbfs_sup != CFSMounter::FS_UNSUPPORTED) ||
	   (m_cifs_sup != CFSMounter::FS_UNSUPPORTED && *type != (int)CFSMounter::CIFS) ||
	   (m_nfs_sup != CFSMounter::FS_UNSUPPORTED && *type != (int)CFSMounter::NFS) ||
	   (m_lufs_sup != CFSMounter::FS_UNSUPPORTED && *type != (int)CFSMounter::LUFS) ||
	   (m_smbfs_sup != CFSMounter::FS_UNSUPPORTED && *type != (int)CFSMounter::SMBFS);

	CMenuWidget mountMenuEntryW(LOCALE_NFS_MOUNT, "network.raw",720);
	mountMenuEntryW.addItem(GenericMenuSeparator);
	mountMenuEntryW.addItem(GenericMenuBack);
	mountMenuEntryW.addItem(GenericMenuSeparatorLine);
	CIPInput ipInput(LOCALE_NFS_IP, g_settings.network_nfs_ip[nr], LOCALE_IPSETUP_HINT_1, LOCALE_IPSETUP_HINT_2);
	CStringInputSMS dirInput(LOCALE_NFS_DIR, dir, 30, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE,"abcdefghijklmnopqrstuvwxyz0123456789-_.,:|!?/ ");
	CMenuOptionChooser *automountInput= new CMenuOptionChooser(LOCALE_NFS_AUTOMOUNT, automount, MESSAGEBOX_NO_YES_OPTIONS, MESSAGEBOX_NO_YES_OPTION_COUNT, true);
	CStringInputSMS options1Input(LOCALE_NFS_MOUNT_OPTIONS, options1, 30, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "abcdefghijklmnopqrstuvwxyz0123456789-_=.,:|!?/ ");
	CMenuForwarder *options1_fwd = new CMenuForwarder(LOCALE_NFS_MOUNT_OPTIONS, true, options1, &options1Input);
	CStringInputSMS options2Input(LOCALE_NFS_MOUNT_OPTIONS, options2, 30, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "abcdefghijklmnopqrstuvwxyz0123456789-_=.,:|!?/ ");
	CMenuForwarder *options2_fwd = new CMenuForwarder(LOCALE_NFS_MOUNT_OPTIONS, true, options2, &options2Input);
	CStringInputSMS userInput(LOCALE_NFS_USERNAME, username, 30, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "abcdefghijklmnopqrstuvwxyz0123456789-_.,:|!?/ ");
	CMenuForwarder *username_fwd = new CMenuForwarder(LOCALE_NFS_USERNAME, (*type==CFSMounter::CIFS || CFSMounter::LUFS), username, &userInput);
	CStringInputSMS passInput(LOCALE_NFS_PASSWORD, password, 30, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "abcdefghijklmnopqrstuvwxyz0123456789-_.,:|!?/ ");
	CMenuForwarder *password_fwd = new CMenuForwarder(LOCALE_NFS_PASSWORD, (*type==CFSMounter::CIFS || CFSMounter::LUFS), NULL, &passInput);
	CMACInput * macInput = new CMACInput(LOCALE_RECORDINGMENU_SERVER_MAC,  g_settings.network_nfs_mac[nr], LOCALE_IPSETUP_HINT_1, LOCALE_IPSETUP_HINT_2);
	CMenuForwarder * macInput_fwd = new CMenuForwarder(LOCALE_RECORDINGMENU_SERVER_MAC, true, g_settings.network_nfs_mac[nr], macInput);

	CNFSMountGuiNotifier notifier(username_fwd, password_fwd, type);

	mountMenuEntryW.addItem(new CMenuOptionChooser(LOCALE_NFS_TYPE, type, NFS_TYPE_OPTIONS, NFS_TYPE_OPTION_COUNT, typeEnabled, &notifier));
	mountMenuEntryW.addItem(new CMenuForwarder(LOCALE_NFS_IP      , true, g_settings.network_nfs_ip[nr], &ipInput       ));
	mountMenuEntryW.addItem(new CMenuForwarder(LOCALE_NFS_DIR     , true, dir                          , &dirInput      ));
	mountMenuEntryW.addItem(new CMenuForwarder(LOCALE_NFS_LOCALDIR, true, local_dir                    , this     , cmd2));
	mountMenuEntryW.addItem(automountInput);
	mountMenuEntryW.addItem(options1_fwd);
	mountMenuEntryW.addItem(options2_fwd);
	mountMenuEntryW.addItem(username_fwd);
	mountMenuEntryW.addItem(password_fwd);
	mountMenuEntryW.addItem(macInput_fwd);
	mountMenuEntryW.addItem(new CMenuForwarder(LOCALE_NFS_MOUNTNOW, true, NULL                         , this     , cmd ));

	int ret = mountMenuEntryW.exec(this,"");
	return ret;
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
		CFSMounter::umount((actionKey.substr(9)).c_str());
		returnval = menu_return::RETURN_EXIT;
	}
	else
		returnval = menu_return::RETURN_REPAINT;

	return returnval;
}
int CNFSUmountGui::menu()
{
	int count = 0;
	CFSMounter::MountInfos infos;
	CMenuWidget umountMenu(LOCALE_NFS_UMOUNT, "network.raw",720);
	umountMenu.addItem(GenericMenuSeparator);
	umountMenu.addItem(GenericMenuBack);
	umountMenu.addItem(GenericMenuSeparatorLine);
	CFSMounter::getMountedFS(infos);
	for (CFSMounter::MountInfos::const_iterator it = infos.begin();
	     it != infos.end();it++)
	{
		if(it->type == "nfs" || it->type == "cifs" || it->type == "lufs" || it->type == "smbfs")
		{
			count++;
			std::string s1 = it->device;
			s1 += " -> ";
			s1 += it->mountPoint;
			std::string s2 = "doumount ";
			s2 += it->mountPoint;
			CMenuForwarder *forwarder = new CMenuForwarderNonLocalized(s1.c_str(), true, NULL, this, s2.c_str());
			forwarder->iconName = NEUTRINO_ICON_MOUNTED;
			umountMenu.addItem(forwarder);
		}
	}
	if(infos.size() > 0)
		return umountMenu.exec(this,"");
	else
		return menu_return::RETURN_REPAINT;
}



int CNFSSmallMenu::exec( CMenuTarget* parent, const std::string & actionKey )
{
	if (actionKey.empty())
	{
		CMenuWidget menu(LOCALE_NETWORKMENU_MOUNT, "network.raw");
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
		for(int i = 0; i < NETWORK_NFS_NR_OF_ENTRIES; i++)
		{
			if(g_settings.network_nfs_automount[i])
				umount2(g_settings.network_nfs_local_dir[i],MNT_FORCE);
		}
		CFSMounter::automount();
		return menu_return::RETURN_REPAINT;
	}
	return menu_return::RETURN_REPAINT;
}

const std::string mntRes2Str(CFSMounter::MountRes res)
{
	switch(res)
	{
		case CFSMounter::MRES_FS_NOT_SUPPORTED:
			return g_Locale->getText(LOCALE_NFS_MOUNTERROR_NOTSUP);
			break;
		case CFSMounter::MRES_FS_ALREADY_MOUNTED:
			return g_Locale->getText(LOCALE_NFS_ALREADYMOUNTED);
			break;
		case CFSMounter::MRES_TIMEOUT:
			return g_Locale->getText(LOCALE_NFS_MOUNTTIMEOUT);
			break;
		case CFSMounter::MRES_UNKNOWN:
			return g_Locale->getText(LOCALE_NFS_MOUNTERROR);
			break;
		case CFSMounter::MRES_OK:
			return g_Locale->getText(LOCALE_NFS_MOUNTOK);
			break;
		default:
			return g_Locale->getText(NONEXISTANT_LOCALE);
			break;
	}
}

const std::string mntRes2Str(CFSMounter::UMountRes res)
{
	switch(res)
	{
		case CFSMounter::UMRES_ERR:
			return g_Locale->getText(LOCALE_NFS_UMOUNTERROR);
			break;
		case CFSMounter::UMRES_OK:
			return g_Locale->getText(NONEXISTANT_LOCALE);
			break;
		default:
			return g_Locale->getText(NONEXISTANT_LOCALE);
			break;
	}
}
