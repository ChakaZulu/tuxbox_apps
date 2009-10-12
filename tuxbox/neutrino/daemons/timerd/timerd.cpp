/*
	Timer-Daemon  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	$Id: timerd.cpp,v 1.66 2009/10/12 07:35:37 rhabarber1848 Exp $

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

#include <stdio.h>
#include <string.h>
#include <sstream>
#include <signal.h>
#include <unistd.h> /* fork */
#include <cstdlib>

#include <timermanager.h>
#include <debug.h>
#include <sectionsdclient/sectionsdclient.h>

#include <connection/basicserver.h>
#include <timerdclient/timerdmsg.h>

int timerd_debug = 0;
char *config_file_name = CONFIGFILE;

static void signalHandler(int /*signum*/)
{
	CTimerManager::getInstance()->shutdown();
	exit(EXIT_SUCCESS);
}

bool parse_command(CBasicMessage::Header &rmsg, int connfd)
{
//	CTimerEvent_NextProgram::EventMap::iterator it = NULL;
	CTimerEventMap events;
	CTimerdMsg::commandModifyTimer msgModifyTimer;
	CTimerdMsg::responseGetSleeptimer rspGetSleeptimer;
	CTimerd::responseGetTimer resp;
	CTimerEventMap::iterator pos;
	switch (rmsg.cmd)
	{
		
		case CTimerdMsg::CMD_REGISTEREVENT :
			CTimerManager::getInstance()->getEventServer()->registerEvent(connfd);
			break;

		case CTimerdMsg::CMD_UNREGISTEREVENT :
			CTimerManager::getInstance()->getEventServer()->unRegisterEvent(connfd);
			break;

		case CTimerdMsg::CMD_GETSLEEPTIMER:
			rspGetSleeptimer.eventID = 0;
			if (CTimerManager::getInstance()->listEvents(events))
			{
				for (pos = events.begin(); pos != events.end(); pos++)
				{
					printf("ID: %u type: %u\n",pos->second->eventID,pos->second->eventType);
					if(pos->second->eventType == CTimerd::TIMER_SLEEPTIMER)
					{
						rspGetSleeptimer.eventID = pos->second->eventID;
						break;
					}
				}
			}
			CBasicServer::send_data(connfd, &rspGetSleeptimer, sizeof(rspGetSleeptimer));
			break;

		case CTimerdMsg::CMD_GETTIMER:						// timer daten abfragen
			CTimerdMsg::commandGetTimer msgGetTimer;
			CBasicServer::receive_data(connfd,&msgGetTimer, sizeof(msgGetTimer));
			if(CTimerManager::getInstance()->listEvents(events))
			{
				if(events[msgGetTimer.eventID])
				{
					CTimerEvent *event = events[msgGetTimer.eventID];
					resp.eventID = event->eventID;
					resp.eventState = event->eventState;
					resp.eventType = event->eventType;
					resp.eventRepeat = event->eventRepeat;
					resp.announceTime = event->announceTime;
					resp.alarmTime = event->alarmTime;
					resp.stopTime = event->stopTime;
					resp.repeatCount = event->repeatCount;
					
					if(event->eventType == CTimerd::TIMER_STANDBY)
						resp.standby_on = static_cast<CTimerEvent_Standby*>(event)->standby_on;
					else if(event->eventType == CTimerd::TIMER_NEXTPROGRAM)
					{
						resp.epgID = static_cast<CTimerEvent_NextProgram*>(event)->eventInfo.epgID;
						resp.epg_starttime = static_cast<CTimerEvent_NextProgram*>(event)->eventInfo.epg_starttime;
						resp.channel_id = static_cast<CTimerEvent_NextProgram*>(event)->eventInfo.channel_id;
						resp.apids = static_cast<CTimerEvent_Record*>(event)->eventInfo.apids;
					}
					else if(event->eventType == CTimerd::TIMER_RECORD)
					{
						CTimerEvent_Record* ev= static_cast<CTimerEvent_Record*>(event);
						resp.epgID = ev->eventInfo.epgID;
						resp.epg_starttime = ev->eventInfo.epg_starttime;
						resp.channel_id = ev->eventInfo.channel_id;
						resp.apids = ev->eventInfo.apids;
						strcpy(resp.recordingDir, ev->recordingDir.substr(0,sizeof(resp.recordingDir)-1).c_str());						
						strcpy(resp.epgTitle, ev->epgTitle.substr(0,sizeof(resp.epgTitle)-1).c_str());						
					}
					else if(event->eventType == CTimerd::TIMER_ZAPTO)
					{
						CTimerEvent_Zapto* ev= static_cast<CTimerEvent_Zapto*>(event);
						resp.epgID = ev->eventInfo.epgID;
						resp.epg_starttime = ev->eventInfo.epg_starttime;
						resp.channel_id = ev->eventInfo.channel_id;
						resp.apids = ev->eventInfo.apids;
					}
					else if(event->eventType == CTimerd::TIMER_REMIND)
					{
						memset(resp.message, 0, sizeof(resp.message));
						strncpy(resp.message, static_cast<CTimerEvent_Remind*>(event)->message, sizeof(resp.message)-1);
					}
					else if (event->eventType == CTimerd::TIMER_EXEC_PLUGIN)
					{
						memset(resp.pluginName, 0, sizeof(resp.pluginName));
						strncpy(resp.pluginName, static_cast<CTimerEvent_ExecPlugin*>(event)->name, sizeof(resp.message)-1);						
					}
				}
			}
			CBasicServer::send_data(connfd, &resp, sizeof(CTimerd::responseGetTimer));
			break;

		case CTimerdMsg::CMD_GETTIMERLIST:
			CTimerdMsg::generalInteger responseInteger;
			responseInteger.number = (CTimerManager::getInstance()->listEvents(events)) ? events.size() : 0;

			if (CBasicServer::send_data(connfd, &responseInteger, sizeof(responseInteger)) == true)
			{
				for (CTimerEventMap::iterator it = events.begin(); it != events.end(); it++)
				{
					CTimerEvent *event = it->second;

					resp.eventID = event->eventID;
					resp.eventState = event->eventState;
					resp.eventType = event->eventType;
					resp.eventRepeat = event->eventRepeat;
					resp.announceTime = event->announceTime;
					resp.alarmTime = event->alarmTime;
					resp.stopTime = event->stopTime;
					resp.repeatCount = event->repeatCount;

					if(event->eventType == CTimerd::TIMER_STANDBY)
						resp.standby_on = static_cast<CTimerEvent_Standby*>(event)->standby_on;
					else if(event->eventType == CTimerd::TIMER_NEXTPROGRAM)
					{
						resp.epgID = static_cast<CTimerEvent_NextProgram*>(event)->eventInfo.epgID;
						resp.epg_starttime = static_cast<CTimerEvent_NextProgram*>(event)->eventInfo.epg_starttime;
						resp.channel_id = static_cast<CTimerEvent_NextProgram*>(event)->eventInfo.channel_id;
						resp.apids = static_cast<CTimerEvent_Record*>(event)->eventInfo.apids;
					}
					else if(event->eventType == CTimerd::TIMER_RECORD)
					{
						CTimerEvent_Record* ev= static_cast<CTimerEvent_Record*>(event);
						resp.epgID = ev->eventInfo.epgID;
						resp.epg_starttime = ev->eventInfo.epg_starttime;
						resp.channel_id = ev->eventInfo.channel_id;
						resp.apids = ev->eventInfo.apids;
						strcpy(resp.recordingDir, ev->recordingDir.substr(0,sizeof(resp.recordingDir)-1).c_str());
						strcpy(resp.epgTitle, ev->epgTitle.substr(0,sizeof(resp.epgTitle)-1).c_str());
					}
					else if(event->eventType == CTimerd::TIMER_ZAPTO)
					{
						CTimerEvent_Zapto* ev= static_cast<CTimerEvent_Zapto*>(event);
						resp.epgID = ev->eventInfo.epgID;
						resp.epg_starttime = ev->eventInfo.epg_starttime;
						resp.channel_id = ev->eventInfo.channel_id;
						resp.apids = ev->eventInfo.apids;
						strcpy(resp.epgTitle, ev->epgTitle.substr(0,sizeof(resp.epgTitle)-1).c_str());
					}
					else if(event->eventType == CTimerd::TIMER_REMIND)
					{
						strcpy(resp.message, static_cast<CTimerEvent_Remind*>(event)->message);
					}
					else if(event->eventType == CTimerd::TIMER_EXEC_PLUGIN)
					{
						strcpy(resp.pluginName, static_cast<CTimerEvent_ExecPlugin*>(event)->name);
					}
					CBasicServer::send_data(connfd, &resp, sizeof(CTimerd::responseGetTimer));
				}
			}
			break;

		case CTimerdMsg::CMD_RESCHEDULETIMER:			// event nach vorne oder hinten schieben
			{
				CBasicServer::receive_data(connfd,&msgModifyTimer, sizeof(msgModifyTimer));
				int ret=CTimerManager::getInstance()->rescheduleEvent(msgModifyTimer.eventID,msgModifyTimer.announceTime,msgModifyTimer.alarmTime, msgModifyTimer.stopTime);
				CTimerdMsg::responseStatus rspStatus;
				rspStatus.status = (ret!=0);
				CBasicServer::send_data(connfd, &rspStatus, sizeof(rspStatus));
				break;
			}

		case CTimerdMsg::CMD_MODIFYTIMER:				// neue zeiten setzen
			{
				CBasicServer::receive_data(connfd,&msgModifyTimer, sizeof(msgModifyTimer));
				CTimerd::responseGetTimer data;
				CTimerd::CTimerEventTypes *type = CTimerManager::getInstance()->getEventType(msgModifyTimer.eventID);

				if (type)
				{
					switch (*type)
					{
						case CTimerd::TIMER_SHUTDOWN:						
						case CTimerd::TIMER_NEXTPROGRAM:
						case CTimerd::TIMER_ZAPTO:
						case CTimerd::TIMER_STANDBY:
						case CTimerd::TIMER_REMIND:
						case CTimerd::TIMER_SLEEPTIMER:
						case CTimerd::TIMER_EXEC_PLUGIN:
						case CTimerd::TIMER_IMMEDIATE_RECORD:
							break;
						case CTimerd::TIMER_RECORD:
						{
							CTimerdMsg::commandRecordDir rdir;
							CBasicServer::receive_data(connfd,&rdir, sizeof(CTimerdMsg::commandRecordDir));
							strcpy(data.recordingDir,rdir.recDir);
							break;
						}
						default:
							break;
					}
				}
				int ret=CTimerManager::getInstance()->modifyEvent(msgModifyTimer.eventID,msgModifyTimer.announceTime,msgModifyTimer.alarmTime,
										  msgModifyTimer.stopTime,msgModifyTimer.repeatCount,msgModifyTimer.eventRepeat,
										  data);
				CTimerdMsg::responseStatus rspStatus;
				rspStatus.status = (ret!=0);
				CBasicServer::send_data(connfd, &rspStatus, sizeof(rspStatus));
				break;
			}

		case CTimerdMsg::CMD_ADDTIMER:						// neuen timer hinzuf�gen
			CTimerdMsg::commandAddTimer msgAddTimer;
			CBasicServer::receive_data(connfd,&msgAddTimer, sizeof(msgAddTimer));

			CTimerdMsg::responseAddTimer rspAddTimer;
			CTimerEvent* event;
			CTimerd::TransferEventInfo evInfo;
			switch(msgAddTimer.eventType)
			{
				case CTimerd::TIMER_STANDBY :
					CTimerdMsg::commandSetStandby standby;
					CBasicServer::receive_data(connfd, &standby, sizeof(CTimerdMsg::commandSetStandby));

					event = new CTimerEvent_Standby(
						msgAddTimer.announceTime,
						msgAddTimer.alarmTime,
						standby.standby_on,
						msgAddTimer.eventRepeat,
						msgAddTimer.repeatCount);
					rspAddTimer.eventID = CTimerManager::getInstance()->addEvent(event);
					break;

				case CTimerd::TIMER_SHUTDOWN :
					event = new CTimerEvent_Shutdown(
						msgAddTimer.announceTime,
						msgAddTimer.alarmTime,
						msgAddTimer.eventRepeat,
						msgAddTimer.repeatCount);
					rspAddTimer.eventID = CTimerManager::getInstance()->addEvent(event);
					break;

				case CTimerd::TIMER_SLEEPTIMER :
					event = new CTimerEvent_Sleeptimer(
						msgAddTimer.announceTime,
						msgAddTimer.alarmTime,
						msgAddTimer.eventRepeat,
						msgAddTimer.repeatCount);
					rspAddTimer.eventID = CTimerManager::getInstance()->addEvent(event);
					break;

				case CTimerd::TIMER_RECORD :
				{
					
					CTimerd::TransferRecordingInfo recInfo;
					CBasicServer::receive_data(connfd, &recInfo, sizeof(CTimerd::TransferRecordingInfo));
				   if(recInfo.recordingSafety)
					{
						int pre,post;
						CTimerManager::getInstance()->getRecordingSafety(pre,post);
						msgAddTimer.announceTime -= pre;
						msgAddTimer.alarmTime -= pre;
						msgAddTimer.stopTime += post;
					}
					event = new CTimerEvent_Record(
						msgAddTimer.announceTime,
						msgAddTimer.alarmTime,
						msgAddTimer.stopTime,
						recInfo.channel_id,
						recInfo.epgID,
						recInfo.epg_starttime,
						recInfo.apids,
						msgAddTimer.eventRepeat,
						msgAddTimer.repeatCount,
						recInfo.recordingDir);
					rspAddTimer.eventID = CTimerManager::getInstance()->addEvent(event);
				
					break;
				}
				case CTimerd::TIMER_IMMEDIATE_RECORD :
					CBasicServer::receive_data(connfd, &evInfo, sizeof(CTimerd::TransferEventInfo));
					event = new CTimerEvent_Record(
						msgAddTimer.announceTime,
						msgAddTimer.alarmTime,
						msgAddTimer.stopTime,
						evInfo.channel_id,
						evInfo.epgID,
						evInfo.epg_starttime,
						evInfo.apids,
						msgAddTimer.eventRepeat,
						msgAddTimer.repeatCount);
					event->eventState = CTimerd::TIMERSTATE_ISRUNNING;
					rspAddTimer.eventID = CTimerManager::getInstance()->addEvent(event);
					break;

				case CTimerd::TIMER_ZAPTO :
					CBasicServer::receive_data(connfd, &evInfo, sizeof(CTimerd::TransferEventInfo));
					if(evInfo.channel_id > 0)
					{
						if(evInfo.recordingSafety)
						{
							int pre;
							CTimerManager::getInstance()->getZaptoSafety(pre);
							msgAddTimer.announceTime -= pre;
							msgAddTimer.alarmTime -= pre;
						}
						event = new CTimerEvent_Zapto(
							msgAddTimer.announceTime,
							msgAddTimer.alarmTime,
							evInfo.channel_id,
							evInfo.epgID,
							evInfo.epg_starttime,
							msgAddTimer.eventRepeat,
							msgAddTimer.repeatCount);
						rspAddTimer.eventID = CTimerManager::getInstance()->addEvent(event);
					}
					break;

				case CTimerd::TIMER_NEXTPROGRAM :
//					CTimerd::EventInfo evInfo;
					CBasicServer::receive_data(connfd, &evInfo, sizeof(CTimerd::TransferEventInfo));
/*
					it = CTimerEvent_NextProgram::events.find(evInfo.uniqueKey);
					if (it == CTimerEvent_NextProgram::events.end())
					{
						event = new CTimerEvent_NextProgram(
							msgAddTimer.announceTime,
							msgAddTimer.alarmTime,
							msgAddTimer.stopTime,
							msgAddTimer.eventRepeat);
						static_cast<CTimerEvent_NextProgram*>(event)->eventInfo = evInfo;
						CTimerEvent_NextProgram::events.insert(make_pair(static_cast<CTimerEvent_NextProgram*>(event)->eventInfo.uniqueKey, static_cast<CTimerEvent_NextProgram*>(event)));
						rspAddTimer.eventID = CTimerManager::getInstance()->addEvent(event);
					}
					else
					{
						event = it->second;
						static_cast<CTimerEvent_NextProgram*>(event)->eventInfo = evInfo;
						event->alarmtime.tm_mon  = msgAddTimer.month;
						event->alarmtime.tm_mday = msgAddTimer.day;
						event->alarmtime.tm_hour = msgAddTimer.hour;
						event->alarmtime.tm_min  = msgAddTimer.min;
						rspAddTimer.eventID = event->eventID;
					}
*/
					break;
				case CTimerd::TIMER_REMIND :
					CTimerdMsg::commandRemind remind;
					CBasicServer::receive_data(connfd, &remind, sizeof(CTimerdMsg::commandRemind));
					event = new CTimerEvent_Remind(msgAddTimer.announceTime,
								       msgAddTimer.alarmTime,
								       remind.message,
								       msgAddTimer.eventRepeat,
								       msgAddTimer.repeatCount);
					rspAddTimer.eventID = CTimerManager::getInstance()->addEvent(event);
					break;
					
				case CTimerd::TIMER_EXEC_PLUGIN :
					CTimerdMsg::commandExecPlugin pluginMsg;
					CBasicServer::receive_data(connfd, &pluginMsg, sizeof(CTimerdMsg::commandExecPlugin));
					event = new CTimerEvent_ExecPlugin(msgAddTimer.announceTime,
									   msgAddTimer.alarmTime,
									   pluginMsg.name,
									   msgAddTimer.eventRepeat,
									   msgAddTimer.repeatCount);
					rspAddTimer.eventID = CTimerManager::getInstance()->addEvent(event);
					break;
				default:
					printf("[timerd] Unknown TimerType\n");
			}

			CBasicServer::send_data(connfd, &rspAddTimer, sizeof(rspAddTimer));

			break;
		case CTimerdMsg::CMD_REMOVETIMER:						//	timer entfernen
			dprintf("TIMERD: command remove\n");
			CTimerdMsg::commandRemoveTimer msgRemoveTimer;
			CBasicServer::receive_data(connfd,&msgRemoveTimer, sizeof(msgRemoveTimer));
			dprintf("TIMERD: command remove %d\n",msgRemoveTimer.eventID);
			CTimerManager::getInstance()->removeEvent(msgRemoveTimer.eventID);
			break;

		case CTimerdMsg::CMD_STOPTIMER:						//	timer stoppen
			dprintf("TIMERD: command stop\n");
			CTimerdMsg::commandRemoveTimer msgStopTimer;
			CBasicServer::receive_data(connfd,&msgStopTimer, sizeof(msgStopTimer));
			dprintf("TIMERD: command stop %d\n",msgStopTimer.eventID);
			CTimerManager::getInstance()->stopEvent(msgStopTimer.eventID);
			break;

		case CTimerdMsg::CMD_TIMERDAVAILABLE:					// testen ob server l�uft ;)
			{
				CTimerdMsg::responseAvailable rspAvailable;
				rspAvailable.available = true;
				CBasicServer::send_data(connfd, &rspAvailable, sizeof(rspAvailable));
			}
			break;
		case CTimerdMsg::CMD_SHUTDOWN:
			{
				bool ret=CTimerManager::getInstance()->shutdown();
				CTimerdMsg::responseStatus rspStatus;
				rspStatus.status = ret;
				CBasicServer::send_data(connfd, &rspStatus, sizeof(rspStatus));
				return false;
			}
			break;
		case CTimerdMsg::CMD_SETAPID:				  // apid setzen
			{
				CTimerdMsg::commandSetAPid data;
				CBasicServer::receive_data(connfd,&data, sizeof(data));
				CTimerManager::getInstance()->modifyEvent(data.eventID , data.apids);
			}
			break;
		case CTimerdMsg::CMD_SETRECSAFETY:				  // aufnahmekorrektur setzen
			{
				CTimerdMsg::commandRecordingSafety data;
				CBasicServer::receive_data(connfd,&data, sizeof(data));
				CTimerManager::getInstance()->setRecordingSafety(data.pre , data.post);
			}
			break;
		case CTimerdMsg::CMD_GETRECSAFETY:				  // aufnahmekorrektur lesen
			{
				CTimerdMsg::commandRecordingSafety data;
				CTimerManager::getInstance()->getRecordingSafety(data.pre , data.post);
				CBasicServer::send_data(connfd, &data, sizeof(data));
			}
			break;
		case CTimerdMsg::CMD_SETZAPTOSAFETY:				  // umschaltkorrektur setzen
			{
				CTimerdMsg::commandZaptoSafety data;
				CBasicServer::receive_data(connfd,&data, sizeof(data));
				CTimerManager::getInstance()->setZaptoSafety(data.pre);
			}
			break;
		case CTimerdMsg::CMD_GETZAPTOSAFETY:				  // umschaltkorrektur lesen
			{
				CTimerdMsg::commandZaptoSafety data;
				CTimerManager::getInstance()->getZaptoSafety(data.pre);
				CBasicServer::send_data(connfd, &data, sizeof(data));
			}
			break;
		default:
			dprintf("unknown command\n");
	}
	return true;
}

void usage(FILE *dest)
{
	fprintf(dest, "timerd\n");
	fprintf(dest, "command line parameters:\n");
	fprintf(dest, "-c, --config=<file>  use alternative timerd.conf\n");
	fprintf(dest, "-d, --debug          enable debugging code (implies -f)\n");
	fprintf(dest, "-f, --fork           do not fork\n");
	fprintf(dest, "-h, --help           display this text and exit\n\n");
}

int main(int argc, char **argv)
{
	bool do_fork = true;

	dprintf("startup\n");

	for (int i = 1; i < argc; i++)
	{
		if ((!strncmp(argv[i], "-d", 2)) || (!strncmp(argv[i], "--debug", 7)))
		{
			timerd_debug = 1;
			do_fork = false;
		}
		else if ((!strncmp(argv[i], "-f", 2)) || (!strncmp(argv[i], "--fork", 6)))
		{
			do_fork = false;
		}
		else if ((!strncmp(argv[i], "-h", 2)) || (!strncmp(argv[i], "--help", 6)))
		{
			usage(stdout);
			return EXIT_SUCCESS;
		}
		else if ((!strncmp(argv[i], "-c", 2)) || (!strncmp(argv[i], "--config", 8)))
		{
			config_file_name = strtok( argv[i],"=" );
			config_file_name = strtok( NULL, "=" );
		}
		else
		{
			usage(stderr);
			return EXIT_FAILURE;
		}
	}

	CBasicServer timerd_server;

	if(do_fork)
	{
		switch(fork())
		{
			case -1:
				perror("[timerd] fork");
				return EXIT_FAILURE;
			case 0:
				break;
			default:
				return EXIT_SUCCESS;
		}
		if(setsid() == -1)
		{
			perror("[timerd] setsid");
			return EXIT_FAILURE;
		}
	}

	if (!timerd_server.prepare(TIMERD_UDS_NAME))
		return -1;

	//catch all signals
	signal(SIGHUP, signalHandler);
	signal(SIGINT, signalHandler);
	signal(SIGQUIT, signalHandler);
	signal(SIGTERM, signalHandler);

	// Start timer thread
	CTimerManager::getInstance();

	//startup Timer
	try
	{
		timerd_server.run(parse_command, CTimerdMsg::ACTVERSION);
	}
	catch(std::exception& e)
	{
		dprintf("caught std-exception in main-thread %s!\n", e.what());
	}
	catch(...)
	{
		dprintf("caught exception in main-thread!\n");
	}
}
