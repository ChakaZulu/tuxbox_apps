/*
  BouquetManager für zapit  -   DBoxII-Project

  $Id: bouquets.cpp,v 1.5 2002/01/05 16:39:32 Simplex Exp $

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
  Revision 1.5  2002/01/05 16:39:32  Simplex
  completed commands for bouquet-editor

  Revision 1.4  2002/01/04 22:52:31  Simplex
  prepared zapitclient,
  added new command structure (version 2),
  added some commands for bouquet editor,

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
CBouquet::~CBouquet()
{
	for (uint i=0; i<tvChannels.size(); i++)
		delete tvChannels[i];
	for (uint i=0; i<radioChannels.size(); i++)
		delete radioChannels[i];
}

channel* CBouquet::getChannelByName(char* serviceName, uint serviceType)
{
	channel* result = NULL;

	ChannelList& channels = tvChannels;
	switch (serviceType)
	{
		case 0:
		case 1:
		case 4: channels = tvChannels; break;
		case 2: channels = radioChannels; break;
	}

	uint i;
	for (i=0; i<channels.size(), channels[i]->name != string(serviceName); i++);

	if (i<channels.size())
		result = channels[i];

	if ((serviceType==0) && (result==NULL))
		result = getChannelByName(serviceName, 2);

	return( result);
}

channel* CBouquet::getChannelByOnidSid(uint onidSid, uint serviceType = 0)
{
	channel* result = NULL;

	printf("searching channel\n");
	ChannelList& channels = tvChannels;
	switch (serviceType)
	{
		case 0:
		case 1:
		case 4: channels = tvChannels; break;
		case 2: channels = radioChannels; break;
	}

	uint i;
	for (i=0; i<channels.size(), ((channels[i]->onid<<16) | channels[i]->sid) != onidSid; i++);

	if (i<channels.size())
		result = channels[i];

	if ((serviceType==0) && (result==NULL))
		result = getChannelByOnidSid(onidSid, 2);

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
	printf("removing channel\n");
	if (oldChannel != NULL)
	{
		printf("removing channel %s\n", oldChannel->name.c_str());
		ChannelList& channels = tvChannels;
		switch (oldChannel->service_type)
		{
			case 1:
			case 4: channels = tvChannels; break;
			case 2: channels = radioChannels; break;
		}

		ChannelList::iterator it = channels.begin();
		while ((it<channels.end()) && !(*it == oldChannel))
			it++;
		if (it<channels.end())
		{
			printf("channel found:");
			printf("%s - ", (*it)->name.c_str());
			channels.erase(it);
			printf("erased ");
			delete oldChannel;
			printf("deleted ");
		}
		printf("done removing channel\n");
	}
	else
	{
		printf("NULL-channel!\n");
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

//			std::map<uint,channel>::iterator itChannel;

			channel* chan = copyChannelByOnidSid( (onid << 16) + sid);

			if ( chan != NULL)
			{
				switch (chan->service_type)
				{
					case 1:
					case 4:
						chan->chan_nr = nChNrTV;
						newBouquet->addService( chan);
						numchans_tv.insert(std::pair<uint, uint>(nChNrTV++, (onid<<16)+sid));
					break;
					case 2:
						chan->chan_nr = nChNrRadio;
						newBouquet->addService( chan);
						numchans_radio.insert(std::pair<uint, uint>(nChNrRadio++, (onid<<16)+sid));
					break;
				}
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

void CBouquetManager::renumServices()
{
	int nChNrRadio = 1;
	int nChNrTV = 1;

	numchans_tv.clear();
	numchans_radio.clear();

	for (uint i=0; i<Bouquets.size(); i++)
	{
		for (uint j=0; j<Bouquets[i]->tvChannels.size(); j++)
		{
			Bouquets[i]->tvChannels[j]->chan_nr = nChNrTV;
			numchans_tv.insert(std::pair<uint, uint>(nChNrTV++, (Bouquets[i]->tvChannels[j]->onid<<16)+Bouquets[i]->tvChannels[j]->sid));
		}
		for (uint j=0; j<Bouquets[i]->radioChannels.size(); j++)
		{
			Bouquets[i]->radioChannels[j]->chan_nr = nChNrRadio;
			numchans_radio.insert(std::pair<uint, uint>(nChNrRadio++, (Bouquets[i]->radioChannels[j]->onid<<16)+Bouquets[i]->radioChannels[j]->sid));
		}
	}

	map<uint, uint>::iterator        numit;
	map<std::string, uint>::iterator nameit;
	map<uint, channel>::iterator     cit;

	extern map<uint, uint> allnumchannels_tv;
	extern map<uint, uint> allnumchannels_radio;
	extern map<std::string, uint> allnamechannels_tv;
	extern map<std::string, uint> allnamechannels_radio;


	int number = 1;
	allnumchannels_tv.clear();
	allnamechannels_tv.clear();
	allnumchannels_radio.clear();
	allnamechannels_radio.clear();
	for (numit = numchans_tv.begin(); numit != numchans_tv.end(); numit++)
	{
		cit = allchans_tv.find(numit->second);
		cit->second.chan_nr = number;
		allnumchannels_tv.insert(std::pair<uint,uint>(number++, (cit->second.onid<<16)+cit->second.sid));
		allnamechannels_tv.insert(std::pair<std::string, uint>(cit->second.name, (cit->second.onid<<16)+cit->second.sid));
	}
	numchans_tv.clear();

	for (nameit = namechans_tv.begin(); nameit != namechans_tv.end(); nameit++)
	{
		cit = allchans_tv.find(nameit->second);
		cit->second.chan_nr = number;
		allnumchannels_tv.insert(std::pair<uint, uint>(number++, (cit->second.onid<<16)+cit->second.sid));
		allnamechannels_tv.insert(std::pair<std::string, uint>(nameit->first, (cit->second.onid<<16)+cit->second.sid));
	}
	namechans_tv.clear();

	number = 1;
	for (numit = numchans_radio.begin(); numit != numchans_radio.end(); numit++)
	{
		cit = allchans_radio.find(numit->second);
		cit->second.chan_nr = number;
		allnumchannels_radio.insert(std::pair<uint,uint>(number++, (cit->second.onid<<16)+cit->second.sid));
		allnamechannels_radio.insert(std::pair<std::string, uint>(cit->second.name, (cit->second.onid<<16)+cit->second.sid));
	}
	numchans_radio.clear();

	for (nameit = namechans_radio.begin(); nameit != namechans_radio.end(); nameit++)
	{
		cit = allchans_radio.find(nameit->second);
		cit->second.chan_nr = number;
		allnumchannels_radio.insert(std::pair<uint, uint>(number++, (cit->second.onid<<16)+cit->second.sid));
		allnamechannels_radio.insert(std::pair<std::string, uint>(nameit->first, (cit->second.onid<<16)+cit->second.sid));
	}
	namechans_radio.clear();

}

CBouquet* CBouquetManager::addBouquet( string name)
{
	CBouquet* newBouquet = new CBouquet(name);
	Bouquets.insert( Bouquets.end(), newBouquet);
	return( newBouquet);
}

void CBouquetManager::deleteBouquet( uint id)
{
	printf ("Deleting bouquet %d\n", id);
	if (id < Bouquets.size())
	{
		CBouquet* bouquet = Bouquets[id];
		printf ("bouquet %s\n", bouquet->Name.c_str());

		BouquetList::iterator it;
		uint i;
		for (i=0, it = Bouquets.begin(); i<id; i++, it++);
		printf ("for ready\n");
		printf ("will erase bouquet %s\n", (*it)->Name.c_str());
		Bouquets.erase( it);
		printf ("erased\n");
		delete bouquet;
		printf ("deleted\n");
	}
}

void CBouquetManager::deleteBouquet( string name)
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

channel* CBouquetManager::copyChannelByOnidSid( unsigned int onid_sid)
{
	channel* chan = NULL;
	map<uint,channel>::iterator itChannel = allchans_tv.find(onid_sid);
	if (itChannel != allchans_tv.end())
	{
		chan = new channel(itChannel->second);
	}
	else
	{
		itChannel = allchans_radio.find( onid_sid);
		if (itChannel != allchans_radio.end())
		{
			chan = new channel(itChannel->second);
		}
	}
	return( chan);
}
