/*
	webserver  -   DBoxII-Project

	Copyright (C) 2001/2002 Dirk Szymanski 'Dirch'

	$Id: bouqueteditapi.cpp,v 1.3 2002/10/03 19:05:12 thegoodguy Exp $

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
#define dprintf(fmt, args...) {if(Parent->Parent->DEBUG) aprintf( "[nhttpd] " fmt, ## args);}

//-------------------------------------------------------------------------
bool CBouqueteditAPI::Execute(CWebserverRequest* request)
{
	unsigned operation = 0;
	const char *operations[9] = {
		"test.dbox2",
		"main",
		"add",
		"move",
		"delete",
		"save",
		"rename",
		"edit",
		"editchannels"
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
	if(operation == 0)		// testing stuff
	{
		request->SendPlainHeader("text/html");		
		aprintf("Teste nun\n");
		request->SocketWrite("alles klar\n");
		return true;
	} 
	else if (operation == 1) //start/main
	{
		dprintf("Bouquet-Editor main...\n");
		request->SendPlainHeader("text/html");
		request->SendHTMLHeader("Bouquet-Editor Main");
		request->SocketWrite("<h1>Bouquet-Editor</h1>\n");
		request->SocketWrite("<p><a href=\"add\">[add Bouquet]</a> <a href=\"save\">[save]</a> </p>");
		int selected = -1;
		if (request->ParameterList["selected"] != "")
		{
		    selected = atoi(request->ParameterList["selected"].c_str());
		}
		BEShowBouquets(request, selected);

		request->SendHTMLFooter();
		return true;
	}
	else if (operation == 2) //add Bouquet
	{
		dprintf("Bouquet-Editor add...\n");
		if (request->ParameterList["name"] == "") {
			request->SendPlainHeader("text/html");
			request->SendHTMLHeader("Bouquet-Editor");
			request->SocketWrite("<h1>Bouquet-Editor add Bouquet</h1>\n");
			request->SocketWrite("<form action=\"add\" method=\"POST\" enctype=\"x-www-form-urlencoded\">\n");
			request->SocketWrite("Bouquetname: <input type=\"Text\" size=\"30\" name=\"name\">");
			request->SocketWrite("<input type=\"submit\" value=\"add\">\n");
			request->SocketWrite("</form>\n");
			request->SendHTMLFooter();
		}
		else
		{
			if (Parent->Zapit->existsBouquet(request->ParameterList["name"]) == 0) {
				Parent->Zapit->addBouquet(request->ParameterList["name"]);
				request->Send302("/bouquetedit/main#akt");
			} else {
				request->SendPlainHeader("text/html");
				request->SendHTMLHeader("Bouquet-Editor");
				request->SocketWrite("Have to add:");
				request->URLDecode(request->ParameterList["name"]);
				request->SocketWrite(request->ParameterList["name"].c_str());
				request->SocketWrite("<br>Error! Bouquet already exists!\n");
				request->SocketWrite("<br><a href=\"main#akt\">back</a>\n");
				request->SendHTMLFooter();
			}
		}
		return true;
	}
	else if (operation == 3) //move Bouquet
	{
		dprintf("Bouquet-Editor move...\n");
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
	else if (operation == 4) //delete Bouquet
	{
		dprintf("Bouquet-Editor delete...\n");
		int selected = -1;
		if (request->ParameterList["selected"] != "") {
			selected = atoi(request->ParameterList["selected"].c_str());
		}
		
		if (request->ParameterList["sure"] != "yes") {
			dprintf("Bouquet-Editor delete...\n");
			request->SendPlainHeader("text/html");
			request->SendHTMLHeader("Bouquet-Editor");
			request->SocketWrite("<h1>delete Bouquet</h1>\n");
			request->SocketWrite("<b>Delete ");
			request->SocketWrite(request->ParameterList["name"]);
			request->SocketWrite("</b><br>\n");
			
			char outbuff[200];
			sprintf(outbuff, "Sure? <a href=\"delete?selected=%i&sure=yes\">[Yep!]</a> <a href=\"main\">[no way!!!]</a>", selected);
			request->SocketWrite(outbuff);
			request->SendHTMLFooter();
		} else {
			Parent->Zapit->deleteBouquet(selected - 1);
			request->Send302("/bouquetedit/main#akt");
		}
		return true;
	}
	else if (operation == 5) //save Bouquets
	{
		dprintf("Bouquet-Editor save...\n");
		request->SendPlainHeader("text/html");
		request->SendHTMLHeader("Bouquet-Editor");
		request->SocketWrite("<p>saving...");
		Parent->Zapit->saveBouquets();
		request->SocketWrite("...");
		Parent->Zapit->commitBouquetChange();
		request->SocketWrite("saved!</p>");
		request->SocketWrite("<a href=\"main\">[back...]</a>");
		request->SendHTMLFooter();
		Parent->UpdateBouquets();
		return true;
	}
	else if (operation == 6) //rename Bouquet
	{
		dprintf("Bouquet-Editor rename...\n");
		if (request->ParameterList["nameto"] == "") {
			request->SendPlainHeader("text/html");
			request->SendHTMLHeader("Bouquet-Editor");
			request->SocketWrite("<h1>Bouquet-Editor rename Bouquet</h1>\n");
			request->SocketWrite("<form action=\"rename\" method=\"POST\" enctype=\"x-www-form-urlencoded\">\n");
			char outbuff[100];
			sprintf(outbuff, "Bouquetname: <input type=\"Text\" size=\"30\" name=\"nameto\" value=\"%s\">\n", request->ParameterList["name"].c_str());
			request->SocketWrite(outbuff);
			sprintf(outbuff, "<input type=\"hidden\" name=\"selected\" value=\"%s\">\n", request->ParameterList["selected"].c_str());
			request->SocketWrite(outbuff);
			request->SocketWrite("<input type=\"submit\" value=\"rename\">\n");
			request->SocketWrite("</form>\n");
			request->SendHTMLFooter();
		}
		else
		{
			Parent->Zapit->renameBouquet(atoi(request->ParameterList["selected"].c_str()) - 1, request->ParameterList["nameto"].c_str());
			request->Send302("/bouquetedit/main#akt");
		}
		return true;
	}
	else if (operation == 7) //edit Bouquet
	{
		dprintf("Bouquet-Editor edit...\n");
		if (request->ParameterList["selected"] != "") {
			int selected = atoi(request->ParameterList["selected"].c_str());
			request->SendPlainHeader("text/html");
			request->SendHTMLHeader("Bouquet-Editor");
			request->SocketWrite("<Script language=\"Javascript\" src=\"/channels.js\">\n</script>\n");
			request->SocketWrite("<h1>Bouquet-Editor edit Bouquet</h1>\n");
			CZapitClient::BouquetChannelList BChannelList;
			Parent->Zapit->getBouquetChannels(selected - 1, BChannelList, CZapitClient::MODE_CURRENT);
			CZapitClient::BouquetChannelList::iterator channels = BChannelList.begin();
			request->SocketWrite("<FORM action=\"editchannels\" method=\"POST\" name=\"channels\" enctype=\"x-www-form-urlencoded\">\n");
			request->SocketWrite("<INPUT TYPE=\"HIDDEN\" name=\"selected\" value=\"");
			request->SocketWrite(request->ParameterList["selected"].c_str());
			request->SocketWrite("\">\n");
			request->SocketWrite("<table><tr><td>\n");
			request->SocketWrite("<INPUT TYPE=\"Button\" Value=\"up\" onClick=\"poschannel(document.channels.bchannels, 0);\">\n");
			request->SocketWrite("<INPUT TYPE=\"Button\" Value=\"down\" onClick=\"poschannel(document.channels.bchannels, 1);\">\n");
			request->SocketWrite("<INPUT TYPE=\"Button\" Value=\">>>\" onClick=\"movechannels(document.channels.bchannels, document.channels.achannels);\">\n");
			request->SocketWrite("<BR>\n");
			request->SocketWrite("<select multiple size=\"20\" name=\"bchannels\">\n");
			char outbuff[100];
			for(; channels != BChannelList.end();channels++)
			{
				sprintf(outbuff, "<option value=\"%i\">", channels->channel_id);
				request->SocketWrite(outbuff);
				request->SocketWrite(channels->name);
				request->SocketWrite("</option>\n");
			}
			request->SocketWrite("</select>\n");
			request->SocketWrite("</td><td>\n");
			CZapitClient::BouquetChannelList AllChannelList;
			Parent->Zapit->getChannels(AllChannelList, CZapitClient::MODE_CURRENT, CZapitClient::SORT_ALPHA);
			CZapitClient::BouquetChannelList::iterator allChannels = AllChannelList.begin();
			request->SocketWrite("<INPUT TYPE=\"Button\" Value=\"<<<\" onClick=\"movechannels(document.channels.achannels, document.channels.bchannels);\">\n");
			request->SocketWrite("<BR>\n");
			request->SocketWrite("<select multiple size=\"20\" name=\"achannels\">\n");
			for(; allChannels != AllChannelList.end();allChannels++)
			{
				if (!Parent->Zapit->existsChannelInBouquet(selected, allChannels->channel_id)){
					sprintf(outbuff, "<option value=\"%i\">", allChannels->channel_id);
					request->SocketWrite(outbuff);
					request->SocketWrite(allChannels->name);
					request->SocketWrite("</option>\n");
				}
			}
			request->SocketWrite("</select>\n");
			request->SocketWrite("</td></tr></table>\n");
			request->SocketWrite("<INPUT TYPE=\"button\" value=\"Fertig\" onClick=\"fertig();\">\n");
			request->SocketWrite("</FORM>\n");
			request->SendHTMLFooter();
		}
		else
		{
			request->Send302("/bouquetedit/main#akt");
		}
		return true;
	}
	else if (operation == 8) //change Bouquet
	{
		dprintf("Bouquet-Editor change...\n");
		if (request->ParameterList["selected"] != "") {
			int selected = atoi(request->ParameterList["selected"].c_str());
			CZapitClient::BouquetChannelList BChannelList;
			Parent->Zapit->getBouquetChannels(selected - 1, BChannelList, CZapitClient::MODE_CURRENT);
			CZapitClient::BouquetChannelList::iterator channels = BChannelList.begin();
			for(; channels != BChannelList.end();channels++)
			{
				Parent->Zapit->removeChannelFromBouquet(selected - 1, channels->channel_id);
			}
			string bchannels = request->ParameterList["bchannels"];
			int pos;
			while ((pos = bchannels.find(',')) >= 0) {
				string bchannel = bchannels.substr(0, pos);
				bchannels = bchannels.substr(pos+1, bchannels.length());
				Parent->Zapit->addChannelToBouquet(selected - 1, atoi(bchannel.c_str()));
				
			}
			if (bchannels.length() > 0)
				Parent->Zapit->addChannelToBouquet(selected - 1, atoi(bchannels.c_str()));
			Parent->Zapit->renumChannellist();
			Parent->UpdateBouquets();
			request->Send302("/bouquetedit/main#akt");
		}
	}
	return false;
}

//-------------------------------------------------------------------------
// Editor funtions (ExecuteBouquetEditor)
//-------------------------------------------------------------------------

void CBouqueteditAPI::BEShowBouquets(CWebserverRequest* request, unsigned int selected)
{
	char outbuff[200];
	CZapitClient::BouquetList AllBouquetList;					// List of all bouquets
	AllBouquetList.clear();
	Parent->Zapit->getBouquets(AllBouquetList, true); 
	CZapitClient::BouquetList::iterator bouquet = AllBouquetList.begin();
	
	unsigned int bouquetSize = AllBouquetList.size();
	sprintf(outbuff, "Bouquets: %i<br>\n", bouquetSize);
	
	request->SocketWrite(outbuff);
	request->SocketWrite("<table>");
	for(; bouquet != AllBouquetList.end();bouquet++)
	{
		if (selected == bouquet->bouquet_nr + 1) {
			request->SocketWrite("<tr><td class=\"c\">");
			request->SocketWrite("<a name=\"akt\"></a>");
		} else
			if ((bouquet->bouquet_nr + 1) % 2 == 1)
				request->SocketWrite("<tr><td class=\"a\">");
			else
				request->SocketWrite("<tr><td class=\"b\">");
		
		sprintf(outbuff, "<a href=\"main?selected=%i#akt\">%s</a>", bouquet->bouquet_nr + 1, bouquet->name);
		request->SocketWrite(outbuff);
		if (selected == bouquet->bouquet_nr + 1)
		{
			request->SocketWrite("</td><td class=\"c\">");
			sprintf(outbuff, 
			"<a href=\"edit?selected=%i\">edit</a>"
			" <a href=\"rename?selected=%i&name=%s\">rename</a>"
			" <a href=\"delete?selected=%i&name=%s\">delete</a>", 
			bouquet->bouquet_nr + 1,
			bouquet->bouquet_nr + 1, bouquet->name,
			bouquet->bouquet_nr + 1, bouquet->name);
			request->SocketWrite(outbuff);
			
			if (bouquet->bouquet_nr > 0) {
				sprintf(outbuff, " <a href=\"move?selected=%i&action=up#akt\">up</a>", bouquet->bouquet_nr + 1);
				request->SocketWrite(outbuff);
			}
			
			if (bouquet->bouquet_nr + 1 < bouquetSize) {
				sprintf(outbuff, " <a href=\"move?selected=%i&action=down#akt\">down</a>", bouquet->bouquet_nr + 1);
				request->SocketWrite(outbuff);
			}
			request->SocketWrite("</td></tr>\n");
		}  
		else
		{
			request->SocketWrite("</td><td>&nbsp;</td></tr>\n");
		}
	}
	request->SocketWrite("</table>");
}
