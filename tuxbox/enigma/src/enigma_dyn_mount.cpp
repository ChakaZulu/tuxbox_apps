#ifdef ENABLE_DYN_MOUNT
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

static eString addChangeMountPoint(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	t_mount mp;

	std::map<eString,eString> opt = getRequestOptions(opts, '&');
	eString action = opt["action"];
	eString id = opt["id"];

	mp.localDir = opt["localdir"];
	mp.fstype = atoi(opt["fstype"].c_str());
	mp.password = opt["password"];
	mp.userName = opt["username"];
	mp.mountDir = opt["mountdir"];
	mp.automount = (opt["automount"] == "on") ? 1 : 0;
	mp.rsize = atoi(opt["rsize"].c_str());
	mp.wsize = atoi(opt["wsize"].c_str());
	mp.ownOptions = opt["ownoptions"];
	mp.ip[0] = atoi(opt["ip0"].c_str());
	mp.ip[1] = atoi(opt["ip1"].c_str());
	mp.ip[2] = atoi(opt["ip2"].c_str());
	mp.ip[3] = atoi(opt["ip3"].c_str());
	mp.mounted = false;
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

	if (async == "on")
		mp.options += "async,";
	if (sync == "on")
		mp.options += "sync,";
	if (atime == "on")
		mp.options += "atime,";
	if (autom == "on")
		mp.options += "autom,";
	if (execm == "on")
		mp.options += "execm,";
	if (noexec == "on")
		mp.options += "noexec,";
	if (ro == "on")
		mp.options += "ro,";
	if (rw == "on")
		mp.options += "rw,";
	if (users == "on")
		mp.options += "users,";
	if (nolock == "on")
		mp.options += "nolock,";
	if (intr == "on")
		mp.options += "intr,";
	if (soft == "on")
		mp.options += "soft,";
	if (udp == "on")
		mp.options += "udp,";
	if (mp.options.length() > 0)
		mp.options = mp.options.substr(0, mp.options.length() - 1); //remove last comma

	eString result;
	if (action == "change")
	{
		eMountMgr::getInstance()->changeMountPoint(atoi(id.c_str()), mp);
		result = "<html><body onUnload=\"parent.window.opener.location.reload(true)\">Mount point changed successfully.";
	}
	else
	{
		eMountMgr::getInstance()->addMountPoint(mp);
		result = "<html><body onUnload=\"parent.window.opener.location.reload(true)\">Mount point added successfully.";
	}

	content->local_header["Content-Type"]="text/html; charset=utf-8";
	return result;
}

static eString removeMountPoint(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::map<eString,eString> opt = getRequestOptions(opts, '&');
	eString id = opt["id"];

	content->local_header["Content-Type"]="text/html; charset=utf-8";
	eMountMgr::getInstance()->removeMountPoint(atoi(id.c_str()));

	return "<html><body onUnload=\"parent.window.opener.location.reload(true)\">Mount point deleted successfully.</body></html>";
}

static eString mountPointWindow(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	eString async, sync, atime, autom, execm, noexec, ro, rw, users, nolock, intr, soft, udp;
	t_mount mp;
	eString cmd;

	std::map<eString,eString> opt = getRequestOptions(opts, '&');
	eString action = opt["action"];
	eString id = opt["id"];

	eString result = readFile(TEMPLATE_DIR + "mountPointWindow.tmp");
	if (action == "change")
	{
		result.strReplace("#TITLE#", "Change Mount Point");
		mp = eMountMgr::getInstance()->getMountPointData(atoi(id.c_str()));
		cmd = "change";
	}
	else
	{
		result.strReplace("#TITLE#", "Add Mount Point");
		mp.rsize = 4096;
		mp.wsize = 4096;
		mp.options = "nolock,intr,soft,udp";
		mp.ip[0] = 0;
		mp.ip[1] = 0;
		mp.ip[2] = 0;
		mp.ip[3] = 0;
		mp.fstype = 0;
		mp.automount = 0;
		cmd = "add";
	}

	result.strReplace("#ACTION#", "/control/addChangeMountPoint");
	result.strReplace("#CMD#", cmd);

	unsigned int pos = 0;
	eString option;
	while (mp.options.length() > 0)
	{
		if ((pos = mp.options.find(",")) != eString::npos)
		{
			option = mp.options.substr(0, pos);
			mp.options = mp.options.substr(pos + 1);
		}
		else
		{
			option = mp.options;
			mp.options = "";
		}

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
			udp = "checked";
	}

	result.strReplace("#LDIR#", mp.localDir);
	result.strReplace("#FSTYPE#", eString().sprintf("%d", mp.fstype));
	result.strReplace("#PW#", mp.password);
	result.strReplace("#USER#", mp.userName);
	result.strReplace("#MDIR#", mp.mountDir);
	result.strReplace("#AUTO#", (mp.automount == 1) ? "checked" : "");
	result.strReplace("#RSIZE#", eString().sprintf("%d", mp.rsize));
	result.strReplace("#WSIZE#", eString().sprintf("%d", mp.wsize));
	result.strReplace("#OWNOPTIONS#", mp.ownOptions);
	result.strReplace("#IP0#", eString().sprintf("%d", mp.ip[0]));
	result.strReplace("#IP1#", eString().sprintf("%d", mp.ip[1]));
	result.strReplace("#IP2#", eString().sprintf("%d", mp.ip[2]));
	result.strReplace("#IP3#", eString().sprintf("%d", mp.ip[3]));
	result.strReplace("#ID#", eString().sprintf("%d", mp.id));
	result.strReplace("#ASYNC#", async);
	result.strReplace("#SYNC#", sync);
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

	return "<html><body onUnload=\"parent.window.opener.location.reload(true)\">" + result + "</body></html>";
}

static eString unmountMountPoint(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	eString result;

	std::map<eString,eString> opt = getRequestOptions(opts, '&');
	eString id = opt["id"];

	content->local_header["Content-Type"]="text/html; charset=utf-8";
	int rc = eMountMgr::getInstance()->unmountMountPoint(atoi(id.c_str()));

	if (rc == 0)
		result = "Mount point unmounted successfully.";
	else
		result = "Mount point unmount failed.";

	return "<html><body onUnload=\"parent.window.opener.location.reload(true)\">" + result + "</body></html>";
}

void ezapMountInitializeDyn(eHTTPDynPathResolver *dyn_resolver, bool lockWeb)
{
	dyn_resolver->addDyn("GET", "/control/addChangeMountPoint", addChangeMountPoint, lockWeb);
	dyn_resolver->addDyn("GET", "/control/removeMountPoint", removeMountPoint, lockWeb);
	dyn_resolver->addDyn("GET", "/control/mountPointWindow", mountPointWindow, lockWeb);
	dyn_resolver->addDyn("GET", "/control/mountMountPoint", mountMountPoint, lockWeb);
	dyn_resolver->addDyn("GET", "/control/unmountMountPoint", unmountMountPoint, lockWeb);
}

#endif
