#ifdef ENABLE_DYN_MOUNT
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/mount.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <pthread.h>
#include <string.h>
#include <vector>
#include <lib/base/estring.h>
#include <configfile.h>
#include <enigma_dyn_utils.h>
#include <enigma_mount.h>

using namespace std;

eMountMgr *eMountMgr::instance = NULL;

pthread_mutex_t g_mut1;
pthread_cond_t g_cond1;
pthread_t g_mnt1;
int g_mntstatus1;

eMountPoint::~eMountPoint()
{
}

eMountPoint::eMountPoint(CConfigFile *config, int i)
{
	mp.localDir   = config->getString(eString().sprintf("localdir_%d", i));
	mp.fstype     = config->getInt32(eString().sprintf("fstype_%d", i));
	mp.password   = config->getString(eString().sprintf("password_%d", i));
	mp.userName   = config->getString(eString().sprintf("username_%d", i));
	mp.mountDir   = config->getString(eString().sprintf("mountdir_%d", i));
	mp.automount  = config->getInt32(eString().sprintf("automount_%d", i));
	mp.rsize      = config->getInt32(eString().sprintf("rsize_%d",i), 4096);
	mp.wsize      = config->getInt32(eString().sprintf("wsize_%d",i), 4096);
	mp.options    = config->getString(eString().sprintf("options_%d", i));
	mp.ownOptions = config->getString(eString().sprintf("ownoptions_%d", i));
	eString sip = config->getString(eString().sprintf("ip_%d", i));
	sscanf(sip.c_str(), "%d.%d.%d.%d", &mp.ip[0], &mp.ip[1], &mp.ip[2], &mp.ip[3]);
	mp.mounted    = isMounted();
	mp.id         = i;
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
	fprintf(out,"ownoptions_%d=%s\n", mp.id, mp.ownOptions.c_str());
	fprintf(out,"automount_%d=%d\n", mp.id, mp.automount);
	fprintf(out,"rsize_%d=%d\n", mp.id, mp.rsize);
	fprintf(out,"wsize_%d=%d\n", mp.id, mp.wsize);
	fprintf(out,"--------------------------------------\n");
}

bool eMountPoint::fileSystemIsSupported(eString fsname)
{
	eString s;
	fsname = fsname.upper();
	std::ifstream in("/proc/filesystems", std::ifstream::in);

	while (in >> s)
	{
		if (s.upper() == fsname)
	  	{
			in.close();
			return true;
		}
	}
	in.close();

	if (fsname == "NFS")
	{
#ifdef HAVE_MODPROBE
		system("modprobe nfs");
#else
		system("insmod sunrpc");
		system("insmod lockd");
		system("insmod nfs");
#endif
	}
	else
	if (fsname == "CIFS")
	{
		system("insmod cifs");
		system("insmod /lib/modules/`uname -r`/kernel/fs/cifs/cifs.ko");
	}
	else
		eDebug("[enigma_mount] filesystem %s not supported.", fsname.c_str());

	sleep(2);

	return false;
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
	while(getline(in, buffer, '\n'))
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
	
	switch(mp.fstype)
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
		default:
			break;
	}
	return found;
}

int eMountPoint::mount()
{
	eString cmd;
	eString ip;
	eString useoptions;
	int rc = 0;
	if (!mp.mounted)
	{
		pthread_mutex_init(&g_mut1, NULL);
		pthread_cond_init(&g_cond1, NULL);
		g_mntstatus1 = -1;
		if (!isMounted())
		{
			if (!(access(mp.localDir.c_str(), R_OK)))
				system(eString("mkdir " + mp.localDir).c_str());
			if (access(mp.localDir.c_str(), R_OK))
			{

				useoptions = mp.options + mp.ownOptions;
				if (useoptions[useoptions.length() - 1] == ',') //remove?
					useoptions = useoptions.left(useoptions.length() - 1); //remove?
				ip.sprintf("%d.%d.%d.%d", mp.ip[0], mp.ip[1], mp.ip[2], mp.ip[3]);
				switch(mp.fstype)
				{
					case 0:	/* NFS */
						cmd = "mount -t ";
						cmd += mp.fstype ? "cifs //" : "nfs ";
						if (fileSystemIsSupported("nfs"))
						{
							cmd = ip + ":" + mp.mountDir + " " + mp.localDir + " -o ";
							cmd += eString().sprintf("rsize=%d,wsize=%d", mp.rsize, mp.wsize);
							if (useoptions)
								cmd += "," + useoptions;
						}
						else
							rc = -4; //NFS filesystem not supported
						break;
					case 1: /* CIFS */
						cmd = "mount -t ";
						cmd += mp.fstype ? "cifs //" : "nfs ";
						if(fileSystemIsSupported("cifs"))
						{
							cmd = ip + "/" + mp.mountDir + " " + mp.localDir + " -o user=";
							cmd += (mp.userName != "") ? mp.userName : "anonymous";
							if (mp.password != "")
								cmd += "pass=" + mp.password;
							cmd += ",unc=//" + ip + "/" + mp.mountDir;
							if (useoptions != "")
								cmd += "," + useoptions;
						}
						else
							rc = -3; //CIFS filesystem not supported
						break;
					case 2:
						cmd = "mount " + mp.mountDir + " " + mp.localDir;
						break;
				}

				if (rc == 0)
				{
					pthread_create(&g_mnt1, 0, mountThread, (void *)cmd.c_str());

					struct timespec timeout;
					int rc1;

					pthread_mutex_lock(&g_mut1);
					timeout.tv_sec = time(NULL) + 8;
					timeout.tv_nsec = 0;
					rc1 = pthread_cond_timedwait(&g_cond1, &g_mut1, &timeout);
					if (rc1 == ETIMEDOUT)
						pthread_cancel(g_mnt1);
					pthread_mutex_unlock(&g_mut1);

					if (g_mntstatus1 != 0)
						rc = -5; //mount failed (timeout)
					else
						mp.mounted = true; //everything is fine :-)
				}
			}
			else
				rc = -10; //couldn't create local dir
		}
		else
			rc = -2; //local dir is already a mountpoint
	}
	else
		rc = -1; //mount point is already mounted
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

}

void eMountMgr::addMountPoint(t_mount pmp)
{
	pmp.id = mountPoints.size();
	mountPoints.push_back(eMountPoint(pmp));
	save();
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

void eMountMgr::automountMountPoints(void)
{
	int rc;
	for (mp_it = mountPoints.begin(); mp_it != mountPoints.end(); mp_it++)
	{
		if (mp_it->mp.automount == 1)
			rc = mp_it->mount();
	}
}

eString eMountMgr::listMountPoints(eString skelleton)
{
	eString result, mountStatus, action;
	if (mountPoints.size() > 0)
		for (mp_it = mountPoints.begin(); mp_it != mountPoints.end(); mp_it++)
		{
			eString tmp = skelleton;
			if (mp_it->mp.mounted)
			{
				mountStatus = "<img src=\"on.gif\" alt=\"online\" border=0>";
				action = button(75, "Unmount", OCKER, "javascript:unmountMountPoint('" + eString().sprintf("%d", mp_it->mp.id) + "')");
			}
			else
			{
				mountStatus = "<img src=\"off.gif\" alt=\"offline\" border=0>";
				action = button(75, "Mount", GREEN, "javascript:mountMountPoint('" + eString().sprintf("%d", mp_it->mp.id) + "')");
			}
			tmp.strReplace("#ACTIONBUTTON#", action);
			action = button(75, "Change", BLUE, "javascript:changeMountPoint('" + eString().sprintf("%d", mp_it->mp.id) + "')");
			tmp.strReplace("#CHANGEBUTTON#", action);
			action = button(75, "Delete", RED, "javascript:deleteMountPoint('" + eString().sprintf("%d", mp_it->mp.id) + "')");
			tmp.strReplace("#DELETEBUTTON#", action);
			tmp.strReplace("#MOUNTED#", mountStatus);
			tmp.strReplace("#ID#", eString().sprintf("%d", mp_it->mp.id));
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
			tmp.strReplace("#FSTYPE#", type);
			tmp.strReplace("#AUTO#", eString().sprintf("%d", mp_it->mp.automount));
			eString options = mp_it->mp.options;
			if (mp_it->mp.ownOptions != "")
				options += ", " + mp_it->mp.ownOptions;
			tmp.strReplace("#OPTIONS#", mp_it->mp.options);
			tmp.strReplace("#RSIZE#", eString().sprintf("%d", mp_it->mp.rsize));
			tmp.strReplace("#WSIZE#", eString().sprintf("%d", mp_it->mp.wsize));
			result += tmp + "\n";
		}
	else
		result = "<tr><td>No mount points available.</td></tr>";

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
	while(getline(in, buffer, '\n'))
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
					mp.mountDir   = getRight(mountDev, ':');
					mp.localDir   = mountOn;
					mp.fstype     = 0;
					mp.password   = "";
					mp.userName   = "";
					mp.automount  = 0;
					mp.rsize      = 0;
					mp.wsize      = 0;
					mp.options    = "";
					mp.ownOptions = "";
					mp.mounted    = true;
					mp.id         = -1; //don't care
					addMountPoint(mp);
				}
			}
			else
			if (mountType.upper() == "CIFS")
			{
				sscanf(mountDev.c_str(), "//%d.%d.%d.%d/%*s", &mp.ip[0], &mp.ip[1], &mp.ip[2], &mp.ip[3]);
				mountDev       = mountDev.right(2); //strip off leading slashes
				mp.mountDir   = getRight(mountDev, '/');
				mp.localDir   = mountOn;
				mp.fstype     = 1;
				mp.password   = "";
				mp.userName   = "";
				mp.automount  = 0;
				mp.rsize      = 0;
				mp.wsize      = 0;
				mp.options    = "";
				mp.ownOptions = "";
				mp.mounted    = true;
				mp.id         = -1; //don't care
				addMountPoint(mp);
			}
			else
			if (!((mountOn == "/") ||(mountOn == "/dev") || (mountOn == "/tmp") || (mountOn == "/proc") || (mountOn == "/dev/pts") ||
			     (mountDev == "/dev/mtdblock") || (mountOn == "")))
			{
				//other file system
				sscanf("//0.0.0.0/nothing", "//%d.%d.%d.%d/%*s", &mp.ip[0], &mp.ip[1], &mp.ip[2], &mp.ip[3]);
				mp.mountDir   = mountDev;
				mp.localDir   = mountOn;
				mp.fstype     = 1;
				mp.password   = "";
				mp.userName   = "";
				mp.automount  = 0;
				mp.rsize      = 0;
				mp.wsize      = 0;
				mp.options    = "";
				mp.ownOptions = "";
				mp.mounted    = true;
				mp.id         = -1; //don't care
				addMountPoint(mp);
			}
		}
	}
	in.close();
}

void eMountMgr::init()
{
	mountPoints.clear();
	CConfigFile *config = new CConfigFile(',');
	if (config->loadConfig(MOUNTCONFIGFILE))
	{
		for (int i = 0; true; i++)
		{
			if (config->getString(eString().sprintf("localdir_%d", i)) != "")
				mountPoints.push_back(eMountPoint(config, i));
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
			mp_it->save(out, i);
			i++;
		}
		fclose(out);
	}
}

void *mountThread(void *cmd)
{
	int ret = system((char *)cmd);
	pthread_mutex_lock(&g_mut1);
	g_mntstatus1 = ret;
	pthread_cond_broadcast(&g_cond1);
	pthread_mutex_unlock(&g_mut1);
	pthread_exit(NULL);
}

#endif
