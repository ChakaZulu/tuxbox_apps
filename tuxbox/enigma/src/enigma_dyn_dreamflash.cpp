#include <map>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/vfs.h>
#include <net/if.h>
#include <sys/mount.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <lib/system/info.h>
#include <lib/system/http_dyn.h>
#include <enigma_dyn_utils.h>
#include <enigma_dyn_dreamflash.h>

using namespace std;

int installImage(eString sourceImage, eString imageName, eString mountDir)
{
	int freeSpace = 0;
	eString imageDir = mountDir + "/images/" + imageName;
	
	// check if enough space is available
	struct statfs s;
	if (statfs(mountDir.c_str(), &s) >= 0) 
		freeSpace = (s.f_bavail * (s.f_bsize / 1024));
	if (freeSpace < 10000)
		return -1;
		
	if (access(mountDir.c_str(), W_OK) != 0)
		return -2;

	// check if directory is available
	if (access(imageDir.c_str(), W_OK) != 0)
		mkdir(imageDir.c_str(), 0777);
	if (access(imageDir.c_str(), W_OK) != 0)
		return -3;
	
	// split image file
	eString squashfsPart = mountDir + "/squashfs.img";
	if (system(eString("dd if=" + sourceImage + " of=" + squashfsPart + " bs=1024 skip=1152 count=4992").c_str()) >> 8)
		return -4;
	eString cramfsPart = mountDir + "/cramfs.img";
	if (system(eString("dd if=" + sourceImage + " of=" + cramfsPart + " bs=1024 skip=0 count=1152").c_str()) >> 8)
		return -5;
	remove(sourceImage.c_str());
	
	// mount squashfs part of the image
	mkdir("/tmp/image", 0777);
	if (system(eString("mount -t squashfs " + squashfsPart + " /tmp/image -o loop").c_str()) >> 8)
	{
		remove(squashfsPart.c_str());
		return -6;
	}
	
	// copy files
	if (system(eString("cp -Rd /tmp/image " + imageDir).c_str()) >> 8)
	{
		umount2("/tmp/image", MNT_FORCE);
		remove(squashfsPart.c_str());
		return -7;
	}
	
	// unmount squashfs part of the image
	umount2("/tmp/image", MNT_FORCE);
	remove(squashfsPart.c_str());
	
	// mount cramfs part of the image
	if (system(eString().sprintf("mount -t cramfs %s /tmp/image -o loop", cramfsPart.c_str()).c_str()) >> 8)
	{
		remove(cramfsPart.c_str());
		return -8;
	}
	
	// copy files
	if (system(eString("cp -Rd /tmp/image " + imageDir + "/root").c_str()) >> 8)
	{
		umount2("/tmp/image", MNT_FORCE);
		remove(cramfsPart.c_str());
		return -9;
	}
	
	// unmount cramfs part of the image
	umount2("/tmp/image", MNT_FORCE);
	remove(cramfsPart.c_str());
	
	// delete temp mount dir
	remove("/tmp/image");
	
	// construct /var
	system(eString("rm -rf " + imageDir + "/var " + imageDir + "/var_init/tmp/init").c_str());
	system(eString("mv " + imageDir + "/var_init " + imageDir + "/var").c_str());
	system(eString("touch " + imageDir + "/var/.init").c_str());
	
	return 0;
}

eString dreamflash(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	eString sourceImage = opt["image"];
	eString mountDir = opt["target"];
	eString imageName = opt["name"];
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	
	int rc = installImage(sourceImage, imageName, mountDir);
	
	eString result = "done: " + eString().sprintf("%d", rc);
	return result;
}

void ezapDreamflashInitializeDyn(eHTTPDynPathResolver *dyn_resolver, bool lockWeb)
{
	dyn_resolver->addDyn("GET", "/cgi-bin/dreamflash", dreamflash, lockWeb);
}
