#include <stdio.h>
#include <stdlib.h>
#include <sys/mount.h>
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
eMountPoint::eMountPoint()
{
	userName = "";
	password = "";
	localDir = "";
	mountDir = "";
	ip[0] = 0;
	ip[1] = 0;
	ip[2] = 0;
	ip[3] = 0;
	fstype = 0;
	automount = 0;
	options = "nolock";
	ownOptions = "";
	rsize = 4096;
	wsize = 4096;
	mounted = false;
	id = 0;
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

int eMountPoint::doMount()
{
	return 0;
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
				message = "Mounting failed (timeout).";
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
	
	CConfigFile *config = new CConfigFile(',');
	if (config->loadConfig(MOUNTCONFIGFILE))
	{
	}
}

eMountMgr::~eMountMgr()
{
	
}

void eMountMgr::addMountPoint()
{

}

void eMountMgr::removeMountPoint()
{

}

void *mountThread(eString cmd)
{
	int ret = system(cmd.c_str());
	pthread_mutex_lock(&g_mut1);
	g_mntstatus1 = ret;
	pthread_cond_broadcast(&g_cond1);
	pthread_mutex_unlock(&g_mut1);
	pthread_exit(NULL);
}


