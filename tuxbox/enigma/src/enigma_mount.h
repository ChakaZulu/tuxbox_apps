#ifndef __enigma_mount_h__
#define __enigma_mount_h__

#include <configfile.h>
#include <string.h>

#define	MOUNTCONFIGFILE	"/var/tuxbox/config/enigma/mount.conf"

void *mountThread(void *cmd);

class eMountPoint
{
private:
	bool in_proc_filesystems(eString);
	int readMounts(eString);
public:
	int id;			//sequential number
	eString	userName;	//username, only for CIFS
	eString	password;	//password, only for CIFS
	eString	localDir;	//local mount dir
	eString	mountDir;	//directory which should be mounted
	int ip[4];		//ip address from machine
	int fstype;		//0...NFS, 1...CIFS
	int automount;		//mount at startup
	eString options;	//rw, intr, soft, udp, nolock
	eString ownOptions;	//rw, intr, soft, udp, nolock
	bool mounted;	//if already mounted or not
	int rsize;		//read size
	int wsize;		//write size

	eMountPoint(CConfigFile *, int);
	eMountPoint(eString, int, eString, eString, eString, int, int, int, eString, eString, int);
	~eMountPoint();
	
	void save(FILE *);
	int mount(void);
	bool unmount(void);
};

class eMountMgr
{
private:
	static eMountMgr *instance;
	std::vector <eMountPoint> mountPoints;
	std::vector <eMountPoint>::iterator mp_it;
public:
	void removeMountPoint(int);
	void addMountPoint(eString, int, eString, eString, eString, int, int, int, eString, eString, int);
	void save();
	void init();
	std::vector<eMountPoint> *mountPointList() {return &mountPoints;}

	static eMountMgr *getInstance() {return (instance) ? instance : new eMountMgr();}

	eMountMgr();
	~eMountMgr();
};
#endif



