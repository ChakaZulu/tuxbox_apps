#ifndef __enigma_mount_h__
#define __enigma_mount_h__

#include <configfile.h>
#include <string.h>

#define	MOUNTCONFIGFILE	"/var/tuxbox/config/enigma/mount.conf"

void *mountThread(void *cmd);

class eMountPoint
{
private:
	bool fileSystemIsSupported(eString);
	bool isMounted(void);
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
	bool mounted;		//if already mounted or not
	int rsize;		//read size
	int wsize;		//write size

	eMountPoint(CConfigFile *, int);
	eMountPoint(eString, int, eString, eString, eString, int, int, int, eString, eString, int, int, int, int, bool, int);
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
	eString listMountPoints(eString);
	void removeMountPoint(int);
	void addMountPoint(eString, int, eString, eString, eString, int, int, int, eString, eString, int, int, int, int, bool);
	void changeMountPoint(eString, int, eString, eString, eString, int, int, int, eString, eString, int, int, int, int, int);
	void getMountPointData(eString *, int *, eString *, eString *, eString *, int *, int *, int *, eString *, eString *, int *, int *, int *, int *, int);
	int mountMountPoint(int);
	int unmountMountPoint(int);
	void save();
	void init();

	static eMountMgr *getInstance() {return (instance) ? instance : instance = new eMountMgr();}

	eMountMgr();
	~eMountMgr();
};
#endif



