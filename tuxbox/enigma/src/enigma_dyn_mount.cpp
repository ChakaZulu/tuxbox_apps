#include <map>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <lib/system/http_dyn.h>
#include <lib/system/econfig.h>
#include <enigma_dyn.h>
#include <enigma_dyn_utils.h>
#include <enigma_dyn_mount.h>
#include <enigma_mount.h>

using namespace std;

eString getConfigMountMgr(void)
{
	eString result = readFile(TEMPLATE_DIR + "mountPoints.tmp");
	eString skelleton = readFile(TEMPLATE_DIR + "mountPoint.tmp");
	result.strReplace("#BODY#", eMountMgr::getInstance()->listMountPoints(skelleton));
	return result;
}

static eString addMountPoint(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::map<eString,eString> opt = getRequestOptions(opts, '&');
	eString localDir = opt["ldir"];
	eString fstype = opt["fstype"];
	eString password = opt["pw"];
	eString userName = opt["user"];
	eString mountDir = opt["mdir"];
	eString automount = opt["auto"];
	eString rsize = opt["rsize"];
	eString wsize = opt["wsize"];
	eString options = opt["options"];
	eString ownOptions = opt["ownoptions"];
	eString ip0 = opt["ip0"];
	eString ip1 = opt["ip1"];
	eString ip2 = opt["ip2"];
	eString ip3 = opt["ip3"];

	content->local_header["Content-Type"]="text/html; charset=utf-8";
	eMountMgr::getInstance()->addMountPoint(localDir, fstype, password, userName, mountDir, atoi(automount.c_str()), atoi(rsize.c_str()), atoi(wsize.c_str()), options, ownOptions, atoi(ip0.c_str()), atoi(ip1.c_str()), atoi(ip2.c_str()), atoi(ip3.c_str()), false);
	return "<html><body>Mount point added successfully.</body></html>";
}

static eString removeMountPoint(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::map<eString,eString> opt = getRequestOptions(opts, '&');
	eString id = opt["id"];

	content->local_header["Content-Type"]="text/html; charset=utf-8";
	eMountMgr::getInstance()->removeMountPoint(atoi(id.c_str()));

	return "<html><body>Mount point removed successfully.</body></html>";
}

static eString changeMountPoint(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::map<eString,eString> opt = getRequestOptions(opts, '&');
	eString localDir = opt["ldir"];
	eString fstype = opt["fstype"];
	eString password = opt["pw"];
	eString userName = opt["user"];
	eString mountDir = opt["mdir"];
	eString automount = opt["auto"];
	eString rsize = opt["rsize"];
	eString wsize = opt["wsize"];
	eString options = opt["options"];
	eString ownOptions = opt["ownoptions"];
	eString ip0 = opt["ip0"];
	eString ip1 = opt["ip1"];
	eString ip2 = opt["ip2"];
	eString ip3 = opt["ip3"];
	eString id = opt["id"];

	content->local_header["Content-Type"]="text/html; charset=utf-8";
	eMountMgr::getInstance()->changeMountPoint(localDir, fstype, password, userName, mountDir, atoi(automount.c_str()), atoi(rsize.c_str()), atoi(wsize.c_str()), options, ownOptions, atoi(ip0.c_str()), atoi(ip1.c_str()), atoi(ip2.c_str()), atoi(ip3.c_str()), atoi(id.c_str()));
	return "<html><body>Mount point changed successfully.</body></html>";
}

static eString addMountPointWindow(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	eString result = readFile(TEMPLATE_DIR + "addMountPointWindow.tmp");

	std::map<eString,eString> opt = getRequestOptions(opts, '&');
	content->local_header["Content-Type"]="text/html; charset=utf-8";

	//TODO: fill in mount point default data...

	return result;
}

static eString editMountPointWindow(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	eString result = readFile(TEMPLATE_DIR + "editMountPointWindow.tmp");

	std::map<eString,eString> opt = getRequestOptions(opts, '&');
	eString id = opt["id"];

	//TODO: get mount point and fill in mount point data...

	content->local_header["Content-Type"]="text/html; charset=utf-8";

	return result;
}

static eString mountMountPoint(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	eString result;

	std::map<eString,eString> opt = getRequestOptions(opts, '&');
	eString id = opt["id"];

	content->local_header["Content-Type"]="text/html; charset=utf-8";
	int rc = eMountMgr::getInstance()->mountMountPoint(atoi(id.c_str()));

	switch(rc)
	{
		case -1:
			result = "Mountpoint is already mounted.";
			break;
 		case -2:
			result = "Local directory is already used as mount point.";
			break;
 		case -3:
			result = "CIFS is not supported.";
			break;
		case -4:
			result = "NFS is not supported.";
			break;
 		case -5:
			result = "Mount failed (timeout).";
			break;
		case -10:
			result = "Unable to create mount directory.";
			break;
		default:
			result = "Mount point mounted successfully.";
			break;
	}

	return "<html><body>" + result + "</body></html>";
}

static eString unmountMountPoint(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	eString result;

	std::map<eString,eString> opt = getRequestOptions(opts, '&');
	eString id = opt["id"];

	content->local_header["Content-Type"]="text/html; charset=utf-8";
	int rc = eMountMgr::getInstance()->unmountMountPoint(atoi(id.c_str()));

	if (rc > 0)
		result = "<html><body>Mount point unmounted successfully.</body></html>";
	else
		result = "<html><body>Mount point unmount failed.</body></html>";
}

void ezapMountInitializeDyn(eHTTPDynPathResolver *dyn_resolver, bool lockWeb)
{
	dyn_resolver->addDyn("GET", "/control/addMountPoint", addMountPoint, lockWeb);
	dyn_resolver->addDyn("GET", "/control/removeMountPoint", removeMountPoint, lockWeb);
	dyn_resolver->addDyn("GET", "/control/addMountPointWindow", addMountPointWindow, lockWeb);
	dyn_resolver->addDyn("GET", "/control/changeMountPoint", changeMountPoint, lockWeb);
	dyn_resolver->addDyn("GET", "/control/editMountPointWindow", editMountPointWindow, lockWeb);
	dyn_resolver->addDyn("GET", "/control/mountMountPoint", mountMountPoint, lockWeb);
	dyn_resolver->addDyn("GET", "/control/unmountMountPoint", unmountMountPoint, lockWeb);
}

