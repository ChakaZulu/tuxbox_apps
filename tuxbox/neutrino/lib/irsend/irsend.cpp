/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2002 Sven Traenkle 'Zwen'
	License: GPL

	Aenderungen: $Log: irsend.cpp,v $
	Aenderungen: Revision 1.5  2009/02/24 19:27:58  seife
	Aenderungen: fix compiler warnings (hint: std::string::npos is a negative number ;)
	Aenderungen:
	Aenderungen: mod_yparser.cpp:324: warning: comparison is always true due to limited range of data type
	Aenderungen:
	Aenderungen: Revision 1.4  2006/09/13 19:30:16  houdini
	Aenderungen: fixed SEND_ONCE in lirc
	Aenderungen: see http://forum.tuxbox.org/forum/viewtopic.php?p=312244#312244
	Aenderungen:
	Aenderungen: Revision 1.3  2004/02/19 23:07:29  zwen
	Aenderungen: removed debug printf
	Aenderungen:
	Aenderungen: Revision 1.2  2004/02/19 23:00:34  zwen
	Aenderungen: Improved neutrino volume/mute handling:
	Aenderungen: - nhttpd now mutes and sets the correct volume (avs,ost & LIRC !)
	Aenderungen:
	Aenderungen: Please rebuild neutrino completely:
	Aenderungen: cd ${cvs}/apps/tuxbox/neutrino
	Aenderungen: rm Makefile Makefile.in configure config.status
	Aenderungen: cd ${cvs}/cdk
	Aenderungen: rm .neutrino
	Aenderungen: make .neutrino
	Aenderungen:
	Aenderungen: Revision 1.1  2004/02/19 22:29:18  zwen
	Aenderungen: - moved irsend to neutrino libs
	Aenderungen:
	Aenderungen: Revision 1.3  2003/09/19 19:25:27  thegoodguy
	Aenderungen: cleanup
	Aenderungen:
	Aenderungen: Revision 1.2  2002/11/26 22:10:00  Zwen
	Aenderungen: - changed config dir for *.lirc files to /var/tuxbox/config/lirc/
	Aenderungen: - support for lirc actions on pressing volume +/- (volplus.lirc/volminus.lirc)
	Aenderungen:   e.g. for sending volume change commands to amplifier via ir
	Aenderungen:
	Aenderungen: Revision 1.1  2002/11/24 19:55:56  Zwen
	Aenderungen: - send ir signals on sleeptimer event (see timer docu)
	Aenderungen:
*/

#include <irsend/irsend.h>

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
						std::string::size_type space_pos1=line.find(' ');
						if(space_pos1==std::string::npos)
						{
							printf("[neutrino] IRSend syntax error in file %s line %d\n",m_configFile.c_str(),linenr);
							status--;
						}
						else
						{
							std::string deviceName=line.substr(0,space_pos1);
							std::string::size_type space_pos2=line.find(' ',space_pos1+1);
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
