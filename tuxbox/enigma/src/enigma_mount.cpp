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
	localDir   = config->getString(eString().sprintf("localdir_%d", i));
	fstype     = config->getInt32(eString().sprintf("fstype_%d", i));
	password   = config->getString(eString().sprintf("password_%d", i));
	userName   = config->getString(eString().sprintf("username_%d", i));
	mountDir   = config->getString(eString().sprintf("mountdir_%d", i));
	automount  = config->getInt32(eString().sprintf("automount_%d", i));
	rsize      = config->getInt32(eString().sprintf("rsize_%d",i), 4096);
	wsize      = config->getInt32(eString().sprintf("wsize_%d",i), 4096);
	options    = config->getString(eString().sprintf("options_%d", i));
	ownOptions = config->getString(eString().sprintf("ownoptions_%d", i));
	eString sip = config->getString(eString().sprintf("ip_%d", i));
	sscanf(sip.c_str(), "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]);
	mounted    = isMounted();
	id         = i;
}

eMountPoint::eMountPoint(eString plocalDir, int pfstype, eString ppassword, eString puserName, eString pmountDir, int pautomount, int prsize, int pwsize, eString poptions, eString pownOptions, int pip0, int pip1, int pip2, int pip3, bool pmounted, int pid)
{
	localDir   = plocalDir;
	fstype     = pfstype;
	password   = ppassword;
	userName   = puserName;
	mountDir   = pmountDir;
	automount  = pautomount;
	rsize      = prsize;
	wsize      = pwsize;
	options    = poptions;
	ownOptions = pownOptions;
	mounted    = false;
	ip[0]      = pip0;
	ip[1]      = pip1;
	ip[2]      = pip2;
	ip[3]      = pip3;
	id         = pid;
	mounted    = pmounted;
}

void eMountPoint::save(FILE *out, int pid)
{
	id = pid;
	fprintf(out,"ip_%d=%d.%d.%d.%d\n", id, ip[0], ip[1], ip[2], ip[3]);
	fprintf(out,"fstype_%d=%d\n", id, fstype);
	fprintf(out,"localdir_%d=%s\n", id, localDir.c_str());
	fprintf(out,"mountdir_%d=%s\n", id, mountDir.c_str());
	fprintf(out,"username_%d=%s\n", id, userName.c_str());
	fprintf(out,"password_%d=%s\n", id, password.c_str());
	fprintf(out,"options_%d=%s\n", id, options.c_str());
	fprintf(out,"ownoptions_%d=%s\n", id, ownOptions.c_str());
	fprintf(out,"automount_%d=%d\n", id, automount);
	fprintf(out,"rsize_%d=%d\n", id, rsize);
	fprintf(out,"wsize_%d=%d\n", id, wsize);
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
	
	switch(fstype)
	{
		case 0: //NFS
			found = (eString().sprintf("%d.%d.%d.%d:%s", ip[0], ip[1], ip[2], ip[3]) == mountDir);
			break;
		case 1: //CIFS
			found = (eString().sprintf("//%d.%d.%d.%d/%s", ip[0], ip[1], ip[2], ip[3]) == mountDir);
			break;
		case 2: //DEVICE
			found = ((mountOn == localDir) && (mountDev == mountDir) && (ip[0] == 0) && (ip[1] == 0) && (ip[2] == 0) && (ip[3] == 0));
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
	if (!mounted)
	{
		pthread_mutex_init(&g_mut1, NULL);
		pthread_cond_init(&g_cond1, NULL);
		g_mntstatus1 = -1;
		if (!isMounted())
		{
			if (!(access(localDir.c_str(), R_OK)))
				system(eString("mkdir" + localDir).c_str());
			if (access(localDir.c_str(), R_OK))
			{

				useoptions = options + ownOptions;
				if (useoptions[useoptions.length() - 1] == ',') //remove?
					useoptions = useoptions.left(useoptions.length() - 1); //remove?
				ip.sprintf("%d.%d.%d.%d", ip[0], ip[1],	ip[2], ip[3]);
				switch(fstype)
				{
					case 0:	/* NFS */
						cmd = "mount -t ";
						cmd += fstype ? "cifs //" : "nfs ";
						if (fileSystemIsSupported("nfs"))
						{
							cmd = ip + ":" + mountDir + " " + localDir + " -o ";
							cmd += eString().sprintf("rsize=%d,wsize=%d", rsize, wsize);
							if (useoptions)
								cmd += "," + useoptions;
						}
						else
							rc = -4; //NFS filesystem not supported
						break;
					case 1: /* CIFS */
						cmd = "mount -t ";
						cmd += fstype ? "cifs //" : "nfs ";
						if(fileSystemIsSupported("cifs"))
						{
							cmd = ip + "/" + mountDir + " " + localDir + " -o user=";
							cmd += (userName != "") ? userName : "anonymous";
							if (password != "")
								cmd += "pass=" + password;
							cmd += ",unc=//" + ip + "/" + mountDir;
							if (useoptions != "")
								cmd += "," + useoptions;
						}
						else
							rc = -3; //CIFS filesystem not supported
						break;
					case 2:
						cmd = "mount " + mountDir + " " + localDir;
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
						mounted = true; //everything is fine :-)
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
	return umount2(localDir.c_str(), MNT_FORCE);
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

void eMountMgr::addMountPoint(eString plocalDir, int pfstype, eString ppassword, eString puserName, eString pmountDir, int pautomount, int prsize, int pwsize, eString poptions, eString pownOptions, int pip0, int pip1, int pip2, int pip3, bool pmounted)
{
	int pid = mountPoints.size();
	mountPoints.push_back(eMountPoint(plocalDir, pfstype, ppassword, puserName, pmountDir, pautomount, prsize, pwsize, poptions, pownOptions, pip0, pip1, pip2, pip3, pmounted, pid));
	save();
	init();
}

void eMountMgr::getMountPointData(eString *plocalDir, int *pfstype, eString *ppassword, eString *puserName, eString *pmountDir, int *pautomount, int *prsize, int *pwsize, eString *poptions, eString *pownOptions, int *pip0, int *pip1, int *pip2, int *pip3, int pid)
{
	for (mp_it = mountPoints.begin(); mp_it != mountPoints.end(); mp_it++)
	{
		if (mp_it->id == pid)
		{
			*plocalDir = mp_it->localDir;
			*pfstype = mp_it->fstype;
			*ppassword = mp_it->password;
			*puserName = mp_it->userName;
			*pmountDir = mp_it->mountDir;
			*pautomount = mp_it->automount;
			*prsize = mp_it->rsize;
			*pwsize = mp_it->wsize;
			*poptions = mp_it->options;
			*pownOptions = mp_it->ownOptions;
			*pip0 = mp_it->ip[0];
			*pip1 = mp_it->ip[1];
			*pip2 = mp_it->ip[2];
			*pip3 = mp_it->ip[3];
			break;
		}
	}
}

void eMountMgr::changeMountPoint(eString plocalDir, int pfstype, eString ppassword, eString puserName, eString pmountDir, int pautomount, int prsize, int pwsize, eString poptions, eString pownOptions, int pip0, int pip1, int pip2, int pip3, int pid)
{
	for (mp_it = mountPoints.begin(); mp_it != mountPoints.end(); mp_it++)
	{
		if (mp_it->id == pid)
		{
			mp_it->localDir = plocalDir;
			mp_it->fstype = pfstype;
			mp_it->password = ppassword;
			mp_it->userName = puserName;
			mp_it->mountDir = pmountDir;
			mp_it->automount = pautomount;
			mp_it->rsize = prsize;
			mp_it->wsize = pwsize;
			mp_it->options = poptions;
			mp_it->ownOptions = pownOptions;
			mp_it->ip[0] = pip0;
			mp_it->ip[1] = pip1;
			mp_it->ip[2] = pip2;
			mp_it->ip[3] = pip3;
			break;
		}
	}
	save();
	init();
}

void eMountMgr::removeMountPoint(int id)
{
	for (mp_it = mountPoints.begin(); mp_it != mountPoints.end(); mp_it++)
		if (mp_it->id == id)
			mountPoints.erase(mp_it);
	save();
}

int eMountMgr::mountMountPoint(int id)
{
	int rc = 0;
	for (mp_it = mountPoints.begin(); mp_it != mountPoints.end(); mp_it++)
		if (mp_it->id == id)
			rc = mp_it->mount();
	return rc;
}

int eMountMgr::unmountMountPoint(int id)
{
	int rc = 0;
	for (mp_it = mountPoints.begin(); mp_it != mountPoints.end(); mp_it++)
		if (mp_it->id == id)
			rc = mp_it->unmount();
	return rc;
}

eString eMountMgr::listMountPoints(eString skelleton)
{
	eString result, mountStatus, action;
	init();
	if (mountPoints.size() > 0)
		for (mp_it = mountPoints.begin(); mp_it != mountPoints.end(); mp_it++)
		{
			eString tmp = skelleton;
			if (mp_it->mounted)
			{
				mountStatus = "<img src=\"on.gif\" alt=\"online\" border=0>";
				action = button(75, "Unmount", OCKER, "javascript:unmountMountPoint('" + eString().sprintf("%d", mp_it->id) + "')");
			}
			else
			{
				mountStatus = "<img src=\"off.gif\" alt=\"offline\" border=0>";
				action = button(75, "Mount", GREEN, "javascript:mountMountPoint('" + eString().sprintf("%d", mp_it->id) + "')");
			}
			tmp.strReplace("#ACTIONBUTTON#", action);
			action = button(75, "Change", BLUE, "javascript:changeMountPoint('" + eString().sprintf("%d", mp_it->id) + "')");
			tmp.strReplace("#CHANGEBUTTON#", action);
			action = button(75, "Delete", RED, "javascript:deleteMountPoint('" + eString().sprintf("%d", mp_it->id) + "')");
			tmp.strReplace("#DELETEBUTTON#", action);
			tmp.strReplace("#MOUNTED#", mountStatus);
			tmp.strReplace("#ID#", eString().sprintf("%d", mp_it->id));
			tmp.strReplace("#LDIR#", mp_it->localDir);
			tmp.strReplace("#MDIR#", mp_it->mountDir);
			tmp.strReplace("#IP#", eString().sprintf("%3d.%3d.%3d.%3d", mp_it->ip[0], mp_it->ip[1], mp_it->ip[2], mp_it->ip[3]));
			tmp.strReplace("#USER#", mp_it->userName);
			tmp.strReplace("#PW#", mp_it->password);
			eString type = "DEV";
			if (mp_it->fstype == 0)
				type = "NFS";
			else
			if (mp_it->fstype == 1)
				type = "CIFS";
			tmp.strReplace("#FSTYPE#", type);
			tmp.strReplace("#AUTO#", eString().sprintf("%d", mp_it->automount));
			eString options = mp_it->options;
			if (mp_it->ownOptions != "")
				options += ", " + mp_it->ownOptions;
			tmp.strReplace("#OPTIONS#", options);
			tmp.strReplace("#RSIZE#", eString().sprintf("%d", mp_it->rsize));
			tmp.strReplace("#WSIZE#", eString().sprintf("%d", mp_it->wsize));
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
	int ip[4];
	eString dir;
	bool found = false;
	
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
					sscanf(mountDev.c_str(), "%d.%d.%d.%d:%*s", &ip[0], &ip[1], &ip[2], &ip[3]);
					dir = getRight(mountDev, ':');
					addMountPoint(mountOn, 0, "", "", dir, 0, 0, 0, "", "", ip[0], ip[1], ip[2], ip[3], true);
				}
			}
			else
			if (mountType.upper() == "CIFS")
			{
				sscanf(mountDev.c_str(), "//%d.%d.%d.%d/%*s", &ip[0], &ip[1], &ip[2], &ip[3]);
				mountDev = mountDev.right(2); //strip off leading slashes
				dir = getRight(mountDev, '/');
				addMountPoint(mountOn, 1, "", "", dir, 0, 0, 0, "", "", ip[0], ip[1], ip[2], ip[3], true);
			}
			else
			if (!((mountOn == "/") ||(mountOn == "/dev") || (mountOn == "/tmp") || (mountOn == "/proc") || (mountOn == "/dev/pts") ||
			     (mountDev == "/dev/mtdblock") || (mountOn == "")))
			{
				//other file system
				addMountPoint(mountOn, 2, "", "", mountDev, 0, 0, 0, "", "", 0, 0, 0, 0, true);
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


