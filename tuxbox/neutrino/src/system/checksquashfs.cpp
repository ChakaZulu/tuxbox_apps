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
	if (mountSquashfsImage(squashfsimage))
	{
		char versionsFile[50] = "";
		sprintf((char*) versionsFile, "%s/.version", LOCAL_MOUNT_DIR);

		CConfigFile configfile('\t');
		const char * versionString = (configfile.loadConfig(versionsFile)) ? (configfile.getString( "version", "????????????????").c_str()) : "????????????????";

		unmountSquashfsImage(squashfsimage);

		return versionString;
	} else {
		const char * versionString = "????????????????";
	}
}

bool CCheckSquashfs::mountSquashfsImage(const char * squashfsimage)
{
	std::string cmd;

	if (mkdir(LOCAL_MOUNT_DIR, 0700) < 0)
	{
		printf("[chkSquashfs] can't create mount directory: %s ", LOCAL_MOUNT_DIR);
		return false;

	} else	{
		cmd  = "mount -o loop -t squashfs ";
		cmd += TMPPATH;
		cmd += squashfsimage;
		cmd += " ";
		cmd += LOCAL_MOUNT_DIR;
		system (cmd.c_str());

		if (system (cmd.c_str()) < 0)
		{
			printf("[chkSquashfs] can't mount squashfs image %s ", squashfsimage);
			return false;

		} else {
			return true;
		}
	}
}

void CCheckSquashfs::unmountSquashfsImage(const char * squashfsimage)
{
	umount(LOCAL_MOUNT_DIR);

	if (rmdir(LOCAL_MOUNT_DIR) < 0)
		printf("[chkSquashfs] can't remove mount directory: %s ", LOCAL_MOUNT_DIR);
}
