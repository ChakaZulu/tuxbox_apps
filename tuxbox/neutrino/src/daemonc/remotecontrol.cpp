/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	Kommentar:

	Diese GUI wurde von Grund auf neu programmiert und sollte nun vom
	Aufbau und auch den Ausbaumoeglichkeiten gut aussehen. Neutrino basiert
	auf der Client-Server Idee, diese GUI ist also von der direkten DBox-
	Steuerung getrennt. Diese wird dann von Daemons uebernommen.


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
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/


#include "remotecontrol.h"
#include "../global.h"


CRemoteControl::CRemoteControl()
{
	memset(&remotemsg, 0, sizeof(remotemsg) );

	current_onid_sid = 0;
	current_EPGid= 0;
	memset(&current_PIDs.PIDs, 0, sizeof(current_PIDs.PIDs) );
	current_PIDs.APIDs.clear();

	pthread_cond_init( &send_cond, NULL );

	pthread_mutexattr_t   mta;
    if (pthread_mutexattr_init(&mta) != 0 )
    	perror("CRemoteControl: pthread_mutexattr_init failed\n");
    if (pthread_mutexattr_settype( &mta, PTHREAD_MUTEX_ERRORCHECK ) != 0 )
		perror("CRemoteControl: pthread_mutexattr_settype failed\n");
	if (pthread_mutex_init( &send_mutex, &mta ) != 0)
		perror("CRemoteControl: pthread_mutex_init failed\n");

	if (pthread_create (&thrSender, NULL, RemoteControlThread, (void *) this) != 0 )
	{
		perror("CRemoteControl: Create RemoteControlThread failed\n");
	}
}



void CRemoteControl::send()
{
	pthread_cond_signal( &send_cond );
	usleep(10);
	//    printf("CRemoteControl: after pthread_cond_signal (with %s)\n", remotemsg.param3);
}

static char* copyStringto(const char* from, char* to, int len, char delim)
{
	const char *fromend=from+len;
	while(*from!=delim && from<fromend && *from)
	{
		*(to++)=*(from++);
	}
	*to=0;
	return (char *)++from;
}

int CRemoteControl::handleMsg(uint msg, uint data)
{

    if ( msg == messages::EVT_CURRENTEPG )
	{
		sectionsd::CurrentNextInfo* info_CN = (sectionsd::CurrentNextInfo*) data;

		if ( ( info_CN->current_uniqueKey >> 16) == current_onid_sid )
		{
			//CURRENT-EPG für den aktuellen Kanal bekommen!;

			current_EPGid= info_CN->current_uniqueKey;

			if ( has_unresolved_ctags )
				processAPIDnames();

			if ( info_CN->flags & sectionsd::epgflags::current_has_linkagedescriptors )
				getSubChannels();
		}
	    return messages_return::handled;
	}
	else if ( msg == messages::EVT_ZAP_COMPLETE )
	{
		if ( data == current_onid_sid )
		{
			g_Zapit->getPIDS( current_PIDs );
			g_RCInput->postMsg( messages::EVT_ZAP_GOTPIDS, current_onid_sid, false );

			processAPIDnames();
		}
	    return messages_return::handled;
	}
	else
		return messages_return::unhandled;
}

void CRemoteControl::getSubChannels()
{
	if ( subChannels.size() == 0 )
	{
		sectionsd::LinkageDescriptorList	linkedServices;
		if ( g_Sectionsd->getLinkageDescriptorsUniqueKey( current_EPGid, linkedServices ) )
		{
			printf("got subchans %d\n",  linkedServices.size());

            are_subchannels = true;
			for (int i=0; i< linkedServices.size(); i++)
			{
				subChannels.insert(  CSubService( linkedServices[i].originalNetworkId<<16 | linkedServices[i].serviceId,
												  linkedServices[i].transportStreamId,
												  linkedServices[i].name) );
			}

			copySubChannelsToZapit();
            g_RCInput->postMsg( messages::EVT_ZAP_GOT_SUBSERVICES, current_onid_sid, false );
		}
	}
}

void CRemoteControl::getNVODs()
{
	if ( subChannels.size() == 0 )
	{
		sectionsd::NVODTimesList	NVODs;
		if ( g_Sectionsd->getNVODTimesServiceKey( current_onid_sid, NVODs ) )
		{
			printf("got nvods %d\n",  NVODs.size());

			are_subchannels = false;
			for (int i=0; i< NVODs.size(); i++)
			{
				if ( NVODs[i].zeit.dauer> 0 )
					subChannels.insert(  CSubService( NVODs[i].onid_sid,
													  NVODs[i].tsid,
													  NVODs[i].zeit.startzeit,
													  NVODs[i].zeit.dauer) );
			}

			copySubChannelsToZapit();
            g_RCInput->postMsg( messages::EVT_ZAP_GOT_SUBSERVICES, current_onid_sid, false );
		}
	}
}

void CRemoteControl::processAPIDnames()
{
	has_unresolved_ctags= false;
	has_ac3 = false;

	for(int count=0; count< current_PIDs.APIDs.size(); count++)
	{
		if ( current_PIDs.APIDs[count].component_tag != -1 )
			has_unresolved_ctags= true;

		if ( strlen( current_PIDs.APIDs[count].desc ) == 3 )
		{
			// unaufgeloeste Sprache...
			strcpy( current_PIDs.APIDs[count].desc, getISO639Description( current_PIDs.APIDs[count].desc ) );
		}

		if ( current_PIDs.APIDs[count].is_ac3 )
		{
			strcat( current_PIDs.APIDs[count].desc, " (AC3)");
			has_ac3 = true;
		}
	}

	if ( has_unresolved_ctags )
	{
		if ( current_EPGid != 0 )
		{
			sectionsd::ComponentTagList tags;
			if ( g_Sectionsd->getComponentTagsUniqueKey( current_EPGid, tags ) )
			{
				has_unresolved_ctags = false;
				has_ac3 = false;

				for (int i=0; i< tags.size(); i++)
				{
					for (int j=0; j< current_PIDs.APIDs.size(); j++)
					{
						if ( current_PIDs.APIDs[j].component_tag == tags[i].componentTag )
						{
							strncpy( current_PIDs.APIDs[j].desc, tags[i].component.c_str(), 25 );
							if ( current_PIDs.APIDs[j].is_ac3 )
								strncat( current_PIDs.APIDs[j].desc, " (AC3)", 25 );
							current_PIDs.APIDs[j].component_tag = -1;
							break;
						}
					}
				}

				CZapitClient::APIDList::iterator e = current_PIDs.APIDs.begin();
				while ( e != current_PIDs.APIDs.end() )
				{
					if ( e->is_ac3 )
					{
						if ( e->component_tag != -1 )
						{
							current_PIDs.APIDs.erase( e );
							continue;
						}
						else
							has_ac3 = true;
					}
					e++;
				}
			}
		}
	}

	g_RCInput->postMsg( messages::EVT_ZAP_GOTAPIDS, current_onid_sid, false );
}

/*
void CRemoteControl::getNVODs( char *channel_name )
{
	static const int max_retry= 20;
	char rip[]="127.0.0.1";
	int rep_cnt= 0;
	CSubServiceListSorted   nvod_list;

	unsigned int onidSid;
	sscanf( channel_name, "%x", &onidSid );

	do
	{
		pthread_mutex_unlock( &send_mutex );
		rep_cnt++;
		if ( rep_cnt> 1 )
		{
			usleep(200000);
			printf("CRemoteControl - retrying getNVODs\n");
		}

		int sock_fd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		SAI servaddr;
		memset(&servaddr,0,sizeof(servaddr));
		servaddr.sin_family=AF_INET;
		servaddr.sin_port=htons(sectionsd::portNumber);
		inet_pton(AF_INET, rip, &servaddr.sin_addr);

		if(connect(sock_fd, (SA *)&servaddr, sizeof(servaddr))==-1)
		{
			perror("CRemoteControl - getNVODs - couldn't connect to sectionsd!\n");
		}
		else
		{
			sectionsd::msgRequestHeader req;
			req.version = 2;
			req.command = sectionsd::timesNVODservice;
			req.dataLength = 4;
			write(sock_fd, &req, sizeof(req));
			write(sock_fd, &onidSid, req.dataLength);
			sectionsd::msgResponseHeader resp;
			memset(&resp, 0, sizeof(resp));
			if(read(sock_fd, &resp, sizeof(sectionsd::msgResponseHeader))<=0)
			{
				close(sock_fd);
				return;
			}

			if(resp.dataLength)
			{
				char* pData = new char[resp.dataLength] ;
				if(recv(sock_fd, pData,  resp.dataLength, MSG_WAITALL)==  resp.dataLength)
				{
					//printf("dataLength: %u\n", resp.dataLength);
					char *p=pData;

					while(p<pData+resp.dataLength)
					{
						unsigned onidsid2=*(unsigned *)p;
						p+=4;
						unsigned short tsid=*(unsigned short *)p;
						p+=2;
						time_t zeit=*(time_t *)p;
						p+=4;
						unsigned dauer = *(unsigned *)p;
						p+=4;

						if (dauer> 0)
							nvod_list.insert( CSubService(onidsid2, tsid, zeit, dauer) );
					}
				}
				delete[] pData;
			}

			if ( ( nvod_list.size()> 0 ) && ( rep_cnt> 1 ) && ( rep_cnt< ( max_retry- 1) ) )
			{
				nvod_list.clear();
				rep_cnt= max_retry- 1;
			}

			close(sock_fd);
		}
		//pthread_mutex_trylock( &send_mutex );
		pthread_mutex_lock( &send_mutex );

	}
	while ( ( nvod_list.size()== 0 ) && ( rep_cnt< max_retry ) && ( strcmp(remotemsg.param3, channel_name )== 0 ) );

	subChannels_internal.clear( channel_name );
	subChannels_internal.are_subchannels= false;
	for(CSubServiceListSorted::iterator nvod=nvod_list.begin(); nvod!=nvod_list.end(); ++nvod)
		subChannels_internal.list.insert(subChannels_internal.list.end(), * nvod );

}
*/

void * CRemoteControl::RemoteControlThread (void *arg)
{
	CRemoteControl* RemoteControl = (CRemoteControl*) arg;

	int sock_fd;
	SAI servaddr;
	char rip[]="127.0.0.1";
	bool redo, do_immediatly;

	while(1)
	{
		//        printf("CRemoteControl: before pthread_cond_wait\n");

		//pthread_mutex_trylock( &RemoteControl->send_mutex );
		pthread_mutex_lock( &RemoteControl->send_mutex );
		pthread_cond_wait( &RemoteControl->send_cond, &RemoteControl->send_mutex );

		//        printf("CRemoteControl: after pthread_cond_wait for %s\n", RemoteControl->remotemsg.param3);

		st_rmsg r_msg;

		do
		{
			//pthread_mutex_trylock( &RemoteControl->send_mutex );
			pthread_mutex_lock( &RemoteControl->send_mutex );
			memcpy( &r_msg, &RemoteControl->remotemsg, sizeof(r_msg) );
			pthread_mutex_unlock( &RemoteControl->send_mutex );

			memset(&servaddr,0,sizeof(servaddr));
			servaddr.sin_family=AF_INET;

#ifdef HAS_SIN_LEN

			servaddr.sin_len = sizeof(servaddr); // needed ???
#endif

			inet_pton(AF_INET, rip, &servaddr.sin_addr);
			sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

			char *return_buf;
			int bytes_recvd = 0;

			servaddr.sin_port=htons(1505);
			if(connect(sock_fd, (SA *)&servaddr, sizeof(servaddr))==-1)
			{
				perror("CRemoteControl::RemoteControlThread - Couldn't connect to serverd zapit!");
				//            		exit(-1);
			}
			//                printf("sending %d\n", r_msg.cmd);


			write(sock_fd, &r_msg, sizeof(r_msg));

			return_buf = (char*) malloc(4);
			memset(return_buf,0,sizeof(return_buf));
			bytes_recvd = recv(sock_fd, return_buf, 3,0);
			if (bytes_recvd <= 0 )
			{
				perror("CRemoteControl::RemoteControlThread - Nothing could be received from serverd zapit\n");
				//                    exit(-1);
			}
			//                printf("Received %d bytes\n", bytes_recvd);
			//                printf("That was returned: %s\n", return_buf);



			char ZapStatus = return_buf[1];

			do_immediatly = false;

			if ( return_buf[0] == '-' )
			{
				printf("zapit failed for function >%s<\n", &return_buf[2]);
			}
			else
			{
				switch ( return_buf[2] )
				{
						case '0':
						printf("Unknown error reported from zapper\n");
						break;
						case '1':
						{
							printf("Zapping by number returned successful\n");
							break;
						}
						case '2':
						printf("zapit should be killed now.\n");
						break;
						case '3':
						{
							// printf("Zapping by name returned successful\n");

							// ueberpruefen, ob wir die Audio-PIDs holen sollen...
							// printf("Checking for Audio-PIDs %s - %s - %d\n", RemoteControl->remotemsg.param3, r_msg.param3, RemoteControl->remotemsg.cmd);
							//pthread_mutex_trylock( &RemoteControl->send_mutex );


/*							pthread_mutex_lock( &RemoteControl->send_mutex );
							if ( ( RemoteControl->remotemsg.cmd== 3 ) &&
							        ( strcmp(RemoteControl->remotemsg.param3, r_msg.param3 )== 0 ) )
							{
								// noch immer der gleiche Kanal, Abfrage 8 starten
								RemoteControl->remotemsg.cmd= 8;

								strcpy( RemoteControl->audio_chans_int.name, r_msg.param3 );
								do_immediatly = true;
								// printf("Audio-PIDs holen for %s\n", RemoteControl->apids.name);
							}
							else
								pthread_mutex_unlock( &RemoteControl->send_mutex );
*/
							break;
						}
						break;
						case '4':
						printf("Shutdown Box returned successful\n");
						break;
						case '5':
						printf("get Channellist returned successful\n");
						printf("Should not be received in remotecontrol.cpp. Exiting\n");
						break;
						case '6':
						printf("Changed to radio-mode\n");
						break;
						case '7':
						printf("Changed to TV-mode\n");
						break;
						case '8':
						case 'd':
						{
							struct  pids    apid_return_buf;
							memset(&apid_return_buf, 0, sizeof(apid_return_buf));

							if ( recv(sock_fd,  &apid_return_buf,  sizeof(apid_return_buf), MSG_WAITALL)==  sizeof(apid_return_buf) )
							;

							unsigned int onid_sid;
							sscanf( r_msg.param3, "%x", &onid_sid );

							g_RCInput->postMsg( messages::EVT_ZAP_COMPLETE, onid_sid, false );
							break;
						}
						case 'e':
						{
							struct  pids    apid_return_buf;
							memset(&apid_return_buf, 0, sizeof(apid_return_buf));

							if ( recv(sock_fd,  &apid_return_buf,  sizeof(apid_return_buf), MSG_WAITALL)==  sizeof(apid_return_buf) )
							;
/*							{
								// PIDs emfangen...

								//pthread_mutex_trylock( &RemoteControl->send_mutex );
								pthread_mutex_lock( &RemoteControl->send_mutex );
								if ( ( strlen( RemoteControl->audio_chans_int.name )!= 0 ) ||
								        ( ( strcmp(RemoteControl->remotemsg.param3, r_msg.param3 )== 0 ) && (return_buf[2] == 'd') ) ||
								        (return_buf[2] == 'e') )
								{
									// noch immer der gleiche Kanal - inzwischen nicht weitergezappt

									if ( (return_buf[2] == 'd') && ( ZapStatus & zapped_chan_is_nvod ) )
									{
										RemoteControl->getNVODs( r_msg.param3 );
										// send_mutex ist danach wieder locked

										//printf("[remotecontrol]: %d nvods for >%s<!\n", RemoteControl->subChannels_internal.list.size(), RemoteControl->subChannels_internal.name.c_str());

										if ( RemoteControl->subChannels_internal.list.size()> 0 )
										{
											// übertragen der ids an zapit initialisieren
											RemoteControl->remotemsg.cmd= 'i';
											RemoteControl->remotemsg.param= 1;
											do_immediatly = true;
										}
									}
									else
									{
										if (return_buf[2] == 'd')
											strcpy( RemoteControl->audio_chans_int.name, r_msg.param3 );
										if (return_buf[2] == 'e')
											strcpy( RemoteControl->audio_chans_int.name, RemoteControl->subChannels_internal.name.c_str() );

										// Nur dann die Audio-Channels /PIDs abholen, wenn nicht NVOD-Basechannel

										RemoteControl->audio_chans_int.count_apids = apid_return_buf.count_apids;
										// printf("[remotecontrol]: %s - %d apids!\n", RemoteControl->audio_chans_int.name, RemoteControl->audio_chans_int.count_apids);
										// printf("%d - %d - %d - %d - %d\n", apid_return_buf.apid[0], apid_return_buf.apid[1], apid_return_buf.apid[2], apid_return_buf.apid[3], apid_return_buf.apid[4] );
										for(int count=0;count<apid_return_buf.count_apids;count++)
										{
											// printf("%s \n", apid_return_buf.apids[count].desc);
											strcpy(RemoteControl->audio_chans_int.apids[count].name, apid_return_buf.apids[count].desc);
											RemoteControl->audio_chans_int.apids[count].ctag= apid_return_buf.apids[count].component_tag;
											RemoteControl->audio_chans_int.apids[count].is_ac3= apid_return_buf.apids[count].is_ac3;
											RemoteControl->audio_chans_int.apids[count].pid= apid_return_buf.apids[count].pid;
										}

										// und auch noch die PIDs kopieren
										RemoteControl->i_ecmpid= apid_return_buf.ecmpid;
										RemoteControl->i_vpid= apid_return_buf.vpid;
										RemoteControl->i_vtxtpid= apid_return_buf.vtxtpid;


										//if (apid_return_buf.count_apids> 1)
										//	RemoteControl->getAPID_Names();
									}

//									pthread_cond_signal( &g_InfoViewer->cond_PIDs_available );
								}
								if (!do_immediatly)
									pthread_mutex_unlock( &RemoteControl->send_mutex );
							}
							else
								printf("pid-description fetch failed!\n");
*/							break;
						}
						case 'i':
						{
/*							//pthread_mutex_trylock( &RemoteControl->send_mutex );
							pthread_mutex_lock( &RemoteControl->send_mutex );
							unsigned short nvodcount= RemoteControl->subChannels_internal.list.size();
							write(sock_fd, &nvodcount, 2);

							//printf("Sending NVODs to zapit\n");
							for(int count=0; count<nvodcount; count++)
							{
								write(sock_fd, &RemoteControl->subChannels_internal.list[count].onid_sid, 4);
								write(sock_fd, &RemoteControl->subChannels_internal.list[count].tsid, 2);
							}

							if (RemoteControl->remotemsg.param== 1)
							{
								// called from NVOD - immediately change to nvod #max...
								RemoteControl->remotemsg.cmd= 'e';
								RemoteControl->subChannels_internal.selected= nvodcount- 1;
								snprintf( (char*) &RemoteControl->remotemsg.param3, 10, "%x", RemoteControl->subChannels_internal.list[nvodcount- 1].onid_sid);

								do_immediatly = true;
							}

							// pthread_mutex_unlock( &RemoteControl->send_mutex );
*/
							break;
						}
						case '9':
						printf("Changed apid\n");
						break;

						default:
						printf("Unknown return-code >%s<, %d\n", return_buf, return_buf[2]);
				}
			}
			if ( !do_immediatly )
				usleep(100000);

			close(sock_fd);

			//pthread_mutex_trylock( &RemoteControl->send_mutex );
			pthread_mutex_lock( &RemoteControl->send_mutex );
			redo= memcmp(&r_msg, &RemoteControl->remotemsg, sizeof(r_msg)) != 0;

		}
		while ( redo );
	}
	return NULL;
}

void CRemoteControl::copySubChannelsToZapit()
{
	CZapitClient::subServiceList 		zapitList;
	CZapitClient::commandAddSubServices	aSubChannel;

	for(CSubServiceListSorted::iterator e=subChannels.begin(); e!=subChannels.end(); ++e)
	{
		aSubChannel.onidsid = e->onid_sid;
		aSubChannel.tsid = e->tsid;
	}
	g_Zapit->setSubServices( zapitList );
}


void CRemoteControl::setAPID(int APID)
{
	pthread_mutex_lock( &send_mutex );

	remotemsg.version=1;
	remotemsg.cmd=9;
	snprintf( (char*) &remotemsg.param, 3, "%.1d", APID);
	selected_apid = APID;
	// printf("changing APID to %d\n", audio_chans_int.selected);

	pthread_mutex_unlock( &send_mutex );
	send();
}

string CRemoteControl::setSubChannel(unsigned numSub)
{
/*	pthread_mutex_lock( &send_mutex );
	if ((subChannels_internal.selected== numSub ) || (numSub < 0) || (numSub >= subChannels_internal.list.size()))
	{
		pthread_mutex_unlock( &send_mutex );
		return "";
	}
	//memset(&audio_chans_int, 0, sizeof(audio_chans_int));

	remotemsg.version=1;
	remotemsg.cmd='e';
	snprintf( (char*) &remotemsg.param3, 10, "%x", subChannels_internal.list[numSub].onid_sid);
	subChannels_internal.selected = numSub;

	pthread_mutex_unlock( &send_mutex );
	send();
	return subChannels_internal.list[numSub].subservice_name;
	*/
}

string CRemoteControl::subChannelUp()
{
//	return setSubChannel( (subChannels_internal.selected + 1) % subChannels_internal.list.size());
}

string CRemoteControl::subChannelDown()
{
/*	if (subChannels_internal.selected == 0 )
	{
		return setSubChannel(subChannels_internal.list.size() - 1);
	}
	else
	{
		return setSubChannel(subChannels_internal.selected - 1);
	}
*/
}

void CRemoteControl::zapTo_onid_sid( unsigned int onid_sid, string channame)
{
	current_onid_sid = onid_sid;

	current_EPGid = 0;

	memset(&current_PIDs.PIDs, 0, sizeof(current_PIDs.PIDs) );

	current_PIDs.APIDs.clear();
    selected_apid = 0;
	has_ac3 = false;

	subChannels.clear();
	selected_subchannel = 0;

	pthread_mutex_lock( &send_mutex );
	remotemsg.version=1;
	remotemsg.cmd= 'd';
	snprintf( (char*) &remotemsg.param3, 10, "%x", onid_sid);

	#ifdef USEACTIONLOG
		char buf[1000];
		sprintf((char*) buf, "zapto: %08x \"%s\"", onid_sid, channame.c_str() );
		g_ActionLog->println(buf);
	#endif
	pthread_mutex_unlock( &send_mutex );

	send();

}


void CRemoteControl::radioMode()
{
	pthread_mutex_lock( &send_mutex );

	remotemsg.version=1;
	remotemsg.cmd=6;

	pthread_mutex_unlock( &send_mutex );

	send();
}

void CRemoteControl::tvMode()
{
	pthread_mutex_lock( &send_mutex );

	remotemsg.version=1;
	remotemsg.cmd=7;

	pthread_mutex_unlock( &send_mutex );

	send();
}


void  CRemoteControl::shutdown()
{
	pthread_mutex_lock( &send_mutex );

	remotemsg.version=1;
	remotemsg.cmd=4;

	pthread_mutex_unlock( &send_mutex );

	send();
}


