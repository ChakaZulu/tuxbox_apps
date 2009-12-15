/*
	$Id: drive_setup.h,v 1.1 2009/12/15 09:51:23 dbt Exp $

	Neutrino-GUI  -   DBoxII-Project

	hdd setup implementation, fdisk frontend for Neutrino gui

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

*/

#ifndef __drive_setup__
#define __drive_setup__

#include <configfile.h>

#include <gui/widget/menue.h>
#include <gui/widget/dirchooser.h>

#include <driver/framebuffer.h>
#include <system/settings.h>

#include <string>
#include <vector>

// maximal count of usable devices
#define MAXCOUNT_DRIVE 3
// possible count of partitions per device
#define MAXCOUNT_PARTS 4

// drive settings
struct SDriveSettings
{
	std::string 	drive_partition_mountpoint[MAXCOUNT_DRIVE][MAXCOUNT_PARTS];

	int 	drive_use_fstab;
	int	drive_mount_mtdblock_partitions;

	int 	drive_activate_ide;
	int 	drive_write_cache[MAXCOUNT_DRIVE];
	int 	drive_partition_activ[MAXCOUNT_DRIVE][MAXCOUNT_PARTS];
	char	drive_partition_fstype[MAXCOUNT_DRIVE][MAXCOUNT_PARTS][8/*chars*/];
	char 	drive_spindown[MAXCOUNT_DRIVE][3/*chars*/];
	char 	drive_partition_size[MAXCOUNT_DRIVE][MAXCOUNT_PARTS][8/*chars*/];
	char 	drive_mmc_module_name[10];
};


enum ON_OFF_NUM	
{
	OFF,
	ON
};
// switch on/off Option
#define OPTIONS_ON_OFF_OPTION_COUNT 2
const CMenuOptionChooser::keyval OPTIONS_ON_OFF_OPTIONS[OPTIONS_ON_OFF_OPTION_COUNT] =
{
	{ OFF, LOCALE_OPTIONS_OFF  },
	{ ON, LOCALE_OPTIONS_ON }
};


enum YES_NO_NUM	
{
	NO,
	YES
};
// switch enable/disable partition
#define OPTIONS_YES_NO_OPTION_COUNT 2
const CMenuOptionChooser::keyval OPTIONS_YES_NO_OPTIONS[OPTIONS_YES_NO_OPTION_COUNT] =
{
	{ NO, LOCALE_DRIVE_SETUP_PARTITION_ACTIVATE_NO  },
	{ YES, LOCALE_DRIVE_SETUP_PARTITION_ACTIVATE_YES }
};


// modes count for enum IDE_DRIVERMODES collection
enum IDE_DRIVERMODES
{
	IDE_OFF,
	IDE_ACTIVE,
	IDE_ACTIVE_IRQ6
};
// switch activate/deactivate ide interface
#define OPTIONS_IDE_ACTIVATE_OPTION_COUNT 3
const CMenuOptionChooser::keyval OPTIONS_IDE_ACTIVATE_OPTIONS[OPTIONS_IDE_ACTIVATE_OPTION_COUNT] =
{
	{ IDE_OFF, LOCALE_OPTIONS_OFF  },
	{ IDE_ACTIVE, LOCALE_DRIVE_SETUP_IDE_ACTIVATE_ON },
	{ IDE_ACTIVE_IRQ6, LOCALE_DRIVE_SETUP_IDE_ACTIVATE_IRQ6 }
};


class CDriveSetup : public CMenuTarget
{
	private:
		enum EDIT_PARTITION_MODE_NUM	
		{
			ADD_MODE,
			EDIT_MODE,
			SWAP_MODE
		};
		
		enum PREPARE_PARTITION_MODE_NUM	
		{
			ADD,
			DELETE,
			DELETE_CLEAN
		};

		// set nums for commands collection, used in v_init_ide_L_cmds, this is also the order of commands in the init file
		// commands count for enum INIT_COMMANDS collection
		#define INIT_COMMANDS_COUNT 6
		enum INIT_COMMANDS	
		{
			LOAD_IDE_CORE,
			LOAD_DBOXIDE,
			LOAD_IDE_DETECT,
			LOAD_IDE_DISK,
			SET_MASTER_HDPARM_OPTIONS,
			SET_SLAVE_HDPARM_OPTIONS
		};

		enum MTAB_INFO_NUM	
		{
			DEVICE,
			MOUNTPOINT,
			FS,
			OPTIONS
		};
		
		#define SWAP_INFO_NUM_COUNT 5
		enum SWAP_INFO_NUM	
		{
			FILENAME,
			TYPE,
			SIZE,
			USED,
			PRIORITY
		};

		enum PARTINFO_TYPE_NUM	
		{
			START_CYL,
			END_CYL,
			SIZE_BLOCKS,
			ID,
			SIZE_CYL,
			COUNT_CYL,
			PART_SIZE
		};

		enum FDISK_INFO_COLUMN_NUM  //column numbers	
		{
			FDISK_INFO_START_CYL	= 1,
			FDISK_INFO_END_CYL	= 2,
			FDISK_INFO_SIZE_BLOCKS	= 3,
			FDISK_INFO_ID		= 4
		};

		#define INIT_FILE_TYPE_NUM_COUNT 2
		enum INIT_FILE_TYPE_NUM	
		{
			INIT_FILE_MODULES,
			INIT_FILE_MOUNTS
		};

		CFrameBuffer 	*frameBuffer;
		CConfigFile	configfile;
		SDriveSettings	d_settings; 

		int x, y, width, height, hheight, mheight;
		int pb_x, pb_y, pb_w, pb_h;
		int msg_timeout; 	// timeout for messages
	

		const char* msg_icon; 	// icon for all hdd setup windows
		char part_num_actionkey[MAXCOUNT_PARTS][17];
		std::string make_part_actionkey[MAXCOUNT_PARTS]; //action key strings for make_partition_$
		std::string mount_unmount_partition[MAXCOUNT_PARTS]; //action key strings for mount_partition_$
		std::string delete_partition[MAXCOUNT_PARTS]; //action key strings for delete_partition_$
		std::string check_partition[MAXCOUNT_PARTS]; //action key strings for check_partition_$
		std::string sel_device_num_actionkey[MAXCOUNT_DRIVE]; //"sel_device_0 ... sel_device_n""
		std::string s_init_mmc_cmd; // system load command for mmc modul
		std::string s_err;
		std::string getErrMsg(neutrino_locale_t locale);

		int current_device; 	//MASTER || SLAVE || MMCARD, current edit device
		int hdd_count; 		// count of hdd drives
 		int part_count[MAXCOUNT_DRIVE /*MASTER || SLAVE || MMCARD*/]; //count of partitions at device
		int next_part_number;// number of next free partition that can be added from device 1...4

		unsigned long long start_cylinder;
		unsigned long long end_cylinder;
		unsigned long long part_size;

		void setStartCylinder();

		bool device_isActive[MAXCOUNT_DRIVE /*MASTER || SLAVE || MMCARD*/];

		std::vector<std::string> v_model_name;		//collection of names of models
		std::vector<std::string> v_fs_modules;		//collection of available fs modules
		std::vector<std::string> v_mmc_modules;		//collection of available mmc modules
		std::vector<std::string> v_init_ide_L_cmds; 	//collection of ide load commands
		std::vector<std::string> v_init_fs_L_cmds; 	//collection of fs load commands
		std::vector<std::string> v_init_fs_U_cmds; 	//collection of fs unload commands
 		std::vector<std::string> v_partname; 		//collection of all partition names, 4 per device
		std::vector<std::string> v_device_temp;  	//collection of temperature of devices
		std::vector<std::string> v_mount_entries;	//collection of available mount entries
		std::vector<std::string> v_hdparm_cmds;		//collection of available hdparm commands

// 		unsigned long long device_size[MAXCOUNT_DRIVE]; // contains sizes of all devices
// 		unsigned long long device_cylinders[MAXCOUNT_DRIVE]; // contains count of devices for all devices
// 		unsigned long long device_cyl_size[MAXCOUNT_DRIVE]; // contains bytes of one cylinder for all devices in bytes
		std::vector<unsigned long long> v_device_size; 	// contains sizes of all devices
		std::vector<unsigned long long> v_device_cylcount; 	// contains count of devices for all devices
		std::vector<unsigned long long> v_device_cyl_size; 	// contains bytes of one cylinder for all devices in bytes
		std::vector<unsigned long long> v_device_heads_count; // contains count of heads
		std::vector<unsigned long long> v_device_sectors_count; // contains count of sectors

		const char *getFsTypeStr(long &fs_type_const);

		bool foundHdd(const std::string& mountpoint);
		bool loadFdiskPartTable(const int& device_num /*MASTER || SLAVE || MMCARD*/, bool use_extra = false /*using extra funtionality table*/);
		bool isActivePartition(const std::string& partname);
		bool isModulLoaded(const std::string& modulname);
		bool isMountedPartition(const std::string& partname);
		bool isSwapPartition(const std::string& partname);
		bool initFsDrivers(bool do_unload_first = true);
		bool loadHddParams(const bool do_reset = false);
		bool initIdeDrivers(const bool irq6 = false);
		bool initModul(const std::string& modulname, bool do_unload_first = true);
		bool mountPartition(const int& device_num /*MASTER || SLAVE || MMCARD*/, const int& part_number,  const std::string& fs_name, const std::string& mountpoint);
		bool mountDevice(const int& device_num);
		bool mountAll();
		bool unmountPartition(const int& device_num /*MASTER || SLAVE || MMCARD*/, const int& part_number);
		bool unmountDevice(const int& device_num);
		bool unmountAll();
		bool mountUmountPartition(const int& device_num, const int& part_number);
		bool saveHddSetup();
		bool unloadFsDrivers();
		bool unloadMmcDrivers();
		bool initMmcDriver();
		bool unloadIdeDrivers();
		bool unloadModul(const std::string& modulname);
		bool writeInitFile(const bool clear = false);
		bool mkFstab(bool write_defaults_only = false);
		bool haveSwap();
		bool isMmcActive();
		bool isIdeInterfaceActive();
		bool linkInitFiles();
		

		bool mkPartition(const int& device_num /*MASTER || SLAVE || MMCARD*/, const int& action, const int& part_number, const unsigned long long& start_cyl = 0, const unsigned long long& size = 0);
		bool mkFs(const int& device_num /*MASTER || SLAVE || MMCARD*/, const int& part_number,  const std::string& fs_name);
		bool chkFs(const int& device_num /*MASTER || SLAVE || MMCARD*/, const int& part_number,  const std::string& fs_name);
		bool formatPartition(const int& device_num, const int& part_number);
		void showStatus(const int& progress_val, const std::string& msg, const int& max = 5); // helper

		unsigned int getFirstUnusedPart(const int& device_num);
		unsigned long long getPartData(const int& device_num /*MASTER || SLAVE || MMCARD*/, const int& part_number, const int& info_t_num /*START||END*/, bool refresh_table = true);
		unsigned long long getPartSize(const int& device_num /*MASTER || SLAVE || MMCARD*/, const int& part_number = -1);
	
		void hide();
		void Init();

		void calPartCount();
		void loadHddCount();
		void loadHddModels();
		void loadFsModulList();
		void loadMmcModulList();
		void loadPartlist(const int& device_num = -1);
		void loadFdiskData();
		void loadDriveTemps();
		void mkMounts();
		void mkDefaultMountpoints();
						
		void showHddSetupMain();
		void showHddSetupSub();

		bool writeDriveSettings();
		void loadDriveSettings();


		unsigned long long getFreeDiskspace(const char *mountpoint);
		unsigned long long getUnpartedDeviceSize(const int& device_num /*MASTER || SLAVE || MMCARD*/);
		unsigned long long getFileEntryLong(const char* filename, const std::string& filter_entry, const int& column_num);

		unsigned long long free_size[MAXCOUNT_DRIVE][MAXCOUNT_PARTS]; // contains free unused size of disc in MB
		unsigned long long calcCyl(const int& device_num /*MASTER || SLAVE || MMCARD*/, const unsigned long long& bytes);

		long long getDeviceInfo(const char *mountpoint/*/hdd...*/, const int& device_info /*KB_BLOCKS,
											KB_AVAILABLE,
											PERCENT_USED,
											PERCENT_FREE,
											FILESYSTEM...*/);
// 		examples: 	getFsTypeStr(getDeviceInfo("/hdd", FILESYSTEM));
// 				getDeviceInfo("/hdd", PERCENT_USED);

		
		std::string getMountInfo(const std::string& partname /*HDA1...HDB4*/, const int& mtab_info_num /*MTAB_INFO_NUM*/);
		std::string getSwapInfo(const std::string& partname /*HDA1...HDB4*/, const int& swap_info_num  /*SWAP_INFO_NUM*/);
		std::string getPartName(const int& device_num/*MASTER || SLAVE || MMCARD*/, const int& part_num /*0...3*/);
		std::string getFileEntryString(const char* filename, const std::string& filter_entry, const int& column_num);
		std::string convertByteString(const unsigned long long& byte_size);
		std::string getUsedMmcModulName();
		std::string getInitIdeFilePath();
		std::string getInitMountFilePath();
		std::string getFstabFilePath();
		std::string getDefaultSysMounts();
		std::string getDefaultFstabEntries();
		std::string getTimeStamp();
		std::string getInitFileHeader(std::string & filename);
		std::string getInitFileMountEntries();
		std::string getInitFileModulEntries(bool with_unload_entries = false);
		std::string getPartEntryString(std::string& partname);

		//helper
		std::string iToString(int int_val);


		int exec(CMenuTarget* parent, const std::string & actionKey);

	public:
		enum DRIVE_NUM	
		{
			MASTER,
			SLAVE,
			MMCARD
		};
		
		#define DEVICE_INFO_COUNT 5
		enum DEVICE_INFO	
		{
			KB_BLOCKS,
			KB_USED,
			KB_AVAILABLE,
			PERCENT_USED,
			PERCENT_FREE,
			FILESYSTEM,
			FREE_HOURS
		};
		
		enum PARTSIZE_TYPE_NUM	
		{
			KB,
			MB
		};
		
		CDriveSetup();
		~CDriveSetup();

		std::string getHddTemp(const int& device_num /*MASTER || SLAVE || MMCARD*/); //hdd temperature
		std::string getModelName(const std::string& mountpoint);
		std::string getDriveSetupVersion();

};

class CDriveSetupFsNotifier : public CChangeObserver
{
	private:
		CMenuForwarder* toDisable;
	public:
		CDriveSetupFsNotifier( CMenuForwarder* );
		bool changeNotify(const neutrino_locale_t, void * Data);
};

#endif
