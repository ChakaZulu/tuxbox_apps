/*
	nhttpd  -  DBoxII-Project

	Copyright (C) 2001/2002 Dirk Szymanski 'Dirch'

	$Id: webapi.cpp,v 1.72 2006/04/13 13:46:17 yjogol Exp $

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

#include <neutrinoMessages.h>

#include <unistd.h>

#include "webserver.h"
#include "webapi.h"
#include "debug.h"
#include "algorithm"
#include "sstream"

#include <config.h>
#include <configfile.h>

#include <zapit/client/zapittools.h>

//-------------------------------------------------------------------------
bool CWebAPI::Execute(CWebserverRequest* request)
{
	int operation = 0;

	const char *operations[] = {
		"test.dbox2", "timer.dbox2","info.dbox2","dbox.dbox2","bouquetlist.dbox2",
		"channellist.dbox2","controlpanel.dbox2",
		"actualepg.dbox2","epg.dbox2","switch.dbox2",NULL};

	dprintf("Executing %s\n",request->Filename.c_str());

	while (operations[operation]) {
		if (request->Filename.compare(operations[operation]) == 0) {
			break;
		}
		operation++;
	}

	if (operations[operation] == NULL) {
		request->Send404Error();
		return false;
	}

	if (request->Method == M_HEAD) {
		request->SendPlainHeader("text/html");
		return true;
	}
	switch(operation)
	{
		case 0:	return Test(request);
			break;
		case 1:	return Timer(request);
			break;
		case 2:	return ShowCurrentStreamInfo(request);
			break;
		case 3:	return Dbox(request);
			break;
		case 4:	return ShowBouquets(request);
			break;
		case 5:	return Channellist(request);
			break;
		case 6:	return Controlpanel(request);
			break;
		case 7:	return ActualEPG(request);
			break;
		case 8:	return EPG(request);
			break;
		case 9:	return Switch(request);
			break;
		default:
			request->Send404Error();
			return false;
	}
}
//-------------------------------------------------------------------------
bool CWebAPI::Test(CWebserverRequest* request)
// testing stuff
{
	request->SendPlainHeader("text/html");		
	return true;
}

void CWebAPI::loadTimerMain(CWebserverRequest* request)
{
//	request->SocketWrite("<HTML><script language=\"JavaScript\">location.href=\"/fb/timer.dbox2\"</script></HTML>\n");
	request->Send302("/fb/timer.dbox2");
}

//-------------------------------------------------------------------------
bool CWebAPI::Timer(CWebserverRequest* request)
// timer functions
{

	if(Parent->Timerd->isTimerdAvailable())
	{
		if(request->ParameterList.size() > 0)
		{
			if(!request->Authenticate())
				return false;
			if(request->ParameterList["action"] == "remove")
			{
				unsigned removeId = atoi(request->ParameterList["id"].c_str());
				Parent->Timerd->removeTimerEvent(removeId);
				loadTimerMain(request);
			}
			else if(request->ParameterList["action"] == "modify-form")
			{
				request->SendPlainHeader("text/html");
				unsigned modyId = atoi(request->ParameterList["id"].c_str());
				modifyTimerForm(request, modyId);
			}
			else if(request->ParameterList["action"] == "modify")
			{
				doModifyTimer(request);
				loadTimerMain(request);
			}
			else if(request->ParameterList["action"] == "new-form")
			{
				request->SendPlainHeader("text/html");
				newTimerForm(request);
			}
			else if(request->ParameterList["action"] == "new")
			{
				doNewTimer(request);
				loadTimerMain(request);
			}
			else if(request->ParameterList["action"] == "none")
			{
				request->SendPlainHeader("text/html");
				ShowTimerList(request);
			}
			else
			{
				request->SendPlainHeader("text/html");
				request->SendHTMLHeader("UNKNOWN ACTION");
				aprintf("Unknown action : %s\n",request->ParameterList["action"].c_str());
				request->SendHTMLFooter ();
			}
		}
		else
		{
			request->SendPlainHeader("text/html");
			ShowTimerList(request);
		}
	}
	else
	{
		request->SendPlainHeader("text/html");
		request->SendHTMLHeader ("Error");
		aprintf("<h1>Error: Timerd not available</h1>\n");
		request->SendHTMLFooter ();
	}
	return true;
}

//-------------------------------------------------------------------------
bool CWebAPI::Dbox(CWebserverRequest* request)
// shows "menu" page
{
	request->SendPlainHeader("text/html");		
	ShowDboxMenu(request);
	return true;
}
//-------------------------------------------------------------------------
bool CWebAPI::ShowBouquets(CWebserverRequest* request)
// show the bouquet list
{
	std::string classname;
	const char * actual;

	request->SendPlainHeader("text/html");
	int BouquetNr = (request->ParameterList["bouquet"] != "")?atoi(request->ParameterList["bouquet"].c_str()):0;
	bool javascript = (request->ParameterList["js"].compare("1") == 0);
	request->SocketWrite("<HTML>\n<HEAD><title>DBOX2-Neutrino Bouquetliste</title>"
			     "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=ISO-8859-1\" />\n"
			     "<link rel=\"stylesheet\" href=\"../global.css\" type=\"text/css\" />\n"
		);

	request->SocketWrite("<SCRIPT LANGUAGE=\"JavaScript\">\n<!--\n function go_to(url1, url2)\n{\n top.content.location.href = url1;\n top.bouquets.location.href = url2;\n }\n//-->\n </SCRIPT>\n</HEAD><BODY>");

	request->SocketWriteLn("<TABLE class=\"bouquetlist\" width=\"100%\">");
	request->SocketWriteLn("<TR><TD><A CLASS=\"blist\" HREF=\"/bouquetedit/main\" TARGET=\"content\">Bouqueteditor</A></TD></TR>\n<TR><TD><HR></TD></TR>");

	if(BouquetNr == 0)
	{
		actual = "<a name=\"akt\"></a>";
		classname = " CLASS=\"bouquet\"";
	}
	else
	{
		actual = "";
		classname = "";
	}

	if(javascript)
		request->SocketWrite("<TR height=20"+ classname + "><TD><a CLASS=\"blist\" HREF=\"javascript:go_to('/fb/channellist.dbox2#akt','/fb/bouquetlist.dbox2?bouquet=0#akt')\">Alle Kanäle</a></TD></TR>\n");
	else
		request->SocketWrite("<TR height=20"+ classname + "><TD><a CLASS=\"blist\" HREF=\"/fb/channellist.dbox2#akt\" TARGET=\"content\">Alle Kanäle</a></TD></TR>\n");
	request->SocketWrite("<TR><TD><HR></TD></TR>\n");
	CZapitClient::BouquetList::iterator bouquet = Parent->BouquetList.begin();
	for(; bouquet != Parent->BouquetList.end();bouquet++)
	{
		if(!bouquet->hidden)
		{
			if((bouquet->bouquet_nr + 1) == (uint) BouquetNr)
			{
				actual = "<a name=\"akt\"></a>";
				classname = " CLASS=\"bouquet\"";
			}
			else
			{
				actual = "";
				classname = "";
			}
			if(javascript)
				request->printf("<tr height=\"20\"%s><TD>%s<NOBR><a CLASS=\"blist\" HREF=\"javascript:go_to('/fb/channellist.dbox2?bouquet=%d#akt','/fb/bouquetlist.dbox2?js=1&bouquet=%d#akt');\">%s</a></NOBR></TD></TR>\n",classname.c_str(), actual,(bouquet->bouquet_nr + 1),(bouquet->bouquet_nr + 1),bouquet->name);
			else
				request->printf("<tr height=\"20\"%s><TD>%s<NOBR><a CLASS=\"blist\" HREF=\"/fb/channellist.dbox2?bouquet=%d#akt\" TARGET=\"content\">%s</a></NOBR></TD></TR>\n",classname.c_str(), actual, (bouquet->bouquet_nr + 1),bouquet->name);
		}
	}
	request->SocketWrite("</TABLE>\n");
	request->SendHTMLFooter();
	return true;

}
//-------------------------------------------------------------------------

bool CWebAPI::Channellist(CWebserverRequest* request)
// show the channel (bouquet) list
{
	request->SendPlainHeader("text/html");
	if( (request->ParameterList.size() == 1) && ( request->ParameterList["bouquet"] != "") )
	{
		ShowBouquet(request,atoi(request->ParameterList["bouquet"].c_str()));
	}
	else
		ShowBouquet(request);
	return true;
}
//-------------------------------------------------------------------------

bool CWebAPI::Controlpanel(CWebserverRequest* request)
// show controlpanel
{	
	int mode;
	CStringList params;
	// TODO: move this back to ShowControlpanel
	// this is needed here until we can query the current locking state
	params["LOCKRC0"] = "00";
	params["LOCKRC1"] = "01";

	if (request->ParameterList.size() > 0)							// parse the parameters first
	{
		if(!request->Authenticate())
				return false;

		if( request->ParameterList["1"].compare("volumemute") == 0)
		{
			bool mute = Parent->Controld->getMute();
			Parent->Controld->setMute( !mute );
		}
		else if( request->ParameterList["1"].compare("volumeplus") == 0)
		{
			char vol = Parent->Controld->getVolume();
			vol+=10;
			if (vol>100)
				vol=100;
			Parent->Controld->setVolume(vol);
		}
		else if( request->ParameterList["1"].compare("volumeminus") == 0)
		{
			char vol = Parent->Controld->getVolume();
			if (vol>=10)
				vol-=10;
			Parent->Controld->setVolume(vol);
		}
		else if( request->ParameterList["1"].compare("standby") == 0)
		{
			Parent->EventServer->sendEvent(NeutrinoMessages::STANDBY_TOGGLE, CEventServer::INITID_HTTPD);
		}
		else if( request->ParameterList["1"].compare("tvmode") == 0)	// switch to tv mode
		{
			mode = NeutrinoMessages::mode_tv;
			Parent->EventServer->sendEvent(NeutrinoMessages::CHANGEMODE, CEventServer::INITID_HTTPD, (void *)&mode,sizeof(int));
			sleep(1);
			Parent->UpdateBouquets();
			request->Send302("channellist.dbox2#akt");
			return true;
		}
		else if(request->ParameterList["1"].compare("radiomode") == 0)	// switch to radio mode
		{
			if(!request->Authenticate())
				return false;
			mode = NeutrinoMessages::mode_radio;
			Parent->EventServer->sendEvent(NeutrinoMessages::CHANGEMODE, CEventServer::INITID_HTTPD, (void *)&mode,sizeof(int));
			sleep(1);
			Parent->UpdateBouquets();
			request->Send302("channellist.dbox2#akt");
			return true;
		}
		else if( request->ParameterList["lockrc"] == "01")
		{
			Parent->EventServer->sendEvent(NeutrinoMessages::LOCK_RC, CEventServer::INITID_HTTPD);
			params["LOCKRC0"] = "01";
			params["LOCKRC1"] = "00";
		}
		else if( request->ParameterList["lockrc"] == "00")
		{
			Parent->EventServer->sendEvent(NeutrinoMessages::UNLOCK_RC, CEventServer::INITID_HTTPD);
		} else
		{
			printf("[CWebApi] param not found\n");
		}

	}

			
	ShowControlpanel(request,params); // show the controlpanel
	return true;
}

//-------------------------------------------------------------------------
bool CWebAPI::ActualEPG(CWebserverRequest* request)
// show epg info about the actual tuned program
{
	ShowActualEpg(request);
	return true;
}

//-------------------------------------------------------------------------
bool CWebAPI::EPG(CWebserverRequest* request)
// show epg for eventid or epgid and startzeit
{

	if (!(request->ParameterList.empty()))
	{											

		if (!(request->ParameterList["eventlist"].empty())) // what the hell has this to to here ?
		{								// TBD: move it here
			request->SendPlainHeader("text/html");
			t_channel_id channel_id;

			sscanf(request->ParameterList["eventlist"].c_str(),
			       SCANF_CHANNEL_ID_TYPE,
			       &channel_id);

			ShowEventList(request, channel_id);
			return true;
		}
		if(request->ParameterList["1"] == "eventlist")				// s.a.
		{
			request->SendPlainHeader("text/html");
			ShowEventList(request, Parent->Zapit->getCurrentServiceID());
			return true;
		}

		if (!(request->ParameterList["eventid"].empty()))
		{
			ShowEpg(request,request->ParameterList["eventid"]);
			return true;
		}
		else if (!(request->ParameterList["epgid"].empty()))
		{
			ShowEpg(request,request->ParameterList["epgid"],request->ParameterList["startzeit"]);
			return true;
		}
	}
	dperror("[HTTPD] Get epgid error\n");
	return false;
}

//-------------------------------------------------------------------------
bool CWebAPI::Switch(CWebserverRequest* request)
// switch something
{
	if(request->ParameterList.size() > 0)
	{

		if(request->ParameterList["zapto"] != "")				// zap to channel and redirect to channel/bouquet list
		{
			if(!request->Authenticate())
				return false;

			Parent->ZapTo(request->ParameterList["zapto"].c_str());
			request->SocketWriteLn("HTTP/1.0 302 Moved Temporarily");

			if(request->ParameterList["bouquet"] != "")
				request->SocketWriteLn("Location: channellist.dbox2?bouquet="+request->ParameterList["bouquet"]+"#akt");
			else
				request->SocketWriteLn("Location: channellist.dbox2#akt");
			return true;
		}

		if(request->ParameterList["zaptosubservice"] != "")			// zap to sub service
		{
			if(!request->Authenticate())
				return false;

			Parent->ZapToSubService(request->ParameterList["zaptosubservice"].c_str());
			request->SocketWriteLn("HTTP/1.0 302 Moved Temporarily");

			if(request->ParameterList["bouquet"] != "")
				request->SocketWriteLn("Location: channellist.dbox2?bouquet="+request->ParameterList["bouquet"]+"#akt");
			else
				request->SocketWriteLn("Location: channellist.dbox2#akt");
			return true;
		}

		if(request->ParameterList["1"] == "shutdown")				// turn box off
		{
			if(!request->Authenticate())
				return false;
			request->SendPlainHeader("text/html");
			request->SendFile("/","shutdown.html");	// send shutdown page
			request->EndRequest();
			sleep(1);															// wait 
			Parent->EventServer->sendEvent(NeutrinoMessages::SHUTDOWN, CEventServer::INITID_HTTPD);
			return true;
		}

	}
	dprintf("Keine Parameter gefunden\n");
	request->Send404Error();
	return false;
}

//-------------------------------------------------------------------------
// Show funtions (Execute)
//-------------------------------------------------------------------------

bool CWebAPI::ShowDboxMenu(CWebserverRequest* request)
{
	CStringList params;
	params["BoxType"] = Parent->Dbox_Hersteller[Parent->Controld->getBoxType()];
	request->ParseFile("dbox.html",params);
	return true;
}


//-------------------------------------------------------------------------
bool CWebAPI::ShowCurrentStreamInfo(CWebserverRequest* request)
{
	int bitInfo[10];
	char buf[100];
	CStringList params;
	CZapitClient::CCurrentServiceInfo serviceinfo;

	request->SendPlainHeader("text/html");		

	serviceinfo = Parent->Zapit->getCurrentServiceInfo();
	params["onid"] = itoh(serviceinfo.onid);
	params["sid"] = itoh(serviceinfo.sid);
	params["tsid"] = itoh(serviceinfo.tsid);
	params["vpid"] = itoh(serviceinfo.vpid);
	params["apid"] = itoh(serviceinfo.apid);
	params["vtxtpid"] = (serviceinfo.vtxtpid != 0)?itoh(serviceinfo.vtxtpid):"nicht verfügbar";
	params["pmtpid"] = (serviceinfo.pmtpid != 0)?itoh(serviceinfo.pmtpid):"nicht verfügbar";
	sprintf((char*) buf, "%d.%d MHz", serviceinfo.tsfrequency/1000, serviceinfo.tsfrequency%1000);
	params["tsfrequency"] = buf;
	params["polarisation"] = serviceinfo.polarisation==1?"h":"v";
	params["ServiceName"] = Parent->GetServiceName(Parent->Zapit->getCurrentServiceID());
	Parent->GetStreamInfo(bitInfo);
	
	sprintf((char*) buf, "%d x %d", bitInfo[0], bitInfo[1] );
	params["VideoFormat"] = buf; //Resolution x y
	sprintf((char*) buf, "%d\n", bitInfo[4]*50);
	params["BitRate"] = buf; //Bitrate bit/sec
	
	switch ( bitInfo[2] ) //format
	{
		case 2: params["AspectRatio"] = "4:3"; break;
		case 3: params["AspectRatio"] = "16:9"; break;
		case 4: params["AspectRatio"] = "2.21:1"; break;
		default: params["AspectRatio"] = "unknown"; break;
	}

	switch ( bitInfo[3] ) //fps
	{
		case 3: params["FPS"] = "25"; break;
		case 6: params["FPS"] = "50"; break;
		default: params["FPS"] = "unknown";
	}

	if (!bitInfo[7]) params["AudioType"]="unknown";
	else {
		const char* layernames[4]={"res","III","II","I"};
		const char* sampfreqnames[4]={"44,1k","48k","32k","res"};
		const char* modenames[4]={"stereo","joint_st","dual_ch","single_ch"};

		long header = bitInfo[7];

		unsigned char layer =	(header>>17)&3;
		unsigned char sampfreq = (header>>10)&3;
		unsigned char mode =	(header>> 6)&3;
		unsigned char copy =	(header>> 3)&1;

		sprintf((char*) buf, "%s (%s/%s) %s", modenames[mode],
								sampfreqnames[sampfreq],
								layernames[layer],
								copy?"c":"");
		params["AudioType"]=buf;
	}

	request->ParseFile("settings.html",params);
	return true;
}

//-------------------------------------------------------------------------
bool CWebAPI::ShowEventList(CWebserverRequest *request,t_channel_id channel_id)
{
	char classname;
	int pos = 0;
	Parent->eList = Parent->Sectionsd->getEventsServiceKey(channel_id);
	CChannelEventList::iterator eventIterator;
	request->SendHTMLHeader("DBOX2-Neutrino Channellist");

	request->SocketWrite("<div CLASS=\"epg\">Programmvorschau: " + Parent->GetServiceName(channel_id));
	request->SocketWrite("</div>\n"
			     "<TABLE class=\"epglist\">\n");

	for( eventIterator = Parent->eList.begin(); eventIterator != Parent->eList.end(); eventIterator++, pos++ )
	{
		classname = (pos&1)?'a':'b';
		char zbuffer[25] = {0};
		struct tm *mtime = localtime(&eventIterator->startTime); //(const time_t*)eventIterator->startTime);
		strftime(zbuffer,20,"%d.%m. %H:%M",mtime);
		request->printf("<TR VALIGN=\"middle\" CLASS=\"%c\">\n",classname);
		request->printf("<TD><NOBR>");
		request->printf("<A HREF=\"/fb/timer.dbox2?action=new&amp;type=%d&amp;alarm=%u&amp;stop=%u&amp;channel_id="
				PRINTF_CHANNEL_ID_TYPE_NO_LEADING_ZEROS
				"&amp;rs=1\">&nbsp;<IMG BORDER=0 SRC=\"/images/record.gif\" WIDTH=\"16\" HEIGHT=\"16\" ALT=\"Sendung aufnehmen\"></A>&nbsp;\n",CTimerd::TIMER_RECORD,(uint) eventIterator->startTime,(uint) eventIterator->startTime + eventIterator->duration,
				channel_id); 
		request->printf("<A HREF=\"/fb/timer.dbox2?action=new&amp;type=%d&amp;alarm=%u&amp;channel_id="
				PRINTF_CHANNEL_ID_TYPE_NO_LEADING_ZEROS
				"\">&nbsp;<IMG BORDER=0 SRC=\"/images/timer.gif\" WIDTH=\"21\" HEIGHT=\"21\" ALT=\"Timer setzen\"></A>&nbsp;\n",CTimerd::TIMER_ZAPTO,(uint) eventIterator->startTime,
				channel_id); 
		request->printf("</NOBR></TD><TD><NOBR>%s&nbsp;<font size=\"-2\">(%d min)</font>&nbsp;</NOBR></TD>\n", zbuffer, eventIterator->duration / 60);
		request->printf("<TD><A CLASS=\"elist\" HREF=epg.dbox2?eventid=%llx>%s</A></TD>\n</TR>\n", eventIterator->eventID, eventIterator->description.c_str());
		if (!(eventIterator->text.empty()))
			request->printf("<TR VALIGN=\"middle\" CLASS=\"%c\"><TD COLSPAN=2><IMG SRC=/images/blank.gif WIDTH=1 HEIGHT=1></TD><TD>%s</TD></TR>\n",classname,eventIterator->text.c_str());

	}

	request->SocketWriteLn("</TABLE>");
	request->SendHTMLFooter();
	return true;
}
//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
bool CWebAPI::ShowBouquet(CWebserverRequest* request, int BouquetNr)
{
	CZapitClient::BouquetChannelList *channellist;
	
	if (BouquetNr > 0)
		channellist = Parent->GetBouquet(BouquetNr, CZapitClient::MODE_CURRENT);
	else
		channellist = Parent->GetChannelList(CZapitClient::MODE_CURRENT);

	Parent->GetChannelEvents();

//	request->SendHTMLHeader("DBOX2-Neutrino Kanalliste");
//	request->SocketWriteLn(
	request->printf(
		"<!DOCTYPE html\n"
		"     PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\n"
		"     \"DTD/xhtml1-strict.dtd\">\n"
		"<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"de\" lang=\"de\">\n"
		"<head>\n"
		"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=ISO-8859-1\" />\n"
		"<meta http-equiv=\"cache-control\" content=\"no-cache\" />\n"
		"<meta http-equiv=\"expires\" content=\"0\" />\n"
		"<link rel=\"stylesheet\" href=\"../global.css\" type=\"text/css\" />\n"
		"<title>DBOX2-Neutrino Kanalliste</title>\n"
		"</head>\n"
		"\n"
		"<body>\n"
		);
	request->SocketWriteLn("<table class=\"bouquetitemlist\">");

	int i = 1;
	char classname;
	t_channel_id current_channel = Parent->Zapit->getCurrentServiceID();
	int prozent;
	CSectionsdClient::responseGetCurrentNextInfoChannelID currentNextInfo;
	char timestr[6];

	CZapitClient::BouquetChannelList::iterator channel = channellist->begin();
	for (; channel != channellist->end();channel++)
	{
		classname = (i++ & 1) ? 'a' : 'b';
		if (channel->channel_id == current_channel)
			classname = 'c';

		std::string bouquetstr = (BouquetNr >= 0) ? ("&amp;bouquet=" + itoa(BouquetNr)) : "";
		
		request->printf("<tr><td colspan=\"2\" class=\"%c\">",classname);
		request->printf("%s<a class=\"clist\" href=\"switch.dbox2?zapto="
				PRINTF_CHANNEL_ID_TYPE_NO_LEADING_ZEROS
				",%s\">%d. %s%s</a>&nbsp;<a href=\"epg.dbox2?eventlist="
				PRINTF_CHANNEL_ID_TYPE_NO_LEADING_ZEROS
				"\">%s</a>",
				((channel->channel_id == current_channel) ? "<a name=\"akt\"></a>" : " "),
				channel->channel_id,
				bouquetstr.c_str(),
				channel->nr,
				channel->name,
				(channel->service_type == ST_NVOD_REFERENCE_SERVICE) ? " (NVOD)" : "",
				channel->channel_id,
				((Parent->ChannelListEvents[channel->channel_id]) ? "<img src=\"../images/elist.gif\" alt=\"Programmvorschau\" style=\"border: 0px\" />" : ""));

		if (channel->channel_id == current_channel)
			request->printf("&nbsp;&nbsp;<a href=\"/fb/info.dbox2\"><img src=\"/images/streaminfo.png\" alt=\"Streaminfo\" style=\"border: 0px\" /></a>");

		request->printf("</td></tr>");

		CChannelEvent *event;
		
		if (channel->service_type == ST_NVOD_REFERENCE_SERVICE)
		{
			CSectionsdClient::NVODTimesList nvod_list;

			if (Parent->Sectionsd->getNVODTimesServiceKey(channel->channel_id, nvod_list))
			{
				CZapitClient::subServiceList subServiceList;
				
				for (CSectionsdClient::NVODTimesList::iterator ni = nvod_list.begin(); ni != nvod_list.end(); ni++)
				{
					CZapitClient::commandAddSubServices cmd;
					CEPGData epg;
					
#warning fixme: verify why there are ntohs conversions here
					cmd.original_network_id = ntohs(ni->original_network_id);
					cmd.service_id = ntohs(ni->service_id);
					cmd.transport_stream_id = ntohs(ni->transport_stream_id);

					t_channel_id channel_id = CREATE_CHANNEL_ID_FROM_SERVICE_ORIGINALNETWORK_TRANSPORTSTREAM_ID(cmd.service_id, cmd.original_network_id, cmd.transport_stream_id);
					
					timeString(ni->zeit.startzeit, timestr); // FIXME: time is wrong (at least on little endian)!

					Parent->Sectionsd->getActualEPGServiceKey(channel_id, &epg); // FIXME: der scheissendreck geht nit!!!

					request->printf("<tr><td align=\"left\" style=\"width: 31px\" class=\"%cepg\">&nbsp;</td>", classname);
					request->printf("<td class=\"%cepg\">%s&nbsp;", classname, timestr);
					request->printf("%s<a href=\"switch.dbox2?zaptosubservice="
							PRINTF_CHANNEL_ID_TYPE_NO_LEADING_ZEROS
							",%s\">%04x:%04x:%04x %s</a>", // FIXME: get name
							(channel_id == current_channel) ? "<a name=\"akt\"></a>" : " ",
							channel_id,
							bouquetstr.c_str(),
							cmd.transport_stream_id,
							cmd.original_network_id,
							cmd.service_id,
							epg.title.c_str());
					request->printf("</td></tr>");

					subServiceList.push_back(cmd);
				}

				if (!(subServiceList.empty()))
					Parent->Zapit->setSubServices(subServiceList);
			}
		}

		
		else if ((event = Parent->ChannelListEvents[channel->channel_id]))
		{
			bool has_current_next = Parent->Sectionsd->getCurrentNextServiceKey(channel->channel_id, currentNextInfo);
			prozent = 100 * (time(NULL) - event->startTime) / event->duration;
			timeString(event->startTime, timestr);
			request->printf("<tr>"
					"<td align=\"left\" style=\"width: 32px\" class=\"%cepg\">"
			                "<table border=\"0\" rules=\"none\" class=\"cslider_table\">"
					"<tr>"
					"<td class=\"cslider_used\" width=\"%d\"></td>"
					"<td class=\"cslider_free\" width=\"%d\"></td>"
					"</tr>"
					"</table></td>"
					, classname
					, (prozent / 10) * 3
					, (10 - (prozent / 10))*3
				);
			request->printf("<td class=\"%cepg\">",classname);
			request->printf("<a class=\"clistsmall\" href=\"epg.dbox2?epgid=%llx\">",event->eventID);
//			request->printf("<a class=\"clistsmall\" href=\"epg.dbox2?epgid=%llx&amp;startzeit=%lx\">",event->eventID,event->startTime);
			request->printf("%s&nbsp;%s&nbsp;"
			                "<span style=\"font-size: 8pt; white-space: nowrap\">(%ld von %d min, %d%%)</span></a>"
					, timestr
					, event->description.c_str()
					, (time(NULL) - event->startTime)/60
					, event->duration / 60,prozent);

			if ((has_current_next) && (currentNextInfo.flags & CSectionsdClient::epgflags::has_next)) {
				timeString(currentNextInfo.next_zeit.startzeit, timestr);
				request->printf("<br />%s&nbsp;%s", timestr, currentNextInfo.next_name.c_str());
			}

			request->printf("</td></tr>\n");
		}
		request->printf("<tr style=\"height: 2px\"><td></td></tr>\n");
	}

	request->printf("</table>\n");

	request->SendHTMLFooter();
	return true;
}
//-------------------------------------------------------------------------

bool CWebAPI::ShowControlpanel(CWebserverRequest* request, CStringList &params)
{
//CStringList params;
char volbuf[5];

	int vol = Parent->Controld->getVolume();
	sprintf(volbuf,"%d",vol);
	params["VOL1"] = volbuf;
	sprintf(volbuf,"%d",100 - vol);
	params["VOL2"] = volbuf;

	if(Parent->Parent->NewGui)
	{
		request->SendPlainHeader("text/html");

		if(	Parent->Controld->getMute())
		{
			params["MUTE0"] = "01";
			params["MUTE1"] = "00";
			params["MUTE_ICON0"] = "<img src=\"/images/if_lsmuted.jpg\" height=\"27\" width=\"100\"><br> <!--\n";
			params["MUTE_ICON1"] = "-->";
		}
		else
		{
			params["MUTE0"] = "00";
			params["MUTE1"] = "01";
			params["MUTE_ICON0"] = " ";
			params["MUTE_ICON1"] = " ";
		}
		request->ParseFile("controlpanel.html",params);
	}
	else
	{
		request->SendPlainHeader("text/html");
		if(	Parent->Controld->getMute())
			params["MUTE"] = "mute";
		else
			params["MUTE"] = "muted";
		request->ParseFile("controlpanel_old.html", params);
	}
	return true;

}

//-------------------------------------------------------------------------
bool CWebAPI::ShowEPG(CWebserverRequest *request,std::string Title, std::string Info1, std::string Info2)
{
	CStringList params;
	params["Title"] = (Title != "")?Title:"Kein EPG vorhanden";
	params["Info1"] = (Info1 != "")?Info1:"keine ausführlichen Informationen verfügbar";
	params["Info2"] = (Info2 != "")?Info2:" ";

	request->SendPlainHeader("text/html");
	request->ParseFile("epg.html",params);
	return true;
}
//-------------------------------------------------------------------------
bool CWebAPI::ShowActualEpg(CWebserverRequest *request)
{
		CEPGData *epg = new CEPGData;
		if(Parent->Sectionsd->getActualEPGServiceKey(Parent->Zapit->getCurrentServiceID(),epg))
			ShowEPG(request,epg->title,epg->info1,epg->info2);			// epg available do show epg 
		else
			ShowEPG(request,epg->title,epg->info1,epg->info2);			// no epg available, TBD: show noepg page
		delete epg;
		return true;
}

//-------------------------------------------------------------------------
bool CWebAPI::ShowEpg(CWebserverRequest *request,std::string EpgID,std::string Startzeit)
{
	unsigned long long epgid;
	uint startzeit;

	const char * idstr = EpgID.c_str();
	sscanf(idstr, "%llx", &epgid);

	if(Startzeit.length() > 0)
	{
		CEPGData *epg = new CEPGData;
		const char * timestr = Startzeit.c_str();
		sscanf(timestr, "%x", &startzeit);

		if(Parent->Sectionsd->getEPGid(epgid,startzeit,epg))			// starttime available then get all infos
			ShowEPG(request,epg->title,epg->info1,epg->info2);
		delete epg;
	}
	else
	{
		CShortEPGData *epg = new CShortEPGData;
		if(Parent->Sectionsd->getEPGidShort(epgid,(CShortEPGData *)epg))	// no starttime, short infos
			ShowEPG(request,epg->title,epg->info1,epg->info2);
		delete epg;
	}
	return true;
}

//-------------------------------------------------------------------------
int minmax(int value,int min, int max)
{
	if(value < min)	return min;
	if(value > max)	return max;
	return value;
}

void CWebAPI::correctTime(struct tm *zt)
{

	zt->tm_year = minmax(zt->tm_year,0,129);
	zt->tm_mon = minmax(zt->tm_mon,0,11);
	zt->tm_mday = minmax(zt->tm_mday,1,31); //-> eine etwas laxe pruefung, aber mktime biegt das wieder grade
	zt->tm_hour = minmax(zt->tm_hour,0,23);
	zt->tm_min = minmax(zt->tm_min,0,59);
	zt->tm_sec = minmax(zt->tm_sec,0,59);
}
//-------------------------------------------------------------------------
bool CWebAPI::ShowTimerList(CWebserverRequest* request)
{

	CTimerd::TimerList timerlist;				// List of bouquets

	timerlist.clear();
	Parent->Timerd->getTimerList(timerlist);
	sort(timerlist.begin(), timerlist.end());

	CZapitClient::BouquetChannelList channellist_tv;     
	CZapitClient::BouquetChannelList channellist_radio;     
	channellist_tv.clear();
	channellist_radio.clear();

	request->SendHTMLHeader("TIMERLISTE");
	request->SocketWrite("<TABLE CLASS=\"timer\" border=0>\n<TR>\n"
	                     "<TD CLASS=\"ctimer\" align=\"center\"><b>Alarm-Zeit</TD>\n"
	                     "<TD CLASS=\"ctimer\" align=\"center\"><b>Stop-Zeit</TD>\n"
	                     "<TD CLASS=\"ctimer\" align=\"center\"><b>Wiederholung</TD>\n"
	                     "<TD CLASS=\"ctimer\" align=\"center\"><b>Wiederholungen</TD>\n"
	                     "<TD CLASS=\"ctimer\" align=\"center\"><b>Typ</TD>\n"
	                     "<TD CLASS=\"ctimer\" align=\"center\"><b>Beschreibung</TD>\n"
	                     "<TD CLASS=\"ctimer\"><TD CLASS=\"ctimer\"></TR>\n");

	int i = 1;
	char classname= 'a';
	CTimerd::TimerList::iterator timer = timerlist.begin();
	for(; timer != timerlist.end();timer++)
	{
		classname = (i++&1)?'a':'b';

		char zAlarmTime[25] = {0};
		struct tm *alarmTime = localtime(&(timer->alarmTime));
		strftime(zAlarmTime,20,"%d.%m. %H:%M",alarmTime);

		char zAnnounceTime[25] = {0};
		struct tm *announceTime = localtime(&(timer->announceTime));
		strftime(zAnnounceTime,20,"%d.%m. %H:%M",announceTime);

		char zStopTime[25] = {0};
		if(timer->stopTime > 0)
		{
			struct tm *stopTime = localtime(&(timer->stopTime));
			strftime(zStopTime,20,"%d.%m. %H:%M",stopTime);     
		}

		request->printf("<TR><TD CLASS=\"%ctimer\" align=center>%s</TD>", classname, zAlarmTime);
		request->printf("<TD CLASS=\"%ctimer\" align=center>%s</TD>", classname, zStopTime);
		char zRep[20+1];
		Parent->timerEventRepeat2Str(timer->eventRepeat,zRep,sizeof(zRep)-1);
		request->printf("<TD CLASS=\"%ctimer\" align=center>%s</TD>", classname, zRep);
		if (timer->eventRepeat == CTimerd::TIMERREPEAT_ONCE)
		{
			request->printf("<TD CLASS=\"%ctimer\" align=\"center\">-</TD>",classname);
		} else
		{
			if (timer->repeatCount == 0)
				request->printf("<TD CLASS=\"%ctimer\" align=\"center\">&#x221E;</TD>",classname);
			else
				request->printf("<TD CLASS=\"%ctimer\" align=\"center\">%dx</TD>",classname, timer->repeatCount);
		}
		char zType[20+1];
		Parent->timerEventType2Str(timer->eventType,zType,sizeof(zType)-1);
		request->printf("<TD CLASS=\"%ctimer\" align=center>%s</TD>", classname, zType);

		// Add Data
		std::string sAddData="";
		switch(timer->eventType)
		{
			case CTimerd::TIMER_NEXTPROGRAM :
			case CTimerd::TIMER_ZAPTO :
			case CTimerd::TIMER_RECORD :
			{
				sAddData = ZapitTools::UTF8_to_Latin1(Parent->Zapit->getChannelName(timer->channel_id).c_str());
				if (sAddData.empty())
					sAddData = Parent->Zapit->isChannelTVChannel(timer->channel_id) ? "Unbekannter TV-Kanal" : "Unbekannter Radiokanal";

				if( timer->apids != TIMERD_APIDS_CONF)
				{
					std::string separator = "";
					sAddData += '(';
					if( timer->apids & TIMERD_APIDS_STD )
					{
						sAddData += "STD";
						separator = "/";
					}
					if( timer->apids & TIMERD_APIDS_ALT )
					{
						sAddData += separator;
						sAddData += "ALT";
						separator = "/";
					}
					if( timer->apids & TIMERD_APIDS_AC3 )
					{
						sAddData += separator;
						sAddData += "AC3";
						separator = "/";
					}
					sAddData += ')';
				}
				if(timer->epgID!=0)
				{
					std::stringstream ss;
					CSectionsdClient sdc;
					CEPGData epgdata;
					if (sdc.getEPGid(timer->epgID, timer->epg_starttime, &epgdata))
					{
						ss << "<BR><A CLASS=\"timer\" HREF=\"epg.dbox2?epgid=" << std::hex << timer->epgID 
							<< "&startzeit=" << timer->epg_starttime
							<< "\">" << epgdata.title << "</A>";
						sAddData+=ss.str();
					}
					else
					{
						sAddData+="<BR>";
						sAddData+=timer->epgTitle;
					}
				}
				else
				{
					sAddData+="<BR>";
					sAddData+=timer->epgTitle;
				}

			}
			break;
			case CTimerd::TIMER_STANDBY :
			{
				sAddData = "Standby: ";
				if(timer->standby_on)
					sAddData+= "An";
				else
					sAddData+="Aus";
			}
			break;
			case CTimerd::TIMER_REMIND :
			{
				sAddData = std::string(timer->message).substr(0,20);
			}
			break;
			case CTimerd::TIMER_EXEC_PLUGIN :
			{
				sAddData = std::string(timer->pluginName);
			}
			break;

			default:{}
		}
		request->printf("<TD CLASS=\"%ctimer\" align=center>%s\n",
			classname, sAddData.c_str());
		request->printf("<TD CLASS=\"%ctimer\" align=center><a HREF=\"/fb/timer.dbox2?action=remove&id=%d\">\n",
			classname, timer->eventID);
		request->SocketWrite("<img border=0 src=\"../images/remove.png\" alt=\"Timer löschen\"></a></TD>\n");
		request->printf("<TD CLASS=\"%ctimer\" align=center><a HREF=\"/fb/timer.dbox2?action=modify-form&id=%d\">", 
			classname, timer->eventID);
		request->printf("<img border=0 src=\"../images/modify.png\" alt=\"Timer ändern\"></a><NOBR></TD></TR>\n");
	}
	classname = (i++&1)?'a':'b';
	request->printf("<TR><TD CLASS=\"%ctimer\" colspan=6><IMG SRC=/images/blank.gif WIDTH=1 HEIGHT=1></TD>\n<TD CLASS=\"%ctimer\" align=\"center\">\n",classname,classname);
	request->SocketWrite("<a HREF=\"javascript:location.reload()\">\n");
	request->SocketWrite("<img border=0 src=\"../images/reload.gif\" alt=\"Aktualisieren\"></a></TD>\n");   
	request->printf("<TD CLASS=\"%ctimer\" align=\"center\">\n",classname);
	request->SocketWrite("<a HREF=\"/fb/timer.dbox2?action=new-form\">\n");
	request->SocketWrite("<img border=0 src=\"../images/new.gif\" alt=\"neuer Timer\"></a></TD></TR>\n");
	request->SocketWrite("</TABLE>\n");
	request->SendHTMLFooter();
	return true;
}
//-------------------------------------------------------------------------
void CWebAPI::modifyTimerForm(CWebserverRequest *request, unsigned timerId)
{
	CTimerd::responseGetTimer timer;             // Timer

	Parent->Timerd->getTimer(timer, timerId);
	
	char zType[20+1];
	Parent->timerEventType2Str(timer.eventType,zType,20);

	request->SendHTMLHeader("TIMER BEARBEITEN");
	// Javascript
	request->SocketWrite("<script language =\"javascript\">\n");
	request->SocketWrite("function my_show(id) {document.getElementById(id).style.visibility=\"visible\";}\n");
	request->SocketWrite("function my_hide(id) {document.getElementById(id).style.visibility=\"hidden\";}\n");
	request->SocketWrite("function focusNMark() { document.modify.wd.select();\n");
	request->SocketWrite("                        document.modify.wd.focus();}\n");
	request->SocketWrite("function onEventChange() { tType=document.modify.rep.value;\n");
	request->printf("  if (tType == \"%d\") my_show(\"WeekdaysRow\"); else my_hide(\"WeekdaysRow\");\n",
		  (int)CTimerd::TIMERREPEAT_WEEKDAYS);
	request->printf("  if (tType == \"%d\") my_hide(\"repcountRow\"); else my_show(\"repcountRow\");\n",
			(int)CTimerd::TIMERREPEAT_ONCE);
	request->SocketWrite("  focusNMark();}\n");
	request->SocketWrite("function onApidDefChange() { \n  if (document.modify.apcf.checked == true) {\n");
   request->SocketWrite("    document.modify.apst.checked=false;\n    document.modify.apal.checked=false;\n");
	request->SocketWrite("    document.modify.apac.checked=false;}\n}\n");
	request->SocketWrite("function onApidChange() {\n");
   request->SocketWrite("if (document.modify.apst.checked == true || document.modify.apal.checked == true ||");
	request->SocketWrite("    document.modify.apac.checked == true) {\n  document.modify.apcf.checked=false;}\n}\n");
	request->SocketWrite("</script>\n");
	
	request->SocketWrite("<TABLE class=\"timer\"><tr CLASS=\"a\"><TD>\n");
	request->SocketWrite("<form method=\"GET\" name=\"modify\" action=\"/fb/timer.dbox2\">\n");
	request->SocketWrite("<INPUT TYPE=\"hidden\" name=\"action\" value=\"modify\">\n");
	request->printf("<INPUT name=\"id\" TYPE=\"hidden\" value=\"%d\">\n",timerId);
	request->SocketWrite("<TABLE border=0 >\n");
	request->printf("<tr CLASS=\"c\"><TD colspan=\"2\" align=\"center\">TIMER %d BEARBEITEN (%s)</TD></TR>\n",
		  timerId,zType);

	struct tm *alarmTime = localtime(&(timer.alarmTime));
	request->printf("<TR><TD align=\"center\"><NOBR>Alarm-Datum <INPUT TYPE=\"text\" name=\"ad\" value=\"%02d\" size=2 maxlength=2>. ",
		  alarmTime->tm_mday );
	request->printf("<INPUT TYPE=\"text\" name=\"amo\" value=\"%02d\" size=2 maxlength=2>.&nbsp",
		  alarmTime->tm_mon +1);
	request->printf("<INPUT TYPE=\"text\" name=\"ay\" value=\"%04d\" size=4 maxlength=4></TD>\n",
		  alarmTime->tm_year + 1900);
	request->printf("<TD align=\"center\"><NOBR>Zeit&nbsp;<INPUT TYPE=\"text\" name=\"ah\" value=\"%02d\" size=2 maxlength=2>&nbsp;:&nbsp;",
		  alarmTime->tm_hour );
	request->printf("<INPUT TYPE=\"text\" name=\"ami\" value=\"%02d\" size=2 maxlength=2></TD></TR>\n",
		  alarmTime->tm_min);
	if(timer.stopTime > 0)
	{
		struct tm *stopTime = localtime(&(timer.stopTime));
		request->printf("<TR><NOBR><TD align=\"center\"><NOBR>Stop-Datum&nbsp;<INPUT TYPE=\"text\" name=\"sd\" value=\"%02d\" size=2 maxlength=2>.&nbsp;",
			 stopTime->tm_mday );
		request->printf("<INPUT TYPE=\"text\" name=\"smo\" value=\"%02d\" size=2 maxlength=2>.&nbsp;",
			 stopTime->tm_mon +1);
		request->printf("<INPUT TYPE=\"text\" name=\"sy\" value=\"%04d\" size=4 maxlength=4></TD>\n",
			 stopTime->tm_year + 1900);
		request->printf("<TD align=\"center\"><NOBR>Zeit&nbsp;<INPUT TYPE=\"text\" name=\"sh\" value=\"%02d\" size=2 maxlength=2>&nbsp;:&nbsp;",
			 stopTime->tm_hour );
		request->printf("<INPUT TYPE=\"text\" name=\"smi\" value=\"%02d\" size=2 maxlength=2></TD></TR>\n",
			 stopTime->tm_min);
		request->printf("<TR><TD align=\"right\">APIDs:\n");
		request->printf("<input type=\"checkbox\" ");
		if(timer.apids == TIMERD_APIDS_CONF)
			request->printf("checked ");
		request->printf("name=\"apcf\" onchange=\"onApidDefChange();\">Default\n");
		request->printf("<td align=\"left\">\n");
		request->printf("<input type=\"checkbox\" ");
		if(timer.apids & TIMERD_APIDS_STD)
			request->printf("checked ");
		request->printf("name=\"apst\" onchange=\"onApidChange();\">Standard\n");
		request->printf("<input type=\"checkbox\" ");
		if(timer.apids & TIMERD_APIDS_ALT)
			request->printf("checked ");
		request->printf("name=\"apal\" onchange=\"onApidChange();\">Alternative\n");
		request->printf("<input type=\"checkbox\" ");
		if(timer.apids & TIMERD_APIDS_AC3)
			request->printf("checked ");
		request->printf("name=\"apac\" onchange=\"onApidChange();\">AC3\n");
		request->printf("</td></tr>\n");
	}
	request->SocketWrite("<TR><TD align=\"center\">Wiederholung\n");
	request->SocketWrite("<select name=\"rep\" onchange=\"onEventChange();\">\n");
	char zRep[21];
	const char * visibility;
	for(int i=0; i<=6;i++)
	{
		if(i!=(int)CTimerd::TIMERREPEAT_BYEVENTDESCRIPTION)
		{
			Parent->timerEventRepeat2Str((CTimerd::CTimerEventRepeat) i, zRep, sizeof(zRep)-1);
			request->printf("<option value=\"%d\"",i);
			if(((int)timer.eventRepeat) == i)
				request->SocketWrite(" selected");
			request->printf(">%s\n",zRep);
		}
	}
	request->printf("<option value=\"%d\"",(int)CTimerd::TIMERREPEAT_WEEKDAYS);
	Parent->timerEventRepeat2Str(CTimerd::TIMERREPEAT_WEEKDAYS, zRep, sizeof(zRep)-1);
	if(timer.eventRepeat >= CTimerd::TIMERREPEAT_WEEKDAYS)
	{
		request->SocketWrite(" selected");
		visibility="visible";
	}
	else
		visibility="hidden";
	request->printf(">%s\n",zRep);
	request->SocketWrite("</select></TD>\n");

	// timer repeats
	request->SocketWrite("<TD id=\"repcountRow\" ");
	if (timer.eventRepeat == CTimerd::TIMERREPEAT_ONCE)
		request->SocketWrite("style=\"visibility:hidden;\" ");
	request->SocketWrite("align=\"center\">Wiederholungen (0 = unbegrenzt) \n<INPUT type=\"text\" name=\"repcount\" id=\"repcount\" ");
	request->printf("value=\"%d\" size=\"4\" maxlength=\"3\"></TD></TR>\n",timer.repeatCount);

	// Weekdays
	char weekdays[8];
	Parent->Timerd->setWeekdaysToStr(timer.eventRepeat, weekdays);
	request->printf("<tr id=\"WeekdaysRow\" style=\"visibility:%s\"><TD align=\"center\">\n", visibility);
	request->printf("Wochentage <INPUT TYPE=\"text\" name=\"wd\" value=\"%s\" size=7 maxlength=7> (Mo-So, X=Timer)</TD></TR>\n",
		 weekdays);

	if(timer.eventType == CTimerd::TIMER_RECORD)
	{
		request->printf("<tr><td colspan=2>Verzeichnis: <INPUT TYPE=\"text\" name=\"rec_dir\" value=\"%s\" size=20 maxlength=%d></td></tr>\n",
		timer.recordingDir, RECORD_DIR_MAXLEN-1);

	}
	request->SocketWrite("<TR><TD colspan=2 height=10></TR>\n"
	                     "<TR><TD align=\"center\"><INPUT TYPE=\"submit\" value=\"OK\"></form></TD>\n"
	                     "<TD align=\"center\"><form method=\"GET\" action=\"/fb/timer.dbox2\">\n"
	                     "<INPUT type=\"hidden\" name=\"action\" value=\"none\">\n"
			     "<INPUT TYPE=\"submit\" value=\"CANCEL\"></form></TD>\n"
			     "</TR></TABLE></TABLE>");
	request->SendHTMLFooter();
}

//-------------------------------------------------------------------------
void CWebAPI::doModifyTimer(CWebserverRequest *request)
{
	unsigned modyId = atoi(request->ParameterList["id"].c_str());
	CTimerd::responseGetTimer timer;
	Parent->Timerd->getTimer(timer, modyId);
	unsigned int repCount = timer.repeatCount;

	struct tm *alarmTime = localtime(&(timer.alarmTime));
	if(request->ParameterList["ad"] != "")
	{
		alarmTime->tm_mday = atoi(request->ParameterList["ad"].c_str());
	}
	if(request->ParameterList["amo"] != "")
	{
		alarmTime->tm_mon = atoi(request->ParameterList["amo"].c_str())-1;
	}
	if(request->ParameterList["ay"] != "")
	{
		alarmTime->tm_year = atoi(request->ParameterList["ay"].c_str())-1900;
	}
	if(request->ParameterList["ah"] != "")
	{
		alarmTime->tm_hour = atoi(request->ParameterList["ah"].c_str());
	}
	if(request->ParameterList["ami"] != "")
	{
		alarmTime->tm_min = atoi(request->ParameterList["ami"].c_str());
	}
	correctTime(alarmTime);
	time_t alarmTimeT = mktime(alarmTime);

	struct tm *stopTime = localtime(&(timer.stopTime));
	if(request->ParameterList["sd"] != "")
	{
		stopTime->tm_mday = atoi(request->ParameterList["sd"].c_str());
	}
	if(request->ParameterList["smo"] != "")
	{
		stopTime->tm_mon = atoi(request->ParameterList["smo"].c_str())-1;
	}
	if(request->ParameterList["sy"] != "")
	{
		stopTime->tm_year = atoi(request->ParameterList["sy"].c_str())-1900;
	}
	if(request->ParameterList["sh"] != "")
	{
		stopTime->tm_hour = atoi(request->ParameterList["sh"].c_str());
	}
	if(request->ParameterList["smi"] != "")
	{
		stopTime->tm_min = atoi(request->ParameterList["smi"].c_str());
	}
	if(request->ParameterList["repcount"] != "")
	{
		repCount = atoi(request->ParameterList["repcount"].c_str());
	}
	correctTime(stopTime);
	time_t stopTimeT = mktime(stopTime);
	time_t announceTimeT = alarmTimeT-60;
	if(timer.eventType == CTimerd::TIMER_RECORD)
		announceTimeT-=120;
	CTimerd::CTimerEventRepeat rep = 
	(CTimerd::CTimerEventRepeat) atoi(request->ParameterList["rep"].c_str());
	if(((int)rep) >= ((int)CTimerd::TIMERREPEAT_WEEKDAYS) && request->ParameterList["wd"] != "")
		Parent->Timerd->getWeekdaysFromStr((int*)&rep, request->ParameterList["wd"].c_str());
	if(timer.eventType == CTimerd::TIMER_RECORD)
		Parent->Timerd->modifyRecordTimerEvent(modyId, announceTimeT, alarmTimeT, stopTimeT, rep,repCount,request->ParameterList["rec_dir"].c_str());
	else
		Parent->Timerd->modifyTimerEvent(modyId, announceTimeT, alarmTimeT, stopTimeT, rep,repCount);
	
	bool changeApids=false;
	unsigned char apids=0;
	if(request->ParameterList["apcf"] == "on")
	{
		changeApids=true;
		apids=0;
	}
	else
	{
		if(request->ParameterList["apst"] == "on")
		{
			changeApids=true;
			apids |= TIMERD_APIDS_STD;
		}
		if(request->ParameterList["apal"] == "on")
		{
			changeApids=true;
			apids |= TIMERD_APIDS_ALT;
		}
		if(request->ParameterList["apac"] == "on")
		{
			changeApids=true;
			apids |= TIMERD_APIDS_AC3;
		}
	}
	if(changeApids)
		Parent->Timerd->modifyTimerAPid(modyId,apids);
}

//-------------------------------------------------------------------------
void CWebAPI::newTimerForm(CWebserverRequest *request)
{
	request->SendHTMLHeader("NEUER TIMER");
	// Javascript
	request->SocketWrite("<script language =\"javascript\">\n"
	                     "function my_show(id) {document.getElementById(id).style.visibility=\"visible\";}\n"
	                     "function my_hide(id) {document.getElementById(id).style.visibility=\"hidden\";}\n"
	                     "function focusNMark() { document.NewTimerForm.ad.select();\n"
	                     "                        document.NewTimerForm.ad.focus();}\n"
	                     "function onEventChange() { tType=document.NewTimerForm.type.value;\n");
	request->printf("  if (tType == \"%d\") my_show(\"StopDateRow\"); else my_hide(\"StopDateRow\");\n",
		  (int)CTimerd::TIMER_RECORD);
	request->printf("  if (tType == \"%d\") my_show(\"StandbyRow\"); else my_hide(\"StandbyRow\");\n",
		  (int)CTimerd::TIMER_STANDBY);
	request->printf("  if (tType == \"%d\" || tType==\"%d\" || tType==\"%d\")\n",
		  (int)CTimerd::TIMER_RECORD, (int)CTimerd::TIMER_NEXTPROGRAM,
		  (int)CTimerd::TIMER_ZAPTO);
	request->SocketWrite("     my_show(\"ProgramRow\"); else my_hide(\"ProgramRow\");\n");
	request->printf("  if (tType == \"%d\") my_show(\"MessageRow\"); else my_hide(\"MessageRow\");\n",
		  (int)CTimerd::TIMER_REMIND);
	request->printf("  if (tType == \"%d\") my_show(\"PluginNameRow\"); else my_hide(\"PluginNameRow\");\n",
		  (int)CTimerd::TIMER_EXEC_PLUGIN);
	request->SocketWrite("  focusNMark();}\n");
	request->SocketWrite("function onEventChange2() { tType=document.NewTimerForm.rep.value;\n");
	request->printf("  if (tType == \"%d\") my_show(\"WeekdaysRow\"); else my_hide(\"WeekdaysRow\");\n",
			(int)CTimerd::TIMERREPEAT_WEEKDAYS);
	request->printf("  if (tType == \"%d\") my_hide(\"repcountRow\"); else my_show(\"repcountRow\");\n",
			(int)CTimerd::TIMERREPEAT_ONCE);
	request->SocketWrite("}\n");
	request->SocketWrite("</script>\n");

	// head of TABLE
	request->SocketWrite("<TABLE class=\"timer\" width=\"70%\"><tr CLASS=\"a\"><TD>\n");
	// Form
	request->SocketWrite("<form method=\"GET\" action=\"/fb/timer.dbox2\" name=\"NewTimerForm\">\n");
	request->SocketWrite("<INPUT TYPE=\"hidden\" name=\"action\" value=\"new\">\n");
	request->SocketWrite("<TABLE border=\"0\" width=\"100%\">\n");
	request->SocketWrite("<tr CLASS=\"c\"><TD colspan=\"2\" align=\"center\">NEUER TIMER</TD></TR>\n");
	// Timer type
	request->SocketWrite("<TR><TD align=\"center\">Timer-Typ\n");
	request->SocketWrite("<select name=\"type\" onchange=\"onEventChange();\">\n");
	for(int i=1; i<=8;i++)
	{
		if(i!=(int)CTimerd::TIMER_NEXTPROGRAM)
		{
			char zType[21];
			Parent->timerEventType2Str((CTimerd::CTimerEventTypes) i, zType, sizeof(zType)-1);
			request->printf("<option value=\"%d\"",i);
			if(i==(int)CTimerd::TIMER_RECORD)
				request->printf(" selected");
			request->printf(">%s\n",zType);
		}
	}
	request->SocketWrite("</select></TD></TR>\n");

	// timer repeat
	request->SocketWrite("<TR><TD align=\"center\">Wiederholung\n");
	request->SocketWrite("<select name=\"rep\" onchange=\"onEventChange2();\">\n");
	char zRep[21];
	for(int i=0; i<=6;i++)
	{
		if(i!=(int)CTimerd::TIMERREPEAT_BYEVENTDESCRIPTION)
		{
			Parent->timerEventRepeat2Str((CTimerd::CTimerEventRepeat) i, zRep, sizeof(zRep)-1);
			request->printf("<option value=\"%d\">%s\n",i,zRep);
		}
	}
	Parent->timerEventRepeat2Str(CTimerd::TIMERREPEAT_WEEKDAYS, zRep, sizeof(zRep)-1);
	request->printf("<option value=\"%d\">%s\n",(int)CTimerd::TIMERREPEAT_WEEKDAYS, zRep);
	request->SocketWrite("</select></TD>\n");

	// timer repeats
	request->SocketWrite("<TD id=\"repcountRow\" style=\"visibility:hidden;\" align=\"center\">Wiederholungen (0 = unbegrenzt) \n");
	request->SocketWrite("<INPUT type=\"text\" name=\"repcount\" id=\"repcount\" value=\"0\" size=\"4\" maxlength=\"3\"></TD></TR>\n");

	
	time_t now_t = time(NULL);
	struct tm *now=localtime(&now_t);
	// alarm day
	request->SocketWrite("<TR><TD align=\"center\">\n<NOBR>");
	request->printf("Alarm-Datum <INPUT TYPE=\"text\" name=\"ad\" value=\"%02d\" size=2 maxlength=2>. \n",
		  now->tm_mday);
	// alarm month
	request->printf("<INPUT TYPE=\"text\" name=\"amo\" value=\"%02d\" size=2 maxlength=2>. \n",
		now->tm_mon+1);
	// alarm year
	request->printf("<INPUT TYPE=\"text\" name=\"ay\" value=\"%04d\" size=4 maxlength=4>\n",
		  now->tm_year+1900);
	// alarm time
	request->SocketWrite("</NOBR></TD><TD align=\"center\"><NOBR>\n");
	request->printf("Zeit <INPUT TYPE=\"text\" name=\"ah\" value=\"%02d\" size=2 maxlength=2> : \n",
		  now->tm_hour);
	request->printf("<INPUT TYPE=\"text\" name=\"ami\" value=\"%02d\" size=2 maxlength=2></NOBR></TD>\n",
		  now->tm_min);
	// stop day
	request->printf("</TR><tr id=\"StopDateRow\"><TD align=\"center\"><NOBR>\n");
	request->printf("Stop-Datum <INPUT TYPE=\"text\" name=\"sd\" value=\"%02d\" size=2 maxlength=2>. \n",
		  now->tm_mday);
	// stop month
	request->printf("<INPUT TYPE=\"text\" name=\"smo\" value=\"%02d\" size=2 maxlength=2>. \n",
		  now->tm_mon+1);
	// stop year
	request->printf("<INPUT TYPE=\"text\" name=\"sy\" value=\"%04d\" size=4 maxlength=4>\n",
		  now->tm_year+1900);
	request->SocketWrite("</NOBR></TD><TD align=\"center\"><NOBR>\n");
	// stop time
	request->printf("Zeit <INPUT TYPE=\"text\" name=\"sh\" value=\"%02d\" size=2 maxlength=2> : \n",
		  now->tm_hour);
	request->printf("<INPUT TYPE=\"text\" name=\"smi\" value=\"%02d\" size=2 maxlength=2></NOBR></TD></TR>\n",
		  now->tm_min);
	// ONID-SID
	request->SocketWrite("<tr id=\"ProgramRow\"><TD>\n");
	request->SocketWrite("<select name=\"channel_id\">\n");
	CZapitClient::BouquetChannelList channellist;     
	channellist.clear();
	Parent->Zapit->getChannels(channellist,CZapitClient::MODE_TV);
	t_channel_id current_channel = Parent->Zapit->getCurrentServiceID();
	CZapitClient::BouquetChannelList::iterator channel = channellist.begin();
	for(; channel != channellist.end();channel++)
	{
		request->printf("<option value=\""
				PRINTF_CHANNEL_ID_TYPE_NO_LEADING_ZEROS
				"\"",
				channel->channel_id);
		if (channel->channel_id == current_channel)
			request->SocketWrite(" selected");
		request->printf(">%s\n",channel->name);
	}
	channellist.clear();
	Parent->Zapit->getChannels(channellist,CZapitClient::MODE_RADIO);
	for (channel = channellist.begin(); channel != channellist.end(); channel++)
	{
		request->printf("<option value=\""
				PRINTF_CHANNEL_ID_TYPE_NO_LEADING_ZEROS
				"\"",
				channel->channel_id);
		if (channel->channel_id == current_channel)
			request->SocketWrite(" selected");
		request->printf(">%s\n",channel->name);
	}
	request->SocketWrite("</selected></td>\n");
	// get Default Recordingdir
	CConfigFile *Config = new CConfigFile(',');
	std::string rec_dir;
	Config->loadConfig(NEUTRINO_CONF);
	rec_dir = Config->getString("network_nfs_recordingdir", "/mnt/filme");
	// recordingDir
	request->printf("<td>Verzeichnis: <INPUT TYPE=\"text\" name=\"rec_dir\" value=\"%s\" size=20 maxlength=%d>\n",
		rec_dir.c_str(), RECORD_DIR_MAXLEN-1);

	request->SocketWrite("</td></TR>\n");
	//standby
	request->SocketWrite("<tr><TD id=\"StandbyRow\" style=\"visibility:hidden\">\n");
	request->SocketWrite("Standby <INPUT TYPE=\"radio\" name=\"sbon\" value=\"1\">An\n");
	request->SocketWrite("<INPUT TYPE=\"radio\" name=\"sbon\" value=\"0\" checked>Aus</TD>\n");
	// weekdays
	request->SocketWrite("<TD id=\"WeekdaysRow\" style=\"visibility:hidden\" align=\"center\">\n");
	request->SocketWrite("Wochentage <INPUT TYPE=\"text\" name=\"wd\" value=\"-------\" size=7 maxlength=7> (Mo-So,X=Ja)</TD></TR>\n");
	//message
	request->SocketWrite("<tr id=\"MessageRow\" style=\"visibility:hidden\"><TD colspan=2>\n");
	request->printf("Nachricht <INPUT TYPE=\"text\" name=\"msg\" value=\"\" size=20 maxlength=%d> ('/'=NL)\n",REMINDER_MESSAGE_MAXLEN-1);
	request->SocketWrite("</TD></TR>\n");
	//plugin name
	request->SocketWrite("<tr id=\"PluginNameRow\" style=\"visibility:hidden\"><TD colspan=2>\n");
	request->printf("Plugin <INPUT TYPE=\"text\" name=\"PluginName\" value=\"\" size=20 maxlength=%d>\n",EXEC_PLUGIN_NAME_MAXLEN-1);
	request->SocketWrite("</TD></TR>\n");
	// Buttons
	request->SocketWrite("<TD align=\"center\"><INPUT TYPE=\"submit\" value=\"OK\"></form>\n"
	                     "<TD align=\"center\"><form method=\"GET\" action=\"/fb/timer.dbox2\">\n"
	                     "<INPUT TYPE=\"hidden\" NAME=\"action\" VALUE=\"none\">\n"
	                     "<INPUT TYPE=\"submit\" value=\"CANCEL\"></form></TD>\n"
	                     "</TABLE></TABLE>\n");
	request->SendHTMLFooter();
}

//------------------------------------------------------------------------
void CWebAPI::doNewTimer(CWebserverRequest *request)
{
	time_t	announceTimeT = 0,
		stopTimeT = 0,
		alarmTimeT = 0,
		tnull = 0;
	unsigned int repCount = 0;
	int alHour=0;

	if(request->ParameterList["alarm"] != "")		// wenn alarm angegeben dann parameter im time_t format
	{
		alarmTimeT = atoi(request->ParameterList["alarm"].c_str());
		if(request->ParameterList["stop"] != "")
			stopTimeT = atoi(request->ParameterList["stop"].c_str());
		if(request->ParameterList["announce"] != "")
			announceTimeT = atoi(request->ParameterList["announce"].c_str());
		else
			announceTimeT = alarmTimeT;
	}
	else if(request->ParameterList["alDate"] != "") //given formatted
	{
		// Alarm Date - Format exact! DD.MM.YYYY
		tnull = time(NULL);
		struct tm *alarmTime=localtime(&tnull);
		alarmTime->tm_sec = 0;
		if(sscanf(request->ParameterList["alDate"].c_str(),"%2d.%2d.%4d",&(alarmTime->tm_mday), &(alarmTime->tm_mon), &(alarmTime->tm_year)) == 3)
		{
			alarmTime->tm_mon -= 1;
			alarmTime->tm_year -= 1900;
		}

		// Alarm Time - Format exact! HH:MM
		if(request->ParameterList["alTime"] != "")
			sscanf(request->ParameterList["alTime"].c_str(),"%2d.%2d",&(alarmTime->tm_hour), &(alarmTime->tm_min));
	
		correctTime(alarmTime);
		alarmTimeT = mktime(alarmTime);
		announceTimeT = alarmTimeT;
		struct tm *stopTime = localtime(&alarmTimeT);
		stopTime->tm_sec = 0;
		// Stop Time - Format exact! HH:MM
		if(request->ParameterList["stTime"] != "")
			sscanf(request->ParameterList["stTime"].c_str(),"%2d.%2d",&(stopTime->tm_hour), &(stopTime->tm_min));
	
		// Stop Date - Format exact! DD.MM.YYYY
		if(request->ParameterList["stDate"] != "")
			if(sscanf(request->ParameterList["stDate"].c_str(),"%2d.%2d.%4d",&(stopTime->tm_mday), &(stopTime->tm_mon), &(stopTime->tm_year)) == 3)
			{
				stopTime->tm_mon -= 1;
				stopTime->tm_year -= 1900;
			}
		correctTime(stopTime);
		stopTimeT = mktime(stopTime);
		alHour = alarmTime->tm_hour;
		if(request->ParameterList["stDate"] == "" && alHour > stopTime->tm_hour)
			stopTimeT += 24* 60 * 60; // add 1 Day
	}
	else	// sonst formular-parameter parsen
	{
		time_t now = time(NULL);
		struct tm *alarmTime=localtime(&now);
		if(request->ParameterList["ad"] != "")
		{
			alarmTime->tm_mday = atoi(request->ParameterList["ad"].c_str());
		}
		if(request->ParameterList["amo"] != "")
		{
			alarmTime->tm_mon = atoi(request->ParameterList["amo"].c_str())-1;
		}
		if(request->ParameterList["ay"] != "")
		{
			alarmTime->tm_year = atoi(request->ParameterList["ay"].c_str())-1900;
		}
		if(request->ParameterList["ah"] != "")
		{
			alarmTime->tm_hour = atoi(request->ParameterList["ah"].c_str());
		}
		if(request->ParameterList["ami"] != "")
		{
			alarmTime->tm_min = atoi(request->ParameterList["ami"].c_str());
		}
		correctTime(alarmTime);
		alarmTimeT = mktime(alarmTime);
		announceTimeT = alarmTimeT;

		struct tm *stopTime = alarmTime;
		if(request->ParameterList["sd"] != "")
		{
		  stopTime->tm_mday = atoi(request->ParameterList["sd"].c_str());
		}
		if(request->ParameterList["smo"] != "")
		{
		  stopTime->tm_mon = atoi(request->ParameterList["smo"].c_str())-1;
		}
		if(request->ParameterList["sy"] != "")
		{
		  stopTime->tm_year = atoi(request->ParameterList["sy"].c_str())-1900;
		}
		if(request->ParameterList["sh"] != "")
		{
		  stopTime->tm_hour = atoi(request->ParameterList["sh"].c_str());
		}
		if(request->ParameterList["smi"] != "")
		{
		  stopTime->tm_min = atoi(request->ParameterList["smi"].c_str());
		}
		correctTime(stopTime);
		stopTimeT = mktime(stopTime);
	}
		
	if(announceTimeT != 0) 
		announceTimeT -= 60;
		
	CTimerd::CTimerEventTypes type;
	if(request->ParameterList["type"] != "")
		type  = (CTimerd::CTimerEventTypes) atoi(request->ParameterList["type"].c_str());
	else // default is: record
		type = CTimerd::TIMER_RECORD;
	
	// repeat
	if(request->ParameterList["repcount"] != "")
	{
		repCount = atoi(request->ParameterList["repcount"].c_str());
	}
	CTimerd::CTimerEventRepeat rep;
	if(request->ParameterList["rep"] != "")
		rep = (CTimerd::CTimerEventRepeat) atoi(request->ParameterList["rep"].c_str());
	else // default: no repeat
		rep = (CTimerd::CTimerEventRepeat)0;
	if(((int)rep) >= ((int)CTimerd::TIMERREPEAT_WEEKDAYS) && request->ParameterList["wd"] != "")
		Parent->Timerd->getWeekdaysFromStr((int*)&rep, request->ParameterList["wd"].c_str());

	CTimerd::RecordingInfo recinfo;
	CTimerd::EventInfo eventinfo;
	eventinfo.epgID = 0;
	eventinfo.epg_starttime = 0;
	eventinfo.apids = TIMERD_APIDS_CONF;
	eventinfo.recordingSafety = (request->ParameterList["rs"] == "1");

	// channel by Id or name
	if(request->ParameterList["channel_id"] != "")
		sscanf(request->ParameterList["channel_id"].c_str(),
		SCANF_CHANNEL_ID_TYPE,
		&eventinfo.channel_id);
	else
		eventinfo.channel_id = Parent->ChannelNameToChannelId(request->ParameterList["channel_name"]);
	       
	void *data=NULL;
	if(type == CTimerd::TIMER_RECORD)
		announceTimeT-=120;
	if(type == CTimerd::TIMER_STANDBY)
	{
		bool standby_on = (request->ParameterList["sbon"]=="1");
		data=&standby_on;
	}
	else if(type==CTimerd::TIMER_NEXTPROGRAM || type==CTimerd::TIMER_ZAPTO)
		data= &eventinfo;
	else if (type==CTimerd::TIMER_RECORD)
	{
		std::string _rec_dir = request->ParameterList["rec_dir"];
		if(_rec_dir == "")
		{
			// get Default Recordingdir
			CConfigFile *Config = new CConfigFile(',');
			Config->loadConfig(NEUTRINO_CONF);
			_rec_dir = Config->getString("network_nfs_recordingdir", "/mnt/filme");
		}
		recinfo = eventinfo;
		strncpy(recinfo.recordingDir, _rec_dir.c_str(), RECORD_DIR_MAXLEN-1);
		data = &recinfo;
	}
	else if(type==CTimerd::TIMER_REMIND)
	{
		char msg[REMINDER_MESSAGE_MAXLEN];
		memset(msg, 0, sizeof(msg));
		strncpy(msg, request->ParameterList["msg"].c_str(),REMINDER_MESSAGE_MAXLEN-1);
		data=msg;
	}
	else if(type==CTimerd::TIMER_EXEC_PLUGIN)
	{
		char msg[EXEC_PLUGIN_NAME_MAXLEN];
		memset(msg, 0, sizeof(msg));
		strncpy(msg, request->ParameterList["PluginName"].c_str(),EXEC_PLUGIN_NAME_MAXLEN-1);
		data=msg;
	}
	// update or add timer
	if(request->ParameterList["update"]=="1")
	{
		CTimerd::TimerList timerlist;
		timerlist.clear();
		Parent->Timerd->getTimerList(timerlist);
		CTimerd::TimerList::iterator timer = timerlist.begin();
		
		// Look for Recording Safety Timers too
		time_t real_alarmTimeT = alarmTimeT;
		if(eventinfo.recordingSafety)
		{
			int pre,post;
			Parent->Timerd->getRecordingSafety(pre,post);
			real_alarmTimeT -= pre;
		}
		
		for(; timer != timerlist.end();timer++)
			if(timer->alarmTime == real_alarmTimeT)
			{
				Parent->Timerd->removeTimerEvent(timer->eventID);
				break;
			}
	}
	Parent->Timerd->addTimerEvent(type,data,announceTimeT,alarmTimeT,stopTimeT,rep,repCount);
}

void CWebAPI::timeString(time_t time, char string[6])
{
	struct tm *tm = localtime(&time);
	if (!strftime(string, 6, "%H:%M", tm))
		sprintf(string, "??:??");
}
