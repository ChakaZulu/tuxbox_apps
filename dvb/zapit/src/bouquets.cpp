/*
  BouquetManager für zapit  -   DBoxII-Project

  $Id: bouquets.cpp,v 1.3 2002/01/02 18:07:54 Simplex Exp $

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

  $Log: bouquets.cpp,v $
  Revision 1.3  2002/01/02 18:07:54  Simplex
  fixed bug with channels of servicetype 4

  Revision 1.2  2001/12/30 18:38:37  Simplex
  intregration of CBouquetManager (part I)

  Revision 1.1  2001/12/27 16:49:28  Simplex
  BouquetManager initial revision (completely untested)


*/

#define BOUQUETS_CPP

#include "bouquets.h"
#include <config.h>

/**** class CBouquet ********************************************************/
channel* CBouquet::getChannelByName(char* serviceName, uint serviceType)
{
	channel* result = NULL;

	ChannelList& channels = tvChannels;
	switch (serviceType)
	{
		case 1:
		case 4: channels = tvChannels; break;
		case 2: channels = radioChannels; break;
	}

	ChannelList::iterator it = channels.begin();
	it = channels.begin();
	while ((it<=channels.end()) && ((*it)->name != string(serviceName)))
		it++;
	if (it<channels.end())
	{
		result = *it;
	}
	return( result);
}

channel* CBouquet::getChannelByOnidSid(uint onidSid, uint serviceType)
{
	channel* result = NULL;

	ChannelList& channels = tvChannels;
	switch (serviceType)
	{
		case 1:
		case 4: channels = tvChannels; break;
		case 2: channels = radioChannels; break;
	}

	ChannelList::iterator it = channels.begin();
	while ((it<=channels.end()) && (((*it)->onid<<16)|(*it)->sid != onidSid))
		it++;
	if (it<channels.end())
	{
		result = *it;
	}
	return( result);
}

void CBouquet::addService( channel* newChannel)
{
	switch (newChannel->service_type)
	{
		case 1:
		case 4:
			tvChannels.insert( tvChannels.end(), newChannel);
		break;
		case 2:
			radioChannels.insert( radioChannels.end(), newChannel);
		break;
	}
}

void CBouquet::removeService( channel* oldChannel)
{

	ChannelList& channels = tvChannels;
	switch (oldChannel->service_type)
	{
		case 1:
		case 4: channels = tvChannels; break;
		case 2: channels = radioChannels; break;
	}

	ChannelList::iterator it = channels.end();
	while ((it>=channels.begin()) && (*it != oldChannel))
		it--;
	if (it>channels.begin())
	{
		channels.erase(it);
	}
}

void CBouquet::moveService(  char* serviceName, uint newPosition, uint serviceType)
{
	ChannelList& channels = tvChannels;
	switch (serviceType)
	{
		case 1:
		case 4: channels = tvChannels; break;
		case 2: channels = radioChannels; break;
	}

	uint i;
	ChannelList::iterator it = channels.begin();
	while ((it<=channels.end()) && ((*it)->name != string(serviceName)))
	{
		it++;
		i++;
	}
	if (it<channels.end())
	{
		moveService( i, newPosition, serviceType);
	}

}

/*
void CBouquet::moveService(  uint onidSid, uint newPosition)
{
}
*/

void CBouquet::moveService(  uint oldPosition, uint newPosition, uint serviceType)
{
	ChannelList& channels = tvChannels;
	switch (serviceType)
	{
		case 1:
		case 4: channels = tvChannels; break;
		case 2: channels = radioChannels; break;
	}
	if ((oldPosition < channels.size()) && (newPosition < channels.size()))
	{
		ChannelList::iterator itOld, itNew;
		uint i;
		for (i=0, itOld = channels.begin(); i<oldPosition; i++, itOld++);
		for (i=0, itNew = channels.begin(); i<newPosition; i++, itNew++);

		channel* tmp = channels[oldPosition];
		channels.erase( itOld);
		channels.insert( itNew, tmp);
	}
}

/**** class CBouquetManager *************************************************/
void CBouquetManager::saveBouquets()
{
	printf("[zapit] creating new bouquets.xml\n");
	FILE* bouq_fd = fopen(CONFIGDIR "/zapit/bouquets.xml", "w");

	if (bouq_fd == NULL)
	{
		perror("fopen " CONFIGDIR "/zapit/bouquets.xml");
		return;
	}

	fprintf(bouq_fd, "<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n<ZAPIT>\n");

	for (uint i=0; i<Bouquets.size(); i++)
	{
		fprintf(bouq_fd, "\t<Bouquet name=\"%s\">\n", Bouquets[i]->Name.c_str());
		for ( uint j=0; j<Bouquets[i]->tvChannels.size(); j++)
		{
			fprintf(bouq_fd, "\t\t<channel serviceID=\"%04x\" name=\"%s\" onid=\"%04x\"/>\n",
					Bouquets[i]->tvChannels[j]->sid,
					Bouquets[i]->tvChannels[j]->name.c_str(),
					Bouquets[i]->tvChannels[j]->onid);
		}
		for ( uint j=0; j<Bouquets[i]->radioChannels.size(); j++)
		{
			fprintf(bouq_fd, "\t\t<channel serviceID=\"%04x\" name=\"%s\" onid=\"%04x\"/>\n",
					Bouquets[i]->radioChannels[j]->sid,
					Bouquets[i]->radioChannels[j]->name.c_str(),
					Bouquets[i]->radioChannels[j]->onid);
		}
		fprintf(bouq_fd, "\t</Bouquet>\n");
	}
	fprintf(bouq_fd, "</ZAPIT>\n");
	fclose(bouq_fd);
}

void CBouquetManager::parseBouquetsXml(XMLTreeNode *root)
{
	XMLTreeNode *search=root->GetChild();
	XMLTreeNode *channel_node;

	while (strcmp(search->GetType(), "Bouquet")) {
		search = search->GetNext();
	}

	int nChNrRadio = 1;
	int nChNrTV = 1;

	typedef enum {isTVChannel, isRadioChannel, isNothing} ChannelType;

	enum ChannelType channelType;
	uint onid, sid;

	numchans_tv.clear();
	numchans_radio.clear();

	while ((search) && (!(strcmp(search->GetType(), "Bouquet"))))
	{
		CBouquet* newBouquet = addBouquet( search->GetAttributeValue("name"));
		channel_node = search->GetChild();

		while (channel_node)
		{
			sscanf(channel_node->GetAttributeValue("serviceID"), "%x", &sid);
			sscanf(channel_node->GetAttributeValue("onid"), "%x", &onid);

			std::map<uint,channel>::iterator itChannel;

			channel* chan;
			itChannel = allchans_tv.find((onid << 16) + sid);
			if (itChannel != allchans_tv.end())
			{
				chan = new channel(itChannel->second);
				channelType = isTVChannel;
			}
			else
			{
				itChannel = allchans_radio.find((onid << 16) + sid);
				if (itChannel != allchans_radio.end())
				{
					chan = new channel(itChannel->second);
					channelType = isRadioChannel;
				}
				else
				{
					channelType = isNothing;
				}
			}

			if (channelType == isTVChannel)
			{
				chan->chan_nr = nChNrTV;
				newBouquet->addService( chan);
				numchans_tv.insert(std::pair<uint, uint>(nChNrTV++, (onid<<16)+sid));
			}
			if (channelType == isRadioChannel)
			{
				chan->chan_nr = nChNrRadio;
				newBouquet->addService( chan);
				numchans_radio.insert(std::pair<uint, uint>(nChNrRadio++, (onid<<16)+sid));
			}
			channel_node = channel_node->GetNext();
		}
		printf(
			"[zapit] Bouquet %s with %d tv- and %d radio-channels.\n",
			newBouquet->Name.c_str(),
			newBouquet->tvChannels.size(),
			newBouquet->radioChannels.size());

		search = search->GetNext();
	}
	printf("[zapit] Found %d bouquets.\n", Bouquets.size());

}
void CBouquetManager::loadBouquets()
{
	XMLTreeParser *parser=new XMLTreeParser("ISO-8859-1");
	FILE *in=fopen(CONFIGDIR "/zapit/bouquets.xml", "r");
	if (!in)
	{
		perror("[zapit] " CONFIGDIR "/zapit/bouquets.xml");
		return;
	}

	char buf[2048];

	int done;
	do
	{
		unsigned int len=fread(buf, 1, sizeof(buf), in);
		done=len<sizeof(buf);
		if (!parser->Parse(buf, len, done))
		{
			printf("[zapit] parse error: %s at line %d\n",
			parser->ErrorString(parser->GetErrorCode()),
			parser->GetCurrentLineNumber());
			fclose(in);
			delete parser;
			return;
		}
	} while (!done);

	if (parser->RootNode())
		parseBouquetsXml(parser->RootNode());

	delete parser;

	fclose(in);
}

CBouquet* CBouquetManager::addBouquet( string name)
{
	CBouquet* newBouquet = new CBouquet(name);
	Bouquets.insert( Bouquets.end(), newBouquet);
	return( newBouquet);
}

void CBouquetManager::removeBouquet( uint id)
{
	if (id < Bouquets.size())
	{
		CBouquet* bouquet = Bouquets[id];

		BouquetList::iterator it;
		uint i;
		for (i=0, it = Bouquets.begin(); i<id; i++, it++);
		Bouquets.erase( it);
		delete bouquet;
	}
}

void CBouquetManager::removeBouquet( string name)
{

	BouquetList::iterator it;
	uint i;
	for (i=0, it = Bouquets.begin(); (i<Bouquets.size()) && (Bouquets[i]->Name != name); i++, it++);

	if (i<Bouquets.size())
	{
		CBouquet* bouquet = *it;
		Bouquets.erase( it);
		delete bouquet;
	}
}

void CBouquetManager::moveBouquet( uint oldId, uint newId)
{
	if ((oldId < Bouquets.size()) && (newId < Bouquets.size()))
	{
		BouquetList::iterator itOld, itNew;
		uint i;
		for (i=0, itOld = Bouquets.begin(); i<oldId; i++, itOld++);
		for (i=0, itNew = Bouquets.begin(); i<newId; i++, itNew++);

		CBouquet* tmp = Bouquets[oldId];
		Bouquets.erase( itOld);
		Bouquets.insert( itNew, tmp);
	}
}

void CBouquetManager::saveAsLast( uint BouquetId, uint channelNr)
{
	FILE* BMSettings;
	BMSettings = fopen("/tmp/zapit_last_bouq", "w");

	if (BMSettings == NULL)
	{
		perror("[zapit] fopen: /tmp/zapit_last_bouq");
	}

	fwrite( &BouquetId, 1, sizeof(BouquetId), BMSettings);
	fwrite( &channelNr, 1, sizeof(channelNr), BMSettings);
	fclose( BMSettings);

}

void CBouquetManager::getLast( uint* BouquetId, uint* channelNr)
{
	FILE* BMSettings;
	BMSettings = fopen("/tmp/zapit_last_bouq", "w");

	if (BMSettings == NULL)
	{
		perror("[zapit] fopen: /tmp/zapit_last_bouq");
	}

	fread( BouquetId, 1, sizeof(BouquetId), BMSettings);
	fread( channelNr, 1, sizeof(channelNr), BMSettings);
	fclose( BMSettings);
}

void CBouquetManager::clearAll()
{
	for (uint i=0; i<Bouquets.size(); i++)
	{
		delete Bouquets[i];
	}
	Bouquets.clear();
}

void CBouquetManager::onTermination()
{
	system("cp /tmp/zapit_last_bouq " CONFIGDIR "/zapit/last_bouq");
}

void CBouquetManager::onStart()
{
	system("cp " CONFIGDIR "/zapit/last_bouq /tmp/zapit_last_bouq");
}

