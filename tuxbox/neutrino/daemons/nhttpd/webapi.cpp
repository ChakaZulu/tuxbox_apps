/*
	webserver  -   DBoxII-Project

	Copyright (C) 2001/2002 Dirk Szymanski 'Dirch'

	$Id: webapi.cpp,v 1.6 2002/10/03 19:05:12 thegoodguy Exp $

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
#include "webapi.h"
#include "debug.h"
#define dprintf(fmt, args...) {if(Parent->Parent->DEBUG) aprintf( "[nhttpd] " fmt, ## args);}

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
		case 2:	return Info(request);
			break;
		case 3:	return Dbox(request);
			break;
		case 4:	return Bouquetlist(request);
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
   request->SocketWrite("<html><script language=\"JavaScript\">location.href=\"/fb/timer.dbox2\"</script></html>\n");
}

//-------------------------------------------------------------------------
bool CWebAPI::Timer(CWebserverRequest* request)
// timer functions
{

	request->SendPlainHeader("text/html");
	if(Parent->Timerd->isTimerdAvailable())
	{
		if((request->ParameterList.size() > 0))
		{
			if(request->ParameterList["action"] == "remove")
			{
				unsigned removeId = atoi(request->ParameterList["id"].c_str());
				Parent->Timerd->removeTimerEvent(removeId);
				loadTimerMain(request);
			}
			else if(request->ParameterList["action"] == "modify-form")
			{
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
				newTimerForm(request);
			}
			else if(request->ParameterList["action"] == "new")
			{
				doNewTimer(request);
				loadTimerMain(request);
			}
			else
			{
				request->SendHTMLHeader("UNKNOWN ACTION");
				aprintf("Unknown action : %s\n",request->ParameterList["action"].c_str());
				request->SendHTMLFooter ();
			}
		}
		else
			ShowTimerList(request);
	}
	else
	{
		request->SendHTMLHeader ("Error");
		aprintf("<h1>Error: Timerd not available</h1>\n");
		request->SendHTMLFooter ();
	}
	return true;
}

//-------------------------------------------------------------------------
bool CWebAPI::Info(CWebserverRequest* request)
// shows informations about current stream
{
	request->SendPlainHeader("text/html");		
	ShowCurrentStreamInfo(request);
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

/*
	else if(operation == 4)		// send services.xml
	{
		request->SendPlainHeader("text/xml");
		request->SendFile(Parent->Zapit_XML_Path,"services.xml");
		request->HttpStatus = 200;
		return true;
	}

	else if(operation == 5)		// send bouquets.xml
	{
		request->SendPlainHeader("text/xml");
		request->SendFile(Parent->Zapit_XML_Path,"bouquets.xml");
		request->HttpStatus = 200;
		return true;
	}
*/
//-------------------------------------------------------------------------
bool CWebAPI::Bouquetlist(CWebserverRequest* request)
// show the bouquet list
{
	request->SendPlainHeader("text/html");
	ShowBouquets(request,(request->ParameterList["bouquet"] != "")?atoi(request->ParameterList["bouquet"].c_str()):0);
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

	if (request->ParameterList.size() > 0)							// parse the parameters first
	{
		if( request->ParameterList["1"] == "volumemute")
		{
			if(!request->Authenticate())
				return false;
			bool mute = Parent->Controld->getMute();
			Parent->Controld->setMute( !mute );
			dprintf("mute\n");
		}
		else if( request->ParameterList["1"] == "volumeplus")
		{
			if(!request->Authenticate())
				return false;
			char vol = Parent->Controld->getVolume();
			vol+=5;
			if (vol>100)
				vol=100;
			Parent->Controld->setVolume(vol);
			dprintf("Volume plus: %d\n",vol);
		}
		else if( request->ParameterList["1"] == "volumeminus")
		{
			if(!request->Authenticate())
				return false;
			char vol = Parent->Controld->getVolume();
			if (vol>0)
				vol-=5;
			Parent->Controld->setVolume(vol);
			dprintf("Volume minus: %d\n",vol);
		}
		else if( request->ParameterList["standby"] != "")
		{
			if(request->ParameterList["standby"] == "on")
			{
				Parent->EventServer->sendEvent(NeutrinoMessages::STANDBY_ON, CEventServer::INITID_THTTPD);
//				standby_mode = true;
			}
			if(request->ParameterList["standby"] == "off")
			{
				Parent->EventServer->sendEvent(NeutrinoMessages::STANDBY_OFF, CEventServer::INITID_THTTPD);
//				standby_mode = false;
			}
		}
		else if(request->ParameterList["1"] == "tvmode")				// switch to tv mode
		{
			if(!request->Authenticate())
				return false;
			mode = NeutrinoMessages::mode_tv;
			Parent->EventServer->sendEvent(NeutrinoMessages::CHANGEMODE, CEventServer::INITID_THTTPD, (void *)&mode,sizeof(int));
			sleep(1);
			Parent->UpdateBouquets();
			request->Send302("channellist.dbox2#akt");
			return true;
		}
		else if(request->ParameterList["1"] == "radiomode")				// switch to radio mode
		{
			if(!request->Authenticate())
				return false;
			mode = NeutrinoMessages::mode_radio;
			Parent->EventServer->sendEvent(NeutrinoMessages::CHANGEMODE, CEventServer::INITID_THTTPD, (void *)&mode,sizeof(int));
			sleep(1);
			Parent->UpdateBouquets();
			request->Send302("channellist.dbox2#akt");
			return true;
		}

	}

	ShowControlpanel(request);									// show the controlpanel
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

	if(request->ParameterList.size() > 0)
	{											

		if(request->ParameterList["eventlist"] != "")				// what the hell has this to to here ?
		{															// TBD: move it here
			request->SendPlainHeader("text/html");
			unsigned id = atol( request->ParameterList["eventlist"].c_str() );
			ShowEventList( request, id );
			return true;
		}
		if(request->ParameterList["1"] == "eventlist")				// s.a.
		{
			request->SendPlainHeader("text/html");
			dprintf("service id: %u\n",Parent->Zapit->getCurrentServiceID());
			ShowEventList( request, Parent->Zapit->getCurrentServiceID() );
			return true;
		}

		if(request->ParameterList["eventid"] != "")
		{
			ShowEpg(request,request->ParameterList["eventid"]);
			return true;
		}
		else if(request->ParameterList["epgid"] != "")
		{
			ShowEpg(request,request->ParameterList["epgid"],request->ParameterList["startzeit"]);
			return true;
		}
	}
	dperror("[THTTPD] Get epgid error\n");
	return false;
}

//-------------------------------------------------------------------------
bool CWebAPI::Switch(CWebserverRequest* request)
// switch something
{
	if(request->ParameterList.size() > 0)
	{

		if(request->ParameterList["zapto"] != "")					// zap to channel and redirect to channel/bouquet list
		{
			if(!request->Authenticate())
				return false;

			Parent->ZapTo(request->ParameterList["zapto"]);
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
			request->SendFile(Parent->Parent->PrivateDocumentRoot,"/shutdown.html");	// send shutdown page
			request->EndRequest();
			sleep(1);															// wait 
			Parent->EventServer->sendEvent(NeutrinoMessages::SHUTDOWN, CEventServer::INITID_THTTPD);
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
	request->ParseFile(Parent->Parent->PrivateDocumentRoot + "/dbox.html",params);
	return true;
}


//-------------------------------------------------------------------------
bool CWebAPI::ShowCurrentStreamInfo(CWebserverRequest* request)
{
	int bitInfo[10];
	char buf[100];
	CStringList params;
	CZapitClient::CCurrentServiceInfo serviceinfo;

	serviceinfo = Parent->Zapit->getCurrentServiceInfo();
	params["onid"] = itoh(serviceinfo.onid);
	params["sid"] = itoh(serviceinfo.sid);
	params["tsid"] = itoh(serviceinfo.tsid);
	params["vpid"] = itoh(serviceinfo.vdid);
	params["apid"] = itoh(serviceinfo.apid);
	params["vtxtpid"] = itoh(serviceinfo.vtxtpid);
	params["tsfrequency"] = itoa(serviceinfo.tsfrequency);
	params["polarisation"] = serviceinfo.polarisation==1?"v":"h";
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

	switch ( bitInfo[6] )
	{
		case 1: params["AudioType"] = "single channel"; break;
		case 2: params["AudioType"] = "dual channel"; break;
		case 3: params["AudioType"] = "joint stereo"; break;
		case 4: params["AudioType"] = "stereo"; break;
		default: params["AudioType"] = "unknown";
	}
	request->ParseFile(Parent->Parent->PrivateDocumentRoot + "/settings.html",params);
	return true;
}

//-------------------------------------------------------------------------
bool CWebAPI::ShowEventList(CWebserverRequest *request,t_channel_id channel_id)
{
char *buf = new char[1400];
char classname;
int pos = 0;
	
	Parent->eList = Parent->Sectionsd->getEventsServiceKey(channel_id);
	CChannelEventList::iterator eventIterator;
	request->SendHTMLHeader("DBOX2-Neutrino Channellist");


	request->SocketWriteLn("<H3 CLASS=\"epg\">Programmvorschau: " + Parent->GetServiceName(channel_id) + "</H3>");

	request->SocketWrite("<TABLE WIDTH=\"90%\" CELLSPACING=\"0\">\n");

    for( eventIterator = Parent->eList.begin(); eventIterator != Parent->eList.end(); eventIterator++, pos++ )
	{
		classname = (pos&1)?'a':'b';
		char zbuffer[25] = {0};
		struct tm *mtime = localtime(&eventIterator->startTime); //(const time_t*)eventIterator->startTime);
		strftime(zbuffer,20,"%d.%m. %H:%M",mtime);
		sprintf(buf,"<TR VALIGN=\"top\" HEIGHT=\"%d\" CLASS=\"%c\">\n",(eventIterator->duration > 20 * 60)?(eventIterator->duration / 60):20 , classname);
		request->SocketWrite(buf); 
		sprintf(buf,"<TD><A HREF=\"/fb/timer.dbox2?action=new&type=%d&alarm=%u&channel_id=%u\">&nbsp;<IMG SRC=\"/images/timer.gif\" WIDTH=\"21\" HEIGHT=\"21\" BORDER=0 ALT=\"Timer setzen\"></A>&nbsp;</TD>\n",CTimerEvent::TIMER_ZAPTO,(uint) eventIterator->startTime,channel_id); 
		request->SocketWrite(buf);
		sprintf(buf, "<TD><NOBR>%s&nbsp;<font size=\"-2\">(%d min)</font>&nbsp;</NOBR></TD>\n", zbuffer, eventIterator->duration / 60);
		sprintf(&buf[strlen(buf)], "<TD><A HREF=epg.dbox2?eventid=%llx>%s</A></TD>\n</TR>\n", eventIterator->eventID, eventIterator->description.c_str());
		request->SocketWrite(buf);
	}
	delete[] buf;
	request->SocketWriteLn("</TABLE>");
	request->SendHTMLFooter();
	return true;
}
//-------------------------------------------------------------------------

bool CWebAPI::ShowBouquets(CWebserverRequest *request, unsigned int BouquetNr)
{
char *buffer = new char[300];
string classname;
	
	request->SocketWriteLn("<html>\n<head><title>DBOX2-Neutrino Bouquetliste</title><link rel=\"stylesheet\" type=\"text/css\" href=\"../global.css\">");
	request->SocketWrite("<SCRIPT LANGUAGE=\"JavaScript\">\n<!--\n function goto(url1, url2)\n{\n parent.frames[1].location.href=url1;\n parent.frames[2].location.href=url2;\n }\n//-->\n </SCRIPT>\n</HEAD><BODY>");

	request->SocketWriteLn("<table cellspacing=0 cellpadding=0 border=0 width=\"100%\">");
	request->SocketWriteLn("<tr><td><A HREF=\"/bouquetedit/main\" TARGET=\"content\">Bouqueteditor</A></td></tr>\n<tr><td><HR></td></tr>");
	classname = (BouquetNr == 0)?" class=\"bouquet\"":"";
	request->SocketWrite("<tr height=20"+ classname + "><td><a class=bouquets href=\"javascript:goto('/fb/channellist.dbox2#akt','/fb/bouquetlist.dbox2?bouquet=0')\">Alle Kanäle</a></td></tr>\n");
	request->SocketWrite("<tr><td><HR></td></tr>\n");
	CZapitClient::BouquetList::iterator bouquet = Parent->BouquetList.begin();
	for(; bouquet != Parent->BouquetList.end();bouquet++)
	{
		classname = ((bouquet->bouquet_nr + 1) == (uint) BouquetNr)?" class=\"bouquet\"":"";
		sprintf(buffer,"<tr height=\"20\"%s><td><a class=bouquets href=\"javascript:goto('/fb/channellist.dbox2?bouquet=%d#akt','/fb/bouquetlist.dbox2?bouquet=%d');\">%s</a></td></tr>\n",classname.c_str(),(bouquet->bouquet_nr + 1),(bouquet->bouquet_nr + 1),bouquet->name);
		request->SocketWrite(buffer);
	}
	request->SocketWrite("</table>\n");
	request->SendHTMLFooter();
	delete[] buffer;
	return true;
}

//-------------------------------------------------------------------------
bool CWebAPI::ShowBouquet(CWebserverRequest* request, int BouquetNr)
{
	dprintf("ShowBouquet\n");
	CZapitClient::BouquetChannelList *channellist;
	if(BouquetNr > 0)
		channellist = &(Parent->BouquetsList[BouquetNr]);
	else
		channellist = &Parent->ChannelList;

	Parent->GetChannelEvents();

	request->SendHTMLHeader("DBOX2-Neutrino Kanalliste");

	request->SocketWriteLn("<table cellspacing=0 border=0>");

	int i = 1;
	string classname;
	char *buffer = new char[400];
	t_channel_id current_channel = Parent->Zapit->getCurrentServiceID();
	int prozent;

	CZapitClient::BouquetChannelList::iterator channel = channellist->begin();
	for(; channel != channellist->end();channel++)
	{
		classname = (i++&1)?'a':'b';
		if(channel->channel_id == current_channel)
			classname = 'c';


		string bouquetstr = (BouquetNr >=0)?"&bouquet="+itoa(BouquetNr):"";
		
		request->SocketWrite("<tr><td colspan=2 class=\""+string(classname)+"\">");
		sprintf(buffer,"%s<a href=\"switch.dbox2?zapto=%d%s\">%d. %s</a>&nbsp;<a href=\"epg.dbox2?eventlist=%u\">%s</a></td></tr>",((channel->channel_id == current_channel)?"<a name=akt></a>":" "),channel->channel_id,bouquetstr.c_str(),channel->nr,channel->name,channel->channel_id,((Parent->ChannelListEvents[channel->channel_id])?"<img src=\"../images/elist.gif\" border=\"0\" alt=\"Programmvorschau\">":""));
		request->SocketWriteLn(buffer);

		CChannelEvent *event = Parent->ChannelListEvents[channel->channel_id];
		if(event)
		{
			prozent = 100 * (time(NULL) - event->startTime) / event->duration;
			request->SocketWrite("<tr><td align=left width=31 class=\""+ string(classname) +"epg\">");
			sprintf(buffer,"<table border=1 rules=none bordercolor=#000000 heigth=10 width=30 cellspacing=0 cellpadding=0><tr><td bgcolor=\"#0000FF\" height=10 width=%d></td><td bgcolor=\"#EAEBFF\" heigth=10 width=%d></td></tr></table></td>",(prozent / 10) * 3,(10 - (prozent / 10))*3);
			request->SocketWrite(buffer);
			request->SocketWrite("<td class=\""+ string(classname) +"epg\">");
			sprintf(buffer,"<a href=epg.dbox2?epgid=%llx&startzeit=%lx>",event->eventID,event->startTime);
			request->SocketWrite(buffer);
			request->SocketWrite((char *) event->description.c_str());
			request->SocketWrite("&nbsp;"); 
			sprintf(buffer,"<font size=-3><nobr>(%ld von %d min, %d%%)</nobr></font></a>&nbsp;</td></tr>\n",(time(NULL) - event->startTime)/60,event->duration / 60,prozent  ); 
			request->SocketWrite(buffer);
		}
	}

	request->SocketWriteLn("</table>");

	request->SendHTMLFooter();
	delete[] buffer;
	return true;
}
//-------------------------------------------------------------------------

bool CWebAPI::ShowControlpanel(CWebserverRequest* request)
{

	if(Parent->Parent->NewGui)
	{
		CStringList params;
//		params["standby"] = standby_mode?"off":"on";
		switch(Parent->Controld->getBoxType())
		{
			case CControldClient::BOXTYPE_NOKIA :			// show the nokia rc
				params["BoxType"] = "<img src=\"/images/nokia.gif\" usemap=\"#nokia\" border=0>";
				break;
			default :										// show sagem / phillips rc
				params["BoxType"] = "<img src=\"/images/sagem.gif\" usemap=\"#sagem\" border=0>";
		}
		request->ParseFile(Parent->Parent->PrivateDocumentRoot + "/controlpanel.html", params);
	}
	else
	{
	char volstr_on[10];
	char volstr_off[10];

		request->SendPlainHeader("text/html");

		string mutefile = Parent->Controld->getMute()?"mute":"muted";
		string mutestring = "<td><a href=\"/fb/controlpanel.dbox2?volumemute\" target=navi onMouseOver=\"mute.src='../images/"+ mutefile+"_on.jpg';\" onMouseOut=\"mute.src='../images/"+ mutefile+"_off.jpg';\"><img src=/images/"+ mutefile+"_off.jpg width=25 height=28 border=0 name=mute></a><br></td>\n";

		char vol = Parent->Controld->getVolume();
		sprintf((char*) &volstr_on, "%d", vol);
		sprintf((char*) &volstr_off, "%d", 100-vol);
	 
		request->SendFile(Parent->Parent->PrivateDocumentRoot,"/controlpanel.include1");
		//muted
		request->SocketWrite(mutestring);
		request->SendFile(Parent->Parent->PrivateDocumentRoot,"/controlpanel.include2");
		//volume bar...
		request->SocketWrite("<td><img src=../images/vol_flashed.jpg width=");
		request->SocketWrite(volstr_on);
		request->SocketWrite(" height=10 border=0><br></td>\n");
		request->SocketWrite("<td><img src=../images/vol_unflashed.jpg width=");
		request->SocketWrite(volstr_off);
		request->SocketWrite(" height=10 border=0><br></td>\n");
		request->SendFile(Parent->Parent->PrivateDocumentRoot,"/controlpanel.include3");
	}
	return true;

}

//-------------------------------------------------------------------------
bool CWebAPI::ShowEPG(CWebserverRequest *request,string Title, string Info1, string Info2)
{
	CStringList params;
	params["Title"] = (Title != "")?Title:"Kein EPG vorhanden";
	params["Info1"] = (Info1 != "")?Info1:"keine ausführlichen Informationen verfügbar";
	params["Info2"] = (Info2 != "")?Info2:" ";
	request->ParseFile(Parent->Parent->PrivateDocumentRoot + "/epg.html",params);
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
bool CWebAPI::ShowEpg(CWebserverRequest *request,string EpgID,string Startzeit)
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

void CWebAPI::correctTime(struct tm *zt)
{
   if(zt->tm_year>129)
      zt->tm_year=129;
   if(zt->tm_year<0)
      zt->tm_year=0;
   if(zt->tm_mon>11)
      zt->tm_mon=11;
   if(zt->tm_mon<0)
      zt->tm_mon=0;
   if(zt->tm_mday>31) //-> eine etwas laxe pruefung, aber mktime biegt das wieder grade
      zt->tm_mday=31;
   if(zt->tm_mday<1)
      zt->tm_mday=1;
   if(zt->tm_hour>23)
      zt->tm_hour=23;
   if(zt->tm_hour<0)
      zt->tm_hour=0;
   if(zt->tm_min>59)
      zt->tm_min=59;
   if(zt->tm_min<0)
      zt->tm_min=0;
   if(zt->tm_sec>59)
      zt->tm_sec=59;
   if(zt->tm_sec<0)
      zt->tm_sec=0;
}
//-------------------------------------------------------------------------
bool CWebAPI::ShowTimerList(CWebserverRequest* request)
{
char *buffer = new char[300];

   CTimerd::TimerList timerlist;             // List of bouquets


   timerlist.clear();
   Parent->Timerd->getTimerList(timerlist);

   CZapitClient::BouquetChannelList channellist;     
   channellist.clear();

   request->SendHTMLHeader("TIMERLIST");
   request->SocketWrite("<center>\n");
   request->SocketWrite("<table border=0>\n");
   request->SocketWrite("<tr>\n");
   request->SocketWrite("<td class=\"cepg\" align=\"left\"><b>Alarm Time</td>\n");
   request->SocketWrite("<td class=\"cepg\" align=\"left\"><b>Stop Time</td>\n");
   request->SocketWrite("<td class=\"cepg\" align=\"left\"><b>Repeat</td>\n");
   request->SocketWrite("<td class=\"cepg\" align=\"left\"><b>Type</td>\n");
   request->SocketWrite("<td class=\"cepg\" align=\"left\"><b>Add. Data</td>\n");
   request->SocketWrite("<td class=\"cepg\"><td class=\"cepg\"></tr>\n");

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

     // sprintf(buffer, "<tr><td class=\"%cepg\" align=center>%d</td>",classname, timer->eventID);
	  //request->SocketWrite(buffer);
      sprintf(buffer, "<tr><td class=\"%cepg\" align=left>%s</td>", classname, zAlarmTime);
  	  request->SocketWrite(buffer);
      sprintf(buffer, "<td class=\"%cepg\" align=left>%s</td>", classname, zStopTime);
  	  request->SocketWrite(buffer);
      char zRep[20+1];
      Parent->timerEventRepeat2Str(timer->eventRepeat,zRep,sizeof(zRep)-1);
      sprintf(buffer,"<td class=\"%cepg\" align=left>%s</td>", classname, zRep);
  	  request->SocketWrite(buffer);
      char zType[20+1];
      Parent->timerEventType2Str(timer->eventType,zType,sizeof(zType)-1);
      sprintf(buffer, "<td class=\"%cepg\" align=left>%s", classname, zType);
  	  request->SocketWrite(buffer);

      // Add Data
      char zAddData[20+1]={0};
      switch(timer->eventType)
      {
         case CTimerEvent::TIMER_NEXTPROGRAM :
         case CTimerEvent::TIMER_ZAPTO :
         case CTimerEvent::TIMER_RECORD :
            {
               if(channellist.size()==0)
               {
                  Parent->Zapit->getChannels(channellist);
               }
               CZapitClient::BouquetChannelList::iterator channel = channellist.begin();
               for(; channel != channellist.end();channel++)
               {
                  if (channel->channel_id == timer->channel_id)
                  {
                     strncpy(zAddData, channel->name, 20);
                     zAddData[20]=0;
                     break;
                  }
               }
               if(channel == channellist.end())
                  strcpy(zAddData,"Unknown");
            }
            break;
         case CTimerEvent::TIMER_STANDBY :
            {
               sprintf(zAddData,"Standby: %s",(timer->standby_on ? "ON" : "OFF"));
            }
            break;
         default:{}
      }
      sprintf(buffer, "<td class=\"%cepg\" align=left>%s\n",
             classname, zAddData);
	  request->SocketWrite(buffer);
      sprintf(buffer, "<td class=\"%cepg\" align=center><a href=\"/fb/timer.dbox2?action=remove&id=%d\">\n",
             classname, timer->eventID);
	  request->SocketWrite(buffer);
   	  request->SocketWrite("<img src=\"../images/remove.gif\" alt=\"Timer löschen\" border=0></a></td>\n");
      sprintf(buffer, "<td class=\"%cepg\" align=center><a href=\"/fb/timer.dbox2?action=modify-form&id=%d\">", 
				  classname, timer->eventID);
	  request->SocketWrite(buffer);
      sprintf(buffer,"<img src=\"../images/modify.gif\" alt=\"Timer ändern\" border=0></a><nobr></td><tr>\n");
	  request->SocketWrite(buffer);

   }
   classname = (i++&1)?'a':'b';
   sprintf(buffer, "<tr><td class=\"%cepg\" colspan=5></td>\n<td class=\"%cepg\" align=\"center\">\n",classname,classname);
   request->SocketWrite(buffer);
   request->SocketWrite("<a href=\"javascript:location.reload()\">\n");
   request->SocketWrite("<img src=\"../images/reload.gif\" alt=\"Aktualisieren\" border=0></a></td>\n");   
	sprintf(buffer, "<td class=\"%cepg\" align=\"center\">\n",classname);
   request->SocketWrite(buffer);
   request->SocketWrite("<a href=\"/fb/timer.dbox2?action=new-form\">\n");
   request->SocketWrite("<img src=\"../images/new.gif\" alt=\"neuer Timer\" border=0></a></td></tr>\n");
   request->SocketWrite("</table>\n");
   request->SendHTMLFooter();
   delete[] buffer;
   return true;
}
//-------------------------------------------------------------------------
void CWebAPI::modifyTimerForm(CWebserverRequest *request, unsigned timerId)
{
	char *buffer = new char[300];
	CTimerd::responseGetTimer timer;             // Timer


	Parent->Timerd->getTimer(timer, timerId);

	char zType[20+1];
	Parent->timerEventType2Str(timer.eventType,zType,20);

	request->SendHTMLHeader("MODIFY TIMER" + timerId);
	request->SocketWrite("<center>");
	request->SocketWrite("<table border=2 ><tr class=\"a\"><td>\n");
	request->SocketWrite("<form method=\"GET\" name=\"modify\" action=\"/fb/timer.dbox2\">\n");
	request->SocketWrite("<input type=\"hidden\" name=\"action\" value=\"modify\">\n");
	sprintf(buffer,"<input name=\"id\" type=\"hidden\" value=\"%d\">\n",timerId);
	request->SocketWrite(buffer);
	request->SocketWrite("<table border=0 >\n");
	sprintf(buffer,"<tr class=\"c\"><td colspan=\"2\" align=\"center\">MODIFY TIMER %d - %s</td></tr>\n",
		  timerId,zType);
	request->SocketWrite(buffer);
	struct tm *alarmTime = localtime(&(timer.alarmTime));
	sprintf(buffer,"<tr><td align=\"right\"><nobr>alarm date: <input type=\"text\" name=\"ad\" value=\"%02d\" size=2 maxlength=2>. ",
		  alarmTime->tm_mday );
	request->SocketWrite(buffer);
	sprintf(buffer,"<input type=\"text\" name=\"amo\" value=\"%02d\" size=2 maxlength=2>.&nbsp",
		  alarmTime->tm_mon +1);
	request->SocketWrite(buffer);
	sprintf(buffer,"<input type=\"text\" name=\"ay\" value=\"%04d\" size=4 maxlength=4></td>\n",
		  alarmTime->tm_year + 1900);
	request->SocketWrite(buffer);
	sprintf(buffer,"<td align=\"center\"><nobr>time:&nbsp;<input type=\"text\" name=\"ah\" value=\"%02d\" size=2 maxlength=2>&nbsp;:&nbsp;",
		  alarmTime->tm_hour );
	request->SocketWrite(buffer);
	sprintf(buffer,"<input type=\"text\" name=\"ami\" value=\"%02d\" size=2 maxlength=2></td></tr>\n",
		  alarmTime->tm_min);
	request->SocketWrite(buffer);
	if(timer.stopTime > 0)
	{
		struct tm *stopTime = localtime(&(timer.stopTime));
		sprintf(buffer,"<tr><nobr><td align=\"right\"><nobr>stop&nbsp;date:&nbsp;<input type=\"text\" name=\"sd\" value=\"%02d\" size=2 maxlength=2>.&nbsp;",
			 stopTime->tm_mday );
		request->SocketWrite(buffer);
		sprintf(buffer,"<input type=\"text\" name=\"smo\" value=\"%02d\" size=2 maxlength=2>.&nbsp;",
			 stopTime->tm_mon +1);
		request->SocketWrite(buffer);
		sprintf(buffer,"<input type=\"text\" name=\"sy\" value=\"%04d\" size=4 maxlength=4></td>\n",
			 stopTime->tm_year + 1900);
		request->SocketWrite(buffer);
		sprintf(buffer,"<td align=\"center\"><nobr>time:&nbsp;<input type=\"text\" name=\"sh\" value=\"%02d\" size=2 maxlength=2>&nbsp;:&nbsp;",
			 stopTime->tm_hour );
		request->SocketWrite(buffer);
		sprintf(buffer,"<input type=\"text\" name=\"smi\" value=\"%02d\" size=2 maxlength=2></td></tr>\n",
			 stopTime->tm_min);
		request->SocketWrite(buffer);
	}
	request->SocketWrite("<TR><td align=\"center\">repeat\n");
	request->SocketWrite("<select name=\"rep\">\n");
	for(int i=0; i<=6;i++)
	{
		char zRep[21];
		Parent->timerEventRepeat2Str((CTimerEvent::CTimerEventRepeat) i, zRep, sizeof(zRep)-1);
		sprintf(buffer,"<option value=\"%d\"",i);
		request->SocketWrite(buffer);
		if(((int)timer.eventRepeat) == i)
		{
			sprintf(buffer," selected");
			request->SocketWrite(buffer);
		}
		sprintf(buffer,">%s\n",zRep);
		request->SocketWrite(buffer);
	}
	request->SocketWrite("</select></TD></TR>\n");

	request->SocketWrite("<tr><td colspan=2 height=10></tr>\n");
	request->SocketWrite("<tr><td><center><input type=\"submit\" value=\"OK\"></center></td>\n");
	request->SocketWrite("<td><center><form method=\"GET\" action=\"/fb/timer.dbox2\"><input type=\"submit\" value=\"CANCEL\"><form></center></td>\n");
	request->SocketWrite("</tr></table></form>");
	request->SendHTMLFooter();
	delete[] buffer;
}

//-------------------------------------------------------------------------
void CWebAPI::doModifyTimer(CWebserverRequest *request)
{
   unsigned modyId = atoi(request->ParameterList["id"].c_str());
   CTimerd::responseGetTimer timer;
   Parent->Timerd->getTimer(timer, modyId);

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
   correctTime(stopTime);
   time_t stopTimeT = mktime(stopTime);
   time_t announceTimeT = alarmTimeT-60;

   CTimerEvent::CTimerEventRepeat rep = 
   (CTimerEvent::CTimerEventRepeat) atoi(request->ParameterList["rep"].c_str());

   Parent->Timerd->modifyTimerEvent(modyId, announceTimeT, alarmTimeT, stopTimeT, rep);
}

//-------------------------------------------------------------------------
void CWebAPI::newTimerForm(CWebserverRequest *request)
{
	char *buffer = new char[300];
	request->SendHTMLHeader("NEW TIMER");
	// Javascript
	request->SocketWrite("<script language =\"javascript\">\n");
	request->SocketWrite("function my_show(id) {document.getElementById(id).style.visibility=\"visible\";}\n");
	request->SocketWrite("function my_hide(id) {document.getElementById(id).style.visibility=\"hidden\";}\n");
	request->SocketWrite("function focusNMark() { document.NewTimerForm.ad.select();\n");
	request->SocketWrite("                        document.NewTimerForm.ad.focus();}\n");
	request->SocketWrite("function onEventChange() { tType=document.NewTimerForm.type.value;\n");
	sprintf(buffer,"  if (tType == \"%d\") my_show(\"StopDateRow\"); else my_hide(\"StopDateRow\");\n",
		  (int)CTimerEvent::TIMER_RECORD);
	request->SocketWrite(buffer);
	sprintf(buffer,"  if (tType == \"%d\") my_show(\"StandbyRow\"); else my_hide(\"StandbyRow\");\n",
		  (int)CTimerEvent::TIMER_STANDBY);
	request->SocketWrite(buffer);
	sprintf(buffer,"  if (tType == \"%d\" || tType==\"%d\" || tType==\"%d\")\n",
		  (int)CTimerEvent::TIMER_RECORD, (int)CTimerEvent::TIMER_NEXTPROGRAM,
		  (int)CTimerEvent::TIMER_ZAPTO);
	request->SocketWrite(buffer);
	request->SocketWrite("     my_show(\"ProgramRow\"); else my_hide(\"ProgramRow\");\n");
	request->SocketWrite("  focusNMark();}\n");
	request->SocketWrite("</script>\n");
	// head of table
	request->SocketWrite("<center><table border=2 width=420><tr class=\"a\"><td>\n");
	// Form
	request->SocketWrite("<form method=\"GET\" action=\"/fb/timer.dbox2\" name=\"NewTimerForm\">\n");
	request->SocketWrite("<input type=\"hidden\" name=\"action\" value=\"new\">\n");
	request->SocketWrite("<table border=0 width=\"100%%\">\n");
	request->SocketWrite("<tr class=\"c\"><td colspan=\"2\" align=\"center\">NEW TIMER</td></tr>\n");
	// Timer type
	request->SocketWrite("<tr><td align=\"center\">timer type\n");
	request->SocketWrite("<select name=\"type\" onchange=\"onEventChange();\">\n");
	for(int i=1; i<=7;i++)
	{
		char zType[21];
		Parent->timerEventType2Str((CTimerEvent::CTimerEventTypes) i, zType, sizeof(zType)-1);
		sprintf(buffer,"<option value=\"%d\">%s\n",i,zType);
		request->SocketWrite(buffer);
	}
	request->SocketWrite("</select>\n");
	// timer repeat
	request->SocketWrite("<td align=\"center\">repeat\n");
	request->SocketWrite("<select name=\"rep\" onchange=\"focusNMark();\">\n");
	for(int i=0; i<=6;i++)
	{
		char zRep[21];
		Parent->timerEventRepeat2Str((CTimerEvent::CTimerEventRepeat) i, zRep, sizeof(zRep)-1);
		sprintf(buffer,"<option value=\"%d\">%s\n",i,zRep);
		request->SocketWrite(buffer);
	}
	request->SocketWrite("</select>\n");

	time_t now_t = time(NULL);
	struct tm *now=localtime(&now_t);
	// alarm day
	request->SocketWrite("<tr><td align=\"right\">\n");
	sprintf(buffer,"alarm date: <input type=\"text\" name=\"ad\" value=\"%02d\" size=2 maxlength=2>. \n",
		  now->tm_mday);
	request->SocketWrite(buffer);
	// alarm month
	sprintf(buffer,"<input type=\"text\" name=\"amo\" value=\"%02d\" size=2 maxlength=2>. \n",
		  now->tm_mon+1);
	request->SocketWrite(buffer);
	// alarm year
	sprintf(buffer,"<input type=\"text\" name=\"ay\" value=\"%04d\" size=4 maxlength=4>\n",
		  now->tm_year+1900);
	request->SocketWrite(buffer);
	// alarm time
	request->SocketWrite("</td><td align=\"center\">\n");
	sprintf(buffer,"time: <input type=\"text\" name=\"ah\" value=\"%02d\" size=2 maxlength=2> : \n",
		  now->tm_hour);
	request->SocketWrite(buffer);
	sprintf(buffer,"<input type=\"text\" name=\"ami\" value=\"%02d\" size=2 maxlength=2></td>\n",
		  now->tm_min);
	request->SocketWrite(buffer);
	// stop day
	sprintf(buffer,"</tr><tr id=\"StopDateRow\" style=\"visibility:hidden\"><td align=\"right\">\n");
	request->SocketWrite(buffer);
	sprintf(buffer,"stop date: <input type=\"text\" name=\"sd\" value=\"%02d\" size=2 maxlength=2>. \n",
		  now->tm_mday);
	request->SocketWrite(buffer);
	// stop month
	sprintf(buffer,"<input type=\"text\" name=\"smo\" value=\"%02d\" size=2 maxlength=2>. \n",
		  now->tm_mon+1);
	request->SocketWrite(buffer);
	// stop year
	sprintf(buffer,"<input type=\"text\" name=\"sy\" value=\"%04d\" size=4 maxlength=4>\n",
		  now->tm_year+1900);
	request->SocketWrite(buffer);
	request->SocketWrite("</td><td align=\"center\">\n");
	// stop time
	sprintf(buffer,"time: <input type=\"text\" name=\"sh\" value=\"%02d\" size=2 maxlength=2> : \n",
		  now->tm_hour);
	request->SocketWrite(buffer);
	sprintf(buffer,"<input type=\"text\" name=\"smi\" value=\"%02d\" size=2 maxlength=2></td></tr>\n",
		  now->tm_min);
	request->SocketWrite(buffer);
	// ONID-SID
	request->SocketWrite("<tr id=\"ProgramRow\" style=\"visibility:hidden\"><td colspan=2>\n");
	request->SocketWrite("<select name=\"channel_id\">\n");
	CZapitClient::BouquetChannelList channellist;     
	channellist.clear();
	Parent->Zapit->getChannels(channellist);
	t_channel_id current_channel = Parent->Zapit->getCurrentServiceID();
	CZapitClient::BouquetChannelList::iterator channel = channellist.begin();
	for(; channel != channellist.end();channel++)
	{
		sprintf(buffer,"<option value=\"%d\"",channel->channel_id);
		request->SocketWrite(buffer);
		if(channel->channel_id == current_channel)
			request->SocketWrite(" selected");
		sprintf(buffer,">%s\n",channel->name);
		request->SocketWrite(buffer);
	}
	request->SocketWrite("</selected></tr>\n");
	//standby
	request->SocketWrite("<tr id=\"StandbyRow\" style=\"visibility:hidden\"><td colspan=2>\n");
	request->SocketWrite("Standby on ?<input type=\"radio\" name=\"sbon\" value=\"1\">Yes\n");
	request->SocketWrite("<input type=\"radio\" name=\"sbon\" value=\"0\" checked>No</td></tr>\n");
	// Buttons
	request->SocketWrite("<td align=\"center\"><input type=\"submit\" value=\"OK\">\n");
	request->SocketWrite("<td align=\"center\"><form method=\"GET\" action=\"/fb/timer.dbox2\"><input type=\"submit\" value=\"CANCEL\"><form></center></td>\n");
	request->SocketWrite("</table></form></table>\n");
	request->SendHTMLFooter();
	delete[] buffer;
}

//-------------------------------------------------------------------------
void CWebAPI::doNewTimer(CWebserverRequest *request)
{
time_t	announceTimeT = 0,
		stopTimeT = 0,
		alarmTimeT = 0;

	if(request->ParameterList["alarm"] != "")		// wenn alarm angegeben dann parameter im time_t format
	{
		alarmTimeT = atoi(request->ParameterList["alarm"].c_str());
		if(request->ParameterList["stop"] != "")
			stopTimeT = atoi(request->ParameterList["stop"].c_str());
		if(request->ParameterList["announce"] != "")
			announceTimeT = atoi(request->ParameterList["announce"].c_str());
	}
	else			// sonst formular-parameter parsen
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
		correctTime(alarmTime);
		stopTimeT = mktime(stopTime);
	}
		
   announceTimeT = alarmTimeT-60;
   CTimerEvent::CTimerEventTypes type  = 
   (CTimerEvent::CTimerEventTypes) atoi(request->ParameterList["type"].c_str());
   CTimerEvent::CTimerEventRepeat rep = 
   (CTimerEvent::CTimerEventRepeat) atoi(request->ParameterList["rep"].c_str());
   bool standby_on = (request->ParameterList["sbon"]=="1");
   CTimerEvent::EventInfo eventinfo;
   eventinfo.epgID      = 0;
   eventinfo.channel_id = atoi(request->ParameterList["channel_id"].c_str());
   void *data=NULL;
   if(type == CTimerEvent::TIMER_STANDBY)
      data=&standby_on;
   else if(type==CTimerEvent::TIMER_NEXTPROGRAM || type==CTimerEvent::TIMER_ZAPTO ||
           type==CTimerEvent::TIMER_RECORD)
      data= &eventinfo;
   if(type!=CTimerEvent::TIMER_RECORD)
   {
      stopTimeT=0;
   }
   Parent->Timerd->addTimerEvent(type,data,announceTimeT,alarmTimeT,stopTimeT,rep);
}

