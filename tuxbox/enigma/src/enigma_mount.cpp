#include <stdio.h>
#include <stdlib.h>
#include <sys/mount.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <pthread.h>
#include <string.h>
#include <vector>
#include <lib/base/estring.h>
#include <configfile.h>
#include "enigma_mount.h"

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

void eMountPoint::save(FILE *out) 
{
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

int eMountPoint::readMounts(eString localdir)
{
	std::ifstream in;
	eString mountDev;
	eString mountOn;
	eString mountType;
	eString buffer;
	std::stringstream tmp;

	in.open("/proc/mounts", std::ifstream::in);
	while(getline(in, buffer, '\n'))
	{
		mountDev = "";
		mountOn = "";
		mountType = "";
		tmp.str(buffer);
		tmp >> mountDev >> mountOn >> mountType;
		tmp.clear();
		if(mountOn == localdir)
		{
			in.close();
			return -1;
		}
	}
	in.close();
	return 0;
}

int eMountPoint::doMount()
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
		if (readMounts(localDir) != -1)
		{
			if (access(localDir.c_str(), R_OK))
			{
				
				useoptions = options + ownOptions;
				if (useoptions[useoptions.length() - 1] == ',')
					useoptions = useoptions.left(useoptions.length() - 1);
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
					int retcode;

					pthread_mutex_lock(&g_mut1);
					timeout.tv_sec = time(NULL) + 8;
					timeout.tv_nsec = 0;
					retcode = pthread_cond_timedwait(&g_cond1, &g_mut1, &timeout);
					if (retcode == ETIMEDOUT)
						pthread_cancel(g_mnt1);
					pthread_mutex_unlock(&g_mut1);

					if (g_mntstatus1 != 0)
						rc = -5; //mount failed (timeout)
					else
						mounted = true;
				}
			}
			else
				rc = -10;
		}
		else
			rc = -2; //local dir is already a mountpoint
	}
	else 
		rc = -1; //mount point is already mounted
	return rc;
}

bool eMountPoint::mount()
{
	eString message;
	switch(doMount())
	{
		case -1:
				message = "Mountpoint is already mounted.";
				break;

		case -2:
				message = "Local directory is already used as mount point.";
				break;

		case -3:
				message = "CIFS is not supported.";
				break;

		case -4:
				message = "NFS is not supported.";
				break;

		case -5:
				message = "Mount failed (timeout).";
				break;

		case -10:
//				createFolder();
				break;
	}

	if (message)
	{
		
	}
	return true;
}

bool eMountPoint::unmount()
{
	if (umount2(localDir.c_str(), MNT_FORCE))
		return false;
	else
		return true;
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
}

void eMountMgr::removeMountPoint(int id)
{
	for (mp_it = mountPoints.begin(); mp_it != mountPoints.end(); mp_it++)
		if (mp_it->id == id)
			mountPoints.erase(mp_it);
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
		for (mp_it = mountPoints.begin(); mp_it != mountPoints.end(); mp_it++)
			mp_it->save(out);
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


