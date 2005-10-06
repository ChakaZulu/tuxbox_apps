/*
	Neutrino-GUI  -   DBoxII-Project

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
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <system/checksquashfs.h>

#include <global.h>
#include <neutrino.h>

#include <string>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/mount.h>

#define LOCAL_MOUNT_DIR   "/tmp/checkSquashfs"
#define TMPPATH           "/var/tmp/"


CCheckSquashfs::CCheckSquashfs()
{
}

const char * CCheckSquashfs::GetVersionInfo(const char * squashfsimage)
{
	char versionFile[] = ".version";
	char versionPath[sizeof(versionFile) + sizeof(LOCAL_MOUNT_DIR)+5] = "";

	sprintf((char*) versionPath, "%s/.version", LOCAL_MOUNT_DIR);

	if (mountSquashfsImage(squashfsimage))
	{
		CConfigFile configfile('\t');
		const char * versionString = (configfile.loadConfig(versionPath)) ? (configfile.getString( "version", "????????????????").c_str()) : "????????????????";

		printf("[chkSquashfs] read version string: %s\n", versionString);

		unmountSquashfsImage(squashfsimage);

		return versionString;
	} else {
		printf("[chkSquashfs] mount error\n");

		return "????????????????";
	}
}

bool CCheckSquashfs::mountSquashfsImage(const char * squashfsimage)
{
	std::string cmd;

	if (mkdir(LOCAL_MOUNT_DIR, 0700) != 0)
	{
		printf("[chkSquashfs] can't create mount directory: %s\n", LOCAL_MOUNT_DIR);
		return false;

	} else	{
		cmd  = "mount -o loop -o ro -t squashfs ";
		cmd += TMPPATH;
		cmd += squashfsimage;
		cmd += " ";
		cmd += LOCAL_MOUNT_DIR;
		system (cmd.c_str());

		if (system(cmd.c_str()) != 0)
		{
			printf("[chkSquashfs] can't mount squashfs image %s\n", squashfsimage);
			return false;

		} else {
			return true;
		}
	}
}

void CCheckSquashfs::unmountSquashfsImage(const char * squashfsimage)
{
	umount(LOCAL_MOUNT_DIR);

	if (rmdir(LOCAL_MOUNT_DIR) != 0)
	{
		umount(LOCAL_MOUNT_DIR);
		if (rmdir(LOCAL_MOUNT_DIR) != 0)
			printf("[chkSquashfs] can't remove mount directory: %s\n", LOCAL_MOUNT_DIR);
	}
}
