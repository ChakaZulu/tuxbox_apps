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

#include "global.h"

#include "flashtool.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>


#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mount.h>

#include <linux/mtd/mtd.h>


CFlashTool::CFlashTool()
{
	statusViewer = NULL;
	mtdDevice = "";
	ErrorMessage = "";
}

string CFlashTool::getErrorMessage()
{
	return ErrorMessage;
}

void CFlashTool::setMTDDevice( string mtddevice )
{
	mtdDevice = mtddevice;
}

void CFlashTool::setStatusViewer( CProgress_StatusViewer* statusview )
{
	statusViewer = statusview;
}

bool CFlashTool::readFromMTD( string filename )
{
	int		fd1, fd2;
	long	filesize;

	if(statusViewer)
	{
		statusViewer->showLocalStatus(0);
		statusViewer->showStatusMessage("");
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

	if( (fd2 = open( filename.c_str(), O_WRONLY )) < 0 )
	{
		ErrorMessage = g_Locale->getText("flashupdate.cantopenfile");
		close(fd1);
		return false;
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

bool CFlashTool::program( string filename )
{
	int		fd1, fd2;
	long	filesize;

	if(statusViewer)
	{
		statusViewer->showLocalStatus(0);
		statusViewer->showStatusMessage("");
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

	printf("filesize: %ld\n", filesize);


	if(statusViewer)
	{
		statusViewer->showLocalStatus(0);
		statusViewer->showStatusMessage(g_Locale->getText("flashupdate.eraseingflash"));
	}

	if(!erase())
	{
		return false;
	}

	if(statusViewer)
	{
		statusViewer->showGlobalStatus(75);
		statusViewer->showLocalStatus(0);
		statusViewer->showStatusMessage(g_Locale->getText("flashupdate.programmingflash"));
	}

	if( (fd2 = open( mtdDevice.c_str(), O_WRONLY )) < 0 )
	{
		ErrorMessage = g_Locale->getText("flashupdate.cantopenmtd");
		close(fd1);
		return false;
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

bool CFlashTool::erase()
{
	int		fd;
	mtd_info_t	meminfo;
	erase_info_t	erase;

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
			statusViewer->showLocalStatus( char(erase.start*100./meminfo.size));
		}

		if(ioctl( fd, MEMERASE, &erase) != 0)
		{
			ErrorMessage = "erase error";
			close(fd);
			return false;
		}
	}

	close(fd);
	return true;
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
				string tmpstr = buf;
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

string CMTDInfo::getMTDName( int pos )
{
	return mtdData[pos]->name;
}

string CMTDInfo::getMTDFileName( int pos )
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

int CMTDInfo::findMTDNumber( string filename )
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

string CMTDInfo::getMTDName( string filename )
{
	return getMTDName( findMTDNumber(filename) );
}

string CMTDInfo::getMTDFileName( string filename )
{
	return getMTDFileName( findMTDNumber(filename) );
}

int CMTDInfo::getMTDSize( string filename )
{
	return getMTDSize( findMTDNumber(filename) );
}

int CMTDInfo::getMTDEraseSize( string filename )
{
	return getMTDEraseSize( findMTDNumber(filename) );
}

