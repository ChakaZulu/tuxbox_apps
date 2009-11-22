/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2002 Sven Traenkle 'Zwen'
	License: GPL

*/

#include "irsend.h"

#include <stdio.h>
#include <unistd.h>
#include <fstream>

#include <liblircdclient.h>

#define LIRCDIR "/var/tuxbox/config/lirc/"

CIRSend::CIRSend(const char * const configfile)
{
	m_configFile = LIRCDIR;
	m_configFile += configfile;
	m_configFile += ".lirc";
}

bool CIRSend::Send()
{
	std::ifstream inp;
	char buffer[101];
	int wait_time;
	int status=0;
	inp.open(m_configFile.c_str(),std::ifstream::in);
	if( inp.is_open() )
	{
		int linenr=0;
		CLircdClient lirc;
		if(lirc.Connect())
		{
			while(inp.good())
			{
				inp.getline(buffer,100);
				linenr++;
				if(buffer[0]!=0)
				{
					std::string line = buffer;
					if(line.substr(0,4)=="WAIT" || line.substr(0,4)=="wait")
					{
						sscanf(line.substr(5).c_str(),"%d",&wait_time);
						if(wait_time > 0)
							usleep(wait_time*1000);
					}
					else
					{
						int duration=0;
						unsigned int space_pos1=line.find(' ');
						if(space_pos1==std::string::npos)
						{
							printf("[neutrino] IRSend syntax error in file %s line %d\n",m_configFile.c_str(),linenr);
							status--;
						}
						else
						{
							std::string deviceName=line.substr(0,space_pos1);
							unsigned int space_pos2=line.find(' ',space_pos1+1);
							if(space_pos2!=std::string::npos)
							{
								sscanf(line.substr(space_pos2+1).c_str(),"%d",&duration);
							}
							if(duration > 0)
								status+=lirc.SendUsecs(deviceName, line.substr(space_pos1+1,space_pos2-space_pos1-1).c_str(),duration*1000);
							else
								status+=lirc.SendOnce(deviceName, line.substr(space_pos1+1,space_pos2-space_pos1-1).c_str());
						}
					}
				}
			}
			lirc.Disconnect();
		}
		inp.close();
	}
	else
	{
//		printf("konnte datei %s nicht oeffnen\n",m_configFile.c_str());
		return false;
	}
	return (status==0);
}
