/***************************************************************************
    copyright            : (C) 2001 by TheDOC
    email                : thedoc@chatville.de
	homepage			 : www.chatville.de
	modified by			 : -
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/*
$Log: settings.cpp,v $
Revision 1.5  2002/03/03 22:56:27  TheDOC
lcars 0.20

Revision 1.6  2001/12/18 02:03:29  tux
VCR-Switch-Eventkram implementiert

Revision 1.5  2001/12/17 18:37:05  tux
Finales Settingsgedoens

Revision 1.4  2001/12/17 16:54:47  tux
Settings halb komplett

Revision 1.3  2001/12/17 01:00:41  tux
scan.cpp fix

Revision 1.3  2001/12/11 13:38:44  TheDOC
new cdk-path-variables, about 10 new features and stuff

Revision 1.2  2001/11/15 00:43:45  TheDOC
 added

*/
#include <stdio.h>
#include <dbox/info.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <ost/dmx.h>
#include <ost/video.h>
#include <ost/frontend.h>
#include <ost/audio.h>
#include <ost/sec.h>
#include <ost/sec.h>
#include <ost/ca.h>
#include <memory.h>
#include <sstream>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <net/if.h> 
#include <arpa/inet.h>


#include "settings.h"
#include "help.h"
#include "cam.h"

settings::settings(cam *c)
{
	cam_obj = c;
	FILE *fp;
	char buffer[100];
	int type = -1;
	isGTX = false;

	printf("----------------> SETTINGS <--------------------\n");
	fp = fopen("/proc/bus/dbox", "r");
	while (!feof(fp))
	{
		fgets(buffer, 100, fp);
		sscanf(buffer, "fe=%d", &type);
		sscanf(buffer, "mID=%d", &box);



		int gtx = 0;
		sscanf(buffer, "gtxID=%x\n", &gtx);
		if (gtx != 0)
		{
			if ((unsigned int)gtx != 0xffffffff)
			{
				isGTX = true;
			}
			else
			{
				isGTX = false;
			}
		}
	
	}	
	fclose(fp);

	if (box == 3)
		printf ("Sagem-Box\n");
	else if (box == 1)
		printf("Nokia-Box\n");
	else if (box == 2)
		printf("Philips-Box\n");
	else
		printf("Unknown Box\n");
	
	isCable = (type == DBOX_FE_CABLE);

	CAID = cam_obj->getCAID();
	printf("Set-CAID: %x\n", CAID);
	
	oldTS = -1;
	usediseqc = true;
	setting.timeoffset = 60;
	setting.ip = 0;
	setting.gwip = 0;
	setting.serverip = 0;
	setting.dnsip = 0;
	if (box == NOKIA)
	{
		setting.rcRepeat = true;
		setting.supportOldRc = true;
	}
	else if (box == SAGEM || box == PHILIPS)
	{
		setting.rcRepeat = false;
		setting.supportOldRc = false;
	}
	setting.output_format = 1;
	setting.video_format = 0;
	setting.switch_vcr = true;
	loadSettings();
}

int settings::getEMMpid(int TS = -1)
{
	if (EMM < 2 || oldTS != TS || TS == -1)
	{
		printf("Getting EMM\n");
		EMM = find_emmpid(CAID);
		oldTS = TS;
	}
	return EMM;
}

int settings::find_emmpid(int ca_system_id) {
	char buffer[1000];
	int fd, r = 1000, count;
	struct dmxSctFilterParams flt;

	fd=open("/dev/ost/demux0", O_RDWR);
	if (fd<0)
	{
		perror("/dev/ost/demux0");
		return -fd;
	}

	memset(&flt.filter.filter, 0, DMX_FILTER_SIZE);
	memset(&flt.filter.mask, 0, DMX_FILTER_SIZE);

	flt.pid=1;
	flt.filter.filter[0]=1;
	flt.filter.mask[0]  =0xFF;
	flt.timeout=10000;
	flt.flags=DMX_ONESHOT;

	if (ioctl(fd, DMX_SET_FILTER, &flt)<0)
	{
		perror("DMX_SET_FILTER");
		return 1;
	}

	ioctl(fd, DMX_START, 0);
	if ((r=read(fd, buffer, r))<=0)
	{
		perror("read");
		return 1;
	}

	close(fd);

	if (r<=0) return 0;

	r=((buffer[1]&0x0F)<<8)|buffer[2];

	count=8;
	while(count<r-1)
	{
    	if ((((buffer[count+2]<<8)|buffer[count+3]) == ca_system_id) && (buffer[count+2] == ((0x18|0x27)&0xD7)))
		return (((buffer[count+4]<<8)|buffer[count+5])&0x1FFF);
		count+=buffer[count+1]+2;
	}
	return 0;
}

bool settings::boxIsCable()
{
	return isCable;
}

bool settings::boxIsSat()
{
	return !isCable;
}

int settings::getCAID()
{
	return CAID;
}

int settings::getTransparentColor()
{
	if (isGTX)
		return 0xFC0F;
	else
		return 0;
}

void settings::setIP(char n1, char n2, char n3, char n4)
{
	ostrstream ostr;
	ostr << "ifconfig eth0 " << (int)n1 << "." << (int)n2 << "." << (int)n3 << "." << (int)n4 << " &" << ends; 
	std::string command = ostr.str();
	cout << command << endl;
	
	setting.ip = (n1 << 24) | (n2 << 16) | (n3 << 8) | n4;

	system(command.c_str());
	/*struct sockaddr_in sin;
	int sk;
	unsigned char *ptr;
	struct ifreq ifr;

	memset(&ifr, 0x00, sizeof ifr);
	memset(&sin, 0x00, sizeof sin);

	sin.sin_family = AF_INET;
	//sin.sin_len    = sizeof sin;
	char test[] = "192.168.40.4";
	if (inet_aton(test, &sin.sin_addr)==0) { // 0 if error occurs
		printf("failed conversion\n");
	}

	strcpy(ifr.ifr_name, "eth0");
	memcpy(&ifr.ifr_addr, &sin, sizeof(ifr.ifr_addr));
	printf("IP: %x\n", ifr.ifr_addr);
	if ((sk = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		printf("no socket\n");
	}

	if (ioctl(sk, SIOCSIFADDR, &ifr)==-1) {
		printf("didn't set IP: %s\n", strerror(errno));
	}

	memcpy(&sin, &ifr.ifr_addr, sizeof(sin));
	//ptr = inet_ntoa((unsigned char*) sin.sin_addr);
	//printf("IP Address is :%s\n",ptr);
	close (sk);*/

}

char settings::getIP(char number)
{
	return (setting.ip >> ((3 - number) * 8)) & 0xff;
}

void settings::setgwIP(char n1, char n2, char n3, char n4)
{
	ostrstream ostr;
	ostr << "route add default gw " << (int)n1 << "." << (int)n2 << "." << (int)n3 << "." << (int)n4 << ends; 
	std::string command = ostr.str();
	cout << command << endl;
	system(command.c_str());

	setting.gwip = (n1 << 24) | (n2 << 16) | (n3 << 8) | n4;
}

char settings::getgwIP(char number)
{
	return (setting.gwip >> ((3 - number) * 8)) & 0xff;
}

void settings::setdnsIP(char n1, char n2, char n3, char n4)
{
	ostrstream ostr;
	ostr << "echo \"nameserver " << (int)n1 << "." << (int)n2 << "." << (int)n3 << "." << (int)n4 << "\" > /etc/resolv.conf" << ends; 
	std::string command = ostr.str();
	cout << command << endl;
	system(command.c_str());

	setting.dnsip = (n1 << 24) | (n2 << 16) | (n3 << 8) | n4;
}

char settings::getdnsIP(char number)
{
	return (setting.dnsip >> ((3 - number) * 8)) & 0xff;
}

void settings::setserverIP(char n1, char n2, char n3, char n4)
{
	setting.serverip = (n1 << 24) | (n2 << 16) | (n3 << 8) | n4;
}

char settings::getserverIP(char number)
{
	return (setting.serverip >> ((3 - number) * 8)) & 0xff;
}

void settings::saveSettings()
{
	ostrstream ostr;
	ostr << "TimeOffset=" << setting.timeoffset << endl;
	ostr << "BoxIP=" << (int)getIP(0) << "." << (int)getIP(1) << "." << (int)getIP(2) << "." << (int)getIP(3) << endl;
	ostr << "GatewayIP=" << (int)getgwIP(0) << "." << (int)getgwIP(1) << "." << (int)getgwIP(2) << "." << (int)getgwIP(3) << endl;
	ostr << "DNSIP=" << (int)getdnsIP(0) << "." << (int)getdnsIP(1) << "." << (int)getdnsIP(2) << "." << (int)getdnsIP(3) << endl;
	if (setting.serverip != 0)
		ostr << "ServerIP=" << (int)getserverIP(0) << "." << (int)getserverIP(1) << "." << (int)getserverIP(2) << "." << (int)getserverIP(3) << endl;

	ostr << "SupportOldRC=";
	if (setting.supportOldRc)
		ostr << "true" << endl;
	else
		ostr << "false" << endl;

	ostr << "RCRepeat=";
	if (setting.rcRepeat)
		ostr << "true" << endl;
	else
		ostr << "false" << endl;

	ostr << "SwitchVCR=";
	if (setting.switch_vcr)
		ostr << "true" << endl;
	else
		ostr << "false" << endl;

	ostr << "ProxyServer=" << setting.proxy_server << endl;
	ostr << "ProxyPort=" << setting.proxy_port << endl;

	ostr << "OutputFormat=" << setting.output_format << endl;
	ostr << "VideoFormat=" << setting.video_format << endl;

	ostr << ends;
	std::string configfile = ostr.str();
	int fd = open(CONFIGDIR "/lcars/lcars.conf", O_WRONLY|O_TRUNC|O_CREAT, 0666);
	write(fd, configfile.c_str(), configfile.length());
	close (fd);
}

void settings::loadSettings()
{
	std::ifstream inFile;
	std::string line[20];
	int linecount = 0;

	inFile.open(CONFIGDIR "/lcars/lcars.conf");
	
	while(linecount < 20 && getline(inFile, line[linecount++]));
	
	for (int i = 0; i < linecount; i++)
	{
		std::istringstream iss(line[i]);
		std::string cmd;
		std::string parm;

		getline(iss, cmd, '=');
		getline(iss, parm, '=');

		if (cmd == "TimeOffset")
		{
			setting.timeoffset = atoi(parm.c_str());
		}
		else if (cmd == "BoxIP" || cmd == "GatewayIP" || cmd == "DNSIP" || cmd == "ServerIP")
		{
			unsigned char ip[4];
			int ipcount = 0;
			std::istringstream iss2(parm);
			std::string ippart;
			while(getline(iss2, ippart, '.'))
			{
				ip[ipcount++] = atoi(ippart.c_str());
			}
			if (ipcount != 4)
			{
				cout << "Error in Config-File on " << cmd << endl;
				continue;
			}
			else
			{
				bool isvalid = false;
				for (int j = 0; j < 4; j++)
				{
					if (ip[j] != 0)
						isvalid = true;
				}
				if (!isvalid)
					continue;
				if (cmd == "BoxIP")
				{
					setIP(ip[0], ip[1], ip[2], ip[3]);
				}
				else if (cmd == "GatewayIP")
				{
					setgwIP(ip[0], ip[1], ip[2], ip[3]);
				}
				else if (cmd == "ServerIP")
				{
					setserverIP(ip[0], ip[1], ip[2], ip[3]);
				}
				else if (cmd == "DNSIP")
				{
					setdnsIP(ip[0], ip[1], ip[2], ip[3]);
				}
			}
		}
		else if (cmd == "SupportOldRC")
		{
			if (parm == "true")
				setting.supportOldRc = true;
			else if (parm == "false")
				setting.supportOldRc = false;
			else
				cout << "Error in Config-File on " << cmd << endl;
		}
		else if (cmd == "RCRepeat")
		{
			if (parm == "true")
				setting.rcRepeat = true;
			else if (parm == "false")
				setting.rcRepeat = false;
			else
				cout << "Error in Config-File on " << cmd << endl;
		}
		else if (cmd == "SwitchVCR")
		{
			if (parm == "true")
				setting.switch_vcr = true;
			else if (parm == "false")
				setting.switch_vcr = false;
			else
				cout << "Error in Config-File on " << cmd << endl;
		}
		else if (cmd == "ProxyServer")
		{
			setProxyServer(parm);
		}
		else if (cmd == "ProxyPort")
		{
			setProxyPort(atoi(parm.c_str()));
		}
		else if (cmd == "OutputFormat")
		{
			setting.output_format = atoi(parm.c_str());
		}
		else if (cmd == "VideoFormat")
		{
			setting.video_format = atoi(parm.c_str());
		}
		else
		{
			cout << "Error in Config-File on " << cmd << endl;
		}
	}

	inFile.close();
}

