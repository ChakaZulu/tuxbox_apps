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

network::network(container &contain) : cont(contain)
{
	xmlrpc_obj.setObjects(&cont);
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
					n->writetext("<br><br><a href=\"/epg/now\">EPG Now</a>");
					n->writetext("<br><br><a href=\"/epg/next\">EPG Next</a>");
					strcpy(writebuffer, "<br><br><a href=\"/channels/lcars.dvb\">Channellist in DVB2000-format</a>");
					write(inbound_connection, writebuffer, strlen(writebuffer));
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
	
						(*n->cont.channels_obj).zapCurrentChannel(n->cont.zap_obj, n->cont.tuner_obj);
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
				else if (!strncmp(command[i], "GET /file", strlen("GET /file")))
				{


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

			if (path[1] == "SID2")
			{
				std::string request(buffer);
				std::string xml = request.substr(request.find("\r\n\r\n"));
				cout << "Parsing\n" << endl;
				n->xmlrpc_obj.setInput(xml);
				n->xmlrpc_obj.parse();
				cout << "End Parsing\n" << endl;
				xml = n->xmlrpc_obj.getOutput();
				
				//cout << xml << endl;
				cout << "Making HTTP\n" << endl;
				ostrstream ostr;
				ostr.clear();
	
				ostr << "HTTP/1.1 200 OK\r\n";
				ostr << "Connection: close\r\n";
				ostr << "Content-Length: " << xml.length() << "\r\n";
				ostr << "Content-Type: text/xml\r\n";
				ostr << "Date: Fri, 17 Jul 1998 19:55:08 GMT\r\n";
				ostr << "Server: LCARS Webserver\r\n\r\n";
				ostr << xml << ends;
	
				std::string send = ostr.str();
				cout << "Sending now XML\n" << endl;
				write(inbound_connection, send.c_str(), send.length());
			}
		}

		close(inbound_connection);
	} 
}
