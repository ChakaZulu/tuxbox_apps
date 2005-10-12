/*
 * $Id: enigma_mount.h,v 1.23 2005/10/12 13:04:14 digi_casi Exp $
 *
 * (C) 2005 by digi_casi <digi_casi@tuxbox.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
 
#ifdef ENABLE_DYN_MOUNT

#ifndef __enigma_mount_h__
#define __enigma_mount_h__

#include <configfile.h>
#include <string.h>
#include <lib/gui/listbox.h>

#define	MOUNTCONFIGFILE	"/var/tuxbox/config/enigma/mount.conf"

typedef struct
{
	int id;			// sequential number
	eString	userName;	// username, only for CIFS
	eString	password;	// password, only for CIFS
	eString	localDir;	// local mount dir
	eString	mountDir;	// directory which should be mounted
	int ip[4];		// ip address from machine
	int fstype;		// 0 = NFS, 1 = CIFS, 2 = DEV, 3 = SMBFS
	int automount;		// mount at startup
	eString options;	// rw, intr, soft, udp, nolock
	bool mounted;		// if already mounted or not
	eString description;    // description
} t_mount;

class eMountPoint
{
private:
	bool fileSystemIsSupported(eString);
	bool isMounted(void);
public:
	
	t_mount mp;
	eMountPoint(CConfigFile *, int);
	eMountPoint(t_mount);
	~eMountPoint();
	
	void save(FILE *, int);
	int mount(void);
	int unmount(void);
	bool isIdentical(eString, eString);
};

class eMountMgr
{
private:
	static eMountMgr *instance;
	std::vector <eMountPoint> mountPoints;
	std::vector <eMountPoint>::iterator mp_it;
	void addMountedFileSystems(void);
public:
	eString listMountPoints(eString); // webif
	eString listMovieSources(); // webif
	void removeMountPoint(int);
	int addMountPoint(t_mount);
	void changeMountPoint(int, t_mount);
	t_mount getMountPointData(int);
	int mountMountPoint(int);
	int mountMountPoint(eString);
	int unmountMountPoint(int);
	bool isMountPointMounted(eString);
	void automountMountPoints(void);
	void unmountAllMountPoints(void);
	int selectMovieSource(int);
	void save();
	void init();

	static eMountMgr *getInstance() {return (instance) ? instance : instance = new eMountMgr();}

	eMountMgr();
	~eMountMgr();
};
#endif

#endif
