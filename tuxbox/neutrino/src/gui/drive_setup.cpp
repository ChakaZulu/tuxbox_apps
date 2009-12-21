/*
	$Id: drive_setup.cpp,v 1.5 2009/12/21 00:08:38 dbt Exp $

	Neutrino-GUI  -   DBoxII-Project

	drive setup implementation, fdisk frontend for Neutrino gui

	based upon ideas for the neutrino ide_setup by Innuendo and riker

	Copyright (C) 2009 Thilo Graf (dbt)
	http://www.dbox2-tuning.de

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

	Special thx for hardware support by stingray www.dbox2.com and gurgel www.hallenberg.com !

NOTE: 	This is only beta. There is a lot to do

TODO:
	- cleanups
	- add mount options
	- error messages
	- kernel 26 support
	- ....


*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include "gui/drive_setup.h"
#include "gui/imageinfo.h"

#include <global.h>
#include <neutrino.h>

#include <gui/widget/hintbox.h>
#include <gui/widget/icons.h>
#include <gui/widget/messagebox.h>
#include <gui/widget/stringinput.h>
#include <gui/widget/stringinput_ext.h>
#include <gui/widget/progressbar.h>

#include <driver/screen_max.h>

#include <zapit/client/zapittools.h>

#include <system/debug.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <set>
#include <ios>
#include <dirent.h>
#include <sys/vfs.h> // statfs
#include <sys/utsname.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/swap.h>
#include <errno.h>

// Paths of system and init files
#define ETC_DIR				"/etc"
#define VAR_ETC_DIR 			"/var/etc"
#define INIT_D_DIR 			ETC_DIR "/init.d"
#define INIT_D_VAR_DIR 			VAR_ETC_DIR "/init.d"
#define INIT_IDE_SCRIPT_NAME 		"06hdd"
#define INIT_IDE_SCRIPT_PATH 		INIT_D_DIR "/" INIT_IDE_SCRIPT_NAME
#define INIT_IDE_VAR_SCRIPT_PATH 	INIT_D_VAR_DIR "/" INIT_IDE_SCRIPT_NAME
#define INIT_MOUNT_SCRIPT_NAME 		"07mounts"
#define INIT_MOUNT_SCRIPT_FILE 		INIT_D_DIR  "/"  INIT_MOUNT_SCRIPT_NAME
#define INIT_MOUNT_VAR_SCRIPT_FILE 	INIT_D_VAR_DIR "/" INIT_MOUNT_SCRIPT_NAME
#ifdef ENABLE_NFSSERVER
#define EXPORTS 			ETC_DIR "/exports"
#define EXPORTS_VAR 			VAR_ETC_DIR "/exports"
#define NFS_START_SCRIPT		INIT_D_DIR "/S31nfsserver"
#define NFS_STOP_SCRIPT			INIT_D_DIR "/K31nfsserver"
#endif /*ENABLE_NFSSERVER*/

#define TEMP_SCRIPT			"/tmp/drive_setup"
#define PREPARE_SCRIPT_FILE		"/tmp/prepare_opt"
#define PART_TABLE			"/tmp/part_table"
#define ERRLOG				"/tmp/drive_setup_err.log" //contains error codes for system commands
#define DRV_CONFIGFILE			CONFIGDIR "/drivesetup.conf"

#define PROC 				"/proc"
#define PROC_MODULES 			PROC "/modules"
#define PROC_PARTITIONS			PROC "/partitions"
#define PROC_MOUNTS			PROC "/self/mounts"
#define PROC_SWAPS 			PROC "/swaps"
#define FSTAB				ETC_DIR "/fstab"
#define FSTAB_VAR			VAR_ETC_DIR "/fstab"

// system commands
#define LOAD		"insmod "
#define UNLOAD		"rmmod "
#define SWAPON		"swapon "
#define SWAPOFF		"swapoff "
#define MKSWAP		"mkswap "
#define DISCTOOL	"fdisk "
#define HDDTEMP		"hddtemp"

#define DEVNULL 	" 2>/dev/null "

// modul type
#define M_TYPE		".o"
// module names
#define DBOXIDE 	"dboxide"
#define IDE_DETECT 	"ide-detect"
#define IDE_CORE 	"ide-core"
#define IDE_DISK 	"ide-disk"
#define M_MMC		"mmc"
#define M_MMC2		"mmc2"
#define M_MMCCOMBO	"mmccombo"

// mmc modul files
#define MF_MMC		M_MMC M_TYPE
#define MF_MMC2		M_MMC2 M_TYPE
#define MF_MMCCOMBO	M_MMCCOMBO M_TYPE

// proc devices
#define IDE0HDA		"/proc/ide/ide0/hda"
#define IDE0HDB		"/proc/ide/ide0/hdb"
#define MMC0DISC	""

// devices
#define HDA 		"/dev/ide/host0/bus0/target0/lun0/disc"
#define HDB 		"/dev/ide/host0/bus0/target1/lun0/disc"
#define MMCA 		"/dev/mmc/disc0/disc"

#define NO_REFRESH	0 /*false*/

// user scripts
#define NEUTRINO_FORMAT_START_SCRIPT 	CONFIGDIR "/formating.start"
#define NEUTRINO_FORMAT_END_SCRIPT 	CONFIGDIR "/formating.end"
#define NEUTRINO_CHKFS_START_SCRIPT 	CONFIGDIR "/checkfs.start"
#define NEUTRINO_CHKFS_END_SCRIPT 	CONFIGDIR "/checkfs.end"

// actionkey patterns
#define MAKE_PARTITION 		"make_partition_"
#define MOUNT_PARTITION 	"mount_partition_"
#define UNMOUNT_PARTITION 	"unmount_partition_"
#define DELETE_PARTITION 	"delete_partition_"
#define CHECK_PARTITION 	"check_partition_"


using namespace std;

typedef struct drives_t
{
	const string device;
	const string proc_device;
} drives_struct_t;

const drives_struct_t drives[MAXCOUNT_DRIVE] =
{
	{HDA, IDE0HDA},		//MASTER
	{HDB, IDE0HDB},		//SLAVE
 	{MMCA, MMC0DISC}, 	//MMCARD
};


CDriveSetup::CDriveSetup():configfile('\t')
{
	frameBuffer = CFrameBuffer::getInstance();

	width 	= w_max (600, 50);
	hheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	mheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
	height 	= hheight+13*mheight+ 10;
	x	= getScreenStartX (width);
	y	= getScreenStartY (height);

	pb_w = width/2;
	pb_h = 50;
	pb_x = x+pb_w/2;
	pb_y = y+height/2-pb_h/2;

	msg_timeout 	= g_settings.timing[SNeutrinoSettings::TIMING_INFOBAR];
	msg_icon 	= NEUTRINO_ICON_PARTITION;

	//generate action key strings for device selection
	for (int i = 0; i < MAXCOUNT_DRIVE; i++)
	{
		string s_i = iToString(i);
		sel_device_num_actionkey[i] = "sel_device_" + s_i;
	}

	//generate action key strings for partition operations
	for (int i = 0; i<MAXCOUNT_PARTS; i++)
	{ 
		make_part_actionkey[i]		= MAKE_PARTITION + iToString(i);
		mount_partition[i] 		= MOUNT_PARTITION + iToString(i);
		unmount_partition[i] 		= UNMOUNT_PARTITION + iToString(i);
		delete_partition[i]		= DELETE_PARTITION + iToString(i);
		check_partition[i]		= CHECK_PARTITION + iToString(i);
		
		sprintf(part_num_actionkey[i], "edit_partition_%d", i);
	}


}

CDriveSetup::~CDriveSetup()
{
}

int CDriveSetup::exec(CMenuTarget* parent, const string &actionKey)
{	
	int   res = menu_return::RETURN_REPAINT;

	if (parent)
	{
		parent->hide();
	}

	if (actionKey=="apply")
	{
		if (saveHddSetup()) 
		{
			Init();
			return menu_return::RETURN_EXIT;
		}
		return res;
	}
	else if (actionKey == "mount_device_partitions")
 	{

		if (mountDevice(current_device)) 
		{
			showHddSetupSub();
			return menu_return::RETURN_EXIT;
		}
		else 
		{
			ShowLocalizedHint(LOCALE_MESSAGEBOX_ERROR, LOCALE_DRIVE_SETUP_PARTITION_MOUNT_ERROR, width, msg_timeout, NEUTRINO_ICON_ERROR);
			return res;
		}
 	}
	else if (actionKey == "unmount_device_partitions")
 	{
		if (unmountDevice(current_device)) 
		{
			showHddSetupSub();
			return menu_return::RETURN_EXIT;
		}
		else 
		{
			ShowLocalizedHint(LOCALE_MESSAGEBOX_ERROR, LOCALE_DRIVE_SETUP_PARTITION_MOUNT_ERROR, width, msg_timeout, NEUTRINO_ICON_ERROR);
			return res;
		}
 	}
	else if (actionKey == "make_swap")
 	{
		bool do_format = (ShowLocalizedMessage(LOCALE_DRIVE_SETUP_PARTITION_CREATE, LOCALE_DRIVE_SETUP_MSG_PARTITION_CREATE, CMessageBox::mbrNo, CMessageBox::mbYes | CMessageBox::mbNo, NEUTRINO_ICON_INFO, width, 5) == CMessageBox::mbrYes);

		if (do_format) 
		{
			strcpy(d_settings.drive_partition_fstype[current_device][next_part_number], "swap"); 
			d_settings.drive_partition_mountpoint[current_device][next_part_number] = "none";
			strcpy(d_settings.drive_partition_size[current_device][next_part_number], "128");
			writeDriveSettings();

			if (formatPartition(current_device, next_part_number)) 
			{ // success
				return menu_return::RETURN_EXIT_ALL;
			}
			else 
			{// formating failed
				ShowLocalizedHint(LOCALE_MESSAGEBOX_ERROR, LOCALE_DRIVE_SETUP_MSG_PARTITION_CREATE_FAILED, width, msg_timeout, NEUTRINO_ICON_ERROR);
			}
		}
		
		return res;
 	}
	//using generated actionkeys for...
	for (int i = 0; i < MAXCOUNT_DRIVE; i++) 
	{
		if (actionKey == sel_device_num_actionkey[i]) //...select device
		{
			current_device = i;
			showHddSetupSub();
			return res;
		}
		for (int ii = 0; ii < MAXCOUNT_PARTS; ii++)	
		{
			if (actionKey == make_part_actionkey[ii])//...make partition
			{ 
				bool do_format = (ShowLocalizedMessage(LOCALE_DRIVE_SETUP_PARTITION_CREATE, LOCALE_DRIVE_SETUP_MSG_PARTITION_CREATE, CMessageBox::mbrNo, CMessageBox::mbYes | CMessageBox::mbNo, NEUTRINO_ICON_INFO, width, 5) == CMessageBox::mbrYes);
	
				if (do_format) 
				{
					writeDriveSettings();
					if (formatPartition(current_device, ii)) 
					{ // success
						calPartCount(); //refresh part counter
						return menu_return::RETURN_EXIT_ALL;
						
					}
					else 
					{// formating failed
						ShowLocalizedHint(LOCALE_MESSAGEBOX_ERROR, LOCALE_DRIVE_SETUP_MSG_PARTITION_CREATE_FAILED, width, msg_timeout, NEUTRINO_ICON_ERROR);
					}
				}
				return res;
			}
			else if (actionKey == mount_partition[ii]) //...mount partition
			{
				if (mountPartition(current_device, ii, d_settings.drive_partition_fstype[current_device][ii], d_settings.drive_partition_mountpoint[current_device][ii])) 
				{
					showHddSetupSub();
					return menu_return::RETURN_EXIT_ALL;
				}
				else 
				{
					ShowHintUTF(LOCALE_MESSAGEBOX_ERROR, getErrMsg(LOCALE_DRIVE_SETUP_PARTITION_MOUNT_ERROR).c_str(), width, msg_timeout, NEUTRINO_ICON_ERROR);
					return menu_return::RETURN_EXIT_ALL; 
				}
			}
			else if (actionKey == unmount_partition[ii]) //...unmount partition
			{
				if (unmountPartition(current_device, ii)) 
				{
					showHddSetupSub();
					return menu_return::RETURN_EXIT_ALL;
				}
				else 
				{
					ShowHintUTF(LOCALE_MESSAGEBOX_ERROR, getErrMsg(LOCALE_DRIVE_SETUP_PARTITION_MOUNT_ERROR).c_str(), width, msg_timeout, NEUTRINO_ICON_ERROR);
					return menu_return::RETURN_EXIT_ALL; 
				}
			}
			else if (actionKey == delete_partition[ii]) //...delete partition
			{
				bool delete_part = (ShowLocalizedMessage(LOCALE_DRIVE_SETUP_PARTITION_DELETE, LOCALE_DRIVE_SETUP_MSG_PARTITION_DELETE, CMessageBox::mbrNo, CMessageBox::mbYes | CMessageBox::mbNo, NEUTRINO_ICON_ERROR, width, 5) == CMessageBox::mbrYes);
		
				if (delete_part) 
				{
					neutrino_locale_t msg_locale;
					const char* del_msg_icon;
					if (!mkPartition(current_device, DELETE_CLEAN, ii))
					{ // delete is failed
						msg_locale = 	LOCALE_DRIVE_SETUP_MSG_PARTITION_DELETE_FAILED;
						del_msg_icon = 	NEUTRINO_ICON_ERROR;
						ShowLocalizedHint(LOCALE_MESSAGEBOX_INFO, msg_locale, width, msg_timeout, del_msg_icon);
						return res;
					}
					else 
					{ // delete was successfull
						msg_locale = 	LOCALE_DRIVE_SETUP_MSG_PARTITION_DELETE_OK;
						del_msg_icon =	NEUTRINO_ICON_INFO;
						ShowLocalizedHint(LOCALE_MESSAGEBOX_INFO, msg_locale, width, msg_timeout, del_msg_icon);
						showHddSetupSub();
						return menu_return::RETURN_EXIT_ALL;
					}
				}
				else
					return res;		
			}
			else if (actionKey == check_partition[ii]) //...check partition
			{
				bool check_part = (ShowLocalizedMessage(LOCALE_DRIVE_SETUP_PARTITION_CHECK, LOCALE_DRIVE_SETUP_MSG_PARTITION_CHECK, CMessageBox::mbrNo, CMessageBox::mbYes | CMessageBox::mbNo, NEUTRINO_ICON_ERROR, width, 5) == CMessageBox::mbrYes);
		
				if (check_part) 
				{
					neutrino_locale_t chk_msg_locale;
					const char* chk_msg_icon;
					if (unmountPartition(current_device, ii)) 
					{
						string fstype = d_settings.drive_partition_fstype[current_device][ii];
						if (!chkFs(current_device, ii, fstype)){ // check is failed
							chk_msg_locale = 	LOCALE_DRIVE_SETUP_MSG_PARTITION_CHECK_FAILED;
							chk_msg_icon = 	NEUTRINO_ICON_ERROR;
							ShowLocalizedHint(LOCALE_MESSAGEBOX_INFO, chk_msg_locale, width, msg_timeout, chk_msg_icon);
							return res;
						}
						else 
						{ // check was successfull
							chk_msg_locale = 	LOCALE_DRIVE_SETUP_MSG_PARTITION_CHECK_OK;
							chk_msg_icon =	NEUTRINO_ICON_INFO;
							ShowLocalizedHint(LOCALE_MESSAGEBOX_INFO, chk_msg_locale, width, msg_timeout, chk_msg_icon);
							mountPartition(current_device, ii, fstype,d_settings.drive_partition_mountpoint[current_device][ii] ); // TODO Error handling
							return menu_return::RETURN_EXIT;
						}
					}
				}
				else
					return res;
		
			}
		}
	}

	Init();
	return res;
}

void CDriveSetup::hide()
{
	frameBuffer->paintBackgroundBoxRel(x,y, width,height);
}

// init members
#define COUNT_INIT_MEMBERS 10

// init menue
void CDriveSetup::Init()
{
	cout<<getDriveSetupVersion()<<endl;
	
	CProgressBar pb;

	fb_pixel_t * pixbuf = new fb_pixel_t[pb_w * pb_h];
	if (pixbuf != NULL)
		frameBuffer->SaveScreen(pb_x, pb_y, pb_w, pb_h, pixbuf);

 	void (CDriveSetup::*pMember[COUNT_INIT_MEMBERS])(void) = {	&CDriveSetup::loadDriveSettings,
									&CDriveSetup::loadHddCount,
									&CDriveSetup::loadHddModels,
									&CDriveSetup::calPartCount,
									&CDriveSetup::mkDefaultMountpoints,
									&CDriveSetup::loadFsModulList,
									&CDriveSetup::loadMmcModulList,
									&CDriveSetup::loadFdiskData,
									&CDriveSetup::loadDriveTemps,
									&CDriveSetup::showHddSetupMain};

	frameBuffer->paintBoxRel(pb_x, pb_y, pb_w, pb_h, COL_MENUCONTENT_PLUS_0, RADIUS_MID);

	for (unsigned int i = 0; i < COUNT_INIT_MEMBERS; i++) 
	{
		pb.paintProgressBar(pb_x+10, pb_y+pb_h-20-SHADOW_OFFSET, pb_w-20, 16, i, COUNT_INIT_MEMBERS, 0, 0, COL_SILVER, COL_INFOBAR_SHADOW, "loading menue...", COL_MENUCONTENT);
		(*this.*pMember[i])();
	}

	if (pixbuf != NULL) 
	{
		frameBuffer->RestoreScreen(pb_x, pb_y, pb_w, pb_h, pixbuf);
		delete[] pixbuf;
	}

}

typedef struct mn_data_t
{
	const neutrino_locale_t entry_locale;
	const neutrino_msg_t 	rcinput_key;
	const char 		*active_icon;
} mn_data_struct_t;

const mn_data_struct_t mn_data[MAXCOUNT_DRIVE] =
{
	{LOCALE_DRIVE_SETUP_HDD_MASTER	, CRCInput::RC_green	, NEUTRINO_ICON_BUTTON_GREEN},	//MASTER
	{LOCALE_DRIVE_SETUP_HDD_SLAVE	, CRCInput::RC_yellow	, NEUTRINO_ICON_BUTTON_YELLOW},	//SLAVE
 	{LOCALE_DRIVE_SETUP_MMC		, CRCInput::RC_blue	, NEUTRINO_ICON_BUTTON_BLUE},	//MMCARD
};

// shows the main drive setup menue
void CDriveSetup::showHddSetupMain()
{
	// mmc active ?
	device_isActive[MMCARD] = isMmcActive();

	// main menue
	CMenuWidget 	*m = new CMenuWidget(LOCALE_DRIVE_SETUP_HEAD, msg_icon, width);

	// apply
	CMenuForwarder *m1 = new CMenuForwarder(LOCALE_DRIVE_SETUP_SAVESETTINGS, true, NULL, this, "apply", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED);

	// activate/deactivate ide interface
	if (isIdeInterfaceActive()) 
	{
		// get the current state of ide modul and set it to current settings, informations comes from init file and neutrino settings
		// TODO get status direct from driver
		string init_file = getInitIdeFilePath();
		string irq6_opt = getFileEntryString(init_file.c_str(), DBOXIDE, 3);
		d_settings.drive_activate_ide = (irq6_opt == "irq6=1") ? IDE_ACTIVE_IRQ6 : IDE_ACTIVE;
	}
	CMenuOptionChooser *m2	= new CMenuOptionChooser(LOCALE_DRIVE_SETUP_IDE_ACTIVATE, &d_settings.drive_activate_ide, OPTIONS_IDE_ACTIVATE_OPTIONS, OPTIONS_IDE_ACTIVATE_OPTION_COUNT, true);

	//select separator
	CMenuSeparator *m4	= new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_DRIVE_SETUP_SELECT);

 	/************show main menue entries***********/
	// intro entries
	m->addItem(GenericMenuSeparator);
	m->addItem(GenericMenuBack);
	m->addItem(GenericMenuSeparatorLine);
	// show apply button/entry
	m->addItem(m1);
	m->addItem(GenericMenuSeparatorLine);

	m->addItem(m2); // show ide options on/off

	// mmc settings
	bool is_mmc_supported = (v_mmc_modules.size()>1) ? true : false;
	if (is_mmc_supported)  //hide mmc item if no modul available
	{
		sprintf(d_settings.drive_mmc_module_name, "%s", getUsedMmcModulName().c_str());
		CMenuOptionStringChooser* m7 = new CMenuOptionStringChooser(LOCALE_DRIVE_SETUP_MMC, d_settings.drive_mmc_module_name, true);
		for (uint i=0; i < v_mmc_modules.size(); i++) 
		{
			m7->addOption(v_mmc_modules[i].c_str());
		}
		// show mmc options
		m->addItem (m7);
	}
	m->addItem(GenericMenuSeparatorLine);

	// extended settings
	CMenuWidget 	*extsettings = new CMenuWidget(LOCALE_DRIVE_SETUP_HEAD, msg_icon, width);
	m->addItem (new CMenuForwarder(LOCALE_DRIVE_SETUP_ADVANCED_SETTINGS, true, NULL, extsettings, NULL, CRCInput::RC_1));
	extsettings->addItem (new CMenuSeparator(CMenuSeparator::ALIGN_LEFT | CMenuSeparator::SUB_HEAD | CMenuSeparator::STRING, LOCALE_DRIVE_SETUP_ADVANCED_SETTINGS));

	// extended settings:intro entries
	extsettings->addItem(GenericMenuSeparator);
	extsettings->addItem(GenericMenuBack);
	extsettings->addItem (new CMenuSeparator(CMenuSeparator::ALIGN_CENTER | CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_DRIVE_SETUP_FSTAB));

	// extended settings:fstab mode
	extsettings->addItem (new CMenuOptionChooser(LOCALE_DRIVE_SETUP_FSTAB_USE, &d_settings.drive_use_fstab, OPTIONS_ON_OFF_OPTIONS, OPTIONS_ON_OFF_OPTION_COUNT, true));
	extsettings->addItem (new CMenuOptionChooser(LOCALE_DRIVE_SETUP_MOUNT_MTDBLOCK_PARTITIONS, &d_settings.drive_mount_mtdblock_partitions, OPTIONS_YES_NO_OPTIONS, OPTIONS_YES_NO_OPTION_COUNT, true));


	// show select separator, only visible if any device activ
	if (hdd_count>0) 
	{	
		m->addItem(m4);
	}

	// capacity
	string s_cap[MAXCOUNT_DRIVE];

	// entries for model, capacity and temperature
	CMenuForwarder	*m5[MAXCOUNT_DRIVE];

	for (int i = 0; i<MAXCOUNT_DRIVE; i++) 
	{
		m5[i] = new CMenuForwarder(mn_data[i].entry_locale, true, v_model_name[i], this, sel_device_num_actionkey[i].c_str(), mn_data[i].rcinput_key, mn_data[i].active_icon);

		if (device_isActive[i])
		{
			// show model entry
			m->addItem (m5[i]);

			// set and converting size string for menu
			s_cap[i] = convertByteString(v_device_size[i]);
			m->addItem( new CMenuForwarder(LOCALE_DRIVE_SETUP_HDD_CAPACITY, false, s_cap[i]));

			// show temperature, do nothing if no value available
			if (v_device_temp[i] !="0")
				m->addItem( new CMenuForwarder(LOCALE_DRIVE_SETUP_HDD_TEMP, false, v_device_temp[i].c_str()));
		}
	}


	m->exec (NULL, "");
	m->hide ();
	delete m;
}


//helper: generate part entries for showHddSetupSubMenue
string CDriveSetup::getPartEntryString(string& partname)
{
	string p_name = partname;

	string s_mountpoint, s_size, s_type, s_fs, s_options;

	ostringstream str_size, str_hours;

	if (isSwapPartition(p_name))
	{ // found swap partition
		string s_mp 		= getSwapInfo(p_name, FILENAME);
		int mp_len		= s_mp.length();
		s_mountpoint		= (mp_len > 10) ? (s_mp.substr(0, 9) + "..." + s_mp.substr(mp_len-6)) : s_mp;
		s_type			= "swap " + getSwapInfo(p_name, TYPE);
		long long l_size 	= atoi(getSwapInfo(p_name, SIZE).c_str())/1024; //MB
		// convert size to string
		str_size << l_size;
		string s_space(str_size.str());
		s_size		= s_space + " MB";
	}
	else if ((!isSwapPartition(p_name)) && (isActivePartition(p_name))) 
	{
		s_mountpoint 		= getMountInfo(p_name, MOUNTPOINT);
		long long l_space 	= getDeviceInfo(s_mountpoint.c_str(), KB_AVAILABLE)/1024; //MB
		long long l_hours	= getDeviceInfo(s_mountpoint.c_str(), FREE_HOURS);
		s_fs 			= getMountInfo(p_name, FS);
		s_options 		= getMountInfo(p_name, OPTIONS);
		// convert size to string
		str_size << l_space;
		string s_space(str_size.str());

		str_hours << l_hours;
		string s_hours(str_hours.str());

		s_size 	= s_space + " MB (ca. " + s_hours + "h)";
	}

	string 	s_entry = s_mountpoint;
		s_entry += char(32);
		s_entry += s_fs;
		s_entry += char(32);
		s_entry += s_type;
		s_entry += s_options;
		s_entry += char(32);
		s_entry += s_size;

	if (s_mountpoint.length() == 0) // no active partition mounted
		s_entry = g_Locale->getText(LOCALE_DRIVE_SETUP_PARTITION_NOT_ACTIVE);

	if (!isActivePartition(p_name)) // no active partition found
		s_entry = g_Locale->getText(LOCALE_DRIVE_SETUP_PARTITION_NOT_CREATED);

	return s_entry;
}

// shows the sub setup menue with informations about partitions and primary settings
void CDriveSetup::showHddSetupSub()
{
	//menue sub
	CMenuWidget 	*sub = new CMenuWidget(LOCALE_DRIVE_SETUP_HEAD, msg_icon, width);

	//menue add
	CMenuWidget 	*sub_add = new CMenuWidget(LOCALE_DRIVE_SETUP_HEAD, msg_icon, width);

	//menue add_swap
	CMenuWidget 	*sub_add_swap = new CMenuWidget(LOCALE_DRIVE_SETUP_HEAD, msg_icon, width);

	//menue partitions
	CMenuWidget 	*part[MAXCOUNT_PARTS];
	for (int i = 0; i<MAXCOUNT_PARTS; i++)
	{
	 	part[i]= new CMenuWidget(LOCALE_DRIVE_SETUP_HEAD, msg_icon, width);
	}

	//menue sub: prepare sub head
	string dev_name = g_Locale->getText(mn_data[current_device].entry_locale);
	CMenuSeparator *subhead = new CMenuSeparator(CMenuSeparator::ALIGN_LEFT | CMenuSeparator::SUB_HEAD | CMenuSeparator::STRING, mn_data[current_device].entry_locale);


	//menue sub: show settings for spindown and writecache only for hdd, these device numbers are < 2 (MMCARD)
	CMenuForwarder *spindown = NULL;
	CMenuOptionChooser *w_cache = NULL;
	if (current_device < MMCARD) 
	{ 
		//menue sub: prepare spindown settings
		CStringInput *hdd_sleep = new CStringInput(LOCALE_DRIVE_SETUP_HDD_SLEEP, d_settings.drive_spindown[current_device], 3, LOCALE_DRIVE_SETUP_HDD_SLEEP_STD, LOCALE_DRIVE_SETUP_HDD_SLEEP_HELP, "0123456789 ");
		spindown = new CMenuForwarder(LOCALE_DRIVE_SETUP_HDD_SLEEP, true, d_settings.drive_spindown[current_device], hdd_sleep );
	
		//menue sub: prepare writecache settings
		w_cache = new CMenuOptionChooser(LOCALE_DRIVE_SETUP_HDD_CACHE, &d_settings.drive_write_cache[current_device], OPTIONS_ON_OFF_OPTIONS, OPTIONS_ON_OFF_OPTION_COUNT, true );
	}

	//menue sub: generate part items
	CMenuForwarderNonLocalized *sub_part_entry[MAXCOUNT_PARTS];
	string partname[MAXCOUNT_PARTS];
	for (int i = 0; i<MAXCOUNT_PARTS; i++) 
	{
 		partname[i] = getPartName(current_device, i);
		unsigned int shortcut_num = i+1; // next possible partition number for key shortcut
		sub_part_entry[i] = new CMenuForwarderNonLocalized(getPartEntryString(partname[i]).c_str(), isActivePartition(partname[i]), NULL, part[i], NULL/*part_num_actionkey[i]*/, CRCInput::convertDigitToKey(shortcut_num));

	}

	//menue sub: prepare item: add partition
	bool add_activate;
	unsigned long long ll_free_part_size = getUnpartedDeviceSize(current_device);
	if ((part_count[current_device] < MAXCOUNT_PARTS) && (ll_free_part_size > 0xA00000)) 
	{ // disable entry if we have no free partition or not enough size
		add_activate = true;
	}
	else 
	{
		add_activate = false;
	}
	next_part_number = getFirstUnusedPart(current_device); //also used from swap_add
 	CMenuForwarder *part_add = new CMenuForwarder(LOCALE_DRIVE_SETUP_HDD_ADD_PARTITION, add_activate, NULL, sub_add, NULL/*"add_partition"*/, CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED);

	//menue sub: prepare item: add swap
	bool add_swap_active;
	if ((!haveSwap()) && (add_activate)) // disable add item if we have already a swap, no free partition or not enough size
		add_swap_active = true;
	else 
		add_swap_active = false;
	
	CMenuForwarder *swap_add = new CMenuForwarder(LOCALE_DRIVE_SETUP_HDD_ADD_SWAP_PARTITION, add_swap_active, NULL, sub_add_swap, NULL/*"add_swap_partition"*/, CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN);

	//menue add: prepare subhead: add part
	string add_subhead_txt = dev_name + " >> " + iToString(next_part_number+1) + ". " + g_Locale->getText(LOCALE_DRIVE_SETUP_HDD_ADD_PARTITION);
	CMenuSeparator *add_subhead = new CMenuSeparator(CMenuSeparator::ALIGN_LEFT | CMenuSeparator::SUB_HEAD | CMenuSeparator::STRING);
	add_subhead->setString(add_subhead_txt);
	
	//menue add_swap: prepare subhead: add swap
	string add_swap_subhead_txt = dev_name + " >> " + g_Locale->getText(LOCALE_DRIVE_SETUP_HDD_ADD_SWAP_PARTITION);
	CMenuSeparator *add_swap_subhead = new CMenuSeparator(CMenuSeparator::ALIGN_LEFT | CMenuSeparator::SUB_HEAD | CMenuSeparator::STRING);
	add_swap_subhead->setString(add_swap_subhead_txt);

	CMenuForwarder *make_swap = new CMenuForwarder(LOCALE_DRIVE_SETUP_HDD_FORMAT_PARTITION, true, NULL, this, "make_swap", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED);

	//menue add: prepare start_cylinder for add partition
	setStartCylinder();
	string s_add_start_cyl = iToString(start_cylinder);
	CMenuForwarder *fw_add_start_cyl = new CMenuForwarder(LOCALE_DRIVE_SETUP_PARTITION_START_CYLINDER, false, s_add_start_cyl.c_str());
	
	//menue sub: set mountstatus of devices for enable/disable menue items
	bool have_mounts = haveMounts(current_device);
	bool have_parts = haveActiveParts(current_device);
	
	//menue sub: prepare item: mount all partitions
 	CMenuForwarder *mount_all =  new CMenuForwarder(LOCALE_DRIVE_SETUP_PARTITION_MOUNT_NOW_DEVICE, (!have_mounts ? have_parts:false), NULL, this, "mount_device_partitions", CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW);

	//menue sub: prepare item: unmount all partitions
 	CMenuForwarder *ummount_all =  new CMenuForwarder(LOCALE_DRIVE_SETUP_PARTITION_UNMOUNT_NOW_DEVICE, (have_mounts ? true:false), NULL, this, "unmount_device_partitions", CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE);

	//menue sub: prepare separator: partlist
	CMenuSeparator *separator = new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_DRIVE_SETUP_HDD_EDIT_PARTITION);

	//menue partitions:
	CMenuSeparator *p_subhead[MAXCOUNT_PARTS];

	//menue partitions: subhead text
	string sh_txt[MAXCOUNT_PARTS];

	//menue partitions: prepare information about possible size, cylinders 
	string s_free_part_size = convertByteString(ll_free_part_size);
	CMenuForwarder *freesize = new CMenuForwarder(LOCALE_DRIVE_SETUP_PARTITION_FREE_SIZE, false, s_free_part_size.c_str());

	//menue partitions: size of current partition
	unsigned long long ll_cur_part_size[MAXCOUNT_PARTS];  
	string p_size[MAXCOUNT_PARTS];
	CMenuForwarder *partsize[MAXCOUNT_PARTS];

	//sub menue main
	sub->addItem(subhead);		//subhead
	//------------------------
	sub->addItem(GenericMenuBack);	//back
	sub->addItem(GenericMenuSeparatorLine);
	//------------------------
	if (current_device != MMCARD) 	//not for mmc!
	{
		sub->addItem(spindown); //spindown
		sub->addItem(w_cache); 	//writecache
		sub->addItem(GenericMenuSeparatorLine);
	}	
	//------------------------
	sub->addItem(part_add); 	//add partition
	sub->addItem(swap_add); 	//add swap
	sub->addItem(mount_all); 	//mount
	sub->addItem(ummount_all); 	//unmount
	//------------------------
	sub->addItem(separator); 	//separator partlist
	//------------------------

	// start cylinder
	unsigned long long start_cyl[MAXCOUNT_PARTS];
	CMenuForwarder *fw_start_cyl[MAXCOUNT_PARTS];

	// end cylinder
	unsigned long long end_cyl[MAXCOUNT_PARTS];
	CMenuForwarder *fw_end_cyl[MAXCOUNT_PARTS];

	//choose aktivate/deactivate partition
	CMenuOptionChooser *activate[MAXCOUNT_PARTS];

	//choose mountpoint
	CMenuForwarder * mp_chooser[MAXCOUNT_PARTS];
	string mp[MAXCOUNT_PARTS];
	CDirChooser * mountdir[MAXCOUNT_PARTS];
	bool entry_activ[MAXCOUNT_PARTS];

	//disable item for add swap if we have already a swap or no free partition or not enough size
	if ((!haveSwap()) && (add_activate)) 
		add_swap_active = true;
	else 
		add_swap_active = false;
	
	//choose filesystem
	CMenuOptionStringChooser * fs_chooser[MAXCOUNT_PARTS];
	
	//fs notifier
	CDriveSetupFsNotifier * fsNotifier[MAXCOUNT_PARTS];

	//size input
	CStringInput * input_part_size[MAXCOUNT_PARTS];
	CMenuForwarder * input_size[MAXCOUNT_PARTS];

#ifdef ENABLE_NFSSERVER
	//choose nfs mode
	CDriveSetupNFSHostNotifier * nfsHostNotifier[MAXCOUNT_PARTS];
	CMenuOptionChooser * nfs_chooser[MAXCOUNT_PARTS];

	//menue partitions: host ip input for nfs exports
	CIPInput * nfs_host_ip[MAXCOUNT_PARTS];
	CMenuForwarder * nfs_host_ip_fw[MAXCOUNT_PARTS]; 
#endif

	//make partition
	CMenuForwarder * mkpart[MAXCOUNT_PARTS];

	//mount partition
	CMenuForwarder * mount[MAXCOUNT_PARTS];

	//unmount partitions
	CMenuForwarder * unmount[MAXCOUNT_PARTS];

	//delete partition
	CMenuForwarder * delete_part[MAXCOUNT_PARTS];

	//check partition
	CMenuForwarder * check_part[MAXCOUNT_PARTS];

	//action key strings
	string ak_make_partition[MAXCOUNT_PARTS];
	string ak_mount_partition[MAXCOUNT_PARTS];
	string ak_unmount_partition[MAXCOUNT_PARTS];
	string ak_delete_partition[MAXCOUNT_PARTS];
	string ak_check_partition[MAXCOUNT_PARTS];

	//count of cylinders in edit view
 	string ed_start_cyl[MAXCOUNT_PARTS];
	string ed_end_cyl[MAXCOUNT_PARTS];

	//stat of partition
	bool is_mounted[MAXCOUNT_PARTS];
	
	//menue partitions: edit mode
	for (int i = 0; i<MAXCOUNT_PARTS; i++)
	{
		sub->addItem (sub_part_entry[i]); //possible parts 1-4 for menue partitions:

		//prepare sub head text
		sh_txt[i] = dev_name + " >> " + iToString(i+1) + ". " + g_Locale->getText(LOCALE_DRIVE_SETUP_HDD_EDIT_PARTITION);
		p_subhead[i] = new CMenuSeparator(CMenuSeparator::ALIGN_LEFT | CMenuSeparator::SUB_HEAD | CMenuSeparator::STRING);
		p_subhead[i]->setString(sh_txt[i]);

		//prepare current partsizes
		ll_cur_part_size[i] = getPartSize(current_device, i);
		p_size[i] = convertByteString(ll_cur_part_size[i]);
		partsize[i] = new CMenuForwarder(LOCALE_DRIVE_SETUP_PARTITION_CURRENT_SIZE, false, p_size[i].c_str());

		//prepare start cylinders
		start_cyl[i] = getPartData(current_device, i, START_CYL, NO_REFRESH);
		ed_start_cyl[i] = iToString(start_cyl[i]);
		fw_start_cyl[i] = new CMenuForwarder(LOCALE_DRIVE_SETUP_PARTITION_START_CYLINDER, false, ed_start_cyl[i].c_str()/*c_start_cyl[i]*/);

		//prepare end cylinders
		end_cyl[i] = getPartData(current_device, i, END_CYL, NO_REFRESH);
		ed_end_cyl[i] = iToString(end_cyl[i]);
		fw_end_cyl[i] = new CMenuForwarder(LOCALE_DRIVE_SETUP_PARTITION_END_CYLINDER, false, ed_end_cyl[i].c_str());

		//enable/disable partition
		activate[i] = new CMenuOptionChooser(LOCALE_DRIVE_SETUP_PARTITION_ACTIVATE, &d_settings.drive_partition_activ[current_device][i], OPTIONS_YES_NO_OPTIONS, OPTIONS_YES_NO_OPTION_COUNT, true );

		//select mountpoint, get it primary from system, if available
		mp[i] = d_settings.drive_partition_mountpoint[current_device][i];
		entry_activ[i] = true;
		if (isMountedPartition(partname[i]))
			d_settings.drive_partition_mountpoint[current_device][i] = getMountInfo(partname[i], MOUNTPOINT);
		else if (isSwapPartition(partname[i])/* || d_settings.drive_partition_fstype[current_device][i] == "swap"*/) 
		{	
			entry_activ[i] = false;
			d_settings.drive_partition_mountpoint[current_device][i] = "none";
		}
		else
			d_settings.drive_partition_mountpoint[current_device][i] = mp[i];
		mountdir[i] 	= new CDirChooser(&d_settings.drive_partition_mountpoint[current_device][i]);
		mp_chooser[i] = new CMenuForwarder(LOCALE_DRIVE_SETUP_PARTITION_MOUNTPOINT, entry_activ[i], d_settings.drive_partition_mountpoint[current_device][i], mountdir[i]);

		//select filesystem
		fsNotifier[i] = new CDriveSetupFsNotifier(mp_chooser[i]);
	 	fs_chooser[i] = new CMenuOptionStringChooser(LOCALE_DRIVE_SETUP_PARTITION_FS, d_settings.drive_partition_fstype[current_device][i], entry_activ[i], fsNotifier[i]);
		for (uint n=0; n < v_fs_modules.size(); n++) 
		{
			if ((v_fs_modules[n] != "jbd") && (v_fs_modules[n] != "fat")) 
				fs_chooser[i]->addOption(v_fs_modules[n].c_str());
		}

		//prepare size input, show size // TODO show real current partsize
		input_part_size[i] = new CStringInput(LOCALE_DRIVE_SETUP_PARTITION_SIZE, d_settings.drive_partition_size[current_device][i], 8, LOCALE_DRIVE_SETUP_PARTITION_SIZE_HELP, LOCALE_DRIVE_SETUP_PARTITION_SIZE_STD, "0123456789 ");
		input_size[i] = new CMenuForwarder(LOCALE_DRIVE_SETUP_PARTITION_SIZE, entry_activ[i], d_settings.drive_partition_size[current_device][i], input_part_size[i] );

#ifdef ENABLE_NFSSERVER
		//prepare option host input
		nfs_host_ip[i] = new CIPInput(LOCALE_DRIVE_SETUP_PARTITION_NFS_HOST_IP , d_settings.drive_partition_nfs_host_ip[current_device][i]   , LOCALE_IPSETUP_HINT_1, LOCALE_IPSETUP_HINT_2);

		//prepare option nfs	
		nfs_host_ip_fw[i] = new CMenuForwarder(LOCALE_DRIVE_SETUP_PARTITION_NFS_HOST_IP, d_settings.drive_partition_nfs[current_device][i], d_settings.drive_partition_nfs_host_ip[current_device][i], nfs_host_ip[i] );
		//prepare option nfs chooser

		nfsHostNotifier[i] = new CDriveSetupNFSHostNotifier (nfs_host_ip_fw[i]);
		nfs_chooser[i] = new CMenuOptionChooser(LOCALE_DRIVE_SETUP_PARTITION_NFS, &d_settings.drive_partition_nfs[current_device][i], OPTIONS_YES_NO_OPTIONS, OPTIONS_YES_NO_OPTION_COUNT, entry_activ[i], nfsHostNotifier[i] );		
#endif

		//prepare make partition
		ak_make_partition[i] = MAKE_PARTITION + iToString(i);
		mkpart[i] = new CMenuForwarder(LOCALE_DRIVE_SETUP_HDD_FORMAT_PARTITION, true, NULL, this, ak_make_partition[i].c_str(), CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED);

		//set mountstatus for enable/disable menue items
		if (isMountedPartition(partname[i]) || isSwapPartition(partname[i]))
			is_mounted[i] = true;
		else
			is_mounted[i] = false ;

		//prepare mount partition
		ak_mount_partition[i] = MOUNT_PARTITION + iToString(i);
		mount[i] = new CMenuForwarder(LOCALE_DRIVE_SETUP_PARTITION_MOUNT_NOW, (is_mounted[i] ? false : true), NULL, this, ak_mount_partition[i].c_str(), CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED);

		//prepare unmount partition
		ak_unmount_partition[i] = UNMOUNT_PARTITION + iToString(i);
		unmount[i] = new CMenuForwarder(LOCALE_DRIVE_SETUP_PARTITION_UNMOUNT_NOW, (is_mounted[i] ? true : false), NULL, this, ak_unmount_partition[i].c_str(), CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN);

		//prepare delete partition
		ak_delete_partition[i] = DELETE_PARTITION + iToString(i);
		delete_part[i] = new CMenuForwarder(LOCALE_DRIVE_SETUP_PARTITION_DELETE, true, NULL, this, ak_delete_partition[i].c_str(), CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW);

		//prepare check partition
		ak_check_partition[i] = CHECK_PARTITION + iToString(i);
		check_part[i] = new CMenuForwarder(LOCALE_DRIVE_SETUP_PARTITION_CHECK, true, NULL, this, ak_check_partition[i].c_str(), CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE);
		
		//edit
		part[i]->addItem(p_subhead[i]);			//subhead
		//------------------------
		part[i]->addItem(GenericMenuSeparator); 	//separator
		part[i]->addItem(GenericMenuBack);		//back
		part[i]->addItem(GenericMenuSeparatorLine);	//separator
		//------------------------
		part[i]->addItem(freesize);			//freesize
		part[i]->addItem(partsize[i]);			//partsize
		part[i]->addItem(fw_start_cyl[i]);		//start cylinder
		part[i]->addItem(fw_end_cyl[i]);		//end cylinder
		//------------------------
		part[i]->addItem(GenericMenuSeparatorLine);	//separator
		//------------------------
		part[i]->addItem(activate[i]);			//enable/disable partition
		part[i]->addItem(fs_chooser[i]);		//select filesystem
		part[i]->addItem(mp_chooser[i]);		//select mountpoint
		part[i]->addItem(input_size[i]);		//input part size
		//------------------------
#ifdef ENABLE_NFSSERVER
		part[i]->addItem(GenericMenuSeparatorLine);	//separator
		part[i]->addItem(nfs_chooser[i]);		//nfs
		part[i]->addItem(nfs_host_ip_fw[i]);		//nfs host ip input
		//------------------------
#endif
		part[i]->addItem(GenericMenuSeparatorLine);	//separator
		//------------------------
		part[i]->addItem(mount[i]);			//mount partition
		part[i]->addItem(unmount[i]);			//unmount partition
		part[i]->addItem(delete_part[i]);		//delete partition
		part[i]->addItem(check_part[i]);		//check partition
		
	}
	
	//add
	sub_add->addItem(add_subhead); 	//add partition-subhead
	//------------------------
	sub_add->addItem(GenericMenuSeparator); 	//separator
	sub_add->addItem(GenericMenuBack);		//back
	sub_add->addItem(GenericMenuSeparatorLine);	//separator
	//------------------------
	sub_add->addItem(freesize);			//freesize
	sub_add->addItem(fw_add_start_cyl);		//start_cylinder
	sub_add->addItem(GenericMenuSeparatorLine);	//separator
	//------------------------
	sub_add->addItem(activate[next_part_number]);	//enable/disable partition
	sub_add->addItem(fs_chooser[next_part_number]);	//select filesystem
	sub_add->addItem(mp_chooser[next_part_number]);	//select mountpoint
	sub_add->addItem(input_size[next_part_number]);	//input part size
	//------------------------
	sub_add->addItem(GenericMenuSeparatorLine);	//separator
	//------------------------
	sub_add->addItem(mkpart[next_part_number]);	//make partition


	//add swap
	sub_add_swap->addItem(add_swap_subhead); 		//add swap-subhead
	//------------------------
	sub_add_swap->addItem(GenericMenuSeparator); 		//separator
	sub_add_swap->addItem(GenericMenuBack);			//back
	sub_add_swap->addItem(GenericMenuSeparatorLine);	//separator
	//------------------------
	sub_add_swap->addItem(freesize);			//freesize
	sub_add_swap->addItem(fw_add_start_cyl);		//start_cylinder
	sub_add_swap->addItem(GenericMenuSeparatorLine);	//separator
	//------------------------
	sub_add_swap->addItem(activate[next_part_number]);	//enable/disable partition
	//------------------------
	sub_add_swap->addItem(GenericMenuSeparatorLine);	//separator
	//------------------------
	sub_add_swap->addItem(make_swap);			//make swap partition






	sub->exec (NULL, "");
	sub->hide ();
	delete sub;
}

//calc the current start cylinder for selected partition at device
void CDriveSetup::setStartCylinder()
{
	if (next_part_number == 0)
		start_cylinder = getPartData(current_device, 0, START_CYL, NO_REFRESH) + 1;
	else 
		start_cylinder = getPartData(current_device, next_part_number-1, END_CYL, NO_REFRESH) + 1;
}


// show progress status
void CDriveSetup::showStatus(const int& progress_val, const string& msg, const int& max)
{
	CProgressBar pb;
	string s_info = msg;
	cout<<"[drive setup] "<<msg<<endl;
	frameBuffer->paintBoxRel(pb_x, pb_y, pb_w, pb_h, COL_MENUCONTENT_PLUS_0, RADIUS_MID);
	pb.paintProgressBar(pb_x+10, pb_y+pb_h-20-SHADOW_OFFSET, pb_w-20, 16, progress_val, max, 0, 0, COL_SILVER, COL_INFOBAR_SHADOW, msg.c_str(), COL_MENUCONTENT);
}

bool CDriveSetup::formatPartition(const int& device_num, const int& part_number)
// make all steps to create, formating and mount partitions
{
	CNeutrinoApp::getInstance()->execute_start_file(NEUTRINO_FORMAT_START_SCRIPT);

	bool ret = true;
	unsigned long long raw_size = atol(d_settings.drive_partition_size[device_num][part_number]);
	if (raw_size > 0)
		part_size = raw_size*1024*1024; //bytes
	else
		part_size = 0;

	string fs = d_settings.drive_partition_fstype[device_num][part_number]; // filesystem
	string mp = d_settings.drive_partition_mountpoint[device_num][part_number]; // mountpoint

	
	fb_pixel_t * pixbuf = new fb_pixel_t[pb_w * pb_h];
	if (pixbuf != NULL)
		frameBuffer->SaveScreen(pb_x, pb_y, pb_w, pb_h, pixbuf);
	

	int i = 1;
 	showStatus(0, "formating...", 7);

	if (unmountDevice(device_num))
		showStatus(i++, "unmount device...");
		

	string partname = getPartName(device_num, part_number);

	if ((isActivePartition(partname)) || (isSwapPartition(partname))) 
	{
		showStatus(i++, "deleting partition...");
			if(mkPartition(device_num, DELETE, part_number, start_cylinder, part_size /*bytes*/))
				showStatus(i, "deleting partition...ok");
	}

	showStatus(i, "creating partition...");
	if (mkPartition(device_num, ADD, part_number, start_cylinder, part_size /*bytes*/))
	{
		i++;
		showStatus(i, "formating...");
		if (mkFs(device_num, part_number, fs)) /*filesystem*/ 
		{			
			i++;
	 		showStatus(i, "checking filesystem...");
	 		if (chkFs(device_num, part_number, fs)) 
			{/*checking fs*/
				i++;
				showStatus(i, "mounting...");
				if (!d_settings.drive_partition_activ[device_num][part_number]) 
				{
					showStatus(i, "partition not mounted, please activate..."); 
					ShowLocalizedHint(LOCALE_MESSAGEBOX_INFO, LOCALE_DRIVE_SETUP_MSG_PARTITION_NOT_MOUNTED_PLEASE_ACTIVATE, width, msg_timeout, NEUTRINO_ICON_INFO);
				}	
				else if (mountDevice(device_num)) 
				{ /*mounting*/
					showStatus(i, "partitions mounted...");
					ShowLocalizedHint(LOCALE_MESSAGEBOX_INFO, LOCALE_DRIVE_SETUP_MSG_PARTITION_CREATE_SUCCESS, width, msg_timeout, NEUTRINO_ICON_PARTITION);
				}
				else 
					ret = false;		
			}
			else
				ret = false; 
 		}
 		else
 			ret = false;
	}
	else
		ret = false;

	CNeutrinoApp::getInstance()->execute_start_file(NEUTRINO_FORMAT_END_SCRIPT);

	if (pixbuf != NULL) 
	{
		frameBuffer->RestoreScreen(pb_x, pb_y, pb_w, pb_h, pixbuf);
		delete[] pixbuf;
	}

	return ret;
}

// loads basicly data from devices generated by fdisk
void CDriveSetup::loadFdiskData()
{
	string device;

 	// cleanup vectors before
	v_device_size.clear();
	v_device_cylcount.clear();
	v_device_cyl_size.clear();
	v_device_heads_count.clear();
	v_device_sectors_count.clear();

	for (int i = 0; i < MAXCOUNT_DRIVE; i++) 
	{
		loadPartlist(i);

		// disc
		device = drives[i].device;

		// generate fdisk part table
		if (loadFdiskPartTable(i))
		{
			// device size
			v_device_size.push_back(getFileEntryLong(PART_TABLE, device, 4));

			// count of cylinders
			v_device_cylcount.push_back(getFileEntryLong(PART_TABLE, "heads", 4));

			// count of heads
			v_device_heads_count.push_back(getFileEntryLong(PART_TABLE, "heads", 0));

			// count of sectors
			v_device_sectors_count.push_back(getFileEntryLong(PART_TABLE, "sectors", 2));

			// sizes of cylinder
			v_device_cyl_size.push_back(getFileEntryLong(PART_TABLE, "Units", 8));	
		}
		else 
		{
			v_device_size.push_back(0);
			v_device_cylcount.push_back(0);
			v_device_heads_count.push_back(0);
			v_device_sectors_count.push_back(0);
			v_device_cyl_size.push_back(0);
		}
		unlink(PART_TABLE);
	}

}

// returns the first possible unused partition number 0...MAXCOUNT_PARTS-1 (not the real numbers 1...n)
unsigned int CDriveSetup::getFirstUnusedPart(const int& device_num)
{
	unsigned int ret = 0;

	loadPartlist(device_num);

	for (unsigned int i = 0; i<MAXCOUNT_PARTS; i++) 
	{
		if (!isActivePartition(v_partname[i])) 
		{
			ret = i;
			break;
		}
	}

	return ret;
}


// possible part names, generated with patterns *********************
typedef struct part_patterns_t
{
	const string part;
} part_patterns_struct_t;

const part_patterns_struct_t part_pattern[MAXCOUNT_DRIVE] =
{
	{"/dev/ide/host0/bus0/target0/lun0/part"},	//MASTER
	{"/dev/ide/host0/bus0/target1/lun0/part"},	//SLAVE
 	{"/dev/mmc/disc0/part"},			//MMCARD	
};

// writes partition names to vector collection v_partname
void CDriveSetup::loadPartlist(const int& device_num)
{
	// cleanup
	v_partname.clear(); 

	char p[1];
	string partname;
	unsigned int p_count;
	
	if (device_num < 0) 
	{ // collect all partitions for all devices
		for (unsigned int i=0; i < MAXCOUNT_DRIVE; i++) 
		{
			for (unsigned int ii=0; ii < MAXCOUNT_PARTS; ii++) 
			{
				p_count = ii+1;
				sprintf(p, "%d", p_count);
				partname = part_pattern[i].part + p;
				v_partname.push_back(partname);
			}
		}
	}
	else 
	{ // collect partitions only for one device
		for (unsigned int i=0; i < MAXCOUNT_PARTS; i++) 
		{
			p_count = i+1;
			sprintf(p, "%d", p_count);
			partname = part_pattern[device_num].part + p;
			v_partname.push_back(partname);
		}
	}
	
}

// unmount all mounted partition or swaps
bool CDriveSetup::unmountAll()
{
	bool ret = true;

	for (unsigned int i=0; i < MAXCOUNT_DRIVE; i++) 
	{
		if(!unmountDevice(i /*MASTER||SLAVE||MMCARD*/))
			ret = false;
	}

	return ret;
}

// unmount all mounted partition or swaps from device
bool CDriveSetup::unmountDevice(const int& device_num)
{
	bool ret = true;
	int i = device_num;

	for (unsigned int ii=0; ii < MAXCOUNT_PARTS; ii++) 
	{
		if(!unmountPartition(i, ii))
			ret = false;
	}

	return ret;
}

// unmount single mounted partition or swap
bool CDriveSetup::unmountPartition(const int& device_num /*MASTER||SLAVE||MMCARD*/, const int& part_number)
{
	int ret_val = 0;

	string partname = getPartName(device_num, part_number);

	if((access(partname.c_str(), R_OK) !=0) || (!isActivePartition(partname))) 
	{ // exit if no available
// 		cout<<"[drive setup] unmount "<<partname<< ", nothing to do...ok"<< endl;
 		return true;
	}
	else 
	{

		string swapoff_cmd = SWAPOFF + partname;

		if (isSwapPartition(partname)) 
		{ // unmount swap
			ret_val = swapoff(partname.c_str());
			if (ret_val !=0) 
			{
				s_err += "unmountPartition (swapoff): ";
				s_err += strerror(errno);
				s_err += "\n";
				s_err += partname;
				cerr<<"[drive setup] "<<s_err<<endl<<endl;
				return false;
			}
			else 
			{
// 				cout<<"[drive setup] unmount swap "<<partname<< "...ok"<< endl;
				return true;
			}
		}
		if (isMountedPartition(partname)) 
		{ // umount mounts
			ret_val = umount(getMountInfo(partname, MOUNTPOINT).c_str());
			if (ret_val !=0) 
			{
				s_err += "unmountPartition: ";
				s_err += strerror(errno);
				s_err += "\n";
				s_err += partname;
				cerr<<"[drive setup] "<<s_err<<endl<<endl;
				return false;
			}
			else 
			{
// 				cout<<"[drive setup] unmount "<<partname<< "...ok"<< endl;
				return true;
			}
		}
	}

	return true;

}

// returns mount stat of device, isMountedPartition("HDA1") = true=mounted, false=not mounted
bool CDriveSetup::isMountedPartition(const string& partname)
{
	string mounts;
	string partx = partname;

	if(access(partx.c_str(), R_OK) !=0) 
	{// exit if no available
		return false;
	}

	if(access(PROC_MOUNTS, R_OK) !=0) 
	{// exit if no available
		cerr<<"[drive setup] "<<__FUNCTION__ <<": "<<PROC_MOUNTS<<" not found..."<< endl;
		return false;
	}

	// get mounts
	ifstream f(PROC_MOUNTS);
	if (!f)	
	{
		cerr<<"[drive setup] "<<__FUNCTION__ <<": error while open "<<PROC_MOUNTS<<" "<< strerror(errno)<< endl;
		return false;
	}
	char ch;
	while(f.get(ch)) 
	{
		mounts += ch;
	}
	f.close();

	//search matching entries and return result
	string::size_type loc = mounts.find( partx, 0 );
	return (( loc != string::npos ) ? true : false);
}

//returns partname from device
string CDriveSetup::getPartName(const int& device_num, const int& part_num)
{
	// real partition name e.g. /dev/hda1 ...
	loadPartlist(device_num);
	return v_partname[part_num];
}

// returns true if modul/driver is loaded, false if unloaded e.g: isModulLoaded("dboxide")
bool CDriveSetup::isModulLoaded(const string& modulname)
{
	string temp = "";
	set <string> modules;
	ifstream input(PROC_MODULES);

	while( input >> temp ) 
	{
		modules.insert(temp);
		getline(input,temp);
	}

	return modules.count(modulname);
}


typedef struct ide_modules_t
{
	const int modul_num;
	const string modul;
} ide_modules_struct_t;

#define IDE_MODULES_COUNT 4
const ide_modules_struct_t ide_modules[IDE_MODULES_COUNT] =
{
	{0, IDE_CORE	},
	{1, DBOXIDE	},
	{2, IDE_DETECT	},
	{3, IDE_DISK	}
};

// load/apply/testing modules and returns true if it sucessfully
bool CDriveSetup::initIdeDrivers(const bool irq6)
{
	if (unloadIdeDrivers()) // unload previously loaded modules
		printf("[drive setup] ide modules unloaded...\n");

	CProgressBar 	pb;

	fb_pixel_t * pixbuf = new fb_pixel_t[pb_w * pb_h];
	if (pixbuf != NULL)
		frameBuffer->SaveScreen(pb_x, pb_y, pb_w, pb_h, pixbuf);

	v_init_ide_L_cmds.clear();
	
	// exec, test commands and add commands to vector
	for (unsigned int i = 0; i < IDE_MODULES_COUNT; i++)
	{
		string modulname = ide_modules[i].modul;
		string load_cmd = LOAD + modulname;

		if (i != LOAD_DBOXIDE)
			v_init_ide_L_cmds.push_back(load_cmd);
		else
			v_init_ide_L_cmds.push_back(((!irq6) ? load_cmd : load_cmd + " irq6=1")); // observe irq6 option!)


		if (!isModulLoaded(modulname))
		{
			if (CNeutrinoApp::getInstance()->execute_sys_command(v_init_ide_L_cmds[i].c_str())!=0) 
			{
				cerr<<"[drive setup] "<<__FUNCTION__ <<": loading "<< modulname<< "...failed "<< strerror(errno)<<endl;
			}
		}

		// TESTING loaded modul
		if (!isModulLoaded(modulname)) 
		{ 
			cerr<<"[drive setup] "<<__FUNCTION__ <<": modul "<< modulname<< " not loaded, loading...failed"<<endl;
			return false;
		}
			// show load progress on screen
			frameBuffer->paintBoxRel(pb_x, pb_y, pb_w, pb_h, COL_MENUCONTENT_PLUS_0, RADIUS_MID);
			pb.paintProgressBar(pb_x+10, pb_y+pb_h-20-SHADOW_OFFSET, pb_w-20, 16, i, IDE_MODULES_COUNT-1, 0, 0, COL_SILVER, COL_INFOBAR_SHADOW, modulname.c_str(), COL_MENUCONTENT);
	}

	// refreshing 
	loadHddCount();

	if (pixbuf != NULL) 
	{
		frameBuffer->RestoreScreen(pb_x, pb_y, pb_w, pb_h, pixbuf);
		delete[] pixbuf;
	}

	return true;
}

// load/apply/testing modules and returns true on sucess
bool CDriveSetup::initFsDrivers(bool do_unload_first)
{
	// reset
	v_init_fs_L_cmds.clear();

	CProgressBar 	pb;

	fb_pixel_t * pixbuf = new fb_pixel_t[pb_w * pb_h];
	if (pixbuf != NULL)
		frameBuffer->SaveScreen(pb_x, pb_y, pb_w, pb_h, pixbuf);

	bool ret = true;
	unsigned int modul_count = v_fs_modules.size();

	// exec commands
	for (unsigned int i = 0; i < modul_count; i++)
	{
		if (v_fs_modules[i]=="swap") break; // skip this

		string depend_modules[2] = {"jbd", "fat"};

		if (do_unload_first) 
		{
			if (v_fs_modules[i]==depend_modules[0]) // unloading depends modules first
				unloadModul("ext3");
			if (v_fs_modules[i]==depend_modules[1])
				unloadModul("vfat");
		}

		if (initModul(v_fs_modules[i], do_unload_first)) 
		{
			if (v_fs_modules[i]=="ext3") // loading depends modules first
				v_init_fs_L_cmds.push_back(LOAD + depend_modules[0]);
			if (v_fs_modules[i]=="vfat")
				v_init_fs_L_cmds.push_back(LOAD + depend_modules[1]);
			v_init_fs_L_cmds.push_back(LOAD + v_fs_modules[i]);
		}
		else
			ret = false;

		// show load progress on screen
		string 	screen_msg = "load ";
			screen_msg += v_fs_modules[i];
		frameBuffer->paintBoxRel(pb_x, pb_y, pb_w, pb_h, COL_MENUCONTENT_PLUS_0, RADIUS_MID);
		pb.paintProgressBar(pb_x+10, pb_y+pb_h-20-SHADOW_OFFSET, pb_w-20, 16, i, modul_count, 0, 0, COL_SILVER, COL_INFOBAR_SHADOW, screen_msg.c_str(), COL_MENUCONTENT);
	}

	if (pixbuf != NULL) 
	{
		frameBuffer->RestoreScreen(pb_x, pb_y, pb_w, pb_h, pixbuf);
		delete[] pixbuf;
	}

	return true;
}

// unload mmc modul and returns true on success
bool CDriveSetup::unloadMmcDrivers()
{
	unsigned int i = 0;
	while (i < v_mmc_modules.size())
	{
		if (isModulLoaded(v_mmc_modules[i])) 
		{
			if (!unloadModul(v_mmc_modules[i]))
				return false;
		}
		i++;
	}
	s_init_mmc_cmd = "";

	return true;
}

// load/apply/testing mmc modul and returns true on success
bool CDriveSetup::initMmcDriver()
{
	// unload first
	if (!unloadMmcDrivers())
		return false;

	string modul_name = (string)d_settings.drive_mmc_module_name;

	s_init_mmc_cmd = LOAD;
	s_init_mmc_cmd += modul_name;

	// exec command
	if (!initModul(modul_name)) 
	{
		cerr<<"[drive setup] "<<__FUNCTION__ <<": loading "<<modul_name<< " failed..."<<endl;
		return false;
	}

	return true;
}

// unload modules and returns true on sucess
bool CDriveSetup::unloadFsDrivers()
{
	CProgressBar 	pb;

	fb_pixel_t * pixbuf = new fb_pixel_t[pb_w * pb_h];
	if (pixbuf != NULL)
		frameBuffer->SaveScreen(pb_x, pb_y, pb_w, pb_h, pixbuf);

	bool ret = true;
	unsigned int modul_count = v_fs_modules.size();

	// exec commands
	for (unsigned int i = 0; i < modul_count; i++)
	{
		if (v_fs_modules[i]=="jbd")
			unloadModul("ext3");

		if (v_fs_modules[i]=="fat")
			unloadModul("vfat");
				
		if (!unloadModul(v_fs_modules[i]))
			ret = false;

		// show unload progress on screen
		string 	screen_msg = "unload ";
			screen_msg += v_fs_modules[i];
		frameBuffer->paintBoxRel(pb_x, pb_y, pb_w, pb_h, COL_MENUCONTENT_PLUS_0, RADIUS_MID);
		pb.paintProgressBar(pb_x+10, pb_y+pb_h-20-SHADOW_OFFSET, pb_w-20, 16, i, modul_count, 0, 0, COL_SILVER, COL_INFOBAR_SHADOW, screen_msg.c_str(), COL_MENUCONTENT);
	}

	if (pixbuf != NULL) 
	{
		frameBuffer->RestoreScreen(pb_x, pb_y, pb_w, pb_h, pixbuf);
		delete[] pixbuf;
	}

	return ret;
}

// unloads ide modules, returns true on success
bool CDriveSetup::unloadIdeDrivers()
{
	CProgressBar pb;

	fb_pixel_t * pixbuf = new fb_pixel_t[pb_w * pb_h];
	if (pixbuf != NULL)
		frameBuffer->SaveScreen(pb_x, pb_y, pb_w, pb_h, pixbuf);

	// exec all commands
	int i = IDE_MODULES_COUNT-1;
	while (i > -1)
	{
		if (!unloadModul(ide_modules[i].modul))
			return false;
		// painting load progress on screen
		frameBuffer->paintBoxRel(pb_x, pb_y, pb_w, pb_h, COL_MENUCONTENT_PLUS_0, RADIUS_MID);
		pb.paintProgressBar(pb_x+10, pb_y+pb_h-20-SHADOW_OFFSET, pb_w-20, 16, i, IDE_MODULES_COUNT-1, 0, 0, COL_SILVER, COL_INFOBAR_SHADOW, ide_modules[i].modul.c_str(), COL_MENUCONTENT);
		i--;
	}

	if (pixbuf != NULL) 
	{
		frameBuffer->RestoreScreen(pb_x, pb_y, pb_w, pb_h, pixbuf);
		delete[] pixbuf;
	}

	return true;
}

#define DEP_MODULES_COUNT 2
typedef struct dep_modules_t
{
	const string dep_moduls;
	const string pre_moduls;
} dep_modules_struct_t;

const dep_modules_struct_t modules[DEP_MODULES_COUNT] =
{
	{"ext3", "jbd"},
	{"vfat", "fat"},
};

// loads modules, returns true on success
bool CDriveSetup::initModul(const string& modulname, bool do_unload_first)
{
	string load_cmd =  LOAD + modulname;

	// loading depend modules
	if ((modulname == modules[0].dep_moduls) || (modulname == modules[1].dep_moduls)) 
	{
		for (unsigned int i=0; i < DEP_MODULES_COUNT ; ++i) 
		{
			string load_pre_cmd = LOAD + modules[i].pre_moduls;

			if (!isModulLoaded(modules[i].pre_moduls))
			{
				if (CNeutrinoApp::getInstance()->execute_sys_command(load_pre_cmd.c_str())!=0) 
				{
					cerr<<"[drive setup] "<<__FUNCTION__ <<": load depend modul "<< modules[i].pre_moduls<<" for "<< modules[i].dep_moduls<<"...failed"<<endl;
				}
			}

			if (!isModulLoaded(modules[i].pre_moduls)) 
			{ // check loaded modules
				cerr<<"[drive setup] "<<__FUNCTION__ <<": depend modul "<< modules[i].pre_moduls<<" not loaded, loading "<< modules[i].dep_moduls<<"...failed"<<endl;
				return false;
			}
		}
	}
	

	if (do_unload_first) 
	{
		if (unloadModul(modulname)) 
		{// ensure that the module is currently unloaded
			if (!isModulLoaded(modulname))
			{
				if (CNeutrinoApp::getInstance()->execute_sys_command(load_cmd.c_str()) !=0) 
				{
					cerr<<"[drive setup] "<<__FUNCTION__ <<": load "<<modulname<< "...failed "<<strerror(errno)<<endl;
				}
			}
			if (!isModulLoaded(modulname)) 
			{ // check loaded modules
				cerr<<"[drive setup] "<<__FUNCTION__ <<": modul "<<modulname<< "not loaded"<<endl;
				return false;
			}
		}
	}
	else 
	{
		if (!isModulLoaded(modulname))
		{
			if (CNeutrinoApp::getInstance()->execute_sys_command(load_cmd.c_str()) !=0) 
			{
				cerr<<"[drive setup] "<<__FUNCTION__ <<": load "<<modulname<< "...failed "<<strerror(errno)<<endl;
			}
		}
			if (!isModulLoaded(modulname)) 
			{ // check loaded modules
				cerr<<"[drive setup] "<<__FUNCTION__ <<": modul "<<modulname<< "not loaded"<<endl;
				return false;
			}
	}

	return true;
}

// unloads modules, returns true on success
bool CDriveSetup::unloadModul(const string& modulname)
{
	string 	unload_cmd = UNLOAD + modulname;

	if (isModulLoaded(modulname))
	{
		if (CNeutrinoApp::getInstance()->execute_sys_command(unload_cmd.c_str()) !=0)
			cerr<<"[drive setup] "<<__FUNCTION__ <<": unload "<<modulname<< "...failed "<<strerror(errno)<<endl;
	}

	if (isModulLoaded(modulname))
	{
		cerr<<"[drive setup] "<<__FUNCTION__ <<": modul "<<modulname<< "not unloaded"<<endl;
		return false;
	}
	return true;
}

// saves settings: load/unload modules and writes init file
bool CDriveSetup::saveHddSetup()
{
	bool res = true;
	bool ide_disabled = true;
	
	if (!writeDriveSettings())
		res = false;

	if (!unmountAll())
		res = false;

	if (d_settings.drive_activate_ide == IDE_ACTIVE) 
		ide_disabled = (!initIdeDrivers() ? true : false);
	
	if (d_settings.drive_activate_ide == IDE_ACTIVE_IRQ6) 
		ide_disabled = (!initIdeDrivers(true) ? true : false);

	if (d_settings.drive_activate_ide == IDE_OFF) 
		ide_disabled = unloadIdeDrivers();

	// hdparm
	if (ide_disabled)
	{
		if (!loadHddParams(true/*reset*/))
			res = false;
		if (!unloadFsDrivers())
			res = false;
	}
	else 
	{
		if (!loadHddParams(false))
			res = false;
		if (!initFsDrivers())
			res = false;
	}

	if (!ide_disabled)
		res = true;

	// mmc stuff
	if ((string)d_settings.drive_mmc_module_name != g_Locale->getText(LOCALE_OPTIONS_OFF)) 
	{
		if (!initMmcDriver()) 
		{
			res = false;
		}
		if (!initFsDrivers(false)) 
		{
			res = false;
		}
	}
	else 
	{
		if (!unloadMmcDrivers())
			res = false;
	}

	// fstab
	if (d_settings.drive_use_fstab) 
	{
		if (!mkFstab())
			res = false;
	}
	else
		mkMounts();

#ifdef ENABLE_NFSSERVER
	// exports
	if (CNeutrinoApp::getInstance()->execute_sys_command(NFS_STOP_SCRIPT) == 0) //first stop server
	{
		if (mkExports())
		{
			if (CNeutrinoApp::getInstance()->execute_sys_command(NFS_START_SCRIPT) != 0)//start server
				cerr << "[drive setup] "<<__FUNCTION__ <<": error while executing "<<NFS_START_SCRIPT<<"..."<< strerror(errno)<<endl;
		}		
	}
	else
		cerr << "[drive setup] "<<__FUNCTION__ <<":  error while executing "<<NFS_STOP_SCRIPT<<"..."<< strerror(errno)<<endl;			
#endif
	
	// write and linking init files
	if ((res) && (writeInitFile(ide_disabled)) && (linkInitFiles())) 
	{
		writeDriveSettings();

		neutrino_locale_t msg_locale;

		if ((ide_disabled) && (!device_isActive[MMCARD]))
			msg_locale = LOCALE_DRIVE_SETUP_MSG_SAVED_DISABLED;
		else
			msg_locale = LOCALE_DRIVE_SETUP_MSG_SAVED;

		//neutrino_locale_t msg_locale = (!ide_disabled) ? LOCALE_DRIVE_SETUP_MSG_SAVED : LOCALE_DRIVE_SETUP_MSG_SAVED_DISABLED;
		ShowLocalizedHint(LOCALE_DRIVE_SETUP_HEAD, msg_locale, width, msg_timeout, NEUTRINO_ICON_INFO);
	}
	else 
	{
		cerr<<"[drive setup] "<<__FUNCTION__ <<": errors while applying settings..."<<endl;
		ShowLocalizedHint(LOCALE_MESSAGEBOX_ERROR, LOCALE_DRIVE_SETUP_MSG_ERROR_SAVE_FAILED, width, msg_timeout, NEUTRINO_ICON_ERROR);
		res = false;
	}

	return res;
}

// writes init files from line collection, parameter clear==true will remove command entries, == disabled interface
bool CDriveSetup::writeInitFile(const bool clear)
{
	bool ret = true;
	
	string init_file [INIT_FILE_TYPE_NUM_COUNT] = {getInitIdeFilePath(), getInitMountFilePath()};

	string 	sys_mounts = getDefaultSysMounts();

	ofstream initfile[INIT_FILE_TYPE_NUM_COUNT] ;

	// write init files
	for (uint i=0; i<(INIT_FILE_TYPE_NUM_COUNT) ; ++i) 
	{
		initfile[i].open(init_file[i].c_str());

		if (!initfile[i]) 
		{ // Error while open
       			cerr << "[drive setup] "<<__FUNCTION__ <<":  write error "<<init_file[i]<<", please check permissions..."<< strerror(errno)<<endl;
			return false;
		}
		// add head lines
		initfile[i] << getInitFileHeader(init_file[i]) <<endl;
	}	


	if (clear) 
	{ // interface is disabled 
		initfile[INIT_FILE_MODULES] << "echo "<<char(34)<<"ide-interface disabled"<<char(34)<<endl; // write disable tag
		if (d_settings.drive_use_fstab)
			initfile[INIT_FILE_MOUNTS] <<("mount -a")<<endl;
		else
			initfile[INIT_FILE_MOUNTS] <<(sys_mounts)<<endl;
	}
	else 
	{
 		// write commands for loading the filesystem modules and hdparm commands
		string md_txt = getInitFileModulEntries();
		initfile[INIT_FILE_MODULES] << (md_txt)<<endl;

		// prepare mount entries
		if (d_settings.drive_use_fstab) 
		{
			string 	m_txt =  getInitFileMountEntries();
			initfile[INIT_FILE_MOUNTS] <<(m_txt)<<endl; // using fstab
		}
		else 
		{
			//initfile <<("mount -a")<<endl; 
			initfile[INIT_FILE_MOUNTS] <<(sys_mounts)<<endl;
		}

		//don't using fstab, write mount entries to init file
		if (!d_settings.drive_use_fstab) 
		{ 
			for (unsigned int m=0; m<(v_mount_entries.size()) ; ++m) 
			{
				initfile[INIT_FILE_MOUNTS] << v_mount_entries[m]<<endl;
			}
		}
		
	}


	for (uint i=0; i<(INIT_FILE_TYPE_NUM_COUNT) ; ++i) 
	{
 		initfile[i].close();
	
		// INIT_IDE_SCRIPT_PATH must be executable
		int chmod_ret = chmod(init_file[i].c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);

		if ( chmod_ret !=0 )
		{
			if (errno == 1)
				ret = true;
			else 
			{
				cerr<<"[drive setup] "<<__FUNCTION__ <<": "<<init_file[i]<<" "<<strerror(errno)<<endl;
				return false;
			}
		}
	
		cout<<"[drive setup] "<<__FUNCTION__ <<": writing "<<init_file[i]<< " ...ok"<<endl;
	}
	
	return ret;
}

// returns true if found hdd, disc is eg: "/proc/ide/ide0/hda"
bool CDriveSetup::foundHdd(const string& disc)
{
	string hdd = disc/* + "/model"*/;
	if(access(hdd.c_str(), R_OK) == 0)
		return true;
	else
		return false;
}

// returns the model name of hdd, disc is eg: "/proc/ide/ide0/hda"
string CDriveSetup::getModelName(const string& disc)
{
	string filename = disc + "/model";
	string model;

	if (foundHdd(disc))
	{
		ifstream f(filename.c_str());
		char ch;
		while(f.get(ch)) {
			model += ch;
		}
		f.close();
		return model;
	}
	return g_Locale->getText(LOCALE_DRIVE_SETUP_MODEL_UNKNOWN);
}

// converts size string to a viable unit e.g. MB or GB, usable in menue entries, as std::string
string CDriveSetup::convertByteString(const unsigned long long& byte_size /*bytes*/)
{
	string ret = "";
	unsigned long long size = byte_size;
	const unsigned int i_size = 4;
	unsigned long l_size[i_size] = {	size/1024 /*kb*/,
						size/1024/1024 /*MB*/,
						size/1024/1024/1024 /*GB*/,
						size/1024/1024/1024/1024 /*TB*/};
	string s_size[i_size];
					
	ostringstream str_size[i_size];

	for (unsigned int i = 0; i < i_size; i++)
	{
		str_size[i] << l_size[i];
		string s_temp_size(str_size[i].str());
		s_size[i] = s_temp_size;
	}

	if (size < 0x400 /*1MB*/) 
	{
		ret = s_size[0] + " kB";
	}
	else if ((size > 0x400 /*1MB*/) && (size < 0x40000000 /*1GB*/)) 
	{
		ret = s_size[1] ;
		ret += " MB";
	}
	else if ((size > 0x40000000 /*1GB*/) && (size < 0x280000000LL /*10GB*/)) 
	{
		ret = s_size[2] ;
		ret += " GB (" + s_size[1] + " MB)";
	}
	else if ((size > 0x280000000LL /*10GB*/) && (size < 0x10000000000LL /*1TB*/)) 
	{
		ret = s_size[2] ;
		ret += " GB";
	}
	else if (size >= 0x10000000000LL /*1TB*/)
	{
		ret = s_size[3] ;
		ret += " TB";
	}
	else
		ret += "0";
		
	
	return ret;
}

// void CDriveSetup::genDriveSizes()
// // collects sizes of all devices to vector v_device_size
// {
// 	v_device_size.clear();
// 
// 	for (unsigned int i = 0; i < MAXCOUNT_DRIVE; i++){
// 		v_device_size.push_back(getDeviceSize(i));
// 	}
// 
// 	for (unsigned int i = 0; i < MAXCOUNT_DRIVE; i++){
// 		cout<<v_device_size[i]<<endl;
// 	}	
// }


// unsigned long CDriveSetup::getFreeDiskspace(const char *mountpoint)
// // returns free disc space from mountpoint e.g "/hdd"
// {
// 	struct statfs s;
// 	statfs(mountpoint, &s);
// 	printf("[drive setup] getFreeDiskspace from mountpoint %s\n", mountpoint);
// 	unsigned long free=s.f_bfree/1024*s.f_bsize/1024;
// 	return free;
// }

// collects temperatures of all devices to vector v_device_temp
void CDriveSetup::loadDriveTemps()
{
	v_device_temp.clear();

	for (unsigned int i = 0; i < MAXCOUNT_DRIVE; i++)
	{
		if (i !=MMCARD)
			v_device_temp.push_back(getHddTemp(i));
		else
			v_device_temp.push_back("0"); // we don't need temperatur for mmc
	}	
}

// returns free disc temperature from disc via hddtemp, device_num is MASTER||SLAVE
string CDriveSetup::getHddTemp(const int& device_num)
{
	int readtemp = 0;
	string disc = drives[device_num].device;

		string	cmdtemp  = HDDTEMP;
			cmdtemp += " -n -w -q ";
			cmdtemp += disc;
			cmdtemp += " ";
			cmdtemp += "> /tmp/hdtemp";
			cmdtemp += " ";
			cmdtemp += "2>/dev/null";

	int cmd_res = CNeutrinoApp::getInstance()->execute_sys_command(cmdtemp.c_str());

	if (cmd_res!=0)
	{
		string cerr_content = "[drive setup] getHddTemp: reading temperature from hdd failed...";
		if (cmd_res == 127) 
			cerr<<cerr_content<<HDDTEMP" not installed"<<endl;
		else
			cerr<<cerr_content<<endl;

		return "0";
	}

	FILE *f=fopen( "/tmp/hdtemp", "r");
	if (!f)
		return "0";

	fscanf(f, "%d", &readtemp );
	fclose(f);
	unlink("/tmp/hdtmp");
	
	ostringstream Str;
	Str << readtemp;
	string temp(Str.str());
	
	return temp;
}

// set/apply/testing hdparm commands and returns true on sucess, parameter "reset = true" sets no commands
bool CDriveSetup::loadHddParams(const bool do_reset)
{
	char opt_hdparm[2/*MASTER, SLAVE*/][15];
	sprintf(opt_hdparm[MASTER]," -S%d -W%d -c1 ", atoi(d_settings.drive_spindown[MASTER])/5, d_settings.drive_write_cache[MASTER]);
	sprintf(opt_hdparm[SLAVE]," -S%d -W%d -c1 ", atoi(d_settings.drive_spindown[SLAVE])/5, d_settings.drive_write_cache[SLAVE]);

	const string str_hdparm_cmd[2] = {	((device_isActive[MASTER]) ? "hdparm" + (string)opt_hdparm[MASTER]  + drives[MASTER].device : ""	),
						((device_isActive[SLAVE]) ? "hdparm" + (string)opt_hdparm[SLAVE] + drives[SLAVE].device : ""		)};
	
	// test/define all commands
	// do nothing on reset
	if (!do_reset)
	{
		int i = 0;
		while (i < MAXCOUNT_DRIVE) 
		{
			if (device_isActive[i])
 			{
				if (CNeutrinoApp::getInstance()->execute_sys_command(str_hdparm_cmd[i].c_str())!=0)
				{ 
					cerr<<"[drive setup] "<<__FUNCTION__ <<": executing "<< str_hdparm_cmd[i]<<" ...failed"<<endl;
 					return false;
				}
 			}
		i++;
		}
	}

	// add to collection
	v_hdparm_cmds.clear();
	for (unsigned int i = 0; i < 2; i++) 
	{
		v_hdparm_cmds.push_back(str_hdparm_cmd[i]);
	}

	return true;
}

/* returns mode of partitions, true=active, false=not active, usage: bool partActive = isActivePartition("/dev/hda1") */
bool CDriveSetup::isActivePartition(const string& partname)
{
 	string part_tab;
	string partx = partname;

	if(access(partx.c_str(), R_OK) !=0) {// exit if no available
 		//cerr<<"[drive setup] "<<partname<<" not found..."<< endl;
		return false;
	}

	if(access(PROC_PARTITIONS, R_OK) !=0) 
	{// exit if no available
		cerr<<"[drive setup] "<<__FUNCTION__ <<": "<<PROC_PARTITIONS<<" not found..."<< endl;
		return false;
	}

	// get parts
	ifstream f(PROC_PARTITIONS);
	if (!f)	
	{
		cerr<<"[drive setup] "<<__FUNCTION__ <<": error while open "<<PROC_PARTITIONS<<" "<< strerror(errno)<<endl;
		return false;
	}
	char ch;
	while(f.get(ch)) 
	{
		part_tab += ch;
	}
	f.close();

	//normalize partion name, remove "/dev/"
	partx.replace(partx.find("/dev/"), 5 ,"");

	//search matching entries and return result
	string::size_type loc = part_tab.find( partx, 0 );
	return (( loc != string::npos ) ? true : false);
}

// generate fstab file, returns true on success
bool CDriveSetup::mkFstab(bool write_defaults_only)
{
	string cur_mmc = getUsedMmcModulName();
	bool mmc_in_use = (!cur_mmc.empty()) ? true : false;

	if ((d_settings.drive_activate_ide != IDE_OFF) || (mmc_in_use)) 
	{// first mounting all hdd partitions if ide interface is activ
		mountAll();
	}
	else 
	{ // or  mounting all hdd partitions if ide interface is not activ
		unmountAll();
	}

	// set fstab path
	string fstab = getFstabFilePath();
	string timestamp = getTimeStamp();

	vector<string> v_fstab_entries;
	v_fstab_entries.push_back("# " + fstab + " generated from neutrino ide/mmc/hdd drive-setup\n #" +  getDriveSetupVersion() + " " +  timestamp );

	if ((fstab == FSTAB) || (write_defaults_only))
		v_fstab_entries.push_back(getDefaultFstabEntries()); // set default fstab entries


	if (!write_defaults_only) 
	{
	// collecting mount settings
		for (unsigned int i = 0; i < MAXCOUNT_DRIVE; i++) 
		{
			for (unsigned int ii = 0; ii < MAXCOUNT_PARTS; ii++) 
			{
				string partname = getPartName(i,ii);
				string mount_entry;
				if (isSwapPartition(partname)) 
				{
					mount_entry = partname;
					mount_entry += " none swap sw 0 0"; // TODO setup for fstab swap options
					v_fstab_entries.push_back(mount_entry);
				}
				else if (isMountedPartition(partname)) 
				{
					string mp = getMountInfo(partname, MOUNTPOINT);
					mount_entry = partname;
					mount_entry += " ";
					mount_entry += mp + " auto rw,sync 0 0"; // TODO setup for fstab mount options
					v_fstab_entries.push_back(mount_entry);
				}
			}
		}
	}

	// write fstab
	ofstream str_fstab(fstab.c_str());
 	if (!str_fstab) 
	{ // Error while open
       		cerr << "[drive setup] "<<__FUNCTION__ <<": write error "<<fstab<<", please check permissions..." << strerror(errno)<<endl;
		return false;
	}
	else 
	{
		for (unsigned int i = 0; i < v_fstab_entries.size(); i++) 
		{
			str_fstab << v_fstab_entries[i] <<endl;
		}
	}
	str_fstab.close();

	cout<<"[drive setup] "<<__FUNCTION__ <<": writing "<<fstab<< "...ok"<<endl;

	return true;
}

// generates mount entries for init file, returns true on success
void CDriveSetup::mkMounts()
{	
	string cur_mmc = getUsedMmcModulName();
	bool mmc_in_use = (!cur_mmc.empty()) ? true : false;

	if ((d_settings.drive_activate_ide != IDE_OFF) || (mmc_in_use)) 
	{// first mounting all hdd partitions if ide interface is activ
		mountAll();
	}
	else 
	{ // or  mounting all hdd partitions if ide interface is not activ
		unmountAll();
	}
		
	// collecting mount entries
	for (unsigned int i = 0; i < MAXCOUNT_DRIVE; i++) 
	{
		for (unsigned int ii = 0; ii < MAXCOUNT_PARTS; ii++) 
		{
			string partname = getPartName(i,ii);
			string mount_entry;
			if (isSwapPartition(partname)) 
			{
				mount_entry = "swapon ";
				mount_entry += partname;
				v_mount_entries.push_back(mount_entry);
			}
			else if (isMountedPartition(partname)) 
			{
				string mp = getMountInfo(partname, MOUNTPOINT);
				mount_entry = "mount ";
				mount_entry += partname;
				mount_entry += " " + mp;
				v_mount_entries.push_back(mount_entry);
			}

		}
	}
	if (!mkFstab(true))
		cerr<<"[drive setup] "<<__FUNCTION__ <<": generating default mount entries in fstab failed..."<<endl;
}

// collects spported and available filsystem modules, writes to vector v_fs_modules, return true on success
void CDriveSetup::loadFsModulList()
{
	DIR *mdir;
	struct dirent *entry;
	struct utsname u;

	string k_name;
	if (!uname(&u))
		k_name = u.release;

	string modulpath = "/lib/modules/" + k_name + "/kernel/fs";

	mdir = opendir(modulpath.c_str());
	if (!mdir) 
	{
		cerr<<"[drive setup] "<<__FUNCTION__ <<": error open directory "<<modulpath<< " "<< strerror(errno)<<endl;
		return ;
	}

	// cleanup  collection
	v_fs_modules.clear();

	// scan module dir
	if (mdir)
	{
		do
		{
		entry = readdir(mdir);
			if (entry)
			{
 				string dir_x = entry->d_name;
				if (	(dir_x == "ext3") 	||
					(dir_x == "ext2") 	||
					(dir_x == "reiserfs")	||
					(dir_x == "xfs")  	||
					(dir_x == "ntfs")  	||
					(dir_x == "hpfs")  	||
					(dir_x == "vfat"))  {
					v_fs_modules.push_back(dir_x);
				}
			}
		} while (entry);
	}
	closedir(mdir);

// 	// for log, show available filesystem moduls
// 	unsigned int i = 0;
// 	string modules = "[drive setup] available filesystem modules: ";
// 	while (i < v_fs_modules.size()){
// 		modules += v_fs_modules[i];
// 		modules += char(32);
// 		i++;
// 	}
// 	cout<<modules<<endl;

}

// collects spported and available mmc modules, writes to vector v_mmc_modules, return true on success
void CDriveSetup::loadMmcModulList()
{
	DIR *mdir;
	struct dirent *entry;
	struct utsname u;

	string k_name;
	if (!uname(&u))
		k_name = u.release;

	string modulpath = "/lib/modules/" + k_name + "/misc";

	mdir = opendir(modulpath.c_str());
	if (!mdir) 
	{
		cerr<<"[drive setup] "<<__FUNCTION__ <<": error while open "<<modulpath<< endl;
		return ;
	}

	// cleanup  collection
	v_mmc_modules.clear();

	// scan module dir
	if (mdir)
	{
		do
		{
		entry = readdir(mdir);
			if (entry)
			{
 				string mod_x = entry->d_name;
				if (	(mod_x == MF_MMC)  	||
					(mod_x == MF_MMC2) 	||
					(mod_x == MF_MMCCOMBO))  {
					string::size_type pos = mod_x.find( ".", 0 );
					mod_x.erase(pos);
					v_mmc_modules.push_back(mod_x);
				}
			}
		} while (entry);
	}
	closedir(mdir);

	v_mmc_modules.push_back(g_Locale->getText(LOCALE_OPTIONS_OFF));

////	for debugging, show available filesystem moduls
// 	unsigned int i = 0;
// 	string modules = "[drive setup] available mmc modules: ";
// 	while (i < v_mmc_modules.size()){
// 		modules += v_mmc_modules[i];
// 		modules += char(32);
// 		i++;
// 	}
// 	cout<<modules<<endl;

}

// returns name of current used mmc modul
string CDriveSetup::getUsedMmcModulName()
{
	unsigned int i = 0;

	while (i < v_mmc_modules.size())
	{
		if (isModulLoaded(v_mmc_modules[i]))
			return v_mmc_modules[i];
		i++;
	}

	return g_Locale->getText(LOCALE_OPTIONS_OFF);
}

// set count of active partitions for current devices
void CDriveSetup::calPartCount()
{
	// reset part_count
	for (unsigned int i = 0; i < MAXCOUNT_DRIVE; i++)
	{
		part_count[i]=0;
	}

	for (unsigned int i = 0; i < MAXCOUNT_DRIVE; i++) 
	{
		for (unsigned int ii = 0; ii < MAXCOUNT_PARTS; ii++) 
		{
			string partname = getPartName(i,ii);
			if (isActivePartition(partname))
				part_count[i]++;
		}			
	}
// 	printf("[drive setup] found partitions: master->%d slave->%d mmc->%d\n", part_count[MASTER], part_count[SLAVE], part_count[MMCARD]);
}


void CDriveSetup::loadHddCount()
{
	hdd_count = 0; // reset

	int i = 0;
	while (i < MAXCOUNT_DRIVE)
	{
		device_isActive[i /*MASTER||SLAVE*/]	= foundHdd(drives[i].proc_device /*IDE0HDA||IDE0HDB*/);
		if (device_isActive[i])
			hdd_count++;
		i++;
	}
// 	printf("[drive setup] found harddiscs: %d\n", hdd_count);
}


void CDriveSetup::loadHddModels()
{
	v_model_name.clear();
	int i = 0;
	while (i < MAXCOUNT_DRIVE)
	{
		v_model_name.push_back(getModelName(drives[i].proc_device /*IDE0HDA||IDE0HDB*/));
// 		printf("[drive setup] detected hdd model: %d. %s\n", i+1, v_model_name[i].c_str());
		i++;
	}
	
}


typedef struct fstypes_t
{
	const long fs_type_const;
	const char *fs_name;
} fstypes_struct_t;

#define FS_TYPES_COUNT 45
const fstypes_struct_t fstypes[FS_TYPES_COUNT] =
{
	{0x073717368,	"squashfs" /*SQUASHFS_SUPER_MAGIC*/	},
	{0xadf5,	"ADFS_SUPER_MAGIC"			},
	{0xADFF,	"AFFS_SUPER_MAGIC"			},
	{0x42465331,	"BEFS_SUPER_MAGIC"			},
	{0x1BADFACE,	"BFS_MAGIC"				},
	{0xFF534D42,	"cifs"/*"CIFS_MAGIC_NUMBER"*/		},
	{0x73757245,	"CODA_SUPER_MAGIC"			},
	{0x012FF7B7,	"COH_SUPER_MAGIC"			},
	{0x28cd3d45,	"cramfs"/*"CRAMFS_MAGIC"*/		},
	{0x1373,	"devfs"/*"DEVFS_SUPER_MAGIC"*/		},
	{0x00414A53,	"EFS_SUPER_MAGIC"			},
	{0x137D,	"EXT_SUPER_MAGIC"			},
	{0xEF51,	"ext2"/*"EXT2_OLD_SUPER_MAGIC"*/	},
	{0xEF53,	"ext2"/*"EXT2_SUPER_MAGIC"*/		},
	{0xEF53,	"ext3"/*"EXT3_SUPER_MAGIC"*/		},
	{0x4244,	"HFS_SUPER_MAGIC"			},
	{0xF995E849,	"HPFS_SUPER_MAGIC"			},
	{0x958458f6,	"HUGETLBFS_MAGIC"			},
	{0x9660,	"ISOFS_SUPER_MAGIC"			},
	{0x72b6,	"jffs2"/*"JFFS2_SUPER_MAGIC"*/		},
	{0x3153464a,	"JFS_SUPER_MAGIC"			},
	{0x137F,	"MINIX_SUPER_MAGIC"			},
	{0x138F,	"MINIX_SUPER_MAGIC2"			},
	{0x2468,	"MINIX2_SUPER_MAGIC"			},
	{0x2478,	"MINIX2_SUPER_MAGIC2"			},
	{0x4d44,	"vfat"/*"MSDOS_SUPER_MAGIC"*/		},
	{0x564c,	"NCP_SUPER_MAGIC"			},
	{0x6969,	"nfs"/*"NFS_SUPER_MAGIC"*/		},
	{0x5346544e,	"NTFS_SB_MAGIC"				},
	{0x9fa1,	"OPENPROM_SUPER_MAGIC"			},
	{0x9fa0,	"procfs"/*"PROC_SUPER_MAGIC"*/		},
	{0x002f,	"QNX4_SUPER_MAGIC"			},
	{0x52654973,	"reiserfs"/*"REISERFS_SUPER_MAGIC"*/	},
	{0x7275,	"ROMFS_MAGIC"				},
	{0x517B,	"smbfs"/*"SMB_SUPER_MAGIC"*/		},
	{0x012FF7B6,	"SYSV2_SUPER_MAGIC"			},
	{0x012FF7B5,	"SYSV4_SUPER_MAGIC"			},
	{0x01021994,	"tmpfs"/*"TMPFS_MAGIC"*/		},
	{0x15013346,	"UDF_SUPER_MAGIC"			},
	{0x00011954,	"UFS_MAGIC"				},
	{0x9fa2,	"USBDEVICE_SUPER_MAGIC"			},
	{0xa501FCF5,	"VXFS_SUPER_MAGIC"			},
	{0x012FF7B4,	"XENIX_SUPER_MAGIC"			},
	{0x58465342,	"xfs"/*"XFS_SUPER_MAGIC"*/		},
	{0x012FD16D,	"_XIAFS_SUPER_MAGIC" 			}
};

/* returns fs type */
const char *CDriveSetup::getFsTypeStr(long &fs_type_const)
{
	for (unsigned int i = 0; i < FS_TYPES_COUNT; i++)
	{
		if (fs_type_const == fstypes[i].fs_type_const)
			return fstypes[i].fs_name;
	}
	
	return "unknown filesystem";
}


// returns device infos as long long, depends from parameter device_info 
long long CDriveSetup::getDeviceInfo(const char *mountpoint, const int& device_info)
{
	struct statfs s;
	long long blocks_used, blocks_percent_used=0;

	if(access(mountpoint, R_OK) !=0) 
	{ // exit if no available
 		cerr<<"[drive setup] "<<__FUNCTION__ <<": mountpoint "<<mountpoint<<" not found..."<< endl;
		return 0;
	}

	if (statfs(mountpoint, &s) != 0)
	{
		perror(mountpoint);
		return 0;
	}

	if (s.f_blocks > 0) 
	{
		blocks_used = s.f_blocks - s.f_bfree;
		blocks_percent_used = (long long)(blocks_used * 100.0 / (blocks_used + s.f_bavail) + 0.5);		
	}
		
	switch (device_info)
	{
		case KB_BLOCKS :
			return (long long) (s.f_blocks * (s.f_bsize / 1024.0));
			break;
		case KB_USED :
			return (long long) ((s.f_blocks - s.f_bfree) * (s.f_bsize / 1024.0));
			break;
		case KB_AVAILABLE :
			return (long long) (s.f_bavail * (s.f_bsize / 1024.0));
			break;
		case PERCENT_USED :
			return blocks_percent_used;
			break;
		case PERCENT_FREE :
			return 100 - blocks_percent_used;
			break;
		case FILESYSTEM :
			return s.f_type;
			break;
		case FREE_HOURS :
			return (long long) (s.f_bavail * (s.f_bsize / 1024.0))/1024/33/60;
			break;
		default : return 0;
	}

}

// returns info of swaps about partitions, filesystem and options from proc/swaps, e.g: getswapInfo(HDA1, MOUNTPOINT)
string CDriveSetup::getSwapInfo(const string& partname, const int& swap_info_num)
{
	string res = "";

	if(access(partname.c_str(), R_OK) !=0) 
	{// exit if no available
// 		cerr<<"[drive setup] "<<partname<<" not found..."<< endl;
		return res;
	}

	if(access(PROC_SWAPS, R_OK) !=0) 
	{ // exit if no available
		cerr<<"[drive setup] "<<__FUNCTION__ <<": error, can't found "<<PROC_SWAPS<<endl;
		return res;
	}

	ifstream in (PROC_SWAPS, ios::in);

	if (!in) 
	{
		cerr<<"[drive setup] "<<__FUNCTION__ <<": error while open "<<PROC_SWAPS<<" "<< strerror(errno)<<endl;
		return res;
	}

	char line[256];
	int column_num = swap_info_num;

	while (in.getline (line, 256))
	{
		string swaps_line = (string)line, str_res;
		string::size_type loc = swaps_line.find( partname, 0 );

		if ( loc != string::npos ) 
		{
			stringstream stream(swaps_line);

			for(int l = 0; l <= 5; l++)
			{
				stream >> str_res;
				if (l==column_num) 
				{
					res = str_res;
					break;
				}
					
			}
		}
	}
	in.close();
	return res;
}

// returns info of mounts about mountpoint, filesystem and options from mtab, e.g: getMountInfo(HDA1, MOUNTPOINT) returns the mountpoint 
string CDriveSetup::getMountInfo(const string& partname, const int& mtab_info_num)
{
	string ret = "";

	if(access(partname.c_str(), R_OK) !=0) 
	{// exit if no available
// 		cerr<<"[drive setup] "<<partname<<" not found..."<< endl;
		return ret;
	}

	if(access(PROC_MOUNTS, R_OK) !=0) 
	{ // exit if no available
		cerr<<"[drive setup] "<<__FUNCTION__ <<": error, can't found "<<PROC_MOUNTS<<endl;
		return ret;
	}

	ret = getFileEntryString(PROC_MOUNTS, partname, mtab_info_num);

	return ret;
}

// helper: return a selectable tab entry from file 
string CDriveSetup::getFileEntryString(const char* filename, const std::string& filter_entry, const int& column_num)
{
	string ret = "";
	char line[256];
	ifstream in (filename, ios::in);

	if (!in) 
	{
		cerr<<"[drive setup] "<<__FUNCTION__ <<": error while open "<<filename<<" "<< strerror(errno)<<endl;
		return ret;
	}

	while (in.getline (line, 256))
	{
		string tab_line = (string)line, str_res;
		string::size_type loc = tab_line.find( filter_entry, 0 );

		if ( loc != string::npos ) 
		{
			stringstream stream(tab_line);

			for(int i = 0; i <= 10; i++)
			{
				stream >> str_res;
				if (i==column_num) 
				{
					ret = str_res;
					in.close();
					return ret;
				}

			}
		}
	}
	in.close();
	return ret;
}

// returns mode of swap partitions, true=is swap, false=no swap, usage: bool part_is_swap = isSwapPartition("/dev/hda1") 
bool CDriveSetup::isSwapPartition(const string& partname)
{
 	string swap_tab;
	string partx = partname;

	if(access(partx.c_str(), R_OK) !=0) 
	{// exit if no available
// 		cerr<<"[drive setup] "<<partname<<" not found..."<< endl;
		return false;
	}

	if(access(PROC_SWAPS, R_OK) !=0) // exit if no available
		return false;

	// get swap table
	ifstream f(PROC_SWAPS);
	if (!f)	
	{
		cerr<<"[drive setup] "<<__FUNCTION__ <<": error while open "<<PROC_SWAPS<< " "<< strerror(errno)<<endl;
		return false;
	}

	char ch;
	while(f.get(ch)) 
	{
		swap_tab += ch;
	}
	f.close();

	//search matching entries and return result
	string::size_type loc = swap_tab.find( partx, 0 );
	return (( loc != string::npos ) ? true : false);

}

// returns true if already exists an active swap partition
bool CDriveSetup::haveSwap()
{
	string swap_entry = getFileEntryString(PROC_SWAPS, "partition", 0);

	if (swap_entry.length() !=0)
		return true;
	else
		return false;
}

// returns free size for new partitions from device in Bytes
unsigned long long CDriveSetup::getUnpartedDeviceSize(const int& device_num)
{
	unsigned long long start_cyl=0, end_cyl=0, used_cyl=0, rest_cyl=0, rest_size=0;

 	if (loadFdiskPartTable(device_num)) 
	{
		unsigned long long cyl_size = getFileEntryLong(PART_TABLE, "Units", 8);
		unsigned long long fullcyl = getFileEntryLong(PART_TABLE, "sectors/track", 4);
		string partname;

		for (int i = 0; i < MAXCOUNT_PARTS; i++)
		{
			partname = getPartName(device_num, i);
			start_cyl = getFileEntryLong(PART_TABLE, partname, 1);
			end_cyl = getFileEntryLong(PART_TABLE, partname, 2);
			used_cyl += end_cyl-start_cyl;
		}
		
		rest_cyl = fullcyl-used_cyl;
		rest_size = rest_cyl*cyl_size;
	}

	return rest_size;
}

// calc cylinders from meagbytes
unsigned long long CDriveSetup::calcCyl(const int& device_num /*MASTER || SLAVE*/, const unsigned long long& bytes)
{

	unsigned long long cyl_max 	= v_device_cylcount[device_num];
	unsigned long long cyl_size	= v_device_cyl_size[device_num];
	unsigned long long size		= bytes;
	unsigned long long cyl_count 	= size / cyl_size;

	// do not allow more then available cylinders and set cylinders to max value if bytes == 0
	if (size == 0)
	{
// 		cout<<"[drive setup] "<<__FUNCTION__ <<": set cylinders to max = "<<cyl_max<<endl;
		return cyl_max;
	}
	else
		return cyl_count;
}

// create temp partable of current device from fdisk
bool CDriveSetup::loadFdiskPartTable(const int& device_num /*MASTER||SLAVE*/, bool use_extra /*using extra functionality table*/ )
{
	string device = drives[device_num].device; 	/*HDA||HDB*/

	if(access(device.c_str(), R_OK) !=0) 
	{
		cerr<<"[drive setup] "<<__FUNCTION__ <<": "<<device<<" not available... "<<endl;
		return false;
	}

	ofstream temp_file( TEMP_SCRIPT );
	if (!temp_file) 
	{ // Error while open
		cerr <<"[drive setup] "<<__FUNCTION__ <<": error while creating temporary part table "<< TEMP_SCRIPT <<" "<< strerror(errno)<<endl;
		return false;
	}

	temp_file <<DISCTOOL<<" "<<device<<" << EOF"<<endl;

	if (use_extra) 
	{
		temp_file <<"x"<<endl;
	}

	temp_file <<"p"<<endl;
	temp_file <<"q"<<endl;
	temp_file <<"EOF"<<endl;
	temp_file.close();

	if (chmod(TEMP_SCRIPT, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) !=0 ) 
	{
		cerr<<"[drive setup] "<<__FUNCTION__ <<": error while setting permissions for "<< TEMP_SCRIPT <<" "<< strerror(errno)<<endl;
		return false;
	}

	string 	cmd = TEMP_SCRIPT;
		cmd += " > ";
		cmd += PART_TABLE;

	if (CNeutrinoApp::getInstance()->execute_sys_command(cmd.c_str())!=0) 
	{
		cerr<<"[drive setup] "<<__FUNCTION__ <<": error while executing "<< TEMP_SCRIPT <<endl;
		return false;
	}

	// remove temp file
 	unlink(TEMP_SCRIPT);

	return true;
}

// returns size of a partition from PROC_PARTITIONS as bytes,
// device_num is e.g MASTER or SLAVE, part_number means not the real number 1...n, use 0...3
// on empty part_number, get size from device eG. hda, hdb...
unsigned long long CDriveSetup::getPartSize(const int& device_num /*MASTER||SLAVE*/, const int& part_number)
{
	unsigned long long res = 0;

	string partname = (part_number !=-1) ? getPartName(device_num, part_number) : drives[device_num].device;

	//normalize part name, remove "/dev/"
	partname.replace(partname.find("/dev/"), 5 ,"");

	if(access(PROC_PARTITIONS, R_OK) !=0) 
	{
		cerr<<"[drive setup] "<<__FUNCTION__ <<": "<< PROC_PARTITIONS <<" not available..."<<endl;
		return 0;
	}

	res = getFileEntryLong(PROC_PARTITIONS, partname, 2)*1024 /*bytes*/;

	return res;
}

// returns informations about partitions from DISCTOOL
// device_num is e.g MASTER or SLAVE, part_number means not the real number 1...n, use 0...n, info_t_num is
// PARTINFO_TYPE_NUM, this is the required return value
// bool load_new_table is default set to true, set to false if no new fdisk table needed, table will generate for one time if it's not available
unsigned long long CDriveSetup::getPartData(const int& device_num /*MASTER||SLAVE*/, const int& part_number, const int& info_t_num /*START_CYL||END_CYL...*/, bool refresh_table)
{
	unsigned long long res = 0;

	// create temp fdisk partable of current device, fdisk table musst be minimum available one time 
	if ((access(PART_TABLE, R_OK) !=0) || (refresh_table)) 
	{
		if (!loadFdiskPartTable(device_num))
		{
			cerr<<"[drive setup] "<<__FUNCTION__ <<": "<<PART_TABLE<<" not loaded..."<<endl;
			return 0;
		}
	}

	// get partition name (dev/hda1...4)
	string partname = getPartName(device_num, part_number);

	if(access(partname.c_str(), R_OK) !=0) 
	{
		//cerr<<"[drive setup] "<<__FUNCTION__ <<": "<<partname<<" not found..."<<endl;
		return 0;
	}

// 	char line[256];
	
	switch (info_t_num) 
	{
		case START_CYL:
			res = getFileEntryLong(PART_TABLE, partname, FDISK_INFO_START_CYL);
			break;
		case END_CYL:
			res = getFileEntryLong(PART_TABLE, partname, FDISK_INFO_END_CYL);
			break;
		case SIZE_BLOCKS:
			res = getFileEntryLong(PART_TABLE, partname, FDISK_INFO_SIZE_BLOCKS);
			break;
		case ID:
			res = getFileEntryLong(PART_TABLE, partname, FDISK_INFO_ID);
			break;
		case SIZE_CYL:
			res = v_device_cyl_size[device_num]; // bytes
			break;
		case COUNT_CYL:
			res = getFileEntryLong(PART_TABLE, partname, FDISK_INFO_END_CYL) - getFileEntryLong(PART_TABLE, partname, FDISK_INFO_START_CYL);
			break;
		case PART_SIZE: // bytes
			unsigned long long count_cyl = getFileEntryLong(PART_TABLE, partname, FDISK_INFO_END_CYL) - getFileEntryLong(PART_TABLE, partname, FDISK_INFO_START_CYL);
			res = v_device_cyl_size[device_num] * count_cyl;
			break;
	}

 	if (refresh_table) // do not kill table, if we need it ! use parameter 4 refresh_table
		unlink(PART_TABLE);

	return res;
}

// helper: return a selectable tab entry from file as unsigned long long
unsigned long long CDriveSetup::getFileEntryLong(const char* filename, const string& filter_entry, const int& column_num)
{
	string str_res = getFileEntryString(filename, filter_entry, column_num);

	if (str_res.empty())
		return 0;

	unsigned long long ret;
	stringstream Str;
	Str << str_res;
	Str >> ret;
	return ret;
}

// prepares or deletes a partition, device_num is e.g MASTER or SLAVE, action is EDIT_PARTITION_MODE_NUM, part_number means not the real number 1...n, use 0...n
bool CDriveSetup::mkPartition(const int& device_num /*MASTER||SLAVE*/, const int& action, const int& part_number, const unsigned long long& start_cyl, const unsigned long long& size)
{
	string device = drives[device_num].device; 	/*HDA||HDB||MMC*/
	bool ret = true;

	// get partition name (dev/hda1...4)
	string partname = getPartName(device_num, part_number);

	ofstream prepare( PREPARE_SCRIPT_FILE );
	if (!prepare) 
	{ // Error while open
		cerr <<"[drive setup] "<<__FUNCTION__ <<": error while preparing "<< PREPARE_SCRIPT_FILE<<" "<< strerror(errno)<<endl;
		return false;
	}

	prepare <<DISCTOOL<<" "<<device<<" <<EOF"<<endl;

	unsigned int part_n = part_number+1; // real part number is needed
		
	switch (action)
	{
		case ADD:
			unsigned long long cyl_max = v_device_cylcount[device_num]; 		//requesting max cylinders of device
			unsigned long long cyl = calcCyl(device_num, size);			//calc cylinders from user definied size
			unsigned long long end_cyl = (cyl == cyl_max) ? cyl_max : start_cyl + cyl;
			prepare <<"n"<<endl;
			prepare <<"p"<<endl;
			prepare <<part_number+1<<endl;
			prepare <<start_cyl<<endl;
			prepare <<end_cyl<<endl;
			break;
		case DELETE:
			if (!unmountPartition(device_num, part_number))
				return false;
			prepare <<"d"<<endl;
			prepare <<part_n<<endl;
			break;
		case DELETE_CLEAN:
			if (!unmountPartition(device_num, part_number))
				return false;
			prepare <<"d"<<endl;
			prepare <<part_n<<endl;
			//reset settings
			strcpy(d_settings.drive_partition_fstype[device_num][part_number],"");
			d_settings.drive_partition_mountpoint[device_num][part_number] = "";
			strcpy(d_settings.drive_partition_size[device_num][part_number],"");
			writeDriveSettings();
			break;
	}

	prepare <<"w"<<endl;
	prepare <<"EOF"<<endl;
	prepare.close();

	// PREPARE_SCRIPT_FILE must be executable
	if (chmod(PREPARE_SCRIPT_FILE, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) !=0 ) 
	{
		cerr<<"[drive setup] "<<__FUNCTION__ <<": error while setting permissions for "<<PREPARE_SCRIPT_FILE<<" "<< strerror(errno)<<endl;
		return false;
	}

	if (unmountPartition(device_num, part_number)) { // unmount first !!
		if (CNeutrinoApp::getInstance()->execute_sys_command(PREPARE_SCRIPT_FILE)!=0) 
		{
			cerr<<"[drive setup] "<<__FUNCTION__ <<": error while executing "<<PREPARE_SCRIPT_FILE<<endl;
			ret = false;
		}

		if ((isActivePartition(partname)) && (action==DELETE) || (action==DELETE_CLEAN)) 
		{ // partition was deleted but part table is not current, reboot is requiered
			bool reboot = (ShowLocalizedMessage(LOCALE_DRIVE_SETUP_HDD_EDIT_PARTITION, LOCALE_DRIVE_SETUP_MSG_REBOOT_REQUIERED, CMessageBox::mbrNo, CMessageBox::mbYes | CMessageBox::mbNo, NEUTRINO_ICON_INFO, width, 5) == CMessageBox::mbrYes);
			if (reboot) 
			{	
				writeDriveSettings();
				CNeutrinoApp::getInstance()->exec(NULL, "reboot");
			}
		}
	}
	else 
	{
		cerr<<"[drive setup] "<<__FUNCTION__ <<": error while unmounting " <<partname<<endl;
		return false;
	}

	unlink(PREPARE_SCRIPT_FILE);
	return true;
}

// gets the path of initfile for loading modules and other hdd stuff
string CDriveSetup::getInitIdeFilePath()
{
	long int fsnum = getDeviceInfo(ETC_DIR, FILESYSTEM);
	if ((fsnum != 0x28cd3d45 /*cramfs*/) && (fsnum != 0x073717368 /*squashfs*/)) 
	{ // we have a writeable etc dir
		unlink(INIT_IDE_VAR_SCRIPT_PATH);
		return INIT_IDE_SCRIPT_PATH;
	}
	else 
		return INIT_IDE_VAR_SCRIPT_PATH;
	
}

// gets the path of initfile for mounts
string CDriveSetup::getInitMountFilePath()
{
	long int fsnum = getDeviceInfo(ETC_DIR, FILESYSTEM);
	if ((fsnum != 0x28cd3d45 /*cramfs*/) && (fsnum != 0x073717368 /*squashfs*/)) 
	{ // we have a writeable etc dir
		unlink(INIT_MOUNT_VAR_SCRIPT_FILE);
		return INIT_MOUNT_SCRIPT_FILE;
	}
	else 
		return INIT_MOUNT_VAR_SCRIPT_FILE;
	
}

// gets the path of fstab file
string CDriveSetup::getFstabFilePath()
{
	long int fsnum = getDeviceInfo(ETC_DIR, FILESYSTEM);
	if ((fsnum != 0x28cd3d45 /*cramfs*/) && (fsnum != 0x073717368 /*squashfs*/)) 
	{ // we have a writeable etc dir
		unlink(FSTAB_VAR); // kill this if available!!!
		return FSTAB;
	}
	else
		return FSTAB_VAR;
	
}

// formats a partition, 1st parameter "device_num" is MASTER, SLAVE..., 3rd parameter "filesystem" means a name of filesystem as string eg, ext3...
bool CDriveSetup::mkFs(const int& device_num /*MASTER||SLAVE*/, const int& part_number,  const std::string& fs_name)
{
	// get partition name (dev/hda1...4)
	string partname = getPartName(device_num, part_number);

	//TODO: check user input for mountpoint, cancel if not allowed or no mointpoint is definied
	if (isActivePartition(partname))
	{	
		if (fs_name.empty())
		{
			string  msg = g_Locale->getText(LOCALE_DRIVE_SETUP_MSG_PARTITION_CREATE_FAILED_NO_FS_DEFINIED);
			ShowHintUTF(LOCALE_MESSAGEBOX_ERROR, msg.c_str(), width, msg_timeout, NEUTRINO_ICON_ERROR);
			return false;
		}
	}

	string mkfs_cmd;

	// ensure: load fs drivers
	if (!initFsDrivers(false)) 
	{
		cerr<<"[drive setup] "<<__FUNCTION__ <<": formating partition failed..."<<endl;
		return false;
	}

	string extfs_opts = " -T largefile -m0 -q";

	if (fs_name=="ext2") 
	{
		mkfs_cmd = "mkfs.ext2";
		mkfs_cmd += extfs_opts;
	}
	else if (fs_name=="ext3") 
	{
		mkfs_cmd = "mkfs.ext3";
		mkfs_cmd += extfs_opts;
	}
	else if (fs_name=="msdos")
		mkfs_cmd = "mkfs.msdos ";
	else if (fs_name=="reiserfs")
		mkfs_cmd = "mkfs.reiserfs -f ";
	else if (fs_name=="vfat")
		mkfs_cmd = "mkfs.vfat ";
	else if (fs_name=="xfs")
		mkfs_cmd = "mkfs.xfs -l version=2 -f -q ";
	else if (fs_name=="swap")
		mkfs_cmd = "mkswap ";
 
	
	bool is_active = isActivePartition(partname);
	
	if (is_active) 
	{
		string cmdformat;
	
		if (fs_name == "swap") // make swap
		{
			cmdformat  = mkfs_cmd;
			cmdformat += partname;
			cmdformat += DEVNULL;

			if (CNeutrinoApp::getInstance()->execute_sys_command(cmdformat.c_str())!=0) 
			{
				cerr<<"[drive setup] "<<__FUNCTION__ <<": mkswap at "<<partname<< " failed..."<<endl;
				return false;
			}
		}
		else // make filesystem
		{
			cmdformat  = mkfs_cmd;
			cmdformat += " ";
			cmdformat += partname;
			//cmdformat += DEVNULL;

			if (CNeutrinoApp::getInstance()->execute_sys_command(cmdformat.c_str())!=0) 
			{
				cerr<<"[drive setup] "<<__FUNCTION__ <<": make filesystem "<<fs_name<< " for "<<partname<< " failed..."<<endl;
				return false;
			}
		}

	}
	else 
	{
		// partition is not active
		cerr<<"[drive setup] "<<__FUNCTION__ <<": make filesystem failed "<<partname<< " is not active!..."<<endl;
		return false;
	}

	return true;
}


#ifdef ENABLE_NFSSERVER
// gets the path of exports file
string CDriveSetup::getExportsFilePath()
{
	long int fsnum = getDeviceInfo(ETC_DIR, FILESYSTEM);
	if ((fsnum != 0x28cd3d45 /*cramfs*/) && (fsnum != 0x073717368 /*squashfs*/))
		return EXPORTS ;// we have a writeable etc dir
	else
		return EXPORTS_VAR ;
}

// generate exports file, returns true on success
bool CDriveSetup::mkExports()
{
	// set exports path
	string exports = getExportsFilePath();
	string timestamp = getTimeStamp();
	vector<string> v_export_entries;
	string head = "# " + exports + " generated from neutrino ide/mmc/hdd drive-setup\n#" +  getDriveSetupVersion() + " " +  timestamp;

	// collecting export entries
	for (unsigned int i = 0; i < MAXCOUNT_DRIVE; i++) 
	{
		for (unsigned int ii = 0; ii < MAXCOUNT_PARTS; ii++) 
		{
			string partname = getPartName(i,ii);
			string export_entry;
			//collects all mountpoints but not swap
			if (d_settings.drive_partition_nfs[i][ii] && !isSwapPartition(partname)) 
			{
				string mp = getMountInfo(partname, MOUNTPOINT);
				export_entry = mp;
				export_entry += " ";
				export_entry += d_settings.drive_partition_nfs_host_ip[i][ii];
				export_entry += "(rw,sync,no_subtree_check)";
				v_export_entries.push_back(export_entry);
			}
		}
	}

	// write exports
	if (v_export_entries.size() != 0)
	{
		ofstream str_exports(exports.c_str());
		if (!str_exports) 
		{ // Error while open
			cerr << "[drive setup] "<<__FUNCTION__ <<": write error "<<exports<<", please check permissions..." << strerror(errno)<<endl;
			return false;
		}
		else 
		{
			str_exports << head <<endl;

			for (unsigned int i = 0; i < v_export_entries.size(); i++) 
			{
				str_exports << v_export_entries[i] <<endl;
			}
		}
		str_exports.close();
		cout<<"[drive setup] "<<__FUNCTION__ <<": writing "<<exports<< "...ok"<<endl;
	}

	else
	{
		if (unlink(exports.c_str()) != 0)
			cerr << "[drive setup] "<<__FUNCTION__ <<": delete "<<exports<<" ..." << strerror(errno)<<endl;
	}

	return true;
}
#endif	

// check fs of a partition, 1st parameter "device_num" is MASTER, SLAVE..., 3rd parameter "filesystem" means a name of filesystem as string eg, ext3...
bool CDriveSetup::chkFs(const int& device_num /*MASTER||SLAVE*/, const int& part_number,  const std::string& fs_name)
{
	bool ret = true;
	CProgressBar pb;

	CNeutrinoApp::getInstance()->execute_start_file(NEUTRINO_CHKFS_START_SCRIPT);
	
	// get partition name (dev/hda1...4)
	string partname = getPartName(device_num, part_number);


	if (!unmountPartition(device_num, part_number)) 
	{
		cerr<<"[drive setup] "<<__FUNCTION__ <<": umounting of: "<<partname<< " failed"<<endl;
		return false;
	}
	

	string chkfs_cmd;
	string extfs_opts = " -y -v";

	if (fs_name=="ext2") 
	{
		chkfs_cmd = "fsck.ext2 ";
		chkfs_cmd += extfs_opts;
	}
	else if (fs_name=="ext3") 
	{
		chkfs_cmd = "fsck.ext3 ";
		chkfs_cmd += extfs_opts;
	}	
	else if (fs_name=="msdos")
		chkfs_cmd = "fsck.msdos -y ";
	else if (fs_name=="reiserfs")
		chkfs_cmd = "fsck.reiserfs -f -y ";
	else if (fs_name=="vfat")
		chkfs_cmd = "fsck.vfat -y ";
	else if (fs_name=="xfs")
		chkfs_cmd = "xfs_repair -v";
	else if (fs_name=="swap")
		return true; // skip swap check

	fb_pixel_t * pixbuf = new fb_pixel_t[pb_w * pb_h];
	if (pixbuf != NULL)
		frameBuffer->SaveScreen(pb_x, pb_y, pb_w, pb_h, pixbuf);

	showStatus(50, "checking Filesystem...", 100);
	
	bool is_active = isActivePartition(partname);
	string 	cmd_check;
	
	if (is_active) 
	{
		// check filesystem
		{
			cmd_check  = chkfs_cmd;
			cmd_check += " ";
			cmd_check += partname;

			if (CNeutrinoApp::getInstance()->execute_sys_command(cmd_check.c_str())!=0) 
			{
				cerr<<"[drive setup] "<<__FUNCTION__ <<": checked filesystem with: "<<cmd_check<< "...with errors"<<endl;
			}

		}

	}
	else 
	{
		// partition is not active
		cerr<<"[drive setup] "<<__FUNCTION__ <<": checking filesystem with: "<<cmd_check<<" failed,\n"<< partname<<" is not active!"<<endl;
		ret = false;
	}

	CNeutrinoApp::getInstance()->execute_start_file(NEUTRINO_CHKFS_END_SCRIPT);

	if (pixbuf != NULL) 
	{
		frameBuffer->RestoreScreen(pb_x, pb_y, pb_w, pb_h, pixbuf);
		delete[] pixbuf;
	}

	return ret;
}

// mounts all available partitions for all devices 
bool CDriveSetup::mountAll()
{
	bool ret = true;

	for (unsigned int i = 0; i < MAXCOUNT_DRIVE; i++) 
	{
		if (!mountDevice(i))
			ret = false;
	}			

	return ret;
}

// mounts all available partitions on device
bool CDriveSetup::mountDevice(const int& device_num)
{
	bool ret = true;
	int i = device_num;

	for (unsigned int ii = 0; ii < MAXCOUNT_PARTS; ii++) 
	{
		string partname = getPartName(i,ii);
		if (d_settings.drive_partition_activ[i][ii]) 
		{// mount only if option is set to activ
			if (isActivePartition(partname))
			{
				if (!mountPartition(i, ii, d_settings.drive_partition_fstype[i][ii], d_settings.drive_partition_mountpoint[i][ii]))
					ret = false;
			}
		}
	}			
	
	return ret;
}

// mounting partitions, returns true on success
bool CDriveSetup::mountPartition(const int& device_num /*MASTER||SLAVE*/, const int& part_number,  const std::string& fs_name, const std::string& mountpoint)
{
	bool ret = true;
	int ret_num = 0 ;

	// get partition name (dev/hda1...4)
	string partname = getPartName(device_num, part_number);

	//TODO: check user input for mountpoint, cancel if not allowed or no mointpoint is definied
	if (isActivePartition(partname))
	{	
		if (mountpoint.empty() || mountpoint == "/" || mountpoint == "/root")
		{
			string  msg = g_Locale->getText(LOCALE_DRIVE_SETUP_PARTITION_MOUNT_NO_MOUNTPOINT);
				msg += "\nmountpoint: " + mountpoint;
			ShowHintUTF(LOCALE_MESSAGEBOX_ERROR, msg.c_str(), width, msg_timeout, NEUTRINO_ICON_ERROR);
			return false;
		}
		if (fs_name.empty())
		{
			string  msg = g_Locale->getText(LOCALE_DRIVE_SETUP_MSG_PARTITION_CREATE_FAILED_NO_FS_DEFINIED);
			ShowHintUTF(LOCALE_MESSAGEBOX_ERROR, msg.c_str(), width, msg_timeout, NEUTRINO_ICON_ERROR);
			return false;
		}
	}

	if((access(partname.c_str(), R_OK) !=0) || (!d_settings.drive_partition_activ[device_num][part_number])) 	
	{// exit if no available or not set to activ
		cout<<"[drive setup] "<<__FUNCTION__ <<":  "<<partname<< " partition not activ, nothing to do...ok"<< endl;
 		return ret;
	}
	
	if (fs_name != "swap")
	{ // mounting
		if (isMountedPartition(partname)) 
		{
// 			cout<<"[drive setup] mount "<<partname<< " allready mounted...ok"<< endl;
			ret = true;
		}
		else 
		{ 
			if (initModul(fs_name, false)) 
			{ //load first fs modul
				ret_num = mount(partname.c_str(),  mountpoint.c_str() , fs_name.c_str(), 16 , NULL);
				if (ret_num!=0) 
				{
					s_err += "mountPartition: "; 
					s_err += strerror(errno);
					s_err += "\n";
					s_err += partname;
					s_err += "\nto ";
					s_err += mountpoint;
					cerr<<"[drive setup] "<<s_err<<endl;
					ret = false;
				}
			// TODO Message on error
			}
		}

	}
	else
	{ // swapon
		if (isSwapPartition(partname)) 
		{
			ret = true;
		}
		else 
		{
			ret_num = swapon(partname.c_str(), 0/*SWAPFLAGS=0*/);
			if (ret_num!=0) 
			{
				cerr<<"[drive setup] "<<__FUNCTION__ <<":  swapon: "<<strerror(errno)<< " " << partname<<endl;
				ret = false;
			}
		}
	}
	
	return ret;
}

// returns default mount entries depends from filesystem
string CDriveSetup::getDefaultSysMounts()
{
	//TODO make it usable for other imagetypes/boxtypes
	string ret = "";

	if (d_settings.drive_mount_mtdblock_partitions)
	{
		long int fsnum = getDeviceInfo(ETC_DIR, FILESYSTEM);
			ret =  "/bin/mount -n -t proc proc /proc\n";
			ret += "/bin/mount -n -t tmpfs tmpfs /tmp\n";
	
		if ((fsnum == 0x28cd3d45 /*cramfs*/) || (fsnum == 0x073717368 /*squashfs*/)) 
		{
	;		ret += "/bin/mount -t jffs2 /dev/mtdblock/3 /var";
		}
		else if (fsnum == 0x6969 /*nfs (yadd)*/) 
		{
			ret += "#/bin/mount -t jffs2 /dev/mtdblock/3 /var";
		}
	}

	return ret;
}

// returns default fstab mount entries
string CDriveSetup::getDefaultFstabEntries()
{
	//TODO make it usable for other imagetypes/boxtypes
	string 	ret = "";

	if (d_settings.drive_mount_mtdblock_partitions)
	{
		ret = 	"proc /proc proc defaults 0 0\n";
		ret += 	"tmpfs /tmp tmpfs defaults 0 0\n";
		ret += 	"sysfs /sys sysfs noauto 0 0\n";
		ret += 	"devpts /dev/pts devpts noauto 0 0";
	}

	return ret;
}

// returns status of mmc, returns true if is active
bool CDriveSetup::isMmcActive()
{
	if ((isModulLoaded(M_MMC)) || (isModulLoaded(M_MMC2)) || (isModulLoaded(M_MMCCOMBO)))
		return true;
	else
		return false;
}

// returns status of ide interface, returns true if is active
bool CDriveSetup::isIdeInterfaceActive()
{
	if ((isModulLoaded(DBOXIDE)) && (isModulLoaded(IDE_CORE)) && (isModulLoaded(IDE_DETECT)) && (isModulLoaded(IDE_DISK)))
		return true;
	else
		return false;
}

// load settings from configfile
void CDriveSetup::loadDriveSettings()
{
	bool have_no_conf = false;

	if(!configfile.loadConfig(DRV_CONFIGFILE)) 
	{
		have_no_conf = true;
	}
	
	cout<<"[drive setup] "<<__FUNCTION__ <<": load settings from "<<DRV_CONFIGFILE<<endl;
	
	// drivesetup
	d_settings.drive_activate_ide = configfile.getInt32("drive_activate_ide", IDE_OFF);
	strcpy(d_settings.drive_mmc_module_name, configfile.getString("drive_mmc_module_name", "").c_str());
	d_settings.drive_use_fstab = configfile.getInt32("drive_use_fstab", YES);
	d_settings.drive_mount_mtdblock_partitions = configfile.getInt32("drive_mount_mtdblock_partitions", NO);

	char mountpoint_opt[28];
	char spindown_opt[14];
	char partsize_opt[22];
	char fstype_opt[24];
	char write_cache_opt[17];
	char partition_activ_opt[23];
	char partition_nfs_opt[23];
	char partition_nfs_host_ip_opt[31];
	for(unsigned int i = 0; i < MAXCOUNT_DRIVE; i++) 
	{
		// d_settings.drive_spindown
		sprintf(spindown_opt, "drive_%d_spindown", i);
		strcpy(d_settings.drive_spindown[i], configfile.getString(spindown_opt,"0").c_str());

		//d_settings.drive_write_cache
		sprintf(write_cache_opt, "drive_%d_write_cache", i);
		d_settings.drive_write_cache[i] = configfile.getInt32(write_cache_opt, OFF);

		for(unsigned int ii = 0; ii < MAXCOUNT_PARTS; ii++) 
		{
			// d_settings.drive_partition_size
			sprintf(partsize_opt, "drive_%d_partition_%d_size", i, ii);
			strcpy(d_settings.drive_partition_size[i][ii], configfile.getString(partsize_opt, "0").c_str());

			// d_settings.drive_partition_fstype
			sprintf(fstype_opt, "drive_%d_partition_%d_fstype", i, ii);
			strcpy(d_settings.drive_partition_fstype[i][ii], configfile.getString(fstype_opt, "").c_str());

			// d_settings.drive_partition_mountpoint
			sprintf(mountpoint_opt, "drive_%d_partition_%d_mountpoint", i, ii);
			d_settings.drive_partition_mountpoint[i][ii] = (string)configfile.getString(mountpoint_opt, "");

			// d_settings.drive_partition_activ
			sprintf(partition_activ_opt, "drive_%d_partition_%d_activ", i, ii);
			d_settings.drive_partition_activ[i][ii] = configfile.getBool(partition_activ_opt, true);

			// d_settings.drive_partition_nfs
			sprintf(partition_nfs_opt, "drive_%d_partition_%d_nfs", i, ii);
			d_settings.drive_partition_nfs[i][ii] = configfile.getBool(partition_nfs_opt, false);

			// d_settings.drive_partition_nfs_host_ip
			sprintf(partition_nfs_host_ip_opt, "drive_%d_partition_%d_nfs_host_ip", i, ii);
			d_settings.drive_partition_nfs_host_ip[i][ii] = (string)configfile.getString(partition_nfs_host_ip_opt, "");
		}
	}

	if (have_no_conf)
	{
		if (writeDriveSettings())
			cout<<"[drive setup] "<<__FUNCTION__ <<": found no "<<DRV_CONFIGFILE<< " defaults used..."<<endl;
	}
}

// saving settings
bool CDriveSetup::writeDriveSettings()
{
	bool ret = true;

	// drivesetup
	configfile.setInt32	( "drive_activate_ide", d_settings.drive_activate_ide);
	configfile.setString	( "drive_mmc_module_name", d_settings.drive_mmc_module_name );
	configfile.setInt32	( "drive_use_fstab", d_settings.drive_use_fstab );
	configfile.setInt32	( "drive_mount_mtdblock_partitions", d_settings.drive_mount_mtdblock_partitions );

	char mountpoint_opt[28];
	char spindown_opt[14];
	char partsize_opt[22];
	char fstype_opt[24];
	char write_cache_opt[17];
	char partition_activ_opt[23];
	char partition_nfs_opt[23];
	char partition_nfs_host_ip[31];
	for(int i = 0; i < MAXCOUNT_DRIVE; i++) 
	{
		// d_settings.drive_spindown
		sprintf(spindown_opt, "drive_%d_spindown", i);
		configfile.setString( spindown_opt, d_settings.drive_spindown[i/*MASTER||SLAVE*/] );

		// d_settings.drive_write_cache
		sprintf(write_cache_opt, "drive_%d_write_cache", i);
		configfile.setInt32( write_cache_opt, d_settings.drive_write_cache[i/*MASTER||SLAVE*/]);

		for(int ii = 0; ii < MAXCOUNT_PARTS; ii++) 
		{
			// d_settings.drive_partition_size
			sprintf(partsize_opt, "drive_%d_partition_%d_size", i, ii);
			configfile.setString( partsize_opt, d_settings.drive_partition_size[i/*MASTER||SLAVE*/][ii] );

			// d_settings.drive_partition_fstype
			sprintf(fstype_opt, "drive_%d_partition_%d_fstype", i, ii);
			configfile.setString( fstype_opt, d_settings.drive_partition_fstype[i/*MASTER||SLAVE*/][ii] );

			// d_settings.drive_partition_mountpoint
			sprintf(mountpoint_opt, "drive_%d_partition_%d_mountpoint", i, ii);
			configfile.setString( mountpoint_opt, d_settings.drive_partition_mountpoint[i/*MASTER||SLAVE*/][ii]);

			// d_settings.drive_partition_activ
			sprintf(partition_activ_opt, "drive_%d_partition_%d_activ", i, ii);
			configfile.setBool(partition_activ_opt, d_settings.drive_partition_activ[i/*MASTER||SLAVE*/][ii]);

			// d_settings.drive_partition_nfs
			sprintf(partition_nfs_opt, "drive_%d_partition_%d_nfs", i, ii);
			configfile.setBool(partition_nfs_opt, d_settings.drive_partition_nfs[i/*MASTER||SLAVE*/][ii]);

			// d_settings.drive_partition_nfs_host_ip
			sprintf(partition_nfs_host_ip, "drive_%d_partition_%d_nfs_host_ip", i, ii);
			configfile.setString( partition_nfs_host_ip, d_settings.drive_partition_nfs_host_ip[i/*MASTER||SLAVE*/][ii]);
		}
	}
	
	if (!configfile.saveConfig(DRV_CONFIGFILE)) 
	{
		cerr<<"[drive setup] "<<__FUNCTION__ <<": error while writing "<<DRV_CONFIGFILE<<endl;
		ret = false;
	}

	return ret;
}

// returns current time string
string CDriveSetup::getTimeStamp()
{
	time_t now = time(0);
	char ret[22];
	strftime(ret, 22, "%d.%m.%Y - %H:%M:%S", localtime(&now));

	return (string)ret;
}

// returns a error message with locale
string CDriveSetup::getErrMsg(neutrino_locale_t locale)
{
	string 	ret =  g_Locale->getText(locale);
		ret += "\n\n" + s_err;
		ret += "\n\n";
		ret += "Please show serial log for more details!";
		
	s_err = "";

	return ret;
}

// returns a revision string
string CDriveSetup::getDriveSetupVersion()
{
	static CImageInfo imageinfo;
	return imageinfo.getModulVersion("BETA! ","$Revision: 1.5 $");
}

// returns text for initfile headers
string CDriveSetup::getInitFileHeader(string& filename)
{
	string timestamp = getTimeStamp();
	// set  head lines for init file
	string	s_init = "#!/bin/sh\n";
		s_init += "echo ";
		s_init += char(34);
		s_init += filename + " generated from neutrino ide/mmc/hdd drive-setup\n";
		s_init += timestamp + "\n";
		s_init += getDriveSetupVersion();
		s_init += " do not edit!";
		s_init += char(34);

	return s_init;
}

// returns commands for mount init file
string CDriveSetup::getInitFileMountEntries()
{
	string mount_entries;

	for(int i = 0; i < MAXCOUNT_DRIVE; i++)
	{
		for(int ii = 0; ii < MAXCOUNT_PARTS; ii++)
		{
			string partname = getPartName(i, ii);
			if (isMountedPartition(partname)) 
			{
				mount_entries += "\t\tumount " + getMountInfo(partname, MOUNTPOINT) + "\n";
			}
		}

	}

	string 	m_txt =  "case $1 in\n";
		m_txt += "\tstart)\n";
		m_txt += "\t\tmount -a\n";
		m_txt += "\t\tswapon -a\n";
		m_txt += "\t\t;;\n";
		m_txt += "\tstop)\n";
		m_txt += mount_entries;
		m_txt += "\t\tswapoff -a\n";
		m_txt += "\t\t;;\n";
		m_txt += "esac\n";
		m_txt += "exit 0";

	return m_txt;
}

//generates directories for default mountpoints hdd1, hdd2 depends of available drives
void CDriveSetup::mkDefaultMountpoints()
{
	long int fsnum = getDeviceInfo("/", FILESYSTEM);

	string root = "";

	if ((fsnum == 0x28cd3d45 /*cramfs*/) || (fsnum == 0x073717368 /*squashfs*/))
		root = "/var";

	string dir_pattern[2] 	= {root +"/hdd", root + "/mmc"};

	for (int i=0; i<hdd_count; i++) //hdd
	{
		char folder[16];
		sprintf(folder, "%s%d", dir_pattern[0].c_str(), i+1);  
		if ( access(folder, F_OK) != 0 ) 
		{ 
			if (mkdir(folder, 0777) !=0) 
				cerr<<"[drive setup] "<<__FUNCTION__ <<":  error while creating "<< folder << " " <<strerror(errno)<<endl;
		}
	}

	if (isMmcActive()) //mmc
	{
		if ( access(dir_pattern[1].c_str(), F_OK) != 0 )
		{
			if (mkdir(dir_pattern[1].c_str(), 0777) !=0) 
				cerr<<"[drive setup] "<<__FUNCTION__ <<":  error while creating "<< dir_pattern[1] << " " <<strerror(errno)<<endl;
		}
	}	
}

// returns commands for modul init file
string CDriveSetup::getInitFileModulEntries(bool with_unload_entries)
{
	string load_entries;
	string unload_entries = "\t\t";

	// add commands for loading the filesystem modules
	for (unsigned int i=0; i<(v_init_fs_L_cmds.size()) ; ++i) 
	{
		load_entries += v_init_fs_L_cmds[i];
		load_entries += "\n\t\t";
		
	}

	// add init commands to enable the ide interface
	for (unsigned int i=0; i<(v_init_ide_L_cmds.size()) ; ++i) 
	{
		load_entries += v_init_ide_L_cmds[i];
		load_entries += "\n\t\t";
	}

	//add commands to activate mmc
	load_entries += s_init_mmc_cmd;
	load_entries += "\n\t\t";

	// add hdparm commands for writecache and spindown
	for (unsigned int i=0; i<(v_hdparm_cmds.size()) ; ++i) 
	{
		load_entries += v_hdparm_cmds[i];
		load_entries += "\n\t\t";
	}

	//generate unload commands only if with_unload_entries is set to true
	if (with_unload_entries)
	{
		//add unload commands to disable the ide interface
		for (unsigned int i=IDE_MODULES_COUNT; i>0 ; i--) 
		{
			unload_entries += UNLOAD + ide_modules[i-1].modul;
			unload_entries += "\n\t\t";
		}
	
		//add unload mmc command

		unload_entries += UNLOAD + getUsedMmcModulName();
		unload_entries += "\n\t\t";
	
		//add unload commands to unload fs modules
		for (unsigned int i=0; i<(v_fs_modules.size()) ; ++i) 
		{
			if (v_fs_modules[i] != "swap") 
			{
				unload_entries += UNLOAD + v_fs_modules[i];
				unload_entries += "\n\t\t";
			}
		}
	
	
		unload_entries += UNLOAD ;
		unload_entries += "jbd\n\t\t";
		unload_entries += UNLOAD;
		unload_entries += "vfat\n";
	}


	string 	e_txt =  "case $1 in\n";
		e_txt += "\tstart)\n\t\t";
		e_txt += load_entries;
		e_txt += "\t\t;;\n";
		e_txt += "\tstop)\n";
		e_txt += unload_entries; //optional
		e_txt += "\t\t;;\n";
		e_txt += "esac\n";
		e_txt += "exit 0";

	return e_txt;

}

#define INIT_SCRIPTS_COUNT 2

// linking initfiles and returns true on success
bool CDriveSetup::linkInitFiles()
{
	string init_dir;
	string init_hdd_file = getInitIdeFilePath();
	string init_mount_file = getInitMountFilePath();

	long int fsnum = getDeviceInfo(ETC_DIR, FILESYSTEM);
	if ((fsnum != 0x28cd3d45 /*cramfs*/) && (fsnum != 0x073717368 /*squashfs*/)) 
		init_dir = INIT_D_DIR; // /etc/init.d is writeable, use this!
	else 
		init_dir = INIT_D_VAR_DIR; // /etc/init.d is not writeable, use /var/etc/init.d!

	string scripts[INIT_SCRIPTS_COUNT] = {	init_hdd_file, 
						init_mount_file}; 

	string symlinks[INIT_SCRIPTS_COUNT][INIT_SCRIPTS_COUNT] = {{init_dir + "/S" + INIT_IDE_SCRIPT_NAME, init_dir + "/K" + INIT_IDE_SCRIPT_NAME},
								  {init_dir + "/S" + INIT_MOUNT_SCRIPT_NAME, init_dir + "/K" + INIT_MOUNT_SCRIPT_NAME}}; 

	for (unsigned int i=0; i<(INIT_SCRIPTS_COUNT) ; ++i)
	{
		for (unsigned int ii=0; ii<(INIT_SCRIPTS_COUNT) ; ++ii)
		{
			if ( access(symlinks[i][ii].c_str(), R_OK) ==0 )
			{
				if (unlink(symlinks[i][ii].c_str()) !=0)
				{
					cerr<<"[drive setup] "<<__FUNCTION__ <<":  error while unlink "<< symlinks[i][ii] << " " <<strerror(errno)<<endl;
				}
			}
			if (symlink(scripts[i].c_str(),symlinks[i][ii].c_str()) !=0)
			{
				cerr<<"[drive setup] "<<__FUNCTION__ <<":  error while creating symlink "<< symlinks[i][ii] << " " <<strerror(errno)<<endl;
				return false;
			}
				cout<<"[drive setup] linking "<<scripts[i]<<"-->"<<symlinks[i][ii]<< " ...ok"<<endl;
		}
	}
	
	return true;
}

//returns true if any partition is available at device
bool CDriveSetup::haveActiveParts(const int& device_num)
{
	int i = device_num;

	for (unsigned int ii = 0; ii < MAXCOUNT_PARTS; ii++) 
	{
		string partname = getPartName(i,ii);
		if (isActivePartition(partname)) 
		{
			return true;
		}				
	}			
	
	return false;
}

//returns true if any partition is mounted at device
bool CDriveSetup::haveMounts(const int& device_num, bool without_swaps)
{
	int i = device_num;

	for (unsigned int ii = 0; ii < MAXCOUNT_PARTS; ii++) 
	{
		string partname = getPartName(i,ii);
		if (!without_swaps)
			if (isMountedPartition(partname) || isSwapPartition(partname)) 
				return true;
			else 
				return isMountedPartition(partname);								
	}			
	
	return false;
}


//helper, converts int to string
string CDriveSetup::iToString(int int_val)
{
    int i = int_val;
    ostringstream i_str;
    i_str << i;
    string i_string(i_str.str());
    return i_string;
}


// class CDriveSetupFsNotifier
//enable disable entry for selecting mountpoint
CDriveSetupFsNotifier::CDriveSetupFsNotifier( CMenuForwarder* f)
{
	toDisable = f;
}
bool CDriveSetupFsNotifier::changeNotify(const neutrino_locale_t, void * Data)
{
	if (*((int *)Data) == 0x73776170 /*swap*/)
		toDisable->setActive(false);
	else
		toDisable->setActive(true);

	return true;
}

#ifdef ENABLE_NFSSERVER
// class CDriveSetupNFSHostNotifier
//enable disable entry for input nfs host ip 
CDriveSetupNFSHostNotifier::CDriveSetupNFSHostNotifier( CMenuForwarder* f)
{
	toDisable = f;
}
bool CDriveSetupNFSHostNotifier::changeNotify(const neutrino_locale_t, void * Data)
{
	if (*((int *)Data) == 0)
		toDisable->setActive(false);
	else
		toDisable->setActive(true);

	return true;
}
#endif


