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
	mounted    = false;
	id         = i;
}

eMountPoint::eMountPoint(eString plocalDir, int pfstype, eString ppassword, eString puserName, eString pmountDir, int pautomount, int prsize, int pwsize, eString poptions, eString pownOptions, int pid)
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
	id         = pid;
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

bool eMountPoint::in_proc_filesystems(eString fsname)
{
	eString s;
	std::ifstream in("/proc/filesystems", std::ifstream::in);

	while (in >> s)
	{
		if (s == fsname)
	  	{
			in.close();
			return true;
		}
	}
	in.close();

	if (fsname == "nfs")
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
	if (fsname == "cifs") 
	{
		system("insmod cifs");
		system("insmod /lib/modules/`uname -r`/kernel/fs/cifs/cifs.ko");
	} 
	else
		eDebug("[enigma_mount] filesystem %s not supported.", fsname.c_str());

	sleep(2);

	return false;
}

int eMountPoint::isMounted(eString localdir)
{
	std::ifstream in;
	eString mountDev;
	eString mountOn;
	eString mountType;
	eString buffer;
	std::stringstream tmp;

	int rc =0;
	in.open("/proc/mounts", std::ifstream::in);
	while(getline(in, buffer, '\n'))
	{
		mountDev = mountOn = mountType = "";
		tmp.str(buffer);
		tmp >> mountDev >> mountOn >> mountType;
		tmp.clear();
		if (mountOn == localdir)
		{
			rc = -1;
			break;
		}
	}
	in.close();
	return rc;
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
		if (isMounted(localDir) != -1)
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
						if (in_proc_filesystems("nfs") || in_proc_filesystems("nfs"))
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
						if(in_proc_filesystems("cifs") || in_proc_filesystems("cifs"))
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
	save();
	mountPoints.clear();
}

void eMountMgr::addMountPoint(eString plocalDir, int pfstype, eString ppassword, eString puserName, eString pmountDir, int pautomount, int prsize, int pwsize, eString poptions, eString pownOptions, int pid)
{
	mountPoints.push_back(eMountPoint(plocalDir, pfstype, ppassword, puserName, pmountDir, pautomount, prsize, pwsize, poptions, pownOptions, pid));
	save();
}

void eMountMgr::changeMountPoint(eString plocalDir, int pfstype, eString ppassword, eString puserName, eString pmountDir, int pautomount, int prsize, int pwsize, eString poptions, eString pownOptions, int pid)
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
			break;
		}
	}
	save();
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

eString eMountMgr::getMountPointData(int id)
{
	eString result;
	for (mp_it = mountPoints.begin(); mp_it != mountPoints.end(); mp_it++)
		if (mp_it->id == id)
		{
			int i = 1;
//TODO: get data 
		}
	return result;
}

eString eMountMgr::listMountPoints(eString skelleton)
{
	eString result, mountStatus, action;
	if (mountPoints.size() > 0)
		for (mp_it = mountPoints.begin(); mp_it != mountPoints.end(); mp_it++)
		{
			eString tmp = skelleton;
			if (mp_it->mounted)
			{
				mountStatus = "<img src=\"on.gif\" alt=\"online\" border=0>";
				action = button(100, "Unmount", RED, "javascript:unmountMountPoint(" + eString().sprintf("%d", mp_it->id) + ")");
			}
			else
			{ 
				mountStatus = "<img src=\"off.gif\" alt=\"offline\" border=0>";
				action = button(100, "Mount", GREEN, "javascript:mountMountPoint(" + eString().sprintf("%d", mp_it->id) + ")");
			}
			tmp.strReplace("#ACTIONBUTTON#", action);
			action = button(100, "Change", BLUE, "javascript:changeMountPoint(" + eString().sprintf("%d", mp_it->id) + ")");
			tmp.strReplace("#CHANGEBUTTON#", action);
			tmp.strReplace("#MOUNTED#", mountStatus);
			tmp.strReplace("#ID#", eString().sprintf("%d", mp_it->id));
			tmp.strReplace("#LDIR#", mp_it->localDir);
			tmp.strReplace("#MDIR#", mp_it->mountDir);
			tmp.strReplace("#IP#", eString().sprintf("%03d.%03d.%03d.%03d", mp_it->ip[0], mp_it->ip[1], mp_it->ip[2], mp_it->ip[3]));
			tmp.strReplace("#USER#", mp_it->userName);
			tmp.strReplace("#PW#", mp_it->password);
			tmp.strReplace("#FSTYPE#", (mp_it->fstype == 0) ? "NFS" : "CIFS");
			tmp.strReplace("#AUTO#", eString().sprintf("%d", mp_it->automount));
			tmp.strReplace("#OPTIONS#", mp_it->options);
			tmp.strReplace("#OWNOPTIONS#", mp_it->ownOptions);
			tmp.strReplace("#RSIZE#", eString().sprintf("%d", mp_it->rsize));
			tmp.strReplace("#WSIZE#", eString().sprintf("%d", mp_it->wsize));
			result += tmp + "\n";
		}
	else
		result = "<tr><td>No mount points available.</td></tr>";
	
	return result;
}

void eMountMgr::init()
{
	mountPoints.clear();
	CConfigFile *config = new CConfigFile(',');
	if (config->loadConfig(MOUNTCONFIGFILE))
	{
		for (int i = 1; true; i++)
		{
			if (config->getString(eString().sprintf("localdir_%d", i)) != "")
				mountPoints.push_back(eMountPoint(config, i));
			else
				break;
		}
	}
}

void eMountMgr::save()
{
	FILE *out = fopen(MOUNTCONFIGFILE, "w");
	if (out)
	{
		int i = 1;
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


