#ifndef __enigma_mount_h__
#define __enigma_mount_h__

#include <configfile.h>
#include <string.h>

#define	MOUNTCONFIGFILE	"/var/tuxbox/config/enigma/mount.conf"

void *mountThread(eString cmd);

class eMountPoint
{
private:
	int doMount();
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
	eString mounted;	//if allready mounted or not
	int rsize;		//read size
	int wsize;		//write size

	eMountPoint();
	eMountPoint(CConfigFile *, int);
	eMountPoint(eString, int, eString, eString, eString, int, int, int, eString, eString, int);
	~eMountPoint();
	
	void save(FILE *);
	bool mount(void);
	bool unmount(void);
};

class eMountMgr
{
private:
	static eMountMgr *instance;
	std::vector <eMountPoint> mountPoints;
public:
	void removeMountPoint();
	void addMountPoint();

	static eMountMgr *getInstance() {return (instance) ? instance : new eMountMgr();}

	eMountMgr();
	~eMountMgr();
};
#endif



