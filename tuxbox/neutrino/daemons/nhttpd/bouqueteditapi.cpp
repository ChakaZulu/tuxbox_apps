/*
	nhttpd  -  DBoxII-Project

	Copyright (C) 2001/2002 Dirk Szymanski 'Dirch'

	$Id: bouqueteditapi.cpp,v 1.26 2004/04/08 18:53:52 thegoodguy Exp $

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

#include "bouqueteditapi.h"
#include "debug.h"

#include <zapit/client/zapittools.h>

//-------------------------------------------------------------------------

bool CBouqueteditAPI::Execute(CWebserverRequest* request)
{
	unsigned operation = 0;
	const char *operations[9] = {
		"main",
		"add",
		"move",
		"delete",
		"save",
		"rename",
		"edit",
		"editchannels",
		"set"
	};

	dprintf("ExecuteBouquetEditor %s\n",request->Filename.c_str());

	while (operation < 9) {
		if (request->Filename.compare(operations[operation]) == 0) {
			break;
		}
		operation++;
	}

	if (operation > 8) {
		request->Send404Error();	// if nothing matches send 404 error .)
		return false;
	}

	if(request->Method == M_HEAD) {
		request->SendPlainHeader("text/html");
		return true;
	}
	switch(operation)
	{

		case 0 :	return showBouquets(request); break;
		case 1 :	return addBouquet(request); break;
		case 2 :	return moveBouquet(request); break;
		case 3 :	return deleteBouquet(request); break;
		case 4 :	return saveBouquet(request); break;
		case 5 :	return renameBouquet(request); break;
		case 6 :	return editBouquet(request); break;
		case 7 :	return changeBouquet(request); break;
		case 8 :	return setBouquet(request); break;
		default:	request->Send404Error();
	}		
	return false;
}

//-------------------------------------------------------------------------
bool CBouqueteditAPI::showBouquets(CWebserverRequest* request)
{
	int selected = -1;
	request->SendPlainHeader("text/html");
		request->SocketWrite(
			"<!DOCTYPE html\n"
			"     PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\n"
			"     \"DTD/xhtml1-strict.dtd\">\n"
			"<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"de\" lang=\"de\">\n"
			"<head>\n"
			"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\" />\n"
			"<meta http-equiv=\"cache-control\" content=\"no-cache\" />\n"
			"<meta http-equiv=\"expires\" content=\"0\" />\n"
			"<link rel=\"stylesheet\" href=\"../global.css\" type=\"text/css\" />\n"
			"<title>Bouquet-Editor Main</title>\n"
			"</head>\n"
			"\n"
			"<body>\n"
			"<h2>Bouquet-Editor</h2>\n");
	if (request->ParameterList["saved"] == "1")
	{
		request->SocketWrite("Bouquets gespeichert . . .<br />"
				     "<script language=\"JavaScript\">\n"
				     "<!--\ntop.bouquets.location.reload();//-->\n"
				     "</script>");
	}
	request->SocketWrite("<p><a href=\"add\">[add Bouquet]</a> <a href=\"save\">[save]</a></p>");
	if (request->ParameterList["selected"] != "")
	{
		selected = atoi(request->ParameterList["selected"].c_str());
	}


	CZapitClient::BouquetList AllBouquetList;					// List of all bouquets

	Parent->Zapit->getBouquets(AllBouquetList, true, true); // UTF-8
	CZapitClient::BouquetList::iterator bouquet = AllBouquetList.begin();
	
	unsigned int bouquetSize = AllBouquetList.size();
//	request->printf("Bouquets: %i<BR>\n", bouquetSize);
	
	request->SocketWrite("<table width=\"90%\">");
	for(; bouquet != AllBouquetList.end();bouquet++)
	{
		char classname = ((bouquet->bouquet_nr & 1) == 0) ? 'a' : 'b';
		
		request->printf("<tr class=\"%c\">\n<td align=\"center\">",(selected == (int) bouquet->bouquet_nr + 1)?'c':classname);
		if (selected == (int) (bouquet->bouquet_nr + 1))
			request->SocketWrite("<a name=\"akt\"></a>");
		// lock/unlock
		if (bouquet->locked)
			request->printf("<a href=\"set?selected=%i&amp;action=unlock#akt\"><img src=\"../images/lock.gif\" alt=\"entsperren\" style=\"border: 0px\" /></a></td>\n", bouquet->bouquet_nr + 1);
		else
			request->printf("<a href=\"set?selected=%i&amp;action=lock#akt\"><img src=\"../images/unlock.gif\" alt=\"sperren\" style=\"border: 0px\" /></a></td>\n", bouquet->bouquet_nr + 1);

		// hide/show
		if (bouquet->hidden)
			request->printf("<td align=\"center\"><a href=\"set?selected=%i&amp;action=show#akt\"><img src=\"../images/hidden.gif\" alt=\"verstecken\" style=\"border: 0px\" />\n", bouquet->bouquet_nr + 1);
		else
			request->printf("<td align=\"center\"><a href=\"set?selected=%i&amp;action=hide#akt\"><img src=\"../images/visible.gif\" alt=\"anzeigen\" style=\"border: 0px\" />\n", bouquet->bouquet_nr + 1);

		request->printf("</a></td><td><a href=\"edit?selected=%i&amp;name=%s\">%s</a></td>", bouquet->bouquet_nr + 1, bouquet->name, bouquet->name);
		request->printf("<td width=\"100\"><nobr><a href=\"rename?selected=%i&amp;name=%s\"><img src=\"../images/modify.png\" alt=\"umbenennen\" style=\"border: 0px\" /></a>&nbsp;\n", bouquet->bouquet_nr + 1, bouquet->name);
		request->printf("<a href=\"delete?selected=%i&amp;name=%s\"><img src=\"../images/remove.png\" alt=\"l&ouml;schen\" style=\"border: 0px\" /></a>&nbsp;\n",
			bouquet->bouquet_nr + 1, bouquet->name);
		

		// move down
		if (bouquet->bouquet_nr + 1 < bouquetSize)
			request->printf("<a href=\"move?selected=%i&amp;action=down#akt\"><img src=\"../images/arrowdown.gif\" alt=\"nach unten\" style=\"border: 0px\" /></a>&nbsp;\n", bouquet->bouquet_nr + 1);

		//move up
		if (bouquet->bouquet_nr > 0)
			request->printf("<a href=\"move?selected=%i&amp;action=up#akt\"><img src=\"../images/arrowup.gif\" alt=\"nach oben\" style=\"border: 0px\" /></a>\n", bouquet->bouquet_nr + 1);

		request->SocketWrite("</nobr></td></tr>\n");
	}
	request->SocketWrite("</table>");

	request->SendHTMLFooter();
	return true;
}

//-------------------------------------------------------------------------

bool CBouqueteditAPI::addBouquet(CWebserverRequest* request)
{
	if(!request->Authenticate())
        	return false;	
	if (request->ParameterList["name"].empty())
	{
		request->SendPlainHeader("text/html");
		request->SocketWrite(
			"<!DOCTYPE html\n"
			"     PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\n"
			"     \"DTD/xhtml1-strict.dtd\">\n"
			"<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"de\" lang=\"de\">\n"
			"<head>\n"
			"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\" />\n"
			"<meta http-equiv=\"cache-control\" content=\"no-cache\" />\n"
			"<meta http-equiv=\"expires\" content=\"0\" />\n"
			"<link rel=\"stylesheet\" href=\"../global.css\" type=\"text/css\" />\n"
			"<title>Bouquet-Editor</title>\n"
			"</head>\n"
			"\n"
			"<body>\n"
			"<h2>Bouquet-Editor</h2>\n"
			"<h3>Neues Bouquet</h3>\n"
			"<form action=\"add\" method=\"post\" accept-charset=\"UTF-8\" enctype=\"x-www-form-urlencoded\">\n"
			"<p>"
			"Name des neuen Bouquets: "
			"<input type=\"text\" size=\"30\" name=\"name\" />\n"
			"<input type=\"submit\" value=\"add\" />"
			"</p>"
			"</form>\n");
		request->SendHTMLFooter();
	}
	else
	{
		std::string tmp = request->ParameterList["name"];
		CWebserverRequest::URLDecode(tmp);
		if (Parent->Zapit->existsBouquet(tmp.c_str()) == -1)
		{
			Parent->Zapit->addBouquet(tmp.c_str());
			request->Send302("/bouquetedit/main#akt");
		}
		else
		{
			request->SendPlainHeader("text/html");
			request->SendHTMLHeader("Bouquet-Editor");
			request->SocketWrite("Have to add:");
			request->URLDecode(request->ParameterList["name"]);
			request->SocketWrite(request->ParameterList["name"].c_str());
			request->SocketWrite("<BR>Error! Bouquet already exists!\n");
			request->SocketWrite("<BR><a href=\"main#akt\">back</a>\n");
			request->SendHTMLFooter();
		}
	}
	return true;
}

//-------------------------------------------------------------------------

bool CBouqueteditAPI::moveBouquet(CWebserverRequest* request)
{
        if(!request->Authenticate())    
                return false;   
	if (request->ParameterList["selected"] != "" && (request->ParameterList["action"] == "up" || request->ParameterList["action"] == "down")) {
		int selected = atoi(request->ParameterList["selected"].c_str());
		if (request->ParameterList["action"] == "up") {
			Parent->Zapit->moveBouquet(selected - 1, (selected - 1) - 1);
			selected--;
		} else {
			Parent->Zapit->moveBouquet(selected - 1, (selected + 1) - 1);
			selected++;
		}
		char redirbuff[100];
		sprintf(redirbuff, "main?selected=%i", selected);
		request->Send302(redirbuff);
	} else {
		request->SendPlainHeader("text/html");
		request->SendHTMLHeader("Bouquet-Editor");
		request->SocketWrite("Error!");
		request->SendHTMLFooter();
	}
	return true;
}

//-------------------------------------------------------------------------

bool CBouqueteditAPI::deleteBouquet(CWebserverRequest* request)
{
	int selected = -1;
        if(!request->Authenticate())    
                return false;   

	if (request->ParameterList["selected"] != "") {
		selected = atoi(request->ParameterList["selected"].c_str());
	}
	
	if (request->ParameterList["sure"] != "yes") {
		request->SendPlainHeader("text/html");
		request->SendHTMLHeader("Bouquet-Editor");
		request->SocketWrite("<H2>Bouquet-Editor</H2><BR><H3>Bouquet löschen</H3>\n");
		request->SocketWrite("<B>Delete ");
		request->SocketWrite(request->ParameterList["name"]);
		request->SocketWrite("</B><BR>\n");
		
		request->printf("Sure? <A href=\"delete?selected=%i&amp;sure=yes\">[Yep!]</A> <A href=\"main\">[no way!!!]</A>", selected);
		request->SendHTMLFooter();
	} else {
		Parent->Zapit->deleteBouquet(selected - 1);
		request->Send302("/bouquetedit/main#akt");
	}
	return true;
}

//-------------------------------------------------------------------------

bool CBouqueteditAPI::saveBouquet(CWebserverRequest* request)
{
        if(!request->Authenticate())    
                return false;   
	Parent->Zapit->saveBouquets();
	Parent->Zapit->commitBouquetChange();
	Parent->UpdateBouquets();
	request->Send302("/bouquetedit/main?saved=1");
	return true;
}

//-------------------------------------------------------------------------

bool CBouqueteditAPI::renameBouquet(CWebserverRequest* request)
{
        if(!request->Authenticate())    
                return false;   
	if (request->ParameterList["selected"] != "") 
	{
		if (request->ParameterList["nameto"] == "") {
			request->SendPlainHeader("text/html");
			request->SendHTMLHeader("Bouquet-Editor");
			request->SocketWrite("<H2>Bouquet-Editor</H2><BR><H3>Bouquet umbenennen</H3>\n");
			request->SocketWrite("<FORM ACTION=\"rename\" METHOD=\"POST\" ENCTYPE=\"x-www-form-urlencoded\">\n");
			request->printf("Bouquetname: <INPUT TYPE=\"Text\" SIZE=\"30\" NAME=\"nameto\" value=\"%s\">\n", request->ParameterList["name"].c_str());
			request->printf("<INPUT TYPE=\"hidden\" NAME=\"selected\" VALUE=\"%s\">\n", request->ParameterList["selected"].c_str());
			request->SocketWrite("<INPUT TYPE=\"submit\" VALUE=\"rename\">\n");
			request->SocketWrite("</FORM>\n");
			request->SendHTMLFooter();
		}
		else
		{
			Parent->Zapit->renameBouquet(atoi(request->ParameterList["selected"].c_str()) - 1, ZapitTools::Latin1_to_UTF8(request->ParameterList["nameto"].c_str()).c_str());
			request->Send302((char*)("/bouquetedit/main?selected=" + request->ParameterList["selected"] + "#akt").c_str());
		}
		return true;
	}
	return false;
}

//-------------------------------------------------------------------------


bool CBouqueteditAPI::editBouquet(CWebserverRequest * request)
{
        if (!(request->Authenticate()))
                return false;   

	if (!(request->ParameterList["selected"].empty()))
	{
		CZapitClient::BouquetChannelList BChannelList;
		CZapitClient::BouquetChannelList::iterator channels;

		int selected = atoi(request->ParameterList["selected"].c_str()) - 1;

		request->SendPlainHeader("text/html");

		request->SocketWrite(
			"<!DOCTYPE html\n"
			"     PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\n"
			"     \"DTD/xhtml1-strict.dtd\">\n"
			"<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"de\" lang=\"de\">\n"
			"<head>\n"
			"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\" />\n"
			"<meta http-equiv=\"cache-control\" content=\"no-cache\" />\n"
			"<meta http-equiv=\"expires\" content=\"0\" />\n"
			"<link rel=\"stylesheet\" href=\"../global.css\" type=\"text/css\" />\n"
			"<title>Bouquet-Editor</title>\n"
			"</head>\n"
			"\n"
			"<body>\n"
			"<script src=\"/channels.js\" type=\"text/javascript\"></script>\n"
			"<h2>Bouquet-Editor</h2>\n"
			"<h3>Bouquet ");
		request->SocketWrite(request->ParameterList["name"].c_str());
		request->SocketWrite(" bearbeiten</h3>\n"
				     "<form action=\"editchannels\" method=\"post\" id=\"channels\" enctype=\"x-www-form-urlencoded\">\n"
				     "<p>"
				     "<input type=\"hidden\" name=\"selected\" value=\"");
		request->SocketWrite(request->ParameterList["selected"].c_str());
		request->SocketWrite("\" />\n"
				     "</p>"
				     "<table cellspacing=\"5\"><tr><td>"
				     "<select multiple=\"multiple\" size=\"20\" name=\"bchannels\">\n");

		// List channels in bouquet
		Parent->Zapit->getBouquetChannels(selected, BChannelList, CZapitClient::MODE_CURRENT, true); // UTF-8
		for(channels = BChannelList.begin(); channels != BChannelList.end(); channels++)
		{
			request->printf("<option value=\""
					PRINTF_CHANNEL_ID_TYPE_NO_LEADING_ZEROS
					"\">%s</option>\n",
					channels->channel_id,
					channels->name);
		}
		request->SocketWrite("</select>"
		                     "</td><td align=\"center\">\n"
				     "<input type=\"button\" value=\"up\" onclick=\"poschannel(document.getElementById('channels').bchannels, 0);\" /><br /><br />\n"
				     "<input type=\"button\" value=\"down\" onclick=\"poschannel(document.getElementById('channels').bchannels, 1);\" /><br /><br />\n"
				     "<input type=\"button\" value=\"&gt;&gt;&gt;\" onclick=\"movechannels(document.getElementById('channels').bchannels, document.getElementById('channels').achannels);\" /><br /><br />\n"
				     "<input type=\"button\" value=\"&lt;&lt;&lt;\" onclick=\"movechannels(document.getElementById('channels').achannels, document.getElementById('channels').bchannels);\" /><br /><br />\n"
		                     "</td><td>\n"
				     "<select multiple=\"multiple\" size=\"20\" name=\"achannels\">\n");
		// List all channels
		Parent->Zapit->getChannels(BChannelList, CZapitClient::MODE_CURRENT, CZapitClient::SORT_ALPHA, true); // UTF-8
		for(channels = BChannelList.begin(); channels != BChannelList.end(); channels++)
		{
			if (!Parent->Zapit->existsChannelInBouquet(selected, channels->channel_id)){
				request->printf("<option value=\""
						PRINTF_CHANNEL_ID_TYPE_NO_LEADING_ZEROS
						"\">%s</option>\n",
						channels->channel_id,
						channels->name);
			}
		}
		request->SocketWrite("</select>\n"
				     "</td></tr></table>\n"
				     "<p>"
				     "<input type=\"button\" value=\"Fertig\" onclick=\"fertig();\" />\n"
				     "</p>"
				     "</form>\n");
		request->SendHTMLFooter();
	}
	else
	{
		request->Send302("/bouquetedit/main#akt");
	}
	return true;
}

//-------------------------------------------------------------------------


bool CBouqueteditAPI::changeBouquet(CWebserverRequest* request)
{
        if(!request->Authenticate())    
                return false;   
	if (!(request->ParameterList["selected"].empty()))
	{
		int selected = atoi(request->ParameterList["selected"].c_str());
		CZapitClient::BouquetChannelList BChannelList;
		Parent->Zapit->getBouquetChannels(selected - 1, BChannelList, CZapitClient::MODE_CURRENT);
		CZapitClient::BouquetChannelList::iterator channels = BChannelList.begin();
		for(; channels != BChannelList.end();channels++)
		{
			Parent->Zapit->removeChannelFromBouquet(selected - 1, channels->channel_id);
		}

		t_channel_id channel_id;
		int delta;
		const char * bchannels = request->ParameterList["bchannels"].c_str();
		while (sscanf(bchannels,
			      SCANF_CHANNEL_ID_TYPE
			      "%n",
			      &channel_id,
			      &delta) > 0)
		{
			Parent->Zapit->addChannelToBouquet(selected - 1, channel_id);
			bchannels += (delta + 1); /* skip the separating ',', too */
		}

		Parent->Zapit->renumChannellist();
		Parent->UpdateBouquets();
		request->Send302((char*)("/bouquetedit/main?selected=" + request->ParameterList["selected"] + "#akt").c_str());
		return true;
	}
	return false;
}

//-------------------------------------------------------------------------

bool CBouqueteditAPI::setBouquet(CWebserverRequest* request)
{
        if(!request->Authenticate())    
                return false;   
	if (request->ParameterList["selected"] != "") {
		int selected = atoi(request->ParameterList["selected"].c_str());
		if(request->ParameterList["action"].compare("hide") == 0)
			Parent->Zapit->setBouquetHidden(selected - 1,true);
		else if(request->ParameterList["action"].compare("show") == 0)
			Parent->Zapit->setBouquetHidden(selected - 1,false);
		else if(request->ParameterList["action"].compare("lock") == 0)
			Parent->Zapit->setBouquetLock(selected - 1,true);
		else if(request->ParameterList["action"].compare("unlock") == 0)
			Parent->Zapit->setBouquetLock(selected - 1,false);
//		request->Send302("/bouquetedit/main#akt");
		request->Send302((char*)("/bouquetedit/main?selected=" + request->ParameterList["selected"] + "#akt").c_str());

		return true;
	}
	return false;
}

