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
	eString localDir = opt["localdir"];
	eString fstype = opt["fstype"];
	eString password = opt["password"];
	eString userName = opt["username"];
	eString mountDir = opt["mountdir"];
	eString automount = opt["automount"];
	eString rsize = opt["rsize"];
	eString wsize = opt["wsize"];
	eString ownOptions = opt["ownoptions"];
	eString ip0 = opt["ip0"];
	eString ip1 = opt["ip1"];
	eString ip2 = opt["ip2"];
	eString ip3 = opt["ip3"];
	eString async = opt["async"];
	eString sync = opt["sync"];
	eString atime = opt["atime"];
	eString autom = opt["autom"];
	eString execm = opt["execm"];
	eString noexec = opt["noexec"];
	eString ro = opt["ro"];
	eString rw = opt["rw"];
	eString users = opt["users"];
	eString nolock = opt["nolock"];
	eString intr = opt["intr"];
	eString soft = opt["soft"];
	eString udp = opt["udp"];

	eString options;
	if (async == "on")
		options += "async,";
	if (sync == "on")
		options += "sync,";
	if (atime == "on")
		options += "atime,";
	if (autom == "on")
		options += "autom,";
	if (execm == "on")
		options += "execm,";
	if (noexec == "on")
		options += "noexec,";
	if (ro == "on")
		options += "ro,";
	if (rw == "on")
		options += "rw,";
	if (users == "on")
		options += "users,";
	if (nolock == "on")
		options += "nolock,";
	if (intr == "on")
		options += "intr,";
	if (soft == "on")
		options += "soft,";
	if (udp == "on")
		options += "udp,";
	if (options.length() > 0)
		options = options.left(options.length() - 1); //remove last comma

	content->local_header["Content-Type"]="text/html; charset=utf-8";
	eMountMgr::getInstance()->addMountPoint(localDir, atoi(fstype.c_str()), password, userName, mountDir, (int)(automount == "on"), atoi(rsize.c_str()), atoi(wsize.c_str()), options, ownOptions, atoi(ip0.c_str()), atoi(ip1.c_str()), atoi(ip2.c_str()), atoi(ip3.c_str()), false);
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
	eString localDir = opt["localdir"];
	eString fstype = opt["fstype"];
	eString password = opt["password"];
	eString userName = opt["username"];
	eString mountDir = opt["mountdir"];
	eString automount = opt["automount"];
	eString rsize = opt["rsize"];
	eString wsize = opt["wsize"];
	eString ownOptions = opt["ownoptions"];
	eString ip0 = opt["ip0"];
	eString ip1 = opt["ip1"];
	eString ip2 = opt["ip2"];
	eString ip3 = opt["ip3"];
	eString async = opt["async"];
	eString sync = opt["sync"];
	eString atime = opt["atime"];
	eString autom = opt["autom"];
	eString execm = opt["execm"];
	eString noexec = opt["noexec"];
	eString ro = opt["ro"];
	eString rw = opt["rw"];
	eString users = opt["users"];
	eString nolock = opt["nolock"];
	eString intr = opt["intr"];
	eString soft = opt["soft"];
	eString udp = opt["udp"];
	eString id = opt["id"];

	eString options;
	if (async == "on")
		options += "async,";
	if (sync == "on")
		options += "sync,";
	if (atime == "on")
		options += "atime,";
	if (autom == "on")
		options += "autom,";
	if (execm == "on")
		options += "execm,";
	if (noexec == "on")
		options += "noexec,";
	if (ro == "on")
		options += "ro,";
	if (rw == "on")
		options += "rw,";
	if (users == "on")
		options += "users,";
	if (nolock == "on")
		options += "nolock,";
	if (intr == "on")
		options += "intr,";
	if (soft == "on")
		options += "soft,";
	if (udp == "on")
		options += "udp,";
	if (options.length() > 0)
		options = options.left(options.length() - 1); //remove last comma

	content->local_header["Content-Type"]="text/html; charset=utf-8";
	eMountMgr::getInstance()->changeMountPoint(localDir, atoi(fstype.c_str()), password, userName, mountDir, (int)(automount == "on"), atoi(rsize.c_str()), atoi(wsize.c_str()), options, ownOptions, atoi(ip0.c_str()), atoi(ip1.c_str()), atoi(ip2.c_str()), atoi(ip3.c_str()), atoi(id.c_str()));
	return "<html><body>Mount point changed successfully.</body></html>";
}

static eString addMountPointWindow(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	eString localDir, password, userName, mountDir, options, ownOptions;
	int fstype = 0, automount = 0, rsize = 4096, wsize = 4096, ip0 = 0, ip1 = 0, ip2 = 0, ip3 = 0;
	eString async = "", sync = "", atime = "", autom = "", execm = "", noexec = "", ro = "", rw = "",
		users = "", nolock = "checked", intr = "checked", soft = "checked", udp = "checked";
	int id = 99;

	eString result = readFile(TEMPLATE_DIR + "mountPointWindow.tmp");
	result.strReplace("#TITLE#", "Add Mount Point");
	result.strReplace("#ACTION#", "/control/addMountPoint");

//	std::map<eString,eString> opt = getRequestOptions(opts, '&');
	content->local_header["Content-Type"]="text/html; charset=utf-8";

	result.strReplace("#LDIR#", localDir);
	result.strReplace("#FSTYPE#", eString().sprintf("%d", fstype));
	result.strReplace("#PW#", password);
	result.strReplace("#USER#", userName);
	result.strReplace("#MDIR#", mountDir);
	if (automount == 1)
		result.strReplace("#AUTO#", "checked");
	else
		result.strReplace("#AUTO#", "");
	result.strReplace("#RSIZE#", eString().sprintf("%d", rsize));
	result.strReplace("#WSIZE#", eString().sprintf("%d", wsize));
	result.strReplace("#OWNOPTIONS#", ownOptions);
	result.strReplace("#IP0#", eString().sprintf("%d", ip0));
	result.strReplace("#IP1#", eString().sprintf("%d", ip1));
	result.strReplace("#IP2#", eString().sprintf("%d", ip2));
	result.strReplace("#IP3#", eString().sprintf("%d", ip3));
	result.strReplace("#ID#", eString().sprintf("%d", id));
	result.strReplace("#ASYNC#", async);
	result.strReplace("#SYNC", sync);
	result.strReplace("#ATIME#", atime);
	result.strReplace("#AUTOM#", autom);
	result.strReplace("#EXECM#", execm);
	result.strReplace("#NOEXEC#", noexec);
	result.strReplace("#RO#", ro);
	result.strReplace("#RW#", rw);
	result.strReplace("#USERS#", users);
	result.strReplace("#NOLOCK#", nolock);
	result.strReplace("#INTR#", intr);
	result.strReplace("#SOFT#", soft);
	result.strReplace("#UDP#", udp);

	return result;
}

static eString editMountPointWindow(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	eString localDir, password, userName, mountDir, options, ownOptions;
	int fstype, automount, rsize, wsize, ip0, ip1, ip2, ip3;
	eString async = "", sync = "", atime = "", autom = "", execm = "", noexec = "", ro = "", rw = "",
		users = "", nolock = "", intr = "", soft = "", udp = "";
	
	eString result = readFile(TEMPLATE_DIR + "mountPointWindow.tmp");
	result.strReplace("#TITLE#", "Change Mount Point");
	result.strReplace("#ACTION#", "/control/changeMountPoint");

	std::map<eString,eString> opt = getRequestOptions(opts, '&');
	eString id = opt["id"];

	eMountMgr::getInstance()->getMountPointData(&localDir, &fstype, &password, &userName, &mountDir, &automount, &rsize, &wsize, &options, &ownOptions, &ip0, &ip1, &ip2, &ip3, atoi(id.c_str()));
	
	while (options.length() > 0)
	{
		eString option = getLeft(options, ',');
		if (option == "async")
			async = "checked";
		else
		if (option == "sync")
			sync = "checked";
		else
		if (option == "atime")
			atime = "checked";
		else
		if (option == "autom")
			autom = "checked";
		else
		if (option == "execm")
			execm = "checked";
		else
		if (option == "noexec")
			noexec = "checked";
		else
		if (option == "ro")
			ro = "checked";
		else
		if (option == "rw")
			rw = "checked";
		else
		if (option == "users")
			users = "checked";
		else
		if (option == "nolock")
			nolock = "checked";
		else
		if (option == "intr")
			intr = "checked";
		else
		if (option == "soft")
			soft = "checked";
		else
		if (option == "udp")
			udp == "checked";
	}
	
	result.strReplace("#LDIR#", localDir);
	result.strReplace("#FSTYPE#", eString().sprintf("%d", fstype));
	result.strReplace("#PW#", password);
	result.strReplace("#USER#", userName);
	result.strReplace("#MDIR#", mountDir);
	if (automount == 1)
		result.strReplace("#AUTO#", "checked");
	else
		result.strReplace("#AUTO#", "");
	result.strReplace("#RSIZE#", eString().sprintf("%d", rsize));
	result.strReplace("#WSIZE#", eString().sprintf("%d", wsize));
	result.strReplace("#OWNOPTIONS#", ownOptions);
	result.strReplace("#IP0#", eString().sprintf("%d", ip0));
	result.strReplace("#IP1#", eString().sprintf("%d", ip1));
	result.strReplace("#IP2#", eString().sprintf("%d", ip2));
	result.strReplace("#IP3#", eString().sprintf("%d", ip3));
	result.strReplace("#ID#", id);
	result.strReplace("#ASYNC#", async);
	result.strReplace("#SYNC", sync);
	result.strReplace("#ATIME#", atime);
	result.strReplace("#AUTOM#", autom);
	result.strReplace("#EXECM#", execm);
	result.strReplace("#NOEXEC#", noexec);
	result.strReplace("#RO#", ro);
	result.strReplace("#RW#", rw);
	result.strReplace("#USERS#", users);
	result.strReplace("#NOLOCK#", nolock);
	result.strReplace("#INTR#", intr);
	result.strReplace("#SOFT#", soft);
	result.strReplace("#UDP#", udp);

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

