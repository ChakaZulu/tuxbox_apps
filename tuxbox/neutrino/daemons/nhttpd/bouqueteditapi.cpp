/*
	nhttpd  -  DBoxII-Project

	Copyright (C) 2001/2002 Dirk Szymanski 'Dirch'

	$Id: bouqueteditapi.cpp,v 1.22 2004/04/02 13:26:58 thegoodguy Exp $

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
	request->SendHTMLHeader("Bouquet-Editor Main");
	request->SocketWrite("<H2>Bouquet-Editor</H2>\n");
	if (request->ParameterList["saved"] == "1")
	{
		request->SocketWrite("Bouquets gespeichert . . .<BR>");
		request->SocketWriteLn("<SCRIPT LANGUAGE=\"JavaScript\">\n<!--\ntop.bouquets.location.reload();//-->\n</SCRIPT>");
	}
	request->SocketWrite("<P><A HREF=\"add\">[add Bouquet]</A> <A HREF=\"save\">[save]</A> </P>");
	if (request->ParameterList["selected"] != "")
	{
		selected = atoi(request->ParameterList["selected"].c_str());
	}


	CZapitClient::BouquetList AllBouquetList;					// List of all bouquets

	Parent->Zapit->getBouquets(AllBouquetList, true, true); // UTF-8
	CZapitClient::BouquetList::iterator bouquet = AllBouquetList.begin();
	
	unsigned int bouquetSize = AllBouquetList.size();
//	request->printf("Bouquets: %i<BR>\n", bouquetSize);
	
	request->SocketWrite("<TABLE WIDTH=\"90%\">");
	for(; bouquet != AllBouquetList.end();bouquet++)
	{
		char classname = ((bouquet->bouquet_nr & 1) == 0) ? 'a' : 'b';
		
		request->printf("<TR CLASS=\"%c\">\n<TD>",(selected == (int) bouquet->bouquet_nr + 1)?'c':classname);
		if (selected == (int) (bouquet->bouquet_nr + 1))
			request->SocketWrite("<A NAME=\"akt\"></A>");
		// lock/unlock
		if (bouquet->locked)
			request->printf("<CENTER><A HREF=\"set?selected=%i&action=unlock#akt\"><IMG border=0 src=\"../images/lock.gif\" TITLE=\"Bouquet entsperren\"></A></CENTER></TD>\n", bouquet->bouquet_nr + 1);
		else
			request->printf("<CENTER><A HREF=\"set?selected=%i&action=lock#akt\"><IMG border=0 src=\"../images/unlock.gif\" TITLE=\"Bouquet sperren\"></A></CENTER></TD>\n", bouquet->bouquet_nr + 1);

		// hide/show
		if (bouquet->hidden)
			request->printf("<TD><CENTER><A HREF=\"set?selected=%i&action=show#akt\"><IMG border=0 src=\"../images/hidden.gif\" TITLE=\"Bouquet verstecken\"></A></CENTER></TD>\n", bouquet->bouquet_nr + 1);
		else
			request->printf("<TD><CENTER><A HREF=\"set?selected=%i&action=hide#akt\"><IMG border=0 src=\"../images/visible.gif\" TITLE=\"Bouquet anzeigen\"></A></CENTER></TD>\n", bouquet->bouquet_nr + 1);

		request->printf("<TD><A HREF=\"edit?selected=%i&name=%s\">%s</A></TD>", bouquet->bouquet_nr + 1, bouquet->name, ZapitTools::UTF8_to_Latin1(bouquet->name).c_str());
		request->printf("<TD WIDTH=\"100\"><NOBR><A HREF=\"rename?selected=%i&name=%s\"><IMG border=0 SRC=\"../images/modify.png\" TITLE=\"Bouquet umbenennen\"></a>&nbsp;\n",bouquet->bouquet_nr + 1, ZapitTools::UTF8_to_Latin1(bouquet->name).c_str());
		request->printf("<A HREF=\"delete?selected=%i&name=%s\"><IMG border=0 src=\"../images/remove.png\" TITLE=\"Bouquet löschen\"></A>&nbsp;\n",
			bouquet->bouquet_nr + 1, ZapitTools::UTF8_to_Latin1(bouquet->name).c_str());
		

		// move down
		if (bouquet->bouquet_nr + 1 < bouquetSize)
			request->printf("<A HREF=\"move?selected=%i&action=down#akt\"><IMG border=0 src=\"../images/arrowdown.gif\" TITLE=\"nach unten\"></A>&nbsp;\n", bouquet->bouquet_nr + 1);

		//move up
		if (bouquet->bouquet_nr > 0)
			request->printf("<A HREF=\"move?selected=%i&action=up#akt\"><IMG border=0 src=\"../images/arrowup.gif\" TITLE=\"nach oben\"></A>\n", bouquet->bouquet_nr + 1);

		request->SocketWrite("</NOBR></TD></TR>\n");
	}
	request->SocketWrite("</TABLE>");

	request->SendHTMLFooter();
	return true;
}

//-------------------------------------------------------------------------

bool CBouqueteditAPI::addBouquet(CWebserverRequest* request)
{
	if(!request->Authenticate())
        	return false;	
	if (request->ParameterList["name"] == "") {
		request->SendPlainHeader("text/html");
		request->SendHTMLHeader("Bouquet-Editor");
		request->SocketWrite("<H2>Bouquet-Editor</H2><BR><H3>Neues Bouquet</H3>\n");
		request->SocketWrite("<FORM ACTION=\"add\" METHOD=\"POST\" ENCTYPE=\"x-www-form-urlencoded\">\n");
		request->SocketWrite("Bouquetname: <INPUT TYPE=\"Text\" SIZE=\"30\" NAME=\"name\">");
		request->SocketWrite("<INPUT TYPE=\"submit\" VALUE=\"add\">\n");
		request->SocketWrite("</FORM>\n");
		request->SendHTMLFooter();
	}
	else
	{
		if (Parent->Zapit->existsBouquet(ZapitTools::Latin1_to_UTF8(request->ParameterList["name"].c_str()).c_str()) == -1) {
			Parent->Zapit->addBouquet(request->ParameterList["name"]);
			request->Send302("/bouquetedit/main#akt");
		} else {
			request->SendPlainHeader("text/html");
			request->SendHTMLHeader("Bouquet-Editor");
			request->SocketWrite("Have to add:");
			request->URLDecode(request->ParameterList["name"]);
			request->SocketWrite(request->ParameterList["name"].c_str());
			request->SocketWrite("<BR>Error! Bouquet already exists!\n");
			request->SocketWrite("<BR><A HREF=\"main#akt\">back</A>\n");
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
		
		request->printf("Sure? <A HREF=\"delete?selected=%i&sure=yes\">[Yep!]</A> <A HREF=\"main\">[no way!!!]</A>", selected);
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
			Parent->Zapit->renameBouquet(atoi(request->ParameterList["selected"].c_str()) - 1, request->ParameterList["nameto"].c_str());
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

