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
$Log: network.cpp,v $
Revision 1.6  2002/05/20 20:08:12  TheDOC
some new timer and epg-stuff

Revision 1.5  2002/05/18 02:55:24  TheDOC
LCARS 0.21TP7

Revision 1.4  2002/03/03 22:56:27  TheDOC
lcars 0.20

Revision 1.7  2001/12/20 00:31:38  tux
Plugins korrigiert

Revision 1.6  2001/12/17 14:00:41  tux
Another commit

Revision 1.5  2001/12/17 03:52:42  tux
Netzwerkfernbedienung fertig

Revision 1.4  2001/12/17 02:36:05  tux
Fernbedienung ueber's Netzwerk, 1. Schritt

Revision 1.3  2001/12/17 01:55:28  tux
scan.cpp fix

Revision 1.2  2001/12/16 22:36:05  tux
IP Eingaben erweitert

Revision 1.3  2001/12/11 13:38:44  TheDOC
new cdk-path-variables, about 10 new features and stuff

Revision 1.2  2001/11/15 00:43:45  TheDOC
 added

*/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <memory.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>

#include "network.h"
#include "channels.h"
#include "pat.h"
#include "pmt.h"

network::network(container &contain, rc *r, control *c) : cont(contain)
{
	xmlrpc_obj.setObjects(&cont);
	rc_obj = r;
	control_obj = c;
}

void network::startThread()
{
	pthread_create(&thread, 0, &network::startlistening, this);
}

void network::writetext(std::string text)
{
	write(inbound_connection, text.c_str(), text.length());
}

void *network::startlistening(void *object)
{
	network *n = (network *) object;
	
	int file_descriptor, inbound_connection, read_size;
	struct sockaddr_in server_address, inbound_address;
	socklen_t inbound_address_size;
	unsigned char *address_holder;
	char buffer[1024];

	file_descriptor = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (file_descriptor < 0)
	{
		perror("socket()");
		exit(1);
	}

	memset((void*)&server_address, 0, sizeof(server_address));
	server_address.sin_port = htons(PORT);
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = htonl (INADDR_ANY);
	if (bind(file_descriptor, (struct sockaddr*) &server_address, sizeof(server_address)) < 0)
	{
		perror("bind()");
		exit(2);
	}

	while(1)
	{
	
		if (listen(file_descriptor, 5) < 0)
		{
			perror("listen()");
			exit(3);
		}
		memset((void*) &inbound_address, 0, sizeof(inbound_address));
		inbound_address.sin_family = AF_INET;
		inbound_address_size = sizeof(inbound_address);
		inbound_connection = accept(file_descriptor, (struct sockaddr*) &inbound_address, &inbound_address_size);
		if (inbound_connection < 0)
		{
			perror("accept()");
			exit(4);
		}

		n->inbound_connection = inbound_connection;

		address_holder = (unsigned char*) &inbound_address.sin_addr.s_addr;
		printf("Connection from %d.%d.%d.%d\n", address_holder[0], address_holder[1], address_holder[2], address_holder[3]);

		char command[20][10000];
		if ((read_size = read(inbound_connection, buffer, 10000)) < 0)
		{
			perror("read()");
			exit(5);
		}
		printf("Bytes read: %d\n", read_size);

		buffer[read_size] = '\0';
		printf("%s", buffer);

		int parm_count = 0;
		int act_pos = 0;
		for (int i = 0; i < read_size; i++)
		{
			if (buffer[i] != '\n')
			{
				command[parm_count][act_pos] = buffer[i];
				act_pos++;
			}
			else
			{
				if (buffer[i] == '\n')
				{
					command[parm_count][act_pos] = '\0';
					act_pos = 0;
					parm_count++;
				}
			}
		}
		command[parm_count][act_pos - 1] = '\0';
		parm_count++;

		printf ("%d Parameter\n", parm_count);
		for (int i = 0; i < parm_count; i++)
		{
			printf("Parameter %d: %s\n", i, command[i]);
		}

		std::string headerok = "HTTP/1.1 200 OK\nConnection: close\nContent-Type: text/html\n\r\n";
		std::string headerfailed = "HTTP/1.1 404 Not found\nConnection: close\nContent-Type: text/html\n\r\n";
		char writebuffer[1024];
		bool post = false;
		std::string postline;
		for (int i = 0; i < parm_count; i++)
		{
			std::string line(command[i]);
			std::string parm[20];
			
			std::istringstream iss(line);
			int counter = 0;
			while(std::getline(iss, parm[counter++], ' '));
			

			if (parm[0] == "GET")
			{
				std::istringstream iss2(parm[1]);
				std::string path[20];
				int counter2 = 0;
				while(std::getline(iss2, path[counter2++], '/'));

				
				if (path[1] == "" && counter2 == 2)
				{
					printf("GET root\n");
					write(inbound_connection, headerok.c_str(), headerok.length());
					write(inbound_connection, (*n->cont.settings_obj).getVersion().c_str(), (*n->cont.settings_obj).getVersion().length());
					strcpy(writebuffer, "<br><br><a href=\"/channels/gethtmlchannels\">Channellist</a>");
					write(inbound_connection, writebuffer, strlen(writebuffer));
					n->writetext("<br><br><a href=\"command\">Command-Parser</a>");
					n->writetext("<br><br><a href=\"rc/frame\">Remote Control</a>");
					n->writetext("<br><br><a href=\"/epg/now\">EPG Now</a>");
					n->writetext("<br><br><a href=\"/epg/next\">EPG Next</a>");
					strcpy(writebuffer, "<br><br><a href=\"/channels/lcars.dvb\">Channellist in DVB2000-format</a>");
					write(inbound_connection, writebuffer, strlen(writebuffer));
				}
				else if (path[1] == "command")
				{
					std::string response = "<html><body><form action=\"http://192.168.40.4/command\" method=post><input type=text name=command size=80><br><input type=submit name=submit value=\"Befehl ausfuehren\"><br></form><form action=\"http://192.168.40.4/command\" method=post><input type=text name=sub size=80><br><input type=submit name=submit value=\"Sub starten\"></form></body></html>";
					write(inbound_connection, headerok.c_str(), headerok.length());
					write(inbound_connection, response.c_str(), response.length());
				}
				else if (path[1] == "version")
				{
					write(inbound_connection, headerok.c_str(), headerok.length());
					write(inbound_connection, (*n->cont.settings_obj).getVersion().c_str(), (*n->cont.settings_obj).getVersion().length());
				}
				else if (path[1] == "channels")
				{
					if (path[2] == "getcurrent")
					{
						write(inbound_connection, headerok.c_str(), headerok.length());
						sprintf(writebuffer, "%d", (*n->cont.channels_obj).getCurrentChannelNumber()); 
						write(inbound_connection, writebuffer, strlen(writebuffer));
					}
					else if (path[2] == "numberchannels")
					{
						write(inbound_connection, headerok.c_str(), headerok.length());
						sprintf(writebuffer, "%d", (*n->cont.channels_obj).numberChannels()); 
						write(inbound_connection, writebuffer, strlen(writebuffer));
					}
					else if (path[2] == "gethtmlchannels")
					{
						write(inbound_connection, headerok.c_str(), headerok.length());
						write(inbound_connection, "<table>", 7);
						for (int count = 0; count < (*n->cont.channels_obj).numberChannels(); count++)
						{
							sprintf(writebuffer, "<tr><td>%d</td><td><a href=\"zapto/%d\">%s</a></td></tr>\n", count, count, (*n->cont.channels_obj).getServiceName(count).c_str()); 
							write(inbound_connection, writebuffer, strlen(writebuffer));
						}
						write(inbound_connection, "</table>", 8);
					}
					else if (path[2] == "getchannels")
					{
						write(inbound_connection, headerok.c_str(), headerok.length());
						for (int count = 0; count < (*n->cont.channels_obj).numberChannels(); count++)
						{
							channel tmp_chan = (*n->cont.channels_obj).getChannelByNumber(count);
							sprintf(writebuffer, "%d %s<br>\n", count, tmp_chan.serviceName); 
							write(inbound_connection, writebuffer, strlen(writebuffer));
						}
					}
					else if (path[2] == "lcars.dvb")
					{
						std::string header = "HTTP/1.1 200 OK\nConnection: close\nAccept-Ranges: bytes\nContent-Type: application/octet-stream\n\r\n";
						write(inbound_connection, header.c_str(), header.length());
						for (int count = 0; count < (*n->cont.channels_obj).numberChannels(); count++)
						{
							dvbchannel tmp_chan = (*n->cont.channels_obj).getDVBChannel(count);
							write(inbound_connection, &tmp_chan, sizeof(dvbchannel));
						}
					}
					else if (path[2] == "scan")
					{
						(*n->cont.scan_obj).scanChannels();
					}
					else if (path[2] == "zapto")
					{
						int number = atoi(path[3].c_str());
																	
						(*n->cont.channels_obj).setCurrentChannel(number);
	
						(*n->cont.channels_obj).zapCurrentChannel();
						(*n->cont.channels_obj).setCurrentOSDProgramInfo(n->cont.osd_obj);
						
						(*n->cont.channels_obj).receiveCurrentEIT();
						(*n->cont.channels_obj).setCurrentOSDProgramEIT(n->cont.osd_obj);
						(*n->cont.channels_obj).updateCurrentOSDProgramAPIDDescr(n->cont.osd_obj);
	
						write(inbound_connection, headerok.c_str(), headerok.length());
						write(inbound_connection, "Done!<br>\n", 10);
						
						if ((*n->cont.channels_obj).getCurrentAPIDcount() == 1)
							sprintf(writebuffer, "VPID: %x APID: %x<br>\n", (*n->cont.channels_obj).getCurrentVPID(), (*n->cont.channels_obj).getCurrentAPID(0));
						else
							sprintf(writebuffer, "VPID: %x APID: %x APID: %x<br>\n", (*n->cont.channels_obj).getCurrentVPID(), (*n->cont.channels_obj).getCurrentAPID(0), (*n->cont.channels_obj).getCurrentAPID(1));
						write(inbound_connection, writebuffer, strlen(writebuffer));
						
						n->writetext("<br><hr>\n");
						event tmp_event;
						char text[100];
						for (int i = 0; i < 2; i++)
						{
							tmp_event = (i == 0) ? (*n->cont.eit_obj).getNow() : (*n->cont.eit_obj).getNext();
							
							sprintf(text, "%s - <a href=/epg/%s>%s</a><br>\n", ctime(&tmp_event.starttime), (i == 0) ? "now" : "next", tmp_event.event_name);
							n->writetext(text);
							
						}
						n->writetext("<hr><br>\n");
	
						strcpy(writebuffer, "<br><a href=\"/video/stop\">Video stop</a>");
						write(inbound_connection, writebuffer, strlen(writebuffer));
					}
				}
				else if (path[1] == "video")
				{
					if (path[2] == "stop")
					{
						(*n->cont.zap_obj).dmx_stop();

						write(inbound_connection, headerok.c_str(), headerok.length());
						write(inbound_connection, "Done!", 6);
					}
					else if (path[2] == "start")
					{
						(*n->cont.zap_obj).dmx_start();
	
						write(inbound_connection, headerok.c_str(), headerok.length());
						write(inbound_connection, "Done!", 6);
					}

				}
				
				else if (path[1] == "epg")
				{
					char text[1000];
					n->writetext(headerok);
					event tmp_event;
					tmp_event = (path[2] == "now") ? (*n->cont.eit_obj).getNext() : (*n->cont.eit_obj).getNow();
					sprintf(text, "Starttime: %s<br>\n", ctime(&tmp_event.starttime));
					n->writetext(text);
					sprintf(text, "Dauer: %d min<br>\n", (int)(tmp_event.duration / 60));
					n->writetext(text);
					sprintf(text, "<b><h3>%s</h3></b><br>\n", tmp_event.event_name);
					n->writetext(text);
					sprintf(text, "%s<br>\n", tmp_event.event_short_text);
					n->writetext(text);
					sprintf(text, "<textarea cols=80 rows=10 readonly>%s</textarea><br>\n", tmp_event.event_extended_text);
					n->writetext(text);
					sprintf(text, "Running Status: 0x%x - EventID: 0x%x<br>\n", tmp_event.running_status, tmp_event.eventid);
					n->writetext(text);

				}
				else if (path[1] == "rc")
				{
					if (path[2] == "frame")
					{
						write(inbound_connection, headerok.c_str(), headerok.length());
						n->writetext("<frameset  rows=\"0,*\"><frame name=\"command\" src=\"\" marginwidth=\"0\" marginheight=\"0\" scrolling=\"no\" frameborder=\"0\"><frame name=\"main\" src=\"/rc/rc\" marginwidth=\"0\" marginheight=\"0\" scrolling=\"auto\" frameborder=\"0\"></frameset>");
					}
					else if (path[2] == "rc")
					{
						write(inbound_connection, headerok.c_str(), headerok.length());
						std::stringstream ostr;
						ostr << "<map name=\"rc\">";
						ostr << "<area target=\"command\" alt=\"1\" shape=\"circle\" coords=\"103,67,8\" href=\"1\">";
						ostr << "<area target=\"command\" alt=\"2\" shape=\"CIRCLE\" coords=\"123,64,8\" href=\"2\">";
						ostr << "<area target=\"command\" alt=\"3\" shape=\"CIRCLE\" coords=\"143,63,8\" href=\"3\">";
						ostr << "<area target=\"command\" alt=\"4\" shape=\"circle\" coords=\"105,88,8\" href=\"4\">";
						ostr << "<area target=\"command\" alt=\"5\" shape=\"CIRCLE\" coords=\"125,85,9\" href=\"5\">";
						ostr << "<area target=\"command\" alt=\"6\" shape=\"CIRCLE\" coords=\"142,81,8\" href=\"6\">";
						ostr << "<area target=\"command\" alt=\"7\" shape=\"circle\" coords=\"102,108,9\" href=\"7\">";
						ostr << "<area target=\"command\" alt=\"8\" shape=\"circle\" coords=\"121,106,8\" href=\"8\">";
						ostr << "<area target=\"command\" alt=\"9\" shape=\"circle\" coords=\"145,101,9\" href=\"9\">";
						ostr << "<area target=\"command\" alt=\"0\" shape=\"circle\" coords=\"102,131,9\" href=\"0\">";
						ostr << "<area target=\"command\" alt=\"standby\" coords=\"54,59,86,75\" href=\"standby\">";
						ostr << "<area target=\"command\" alt=\"home\" coords=\"55,80,86,95\" href=\"home\">";
						ostr << "<area target=\"command\" alt=\"dbox\" coords=\"56,99,86,117\" href=\"dbox\">";
						ostr << "<area target=\"command\" alt=\"blue\" shape=\"circle\" coords=\"143,143,9\" href=\"blue\">";
						ostr << "<area target=\"command\" alt=\"yellow\" shape=\"circle\" coords=\"102,156,12\" href=\"yellow\">";
						ostr << "<area target=\"command\" alt=\"green\" shape=\"circle\" coords=\"80,174,11\" href=\"green\">";
						ostr << "<area target=\"command\" alt=\"red\" shape=\"circle\" coords=\"66,193,11\" href=\"red\">";
						ostr << "<area target=\"command\" alt=\"ok\" shape=\"CIRCLE\" coords=\"106,238,18\" href=\"ok\">";
						ostr << "<area target=\"command\" alt=\"up\" coords=\"83,200,134,217\" href=\"up\">";
						ostr << "<area target=\"command\" alt=\"down\" coords=\"86,255,129,275\" href=\"down\" shape=\"RECT\">";
						ostr << "<area target=\"command\" alt=\"right\" coords=\"128,218,149,262\" href=\"right\" shape=\"RECT\">";
						ostr << "<area target=\"command\" alt=\"left\" coords=\"72,218,90,256\" href=\"left\" shape=\"RECT\">";
						ostr << "<area target=\"command\" alt=\"plus\" shape=\"circle\" coords=\"62,266,8\" href=\"plus\">";
						ostr << "<area target=\"command\" alt=\"minus\" shape=\"circle\" coords=\"74,289,10\" href=\"minus\">";
						ostr << "<area target=\"command\" alt=\"mute\" shape=\"CIRCLE\" coords=\"104,314,8\" href=\"mute\">";
						ostr << "<area target=\"command\" alt=\"help\" shape=\"CIRCLE\" coords=\"146,329,11\" href=\"help\">";
						ostr << "</map>";
						ostr << "<img src=\"/datadir/rc.jpg\" width=\"200\" height=\"500\"border=\"0\" usemap=\"#rc\">";
						ostr << std::ends;
						n->writetext(ostr.str());
					}
					else if (path[2] == "1")
					{
						n->rc_obj->cheat_command(RC1_1);
					}
					else if (path[2] == "2")
					{
						n->rc_obj->cheat_command(RC1_2);
					}
					else if (path[2] == "3")
					{
						n->rc_obj->cheat_command(RC1_3);
					}
					else if (path[2] == "4")
					{
						n->rc_obj->cheat_command(RC1_4);
					}
					else if (path[2] == "5")
					{
						n->rc_obj->cheat_command(RC1_5);
					}
					else if (path[2] == "6")
					{
						n->rc_obj->cheat_command(RC1_6);
					}
					else if (path[2] == "7")
					{
						n->rc_obj->cheat_command(RC1_7);
					}
					else if (path[2] == "8")
					{
						n->rc_obj->cheat_command(RC1_8);
					}
					else if (path[2] == "9")
					{
						n->rc_obj->cheat_command(RC1_9);
					}
					else if (path[2] == "0")
					{
						n->rc_obj->cheat_command(RC1_0);
					}
					else if (path[2] == "standby")
					{
						n->rc_obj->cheat_command(RC1_STANDBY);
					}
					else if (path[2] == "home")
					{
						n->rc_obj->cheat_command(RC1_HOME);
					}
					else if (path[2] == "dbox")
					{
						n->rc_obj->cheat_command(RC1_DBOX);
					}
					else if (path[2] == "blue")
					{
						n->rc_obj->cheat_command(RC1_BLUE);
					}
					else if (path[2] == "yellow")
					{
						n->rc_obj->cheat_command(RC1_YELLOW);
					}
					else if (path[2] == "green")
					{
						n->rc_obj->cheat_command(RC1_GREEN);
					}
					else if (path[2] == "red")
					{
						n->rc_obj->cheat_command(RC1_RED);
					}
					else if (path[2] == "up")
					{
						n->rc_obj->cheat_command(RC1_UP);
					}
					else if (path[2] == "down")
					{
						n->rc_obj->cheat_command(RC1_DOWN);
					}
					else if (path[2] == "right")
					{
						n->rc_obj->cheat_command(RC1_RIGHT);
					}
					else if (path[2] == "left")
					{
						n->rc_obj->cheat_command(RC1_LEFT);
					}
					else if (path[2] == "ok")
					{
						n->rc_obj->cheat_command(RC1_OK);
					}
					else if (path[2] == "mute")
					{
						n->rc_obj->cheat_command(RC1_MUTE);
					}
					else if (path[2] == "plus")
					{
						n->rc_obj->cheat_command(RC1_VOLPLUS);
					}
					else if (path[2] == "minus")
					{
						n->rc_obj->cheat_command(RC1_VOLMINUS);
					}
					else if (path[2] == "help")
					{
						n->rc_obj->cheat_command(RC1_HELP);
					}


				}
				
				else if (path[1] == "datadir")
				{
					std::string filename = DATADIR "/lcars/" + path[2];
					int fd = open(filename.c_str(), O_RDONLY);
					char c;
					while(read(fd, &c, 1))
						write(inbound_connection, &c, 1);
					close(fd);

				}
				else
				{
					write(inbound_connection, headerfailed.c_str(), headerfailed.length());
				}
			}
			else if (parm[0] == "POST")
			{
				post = true;
				postline = command[i];
				std::cout << "Postline: " << postline << std::endl;
			}
		}
		if (post)
		{
			std::string line(postline);
			std::string parm[20];
			
			std::istringstream iss(line);
			int counter = 0;
			while(std::getline(iss, parm[counter++], ' '));

			std::istringstream iss2(parm[1]);
			std::string path[20];
			int counter2 = 0;
			while(std::getline(iss2, path[counter2++], '/'));

			std::cout << "PATH[1]: " << path[1] << std::endl;
			if (path[1] == "command")
			{
				std::string parameters(command[parm_count - 1]);
				std::stringstream iss(parameters);
				std::string tmp_string;
				std::vector<std::string> var_list;
				std::string command;
				var_list.clear();
				while(std::getline(iss, tmp_string, '&'))
					var_list.insert(var_list.end(), tmp_string);
				for (int i = 0; (unsigned int) i < var_list.size(); i++)
				{
					std::cout << "command2" << std::endl;				
					std::stringstream iss2(var_list[i]);
					std::getline(iss2, tmp_string, '=');
					if (tmp_string == "command")
					{
						std::getline(iss2, command, '=');
						break;
					}
					else if (tmp_string == "sub")
					{
						std::getline(iss2, command, '=');
						break;
					}
				}
				
				if (tmp_string == "command")
				{
					std::replace(command.begin(), command.end(), '+', ' ');
					std::cout << "Command: " << command << std::endl;
					n->control_obj->runCommand(n->control_obj->parseCommand(command));
				}
				else if (tmp_string == "sub")
				{
					std::replace(command.begin(), command.end(), '+', ' ');
					std::cout << "Command: " << command << std::endl;
					n->control_obj->runSub(command);
				}
				std::string response = "<html><body><form action=\"http://192.168.40.4/command\" method=post><input type=text name=command size=80><br><input type=submit name=submit value=\"Befehl ausfuehren\"><br></form><form action=\"http://192.168.40.4/command\" method=post><input type=text name=sub size=80><br><input type=submit name=submit value=\"Sub starten\"></form></body></html>";
				write(inbound_connection, headerok.c_str(), headerok.length());
				write(inbound_connection, response.c_str(), response.length());

			}
			else if (path[1] == "SID2")
			{
				std::string request(buffer);
				std::string xml = request.substr(request.find("\r\n\r\n"));
				std::cout << "Parsing\n" << std::endl;
				n->xmlrpc_obj.setInput(xml);
				n->xmlrpc_obj.parse();
				std::cout << "End Parsing\n" << std::endl;
				xml = n->xmlrpc_obj.getOutput();
				
				//std::cout << xml << std::endl;
				std::cout << "Making HTTP\n" << std::endl;
				std::stringstream ostr;
				ostr.clear();
	
				ostr << "HTTP/1.1 200 OK\r\n";
				ostr << "Connection: close\r\n";
				ostr << "Content-Length: " << xml.length() << "\r\n";
				ostr << "Content-Type: text/xml\r\n";
				ostr << "Date: Fri, 17 Jul 1998 19:55:08 GMT\r\n";
				ostr << "Server: LCARS Webserver\r\n\r\n";
				ostr << xml << std::ends;
	
				std::string send = ostr.str();
				std::cout << "Sending now XML\n" << std::endl;
				//std::cout << send << std::endl;
				write(inbound_connection, send.c_str(), send.length());
			}
			
		}

		close(inbound_connection);
	} 
}
