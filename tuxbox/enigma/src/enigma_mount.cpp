/*
 * $Id: enigma_mount.cpp,v 1.51 2005/12/04 14:18:04 digi_casi Exp $
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

#ifdef ENABLE_EXPERT_WEBIF
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <pthread.h>
#include <string.h>
#include <vector>
#include <lib/base/estring.h>
#include <lib/gui/enumber.h>
#include <configfile.h>
#include <enigma_dyn_utils.h>
#include <enigma_mount.h>
#include <configfile.h>

using namespace std;

eMountMgr *eMountMgr::instance;

eMountPoint::~eMountPoint()
{
}

eMountPoint::eMountPoint(t_mount pmp)
{
	mp = pmp;
}

void eMountPoint::save(FILE *out, int pid)
{
	mp.id = pid;
	fprintf(out,"ip_%d=%d.%d.%d.%d\n", mp.id, mp.ip[0], mp.ip[1], mp.ip[2], mp.ip[3]);
	fprintf(out,"fstype_%d=%d\n", mp.id, mp.fstype);
	fprintf(out,"localdir_%d=%s\n", mp.id, mp.localDir.c_str());
	fprintf(out,"mountdir_%d=%s\n", mp.id, mp.mountDir.c_str());
	fprintf(out,"username_%d=%s\n", mp.id, mp.userName.c_str());
	fprintf(out,"password_%d=%s\n", mp.id, mp.password.c_str());
	fprintf(out,"options_%d=%s\n", mp.id, mp.options.c_str());
	fprintf(out,"description_%d=%s\n", mp.id, mp.description.c_str());
	fprintf(out,"automount_%d=%d\n", mp.id, mp.automount);
	fprintf(out,"\n");
}

bool eMountPoint::fileSystemIsSupported(eString fsname)
{
	eString s;
	bool found = false;
	fsname = fsname.upper();
	std::ifstream in("/proc/filesystems", std::ifstream::in);

	while (in >> s)
	{
		if (found = (s.upper() == fsname))
			break;
	}
			
	in.close();
	return found;
}

bool eMountPoint::isMounted()
{
	std::ifstream in;
	eString mountDev;
	eString mountOn;
	eString mountType;
	eString buffer;
	std::stringstream tmp;

	bool found = false;
	in.open("/proc/mounts", std::ifstream::in);
	while (getline(in, buffer, '\n'))
	{
		mountDev = mountOn = mountType = "";
		tmp.str(buffer);
		tmp >> mountDev >> mountOn >> mountType;
		tmp.clear();
		if (found = isIdentical(mountOn, mountDev))
			break;
	}
	in.close();
	return found;
}

bool eMountPoint::isIdentical(eString mountOn, eString mountDev)
{
	bool found = false;
	eString dir;
	
	switch (mp.fstype)
	{
		case 0: //NFS
			found = (eString().sprintf("%d.%d.%d.%d:%s", mp.ip[0], mp.ip[1], mp.ip[2], mp.ip[3], mp.mountDir.c_str()) == mountDev);
			break;
		case 1: //CIFS
			found = (eString().sprintf("//%d.%d.%d.%d/%s", mp.ip[0], mp.ip[1], mp.ip[2], mp.ip[3], mp.mountDir.c_str()) == mountDev);
			break;
		case 2: //DEVICE
			found = ((mountOn == mp.localDir) && (mountDev == mp.mountDir) && (mp.ip[0] == 0) && (mp.ip[1] == 0) && (mp.ip[2] == 0) && (mp.ip[3] == 0));
			break;
		case 3: //SMBFS
			dir = mp.mountDir;
			found = ((mountOn == mp.localDir) && (mountDev.upper() == dir.upper()));
			break;
		default:
			break;
	}
	return found;
}

int eMountPoint::mount()
{
	eString cmd;
	eString ip;
	int rc = 0;

	static int lastpid=-1;
	
//	if (!mp.mounted)
//	{
//		if (!isMounted())
//		{
			if (access(mp.localDir.c_str(), R_OK) == -1)
				system(eString("mkdir " + mp.localDir).c_str());
			if (access(mp.localDir.c_str(), R_OK) == 0)
			{
				ip.sprintf("%d.%d.%d.%d", mp.ip[0], mp.ip[1], mp.ip[2], mp.ip[3]);
				switch (mp.fstype)
				{
					case 0:	/* NFS */
						if (fileSystemIsSupported("nfs"))
						{
							cmd = "mount -t nfs ";
							cmd += ip + ":" + mp.mountDir + " " + mp.localDir;
							cmd += (mp.options) ? (" -o " + mp.options) : "";
						}
						else
							rc = -4; //NFS filesystem not supported
						break;
					case 1: /* CIFS */
						if (fileSystemIsSupported("cifs"))
						{
							cmd = "mount -t cifs //";
							cmd += ip + "/" + mp.mountDir + " " + mp.localDir + " -o user=";
							cmd += (mp.userName) ? mp.userName : "anonymous";
							cmd += (mp.password) ? (",pass=" + mp.password) : "";
							cmd += ",unc=//" + ip + "/" + mp.mountDir;
							cmd += (mp.options) ? ("," + mp.options) : "";
						}
						else
							rc = -3; //CIFS filesystem not supported
						break;
					case 2:
						cmd = "mount " + mp.mountDir + " " + mp.localDir;
						break;
					case 3: /* SMBFS */
						if (fileSystemIsSupported("smbfs"))
						{
							cmd = "smbmount ";
							cmd += mp.mountDir;
							cmd += " " + ((mp.password) ? mp.password : "guest");
							cmd += " -U " + ((mp.userName) ? mp.userName : "guest");
							cmd += " -I " + ip;
							cmd += " -c \"mount " + mp.localDir + "\"";
						}
						else
							rc = -3; //SMBFS filesystem not supported
						break;
				}

				if (rc == 0)
				{
					eDebug("[ENIGMA_MOUNT] mounting: %s", cmd.c_str());

					switch (lastpid = fork())
					{
						case -1:
							eDebug("fork failed!");
							return -5;
						case 0:
						{
							for (unsigned int i = 0; i < 90; ++i )
								close(i);

							int retry = 10;
							while ((rc = system(cmd.c_str())) >> 8 && retry-- > 0);
							eDebug("[ENIGMA_MOUNT] mount rc = %d", rc);
							
							if (mp.localDir == "/hdd")
							{
								sleep(5);
								system("wget http://127.0.0.1/cgi-bin/reloadRecordings > /dev/null");
							}
							_exit(0);
							break;
						}
					}
				}
			}
			else
				rc = -10; //couldn't create local dir
//		}
//		else
//			rc = -2; //local dir is already a mountpoint
//	}
//	else
//		rc = -1; //mount point is already mounted

	return rc;
/*
 -1: "Mountpoint is already mounted.";
 -2: "Local directory is already used as mount point.";
 -3: "CIFS is not supported.";
 -4: "NFS is not supported.";
 -5: "Mount failed (timeout).";
-10: "Unable to create mount dir.";
*/
}

int eMountPoint::unmount()
{
	mp.mounted = false;
	return umount2(mp.localDir.c_str(), MNT_FORCE);
}

eMountMgr::eMountMgr()
{
	if (!instance)
		instance = this;

	init();
}

eMountMgr::~eMountMgr()
{
	mountPoints.clear();
	instance = NULL;
}

int eMountMgr::addMountPoint(t_mount pmp)
{
	pmp.id = mountPoints.size();
	mountPoints.push_back(eMountPoint(pmp));
	save();
	return pmp.id;
}

t_mount eMountMgr::getMountPointData(int pid)
{
	t_mount tmp;
	for (mp_it = mountPoints.begin(); mp_it != mountPoints.end(); mp_it++)
	{
		if (mp_it->mp.id == pid)
		{
			tmp = mp_it->mp;
			break;
		}
	}
	return tmp;
}

void eMountMgr::changeMountPoint(int pid, t_mount pmp)
{
	for (mp_it = mountPoints.begin(); mp_it != mountPoints.end(); mp_it++)
	{
		if (mp_it->mp.id == pid)
		{
			mp_it->mp = pmp;
			break;
		}
	}
	save();
}

void eMountMgr::removeMountPoint(int id)
{
	for (mp_it = mountPoints.begin(); mp_it != mountPoints.end(); mp_it++)
	{
		if (mp_it->mp.id == id)
		{
			mountPoints.erase(mp_it);
			break;
		}
	}
	save();
}

int eMountMgr::mountMountPoint(int id)
{
	int rc = 0;
	for (mp_it = mountPoints.begin(); mp_it != mountPoints.end(); mp_it++)
	{
		if (mp_it->mp.id == id)
		{
			rc = mp_it->mount();
			break;
		}
	}
	return rc;
}

int eMountMgr::mountMountPoint(eString localDir)
{
	int rc = 0;
	for (mp_it = mountPoints.begin(); mp_it != mountPoints.end(); mp_it++)
	{
		if (mp_it->mp.localDir == localDir)
		{
			if (!mp_it->mp.mounted)
				rc = mp_it->mount();
			break;
		}
	}
	return rc;
}

bool eMountMgr::isMountPointMounted(eString localDir)
{
	bool rc = false;
	for (mp_it = mountPoints.begin(); mp_it != mountPoints.end(); mp_it++)
	{
		if (mp_it->mp.localDir == localDir)
		{
			if (mp_it->mp.mounted)
				rc = true;
			break;
		}
	}
	return rc;
}

int eMountMgr::unmountMountPoint(int id)
{
	int rc = 0;
	for (mp_it = mountPoints.begin(); mp_it != mountPoints.end(); mp_it++)
	{
		if (mp_it->mp.id == id)
		{
			rc = mp_it->unmount();
			break;
		}
	}
	return rc;
}

int eMountMgr::selectMovieSource(int id)
{
	int rc = 0;
	eDebug("[ENIGMA_MOUNT] selectMovieSource id: %d", id);
	for (mp_it = mountPoints.begin(); mp_it != mountPoints.end(); mp_it++)
	{
		if ((mp_it->mp.localDir == "/hdd") && mp_it->mp.mounted)
		{
			eDebug("[ENIGMA_MOUNT] selectMovieSource unmounting: %s", mp_it->mp.mountDir.c_str());
			rc = mp_it->unmount();
		}
	}
	sleep(3);
	for (mp_it = mountPoints.begin(); mp_it != mountPoints.end(); mp_it++)
	{
		if (mp_it->mp.id == id)
		{
			eDebug("[ENIGMA_MOUNT] selectMovieSource mounting: %s", mp_it->mp.mountDir.c_str());
			mp_it->mp.localDir = "/hdd"; // force /hdd 
			rc = mp_it->mount();
			break;
		}
	}
	
	int time;
	rc = 12;
	do
	{
		time = rc;
		rc = sleep(time);
	}
	while ((rc > 0) && (rc < time));
	
	eDebug("[ENIGMA_MOUNT] selectMovieSource rc: %d", rc);
	return rc;
}

void eMountMgr::automountMountPoints(void)
{
	eDebug("[ENIGMA_MOUNT] automountMountPoints...");
	for (mp_it = mountPoints.begin(); mp_it != mountPoints.end(); mp_it++)
	{
		eDebug("[ENIGMA_MOUNT] automountMountPoints: %s - %d", mp_it->mp.mountDir.c_str(), mp_it->mp.automount);
		if (mp_it->mp.automount == 1)
		{
			eDebug("[ENIGMA_MOUNT] automounting %s", mp_it->mp.mountDir.c_str());
			mp_it->mount();
		}
	}
}

void eMountMgr::unmountAllMountPoints(void)
{
	for (mp_it = mountPoints.begin(); mp_it != mountPoints.end(); mp_it++)
	{
		if (mp_it->mp.mounted == 1)
			mp_it->unmount();
	}
}

eString eMountMgr::listMountPoints(eString skelleton)
{
	eString result, mountStatus, action;
	init();
	if (mountPoints.size() > 0)
		for (mp_it = mountPoints.begin(); mp_it != mountPoints.end(); mp_it++)
		{
			eString tmp = skelleton;
			if (mp_it->mp.mounted)
			{
				mountStatus = "<img src=\"on.gif\" alt=\"online\" border=0>";
				action = button(75, "Unmount", RED, "javascript:unmountMountPoint('" + eString().sprintf("%d", mp_it->mp.id) + "')", "#FFFFFF");
			}
			else
			{
				mountStatus = "<img src=\"off.gif\" alt=\"offline\" border=0>";
				action = button(75, "Mount", BLUE, "javascript:mountMountPoint('" + eString().sprintf("%d", mp_it->mp.id) + "')", "#FFFFFF");
			}
			tmp.strReplace("#ACTIONBUTTON#", action);
			tmp.strReplace("#CHANGEID#", eString().sprintf("%d", mp_it->mp.id));
			tmp.strReplace("#DELETEID#", eString().sprintf("%d", mp_it->mp.id));
			tmp.strReplace("#MOUNTED#", mountStatus);
			tmp.strReplace("#LDIR#", mp_it->mp.localDir);
			tmp.strReplace("#MDIR#", mp_it->mp.mountDir);
			tmp.strReplace("#IP#", eString().sprintf("%3d.%3d.%3d.%3d", mp_it->mp.ip[0], mp_it->mp.ip[1], mp_it->mp.ip[2], mp_it->mp.ip[3]));
			tmp.strReplace("#USER#", mp_it->mp.userName);
			tmp.strReplace("#PW#", mp_it->mp.password);
			eString type = "DEV";
			if (mp_it->mp.fstype == 0)
				type = "NFS";
			else
			if (mp_it->mp.fstype == 1)
				type = "CIFS";
			else
			if (mp_it->mp.fstype == 3)
				type = "SMBFS";
			tmp.strReplace("#FSTYPE#", type);
			tmp.strReplace("#AUTO#", eString().sprintf("%d", mp_it->mp.automount));
			tmp.strReplace("#OPTIONS#", mp_it->mp.options);
			tmp.strReplace("#DESCRIPTION#", mp_it->mp.description);
			result += tmp + "\n";
		}
	else
		result = "<tr><td>No mount points available.</td></tr>";

	return result;
}

eString eMountMgr::listMovieSources()
{
	eString tmp, result;
	init();
	if (mountPoints.size() > 0)
		for (mp_it = mountPoints.begin(); mp_it != mountPoints.end(); mp_it++)
		{
			tmp = "<option #SEL# value=\"" + eString().sprintf("%d", mp_it->mp.id) + "\">" + ((mp_it->mp.description) ? mp_it->mp.description : mp_it->mp.mountDir) + "</option>";
			if (mp_it->mp.mounted && (mp_it->mp.localDir == "/hdd"))
				tmp.strReplace("#SEL#", "selected");
			else
				tmp.strReplace("#SEL#", "");

			result += tmp + "\n";
		}
	else
		result = "<option selected value=\"0\">No movie source available.</option>";
		
	if (result.find("selected") == eString::npos)
		result = "<option selected value=\"0\">No movie source selected.</option>\n" + result;

	return result;
}

void eMountMgr::addMountedFileSystems()
{
	std::ifstream in;
	eString mountDev;
	eString mountOn;
	eString mountType;
	eString buffer;
	std::stringstream tmp;
	bool found = false;
	t_mount mp;

	in.open("/proc/mounts", std::ifstream::in);
	while (getline(in, buffer, '\n'))
	{
		mountDev = mountOn = mountType = "";
		tmp.str(buffer);
		tmp >> mountDev >> mountOn >> mountType;
		tmp.clear();
		
		for (mp_it = mountPoints.begin(); mp_it != mountPoints.end(); mp_it++)
		{
			if (found = mp_it->isIdentical(mountOn, mountDev))
				break;	
		}
		
		if (!found)
		{
			//add the mount point
			if (mountType.upper() == "NFS")
			{
				if (mountDev.find("/dev") != eString::npos)
				{
					sscanf(mountDev.c_str(), "%d.%d.%d.%d:%*s", &mp.ip[0], &mp.ip[1], &mp.ip[2], &mp.ip[3]);
					mp.mountDir = getRight(mountDev, ':');
					mp.localDir = mountOn;
					mp.fstype = 0;
					mp.password = "";
					mp.userName = "";
					mp.automount = 0;
					mp.options = "";
					mp.description = "";
					mp.mounted = true;
					mp.id = -1; //don't care
					addMountPoint(mp);
				}
			}
			else
			if (mountType.upper() == "CIFS")
			{
				sscanf(mountDev.c_str(), "//%d.%d.%d.%d/%*s", &mp.ip[0], &mp.ip[1], &mp.ip[2], &mp.ip[3]);
				mountDev = mountDev.right(2); //strip off leading slashes
				mp.mountDir = getRight(mountDev, '/');
				mp.localDir = mountOn;
				mp.fstype = 1;
				mp.password = "";
				mp.userName = "";
				mp.automount = 0;
				mp.options = "";
				mp.description = "";
				mp.mounted = true;
				mp.id = -1; //don't care
				addMountPoint(mp);
			}
			else
			if (!((mountOn == "/") ||(mountOn == "/dev") || (mountOn == "/tmp") || (mountOn == "/proc") || (mountOn == "/dev/pts") ||
			     (mountDev.find("/dev/mtdblock") != eString::npos) || (mountDev == "usbfs") || (mountOn == "")))
			{
				//other file system
				sscanf("//0.0.0.0/nothing", "//%d.%d.%d.%d/%*s", &mp.ip[0], &mp.ip[1], &mp.ip[2], &mp.ip[3]);
				mp.mountDir = mountDev;
				mp.localDir = mountOn;
				mp.fstype = 2;
				mp.password = "";
				mp.userName = "";
				mp.automount = 0;
				mp.options = "";
				mp.description = "";
				mp.mounted = true;
				mp.id = -1; //don't care
				addMountPoint(mp);
			}
		}
	}
	in.close();
}

void eMountMgr::init()
{
	t_mount mp;
	mountPoints.clear();
	CConfigFile *config = new CConfigFile(',');
	if (config->loadConfig(MOUNTCONFIGFILE))
	{
		for (int i = 0; true; i++)
		{
			if (config->getString(eString().sprintf("localdir_%d", i)) != "")
			{
				mp.localDir = config->getString(eString().sprintf("localdir_%d", i));
				mp.fstype = config->getInt32(eString().sprintf("fstype_%d", i));
				mp.password = config->getString(eString().sprintf("password_%d", i));
				mp.userName = config->getString(eString().sprintf("username_%d", i));
				mp.mountDir = config->getString(eString().sprintf("mountdir_%d", i));
				mp.automount = config->getInt32(eString().sprintf("automount_%d", i));
				mp.options = config->getString(eString().sprintf("options_%d", i));
				mp.description = config->getString(eString().sprintf("description_%d", i));
				eString sip = config->getString(eString().sprintf("ip_%d", i));
				sscanf(sip.c_str(), "%d.%d.%d.%d", &mp.ip[0], &mp.ip[1], &mp.ip[2], &mp.ip[3]);
				mp.id = i;
				eMountPoint m = eMountPoint(mp);
				m.mp.mounted = m.isMounted();
				mountPoints.push_back(m);
			}
			else
				break;
		}
	}
	delete config;
	
	addMountedFileSystems();
}

void eMountMgr::save()
{
	FILE *out = fopen(MOUNTCONFIGFILE, "w");
	if (out)
	{
		int i = 0;
		for (mp_it = mountPoints.begin(); mp_it != mountPoints.end(); mp_it++)
		{
			if ((mp_it->mp.fstype != 2) || (mp_it->mp.mountDir.find("disc") != eString::npos) || (mp_it->mp.mountDir.find("part1") != eString::npos)) // just save NFS, CIFS, and SMBFS
			{
				mp_it->save(out, i);
				i++;
			}
		}
		fclose(out);
	}
}

#endif
