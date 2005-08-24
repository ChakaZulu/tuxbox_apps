#ifdef ENABLE_DYN_DREAMFLASH

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
	eString imageDir = mountDir + "/image/" + imageName;
	
	static int lastpid = -1;

	eDebug("[DREAMFLASH] installImage: installation device = %s, image file = %s, image dir = %s", mountDir.c_str(), sourceImage.c_str(), imageDir.c_str());
	
	if (lastpid != -1)
	{
		kill(lastpid, SIGKILL);
		waitpid(lastpid, 0, 0);
		lastpid = -1;
	}
	
	// check if enough space is available
	struct statfs s;
	if (statfs(mountDir.c_str(), &s) >= 0) 
		freeSpace = (s.f_bavail * (s.f_bsize / 1024));
	eDebug("[DREAMFLASH] installImage: free space on device = %d", freeSpace);
	if (freeSpace < 20000)
		return -1;
		
	if (access(mountDir.c_str(), W_OK) != 0)
		return -2;

	// check if directory is available, delete it and recreate it
	if (access(imageDir.c_str(), W_OK) == 0)
		system(eString("rm -rf " + imageDir).c_str());
	system(eString("mkdir " + imageDir + " --mode=777 --parents").c_str());
	if (access(imageDir.c_str(), W_OK) != 0)
		return -3;
		
	// flashwizzard compatibility link :-) hmm... @musicbob, u should still fix flashwizzard ;-)
	system(eString("ln -s " + imageDir + " " + mountDir + "/fwpro/" + imageName).c_str());
	
	// split image file
	eString squashfsPart = mountDir + "/squashfs.img";
	remove(eString(squashfsPart + "/squashfs.img").c_str());
	if (system(eString("dd if=" + sourceImage + " of=" + squashfsPart + " bs=1024 skip=1152 count=4992").c_str()) >> 8)
		return -4;
	eString cramfsPart = mountDir + "/cramfs.img";
	remove(eString(cramfsPart + "/cramfs.img").c_str());
	if (system(eString("dd if=" + sourceImage + " of=" + cramfsPart + " bs=1024 skip=0 count=1152").c_str()) >> 8)
		return -5;
	remove(sourceImage.c_str());
	
	switch (lastpid = fork())
	{
		case -1:
			eDebug("[DREAMFLASH] fork failed.");
			return -6;
		case 0:
		{
			eDebug("[DREAMFLASH] installImage: forked.");
			for (unsigned int i = 0; i < 90; ++i )
				close(i);

			// mount squashfs part of the image
			system("rm -rf /tmp/image");
			system("mkdir /tmp/image");
			if (system(eString("mount -t squashfs " + squashfsPart + " /tmp/image -o loop").c_str()) >> 8)
			{
				remove(squashfsPart.c_str());
				eDebug("[DREAMFLASH] mounting squashfs image part failed.");
				_exit(-6);
			}
			// copy files
			if (system(eString("cp -rd /tmp/image/* " + imageDir).c_str()) >> 8)
			{
				system("umount /tmp/image");
				remove(squashfsPart.c_str());
				eDebug("[DREAMFLASH] copying squashfs image part failed.");
				_exit(-7);
			}
			
			eDebug("[DREAMFLASH] installImage: squashfs installed.");
			
			// unmount squashfs part of the image
			system("umount /tmp/image");
			remove(squashfsPart.c_str());
			
			// mount cramfs part of the image
			if (system(eString().sprintf("mount -t cramfs %s /tmp/image -o loop", cramfsPart.c_str()).c_str()) >> 8)
			{
				remove(cramfsPart.c_str());
				eDebug("[DREAMFLASH] mounting cramfs image part failed.");
				_exit(-8);
			}
			// copy files
			if (system(eString("cp -rd /tmp/image/* " + imageDir).c_str()) >> 8)
			{
				system("umount /tmp/image");
				remove(cramfsPart.c_str());
				eDebug("[DREAMFLASH] copying cramfs image part failed.");
				_exit(-9);
			}
			// unmount cramfs part of the image
			system("umount /tmp/image");
			remove(cramfsPart.c_str());
			
			eDebug("[DREAMFLASH] installImage: cramfs installed.");
	
			// delete temp mount dir
			system("rm -rf /tmp/image");
			
			// construct /var
			system(eString("rm -rf " + imageDir + "/var " + imageDir + "/var_init/tmp/init").c_str());
			system(eString("mv " + imageDir + "/var_init " + imageDir + "/var").c_str());
			system(eString("touch " + imageDir + "/var/.init").c_str());
			
			eDebug("[DREAMFLASH] installImage: /var prepared.");
			
			// remove jffs2 mounting from rcS
			system(eString("mv " + imageDir + "/etc/init.d/rcS " + imageDir + "/etc/init.d/rcS.org").c_str());
			system(eString("sed 's?/bin/mount -t jffs2 /dev/mtdblock/1 /var?#/bin/mount -t jffs2 /dev/mtdblock/1 /var?g' " + imageDir + "/etc/init.d/rcS.org > " + imageDir + "/etc/init.d/rcS").c_str());
			system(eString("chmod +x " + imageDir + "/etc/init.d/rcS").c_str());
			
			eDebug("[DREAMFLASH] installing image completed successfully.");
	
			_exit(0);
			break;
		}
	}
	
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

#endif // ENABLE_DYN_DREAMFLASH
