/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2002 Sven Traenkle 'Zwen'
	License: GPL

	Aenderungen: $Log: irsend.cpp,v $
	Aenderungen: Revision 1.2  2002/11/26 22:10:00  Zwen
	Aenderungen: - changed config dir for *.lirc files to /var/tuxbox/config/lirc/
	Aenderungen: - support for lirc actions on pressing volume +/- (volplus.lirc/volminus.lirc)
	Aenderungen:   e.g. for sending volume change commands to amplifier via ir
	Aenderungen:
	Aenderungen: Revision 1.1  2002/11/24 19:55:56  Zwen
	Aenderungen: - send ir signals on sleeptimer event (see timer docu)
	Aenderungen:
*/

#include <stdio.h>
#include <unistd.h>
#include <fstream>

#include "irsend.h"
#include "liblircdclient.h"

#define LIRCDIR "/var/tuxbox/config/lirc/"

CIRSend::CIRSend(string configfile)
{
	m_configFile = LIRCDIR + configfile + ".lirc";
}

bool CIRSend::Send()
{
	ifstream inp;
	char buffer[101];
	int wait_time;
	int status=0;
	inp.open(m_configFile.c_str(),ifstream::in);
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
					string line = buffer;
					if(line.substr(0,4)=="WAIT" || line.substr(0,4)=="wait")
					{
						sscanf(line.substr(5).c_str(),"%d",&wait_time);
						if(wait_time > 0)
							usleep(wait_time*1000);
					}
					else
					{
						int duration=0;
						unsigned int space_pos1=line.find(" ");
						if(space_pos1==string::npos)
						{
							printf("[neutrino] IRSend syntax error in file %s line %d\n",m_configFile.c_str(),linenr);
							status--;
						}
						else
						{
							string deviceName=line.substr(0,space_pos1);
							unsigned int space_pos2=line.find(" ",space_pos1+1);
							if(space_pos2!=string::npos)
							{
								sscanf(line.substr(space_pos2+1).c_str(),"%d",&duration);
							}
							if(duration > 0)
								status+=lirc.SendUsecs(deviceName, line.substr(space_pos1+1,space_pos2-space_pos1-1).c_str(),duration*1000);
							else
								status+=lirc.SendOnce(deviceName, buffer);
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
