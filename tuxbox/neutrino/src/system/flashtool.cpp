/*
	Neutrino-GUI  -   DBoxII-Project

	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA
*/


#include "flashtool.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mount.h>

#include <linux/mtd/mtd.h>
#include <libcramfs.h>

#include <dbox/fp.h>

#include <global.h>


CFlashTool::CFlashTool()
{
	fd_fp = -1;
	statusViewer = NULL;
	mtdDevice = 	"";
	ErrorMessage = 	"";
}

CFlashTool::~CFlashTool()
{
	if(fd_fp!=-1)
	{
		close(fd_fp);
	}
}

std::string CFlashTool::getErrorMessage()
{
	return ErrorMessage;
}

void CFlashTool::setMTDDevice( std::string mtddevice )
{
	mtdDevice = mtddevice;
}

void CFlashTool::setStatusViewer( CProgress_StatusViewer* statusview )
{
	statusViewer = statusview;
}

bool CFlashTool::readFromMTD( std::string filename, int globalProgressEnd )
{
	int		fd1, fd2;
	long	filesize;
	int		globalProgressBegin = 0;

	if(statusViewer)
	{
		statusViewer->showLocalStatus(0);
	}

	if(mtdDevice=="")
	{
		ErrorMessage = "mtd-device not set";
		return false;
	}

	if(filename=="")
	{
		ErrorMessage = "filename not set";
		return false;
	}

	if( (fd1 = open( mtdDevice.c_str(), O_RDONLY )) < 0 )
	{
		ErrorMessage = g_Locale->getText("flashupdate.cantopenmtd");
		return false;
	}

	if( (fd2 = open( filename.c_str(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR  |  S_IRGRP | S_IWGRP  |  S_IROTH | S_IWOTH)) < 0 )
	{
		ErrorMessage = g_Locale->getText("flashupdate.cantopenfile");
		close(fd1);
		return false;
	}

	if(statusViewer)
	{
		globalProgressBegin = statusViewer->getGlobalStatus();
	}
	filesize = CMTDInfo::getInstance()->getMTDSize(mtdDevice);

	char buf[1024];
	long fsize = filesize;
	while(fsize>0)
	{
		long block = fsize;
		if(block>(long)sizeof(buf))
		{
			block = sizeof(buf);
		}
		read( fd1, &buf, block);
		write( fd2, &buf, block);
		fsize -= block;
		char prog = char(100-(100./filesize*fsize));
		if(statusViewer)
		{
			statusViewer->showLocalStatus(prog);
			if(globalProgressEnd!=-1)
			{
				int globalProg = globalProgressBegin + int((globalProgressEnd-globalProgressBegin) * prog/100. );
				statusViewer->showGlobalStatus(globalProg);
			}
		}
	}

	if(statusViewer)
	{
		statusViewer->showLocalStatus(100);
	}

	close(fd1);
	close(fd2);
	return true;
}

bool CFlashTool::program( std::string filename, int globalProgressEndErase, int globalProgressEndFlash )
{
	int		fd1, fd2;
	long	filesize;
	int		globalProgressBegin = 0;

	if(statusViewer)
	{
		statusViewer->showLocalStatus(0);
	}

	if(mtdDevice=="")
	{
		ErrorMessage = "mtd-device not set";
		return false;
	}

	if(filename=="")
	{
		ErrorMessage = "filename not set";
		return false;
	}

	if( (fd1 = open( filename.c_str(), O_RDONLY )) < 0 )
	{
		ErrorMessage = g_Locale->getText("flashupdate.cantopenfile");
		return false;
	}

	filesize = lseek( fd1, 0, SEEK_END);
	lseek( fd1, 0, SEEK_SET);

	if(filesize==0)
	{
		ErrorMessage = g_Locale->getText("flashupdate.fileis0bytes");
		return false;
	}

	if(statusViewer)
	{
		statusViewer->showLocalStatus(0);
		statusViewer->showStatusMessage(g_Locale->getText("flashupdate.eraseingflash"), true); // UTF-8
	}

	//jetzt wirds kritisch - daher filehandle auf fp öffen um reset machen zu können
	if ((fd_fp = open("/dev/dbox/fp0",O_RDWR)) <= 0)
	{
		perror("[neutrino] open fp0");
		fd_fp = -1;
	}

	if(!erase(globalProgressEndErase))
	{
		return false;
	}

	if(statusViewer)
	{
		if(globalProgressEndErase!=-1)
		{
			statusViewer->showGlobalStatus(globalProgressEndErase);
		}
		statusViewer->showLocalStatus(0);
		statusViewer->showStatusMessage(g_Locale->getText("flashupdate.programmingflash"), true); // UTF-8
	}

	if( (fd2 = open( mtdDevice.c_str(), O_WRONLY )) < 0 )
	{
		ErrorMessage = g_Locale->getText("flashupdate.cantopenmtd");
		close(fd1);
		return false;
	}

	if(statusViewer)
	{
		globalProgressBegin = statusViewer->getGlobalStatus();
	}

	char buf[1024];
	long fsize = filesize;
	while(fsize>0)
	{
		long block = fsize;
		if(block>(long)sizeof(buf))
		{
			block = sizeof(buf);
		}
		read( fd1, &buf, block);
		write( fd2, &buf, block);
		fsize -= block;
		char prog = char(100-(100./filesize*fsize));
		if(statusViewer)
		{
			statusViewer->showLocalStatus(prog);
			if(globalProgressEndFlash!=-1)
			{
				int globalProg = globalProgressBegin + int((globalProgressEndFlash-globalProgressBegin) * prog/100. );
				statusViewer->showGlobalStatus(globalProg);
			}
		}
	}

	if(statusViewer)
	{
		statusViewer->showLocalStatus(100);
	}

	close(fd1);
	close(fd2);
	return true;
}

bool CFlashTool::erase(int globalProgressEnd)
{
	int				fd;
	mtd_info_t		meminfo;
	erase_info_t	erase;
	int				globalProgressBegin = 0;

	if( (fd = open( mtdDevice.c_str(), O_RDWR )) < 0 )
	{
		ErrorMessage = g_Locale->getText("flashupdate.cantopenmtd");
		return false;
	}

	if( ioctl( fd, MEMGETINFO, &meminfo ) != 0 )
	{
		ErrorMessage = "can't get mtd-info";
		return false;
	}

	if(statusViewer)
	{
		globalProgressBegin = statusViewer->getGlobalStatus();
	}

	erase.length = meminfo.erasesize;
	for (erase.start = 0; erase.start < meminfo.size;erase.start += meminfo.erasesize)
	{
		/*
		printf( "\rErasing %u Kibyte @ %x -- %2u %% complete.",
		                 meminfo.erasesize/1024, erase.start,
		                 erase.start*100/meminfo.size );
		*/
		if(statusViewer)
		{
			int prog = int(erase.start*100./meminfo.size);
			statusViewer->showLocalStatus(prog);
			if(globalProgressEnd!=-1)
			{
				int globalProg = globalProgressBegin + int((globalProgressEnd-globalProgressBegin) * prog/100. );
				statusViewer->showGlobalStatus(globalProg);
			}
		}

		if(ioctl( fd, MEMERASE, &erase) != 0)
		{
			ErrorMessage = g_Locale->getText("flashupdate.erasefailed");
			close(fd);
			return false;
		}
	}

	close(fd);
	return true;
}

bool CFlashTool::check_cramfs( std::string filename )
{
	int retVal = cramfs_crc( (char*) filename.c_str() );
	printf("flashcheck returned: %d\n", retVal);
	return retVal==1; 
}

void CFlashTool::reboot()
{
	if(fd_fp!=-1)
	{
		/* Nokia FP_IOCTL_REBOOT does not work after writing directly to flash */
		if(g_info.box_Type == CControldClient::TUXBOX_MAKER_NOKIA)
		{
			if (ioctl(fd_fp,FP_IOCTL_POWEROFF)< 0)
			{
				perror("FP_IOCTL_POWEROFF:");
			}
		}
		if (ioctl(fd_fp,FP_IOCTL_REBOOT)< 0)
		{
			perror("FP_IOCTL_REBOOT:");
		}
		close(fd_fp);
		fd_fp = -1;
	}
}

//-----------------------------------------------------------------------------------------------------------------


CFlashVersionInfo::CFlashVersionInfo(const std::string versionString)
{
	//SBBBYYYYMMTTHHMM -- formatsting

	// recover type
	snapshot = versionString[0];

	// recover baseimage version
	baseImageVersion[0] = versionString[1];
	baseImageVersion[1] = '.';
	if (versionString[2] == '0')
	{
	    baseImageVersion[2] = versionString[3];
	    baseImageVersion[3] = 0;
	}
	else
	{
	    baseImageVersion[2] = versionString[2];
	    baseImageVersion[3] = versionString[3];
	    baseImageVersion[4] = 0;
	}

	// recover date
	date[0] = versionString[10];
	date[1] = versionString[11];
	date[2] = '.';
	date[3] = versionString[8];
	date[4] = versionString[9];
	date[5] = '.';
	date[6] = versionString[4];
	date[7] = versionString[5];
	date[8] = versionString[6];
	date[9] = versionString[7];
	date[10] = 0;

	// recover time stamp
	time[0] = versionString[12];
	time[1] = versionString[13];
	time[2] = ':';
	time[3] = versionString[14];
	time[4] = versionString[15];
	time[5] = 0;
}

const char * const CFlashVersionInfo::getDate() const
{
	return date;
}

const char * const CFlashVersionInfo::getTime() const
{
	return time;
}

const char * const CFlashVersionInfo::getBaseImageVersion() const
{
	return baseImageVersion;
}

const char * const CFlashVersionInfo::getType() const
{
	switch (snapshot)
	{
	case '0':
		return "Release";
	case '1':
		return "Snapshot";
	case '2':
		return "Internal";
	default:
		return "Unknown";
	}
}


//-----------------------------------------------------------------------------------------------------------------

CMTDInfo::CMTDInfo()
{
	getPartitionInfo();
}

CMTDInfo::~CMTDInfo()
{
	for(int x=0;x<getMTDCount();x++)
	{
		delete mtdData[x];
	}
	mtdData.clear();
}


CMTDInfo* CMTDInfo::getInstance()
{
	static CMTDInfo* MTDInfo = NULL;

	if(!MTDInfo)
	{
		MTDInfo = new CMTDInfo();
	}
	return MTDInfo;
}

void CMTDInfo::getPartitionInfo()
{
	FILE* fd = fopen("/proc/mtd", "r");
	if(!fd)
	{
		perror("cannot read /proc/mtd");
		return;
	}
	char buf[1000];
	fgets(buf,sizeof(buf),fd);
	while(!feof(fd))
	{
		if(fgets(buf,sizeof(buf),fd)!=NULL)
		{
			char mtdname[50]="";
			int mtdnr=0;
			int mtdsize=0;
			int mtderasesize=0;
			sscanf(buf, "mtd%d: %x %x \"%s\"\n", &mtdnr, &mtdsize, &mtderasesize, mtdname);
			SMTDPartition* tmp = new SMTDPartition;
				tmp->size = mtdsize;
				tmp->erasesize = mtderasesize;
				std::string tmpstr = buf;
				tmp->name = tmpstr.substr( tmpstr.find("\"")+1, tmpstr.rfind("\"")-tmpstr.find("\"")-1);
				sprintf((char*) &buf, "/dev/mtd/%d", mtdnr);
				tmp->filename = buf;
			mtdData.insert( mtdData.end(), tmp);
		}
	}
	fclose(fd);
}

int CMTDInfo::getMTDCount()
{
	return mtdData.size();
}

std::string CMTDInfo::getMTDName( int pos )
{
	return mtdData[pos]->name;
}

std::string CMTDInfo::getMTDFileName( int pos )
{
	return mtdData[pos]->filename;
}

int CMTDInfo::getMTDSize( int pos )
{
	return mtdData[pos]->size;
}

int CMTDInfo::getMTDEraseSize( int pos )
{
	return mtdData[pos]->erasesize;
}

int CMTDInfo::findMTDNumber( std::string filename )
{
	for(int x=0;x<getMTDCount();x++)
	{
		if(filename == getMTDFileName(x))
		{
			return x;
		}
	}
	return -1;
}

std::string CMTDInfo::getMTDName( std::string filename )
{
	return getMTDName( findMTDNumber(filename) );
}

std::string CMTDInfo::getMTDFileName( std::string filename )
{
	return getMTDFileName( findMTDNumber(filename) );
}

int CMTDInfo::getMTDSize( std::string filename )
{
	return getMTDSize( findMTDNumber(filename) );
}

int CMTDInfo::getMTDEraseSize( std::string filename )
{
	return getMTDEraseSize( findMTDNumber(filename) );
}
