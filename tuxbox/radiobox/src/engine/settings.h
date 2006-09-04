/*
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


#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include <string>
#include <configfile.h>

#define NETWORK_NFS_NR_OF_ENTRIES	4

class CRadioboxSettings
{
private:
	
	CConfigFile	configfile;
	void Defaults();

public:
	
	void Load(  );
	void Save(  );

	CRadioboxSettings( );
	~CRadioboxSettings( );

public:

	std::string	playlist_root;
	std::string library_root;
	std::string handler;

	enum PLAYORDER 
	{
		PO_Random = 0,
		PO_Normal,
		PO_RepeatAll,
		PO_RepeatFile
	} playorder;

	unsigned int    network_nfs_count;
	int             network_nfs_automount[NETWORK_NFS_NR_OF_ENTRIES];
	std::string     network_nfs_ip[NETWORK_NFS_NR_OF_ENTRIES];
	char            network_nfs_dir[NETWORK_NFS_NR_OF_ENTRIES][100];
	char            network_nfs_local_dir[NETWORK_NFS_NR_OF_ENTRIES][100];
	int             network_nfs_type[NETWORK_NFS_NR_OF_ENTRIES];
	char            network_nfs_username[NETWORK_NFS_NR_OF_ENTRIES][31];
	char            network_nfs_password[NETWORK_NFS_NR_OF_ENTRIES][31];
	char            network_nfs_mount_options1[NETWORK_NFS_NR_OF_ENTRIES][31];
	char            network_nfs_mount_options2[NETWORK_NFS_NR_OF_ENTRIES][31];
	

};





#endif /* __SETTINGS_H__ */
