#ifdef ENABLE_DYN_FLASH

#include <map>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/reboot.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <iomanip>
#include <iostream>
#include <fstream>

#include <enigma.h>
#include <enigma_main.h>
#include <enigma_standby.h>
#include <timer.h>

#include <lib/system/info.h>
#include <lib/system/http_dyn.h>
#include <lib/system/econfig.h>
#include <enigma_dyn.h>
#include <enigma_dyn_utils.h>
#include <enigma_dyn_conf.h>
#include <enigma_dyn_flash.h>
#include <configfile.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,7)
#include <linux/compiler.h>
#include <mtd/mtd-user.h>
#else
#include <linux/mtd/mtd.h>
#endif

eFlashMgr flashMgr;

using namespace std;

eFlashOperationsHandler *eFlashOperationsHandler::instance;

eFlashOperationsHandler::eFlashOperationsHandler():messages(this,1)
{
	if (!instance)
		instance = this;
	CONNECT(messages.recv_msg, eFlashOperationsHandler::gotMessage);
	run();
}

eFlashOperationsHandler::~eFlashOperationsHandler()
{
	messages.send(Message::quit);
	if (thread_running())
		kill();
	if (instance == this)
		instance = 0;
}

void eFlashOperationsHandler::thread()
{
	nice(5);
	exec();
}

void eFlashOperationsHandler::readPartition(const char *mtd, const char *filename)
{
	messages.send(Message(Message::read, mtd?strdup(mtd):0, filename?strdup(filename):0));
}

void eFlashOperationsHandler::writePartition(const char *mtd, const char *filename)
{
	messages.send(Message(Message::write, mtd?strdup(mtd):0, filename?strdup(filename):0));
}

void eFlashOperationsHandler::gotMessage(const Message &msg )
{
	switch (msg.type)
	{
		case Message::read:
			readFlash(eString(msg.mtd), eString(msg.filename));
			break;
		case Message::write:
			writeFlash(eString(msg.mtd), eString(msg.filename));
			break;
		case Message::quit:
			quit(0);
			break;
		default:
			eDebug("unhandled thread message");
	}
}

int eFlashOperationsHandler::writeFlash(eString mtd, eString filename)
{
	int fd1, fd2;
	
	progressMessage1 = progressMessage2 = "";
	progressComplete = 0;
	
	int mtdno = -1;
	sscanf(mtd.c_str(), "mtd%d", &mtdno);
	eString mtddev = "/dev/mtd/" + eString().sprintf("%d", mtdno);

	if (access(filename.c_str(), R_OK) != 0)
		return -1;

	if ((fd1 = open(filename.c_str(), O_RDONLY)) < 0)
		return -2;

	unsigned long filesize = lseek(fd1, 0, SEEK_END);
	lseek(fd1, 0, SEEK_SET);

	if (filesize == 0)
	{
		close(fd1);
		return -3;
	}

	if ((fd2 = open(mtddev.c_str(), O_WRONLY)) < 0)
	{
		close(fd1);
		return -4;
	}

	mtd_info_t meminfo;
	if (ioctl(fd2, MEMGETINFO, &meminfo) != 0)
	{
		close(fd1);
		close(fd2);
		return -5;
	}

	if (filesize > meminfo.size)
	{
		close(fd1);
		close(fd2);
		return -6;
	}

	if (filesize < ((meminfo.size / 100) * 70)) // if image size is less than 70% of memory size something probably is not right...
	{
		close(fd1);
		close(fd2);
		return -7;
	}

	progressMessage1 = "Flashing in process, please do NOT switch receiver off!";

	sync();
	Decoder::Flush();

	// without this nice we have not enough priority for file operations... then the update ist very slow...
	nice(-10);

	progressMessage2 = "Erasing flash partition " + mtddev + "...";
	
	erase_info_t erase;
	erase.length = meminfo.erasesize;
	for (erase.start = 0; erase.start < meminfo.size; erase.start += meminfo.erasesize)
	{
		progressComplete = erase.start * 100 / meminfo.size;
#if 0
		if (ioctl(fd2, MEMERASE, &erase) != 0)
		{
			close(fd1);
			close(fd2);
			return -8;
		}
#else
		sleep(1);
#endif
	}

	progressMessage2 = "Writing " + filename + " to flash partition " + mtddev + "...";

	char buf[meminfo.erasesize];
	long fsize = filesize;
	while (fsize > 0)
	{
		long block = fsize;
		if (block > (long)sizeof(buf))
			block = sizeof(buf);
		read(fd1, buf, block);
#if 0
		write(fd2, buf, block);
#else
		usleep(5000);
#endif
		fsize -= block;
		progressComplete = ((filesize - fsize) * 100) / filesize;
	}

	close(fd1);
	close(fd2);
	
	progressMessage1 = "Writing image to flash completed successfully.";
	progressMessage2 = "Rebooting...";
	
	sleep(5); // wait 5 seconds, then reboot...
	
	::reboot(RB_AUTOBOOT);
	system("reboot");
	
	return 0; // we made it ;-) but we probably never will get here :-)
}

int eFlashOperationsHandler::readFlash(eString mtd, eString filename)
{
	int fd1, fd2;
	long filesize;
	mtd_info_t meminfo;
	
	progressMessage1 = progressMessage2 = "";
	progressComplete = 0;
	
	int mtdno = -1;
	sscanf(mtd.c_str(), "mtd%d", &mtdno);
	eString mtddev = "/dev/mtd/" + eString().sprintf("%d", mtdno);

	if ((fd1 = open(mtddev.c_str(), O_RDONLY)) < 0)
	{
		progressMessage1 = "Error during Operation";
		progressMessage2 = "Could not open /dev/mtd.";
		return -1;
	}

	if (ioctl(fd1, MEMGETINFO, &meminfo) != 0)
	{
		progressMessage1 = "Error during Operation";
		progressMessage2 = "Could not execute ioctl MEMGETINFO.";
		return -2;
	}

	if ((fd2 = open(filename.c_str(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR  |  S_IRGRP | S_IWGRP  |  S_IROTH | S_IWOTH)) < 0)
	{
		close(fd1);
		progressMessage1 = "Error during Operation";
		progressMessage2 = "Could not open file.";
		return -3;
	}

	filesize = meminfo.size;

	progressMessage1 = "Please wait while flash partition " + mtddev + " is being saved to " + filename + "...";
	progressMessage2 = "Saving flash partition...";
	
	sleep(1);

	char buf[meminfo.erasesize];
	long fsize = filesize;
	while(fsize > 0)
	{
		long block = fsize;
		if (block > (long)sizeof(buf))
			block = sizeof(buf);
		read(fd1, &buf, block);
		write(fd2, &buf, block);
		fsize -= block;
		progressComplete = ((filesize - fsize) * 100) / filesize;
		sleep(1);
	}
	close(fd1);
	close(fd2);
	
	progressMessage2 = "Saving flash partition completed successfully.";

	return 0; // we made it ;-)
}


eFlashMgr::eFlashMgr()
{
	t_mtd mtd;
	eString dev, size, erasesize, name;
	std::stringstream tmp;
	eString t;
	
	mtds.clear();
	eString procmtd = readFile("/proc/mtd");
	tmp.str(procmtd);
	tmp >> h1 >> h2 >> h3 >> h4;
	h1.left(h1.length() - 1);
	tmp >> mtd.dev;
	mtd.dev.left(mtd.dev.length() - 1);
	while (tmp)
	{
		mtd.size = mtd.erasesize = mtd.name = "";
		tmp >> mtd.size;
		tmp >> mtd.erasesize;
		tmp >> t;
		while ((t.find("mtd") == eString::npos) && tmp)
		{
			mtd.name += t + " ";
			tmp >> t;
		}
		mtd.name.left(mtd.name.length() - 1);
		mtd.name.strReplace("\"", "");
		mtds.push_back(mtd);
		mtd.dev = t;
	}
}

eFlashMgr::~eFlashMgr()
{
}

eString eFlashMgr::htmlList(void)
{
	eString tbody;
	eString result = readFile(TEMPLATE_DIR + "flashMgr.tmp");
	result.strReplace("#H1#", h1);
	result.strReplace("#H2#", h2);
	result.strReplace("#H3#", h3);
	result.strReplace("#H4#", h4);

	for (std::list<t_mtd>::iterator mtd_it = mtds.begin(); mtd_it != mtds.end(); mtd_it++)
	{
		eString tmp = readFile(TEMPLATE_DIR + "mtd.tmp");
		tmp.strReplace("#DEV#", mtd_it->dev);
		tmp.strReplace("#SIZE#", mtd_it->size);
		tmp.strReplace("#ERASESIZE#", mtd_it->erasesize);
		tmp.strReplace("#NAME#", mtd_it->name);
		tbody += tmp;
	}
	result.strReplace("#TBODY#", tbody);

	return result;
}

eString eFlashMgr::getMTDName(eString mtd)
{
	for (std::list<t_mtd>::iterator mtd_it = mtds.begin(); mtd_it != mtds.end(); mtd_it++)
		if (mtd == mtd_it->dev)
			return mtd_it->name;
	
	return "";
}

eString writeFlashPartition(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	eString mtd = opt["mtd"];
	eString fileName = opt["file"];
	
	eFlashOperationsHandler::getInstance()->writePartition(mtd.c_str(), fileName.c_str());
	
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	eString result = readFile(TEMPLATE_DIR + "flashMgrProgress.tmp");
	return result;
}

eString readFlashPartition(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	eString mtd = opt["mtd"];
	eString fileName = opt["file"];
	
	eFlashOperationsHandler::getInstance()->readPartition(mtd.c_str(), fileName.c_str());
	
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	eString result = readFile(TEMPLATE_DIR + "flashMgrProgress.tmp");
	return result;
}

eString flashProgressData(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	eString result = readFile(TEMPLATE_DIR + "flashMgrProgressData.tmp");
	result.strReplace("#PROGRESSMESSAGE1#", eFlashOperationsHandler::getInstance()->getProgressMessage1());
	result.strReplace("#PROGRESSMESSAGE2#", eFlashOperationsHandler::getInstance()->getProgressMessage2());
	result.strReplace("#PROGRESSCOMPLETE#", eString().sprintf("%d", eFlashOperationsHandler::getInstance()->getProgressComplete()));
	return result;
}

eString showWriteMenu(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	eString mtd = opt["mtd"];
	eString fileName = opt["file"];
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	eString result = readFile(TEMPLATE_DIR + "flashMgrWriteMenu.tmp");
	result.strReplace("#MTD#", mtd);
	result.strReplace("#MTDNAME#", flashMgr.getMTDName(mtd));
	result.strReplace("#FILE#", fileName);
	
	eString files;
	DIR *d = opendir("/tmp");
	if (d)
	{
		while (struct dirent *e = readdir(d))
		{
			eString filename = eString(e->d_name);
			if (filename.right(4).upper() == ".IMG")
				files += "<option value=\"" + filename + "\">" + filename + "</option>";
		}
		closedir(d);
	}
	if (!files)
		files += "<option>no images available</option>";

	result.strReplace("#FILES#", files);
	return result;
}

eString showReadMenu(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	eString mtd = opt["mtd"];
	eString fileName = opt["file"];
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	eString result = readFile(TEMPLATE_DIR + "flashMgrReadMenu.tmp");
	result.strReplace("#MTD#", mtd);
	result.strReplace("#MTDNAME#", flashMgr.getMTDName(mtd));
	result.strReplace("#FILE#", fileName);
	return result;
}

eString getConfigFlashMgr(void)
{
	return flashMgr.htmlList();
}

void ezapFlashInitializeDyn(eHTTPDynPathResolver *dyn_resolver, bool lockWeb)
{
	dyn_resolver->addDyn("GET", "/cgi-bin/showWriteMenu", showWriteMenu, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/showReadMenu", showReadMenu, lockWeb);
	dyn_resolver->addDyn("GET", "/writeFlashPartition", writeFlashPartition, lockWeb);
	dyn_resolver->addDyn("GET", "/readFlashPartition", readFlashPartition, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/flashProgressData", flashProgressData, lockWeb);
}
#endif
