/*
 * $Id: bouquets.cpp,v 1.22 2002/04/06 11:26:11 obi Exp $
 *
 * BouquetManager for zapit - d-box2 linux project
 *
 * (C) 2002 by Simplex <simplex@berlios.de>,
 *             rasc <rasc@berlios.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "bouquets.h"

/**** class CBouquet ********************************************************/
CBouquet::CBouquet( const CBouquet& bouquet)
{
	Name = bouquet.Name;
	bHidden = bouquet.bHidden;
	bLocked = bouquet.bLocked;
	for (unsigned int i=0; i< bouquet.tvChannels.size(); i++)
		addService( new channel(*(bouquet.tvChannels[i])));
	for (unsigned int i=0; i< bouquet.radioChannels.size(); i++)
		addService( new channel(*(bouquet.radioChannels[i])));
}

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

	ChannelList* channels = &tvChannels;
	switch (serviceType)
	{
		case 0:
		case 1:
		case 4: channels = &tvChannels; break;
		case 2: channels = &radioChannels; break;
	}

	uint i;
	for (i=0; i<channels->size(), (*channels)[i]->name != string(serviceName); i++);

	if (i<channels->size())
		result = (*channels)[i];

	if ((serviceType==0) && (result==NULL))
		result = getChannelByName(serviceName, 2);

	return( result);
}

//
// -- servicetype 0 queries TV and Radio Channels
//

channel* CBouquet::getChannelByOnidSid(uint onidSid, uint serviceType = 0)
{
	channel* result = NULL;

	ChannelList* channels = &tvChannels;
	switch (serviceType)
	{
		case 0:
		case 1:
		case 4: channels = &tvChannels; break;
		case 2: channels = &radioChannels; break;
	}

	uint i;
	for (i=0; (i<channels->size()) && ((*channels)[i]->OnidSid() != onidSid); i++);

	if (i<channels->size())
		result = (*channels)[i];

	if ((serviceType==0) && (result==NULL))
	{
		result = getChannelByOnidSid(onidSid, 2);
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
	if (oldChannel != NULL)
	{
		ChannelList* channels = &tvChannels;
		switch (oldChannel->service_type)
		{
			case 1:
			case 4: channels = &tvChannels; break;
			case 2: channels = &radioChannels; break;
		}

		ChannelList::iterator it = channels->begin();
		while ((it<channels->end()) && !(*it == oldChannel))
			it++;
		if (it<channels->end())
		{
			channels->erase(it);
			delete oldChannel;
		}
	}
}

void CBouquet::moveService(  char* serviceName, uint newPosition, uint serviceType)
{
	ChannelList* channels = &tvChannels;
	switch (serviceType)
	{
		case 1:
		case 4: channels = &tvChannels; break;
		case 2: channels = &radioChannels; break;
	}

	uint i=0;
	ChannelList::iterator it = channels->begin();
	while ((it<=channels->end()) && ((*it)->name != string(serviceName)))
	{
		it++;
		i++;
	}
	if (it<channels->end())
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
	ChannelList* channels = &tvChannels;
	switch (serviceType)
	{
		case 1:
		case 4: channels = &tvChannels; break;
		case 2: channels = &radioChannels; break;
	}
	if ((oldPosition < channels->size()) && (newPosition < channels->size()))
	{
		ChannelList::iterator itOld, itNew;
		uint i;
		for (i=0, itOld = channels->begin(); i<oldPosition; i++, itOld++);
		for (i=0, itNew = channels->begin(); i<newPosition; i++, itNew++);

		channel* tmp = (*channels)[oldPosition];
		channels->erase( itOld);
		channels->insert( itNew, tmp);
	}
}


/**** class CBouquetManager *************************************************/

string CBouquetManager::convertForXML( string s)
{
	string r;
	uint i;
	for (i=0; i<s.length(); i++)
	{
		switch (s[i])
		{
		  case '&':
			r += "&amp;";
		  break;
		  case '<':
			r += "&lt;";
		  break;
		  case '\"':
			r += "&quot;";
		  break;
		  default:
			if ((s[i]>=32) && (s[i]<128))
				r += s[i];
			else if (s[i] > 128)
			{
				char val[5];
				sprintf(val, "%d", s[i]);
				r = r + "&#" + val + ";";
			}
		}
	}
	return(r);
}

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
		if (Bouquets[i] != remainChannels)
		{
			fprintf(bouq_fd, "\t<Bouquet name=\"%s\" hidden=\"%d\" locked=\"%d\">\n",
			        convertForXML(Bouquets[i]->Name).c_str(),
			        Bouquets[i]->bHidden ? 1 : 0,
			        Bouquets[i]->bLocked ? 1 : 0);
			for ( uint j=0; j<Bouquets[i]->tvChannels.size(); j++)
			{
				fprintf(bouq_fd, "\t\t<channel serviceID=\"%04x\" name=\"%s\" onid=\"%04x\"/>\n",
						Bouquets[i]->tvChannels[j]->sid,
						convertForXML(Bouquets[i]->tvChannels[j]->name).c_str(),
						Bouquets[i]->tvChannels[j]->onid);
			}
			for ( uint j=0; j<Bouquets[i]->radioChannels.size(); j++)
			{
				fprintf(bouq_fd, "\t\t<channel serviceID=\"%04x\" name=\"%s\" onid=\"%04x\"/>\n",
						Bouquets[i]->radioChannels[j]->sid,
						convertForXML(Bouquets[i]->radioChannels[j]->name).c_str(),
						Bouquets[i]->radioChannels[j]->onid);
			}
			fprintf(bouq_fd, "\t</Bouquet>\n");
		}
	}
	fprintf(bouq_fd, "</ZAPIT>\n");
	fclose(bouq_fd);
}

void CBouquetManager::parseBouquetsXml(XMLTreeNode *root)
{
	XMLTreeNode *search=root->GetChild();
	XMLTreeNode *channel_node;

	int nChNrRadio = 1;
	int nChNrTV = 1;

	if (search)
	{
		while (strcmp(search->GetType(), "Bouquet"))
		{
			search = search->GetNext();
		}

		uint onid, sid;

		numchans_tv.clear();
		numchans_radio.clear();

		printf("[zapit] reading Bouquets ");
		while ((search) && (!(strcmp(search->GetType(), "Bouquet"))))
		{
			CBouquet* newBouquet = addBouquet( search->GetAttributeValue("name"));
			char* hidden = search->GetAttributeValue("hidden");
			char* locked = search->GetAttributeValue("locked");
			newBouquet->bHidden = hidden ? (strcmp(hidden, "1") == 0) : false;
			newBouquet->bLocked = locked ? (strcmp(locked, "1") == 0) : false;
			channel_node = search->GetChild();

			while (channel_node)
			{
				sscanf(channel_node->GetAttributeValue("serviceID"), "%x", &sid);
				sscanf(channel_node->GetAttributeValue("onid"), "%x", &onid);

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
			printf(".");
/*		printf(
			"[zapit] Bouquet %s with %d tv- and %d radio-channels.\n",
			newBouquet->Name.c_str(),
			newBouquet->tvChannels.size(),
			newBouquet->radioChannels.size());
*/
			search = search->GetNext();
		}
	}

	makeRemainingChannelsBouquet( nChNrTV, nChNrRadio, (Bouquets.size() == 0) ? "Alle Kanäle" : "Andere");  // TODO: use locales

	printf("\n[zapit] Found %d bouquets.\n", Bouquets.size());

}

void CBouquetManager::loadBouquets(bool ignoreBouquetFile = false)
{
	FILE* in;
	XMLTreeParser* parser;
	if (!ignoreBouquetFile)
	{
		parser=new XMLTreeParser("ISO-8859-1");
		in=fopen(CONFIGDIR "/zapit/bouquets.xml", "r");
		if (!in)
		{
			perror("[zapit] " CONFIGDIR "/zapit/bouquets.xml");
			ignoreBouquetFile = true;
		}
	}

	if (!ignoreBouquetFile)
	{
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
	else
	{
		makeRemainingChannelsBouquet( 1, 1, "Alle Kanäle");    // TODO: use locales
	}
	storeBouquets();
}

void CBouquetManager::storeBouquets()
{
	for (unsigned int i=0; i<storedBouquets.size(); i++)
	{
		delete storedBouquets[i];
	}
	storedBouquets.clear();

	for (unsigned int i=0; i<Bouquets.size(); i++)
	{
		storedBouquets.insert( storedBouquets.end(), new CBouquet( *Bouquets[i]));
	}
}

void CBouquetManager::restoreBouquets()
{
	for (unsigned int i=0; i<Bouquets.size(); i++)
	{
		delete Bouquets[i];
	}
	Bouquets.clear();

	for (unsigned int i=0; i<storedBouquets.size(); i++)
	{
		Bouquets.insert( Bouquets.end(), new CBouquet( *storedBouquets[i]));
	}
}

void CBouquetManager::makeRemainingChannelsBouquet( unsigned int tvChanNr, unsigned int radioChanNr, string strTitle )
{
	ChannelList allChannels;
	ChannelList numberedChannels;
	ChannelList unnumberedChannels;

	if (remainChannels != NULL)
	{
		deleteBouquet(strTitle);
	}
	remainChannels = addBouquet(strTitle);

	for ( map<uint, channel>::iterator it=allchans_tv.begin(); it!=allchans_tv.end(); it++)
	{
		if (it->second.chan_nr > 0)
			numberedChannels.insert( numberedChannels.end(), &(it->second));
		else
			unnumberedChannels.insert( unnumberedChannels.end(), &(it->second));
	}
	sort(numberedChannels.begin(), numberedChannels.end(), CmpChannelByChNr());
	sort(unnumberedChannels.begin(), unnumberedChannels.end(), CmpChannelByChName());

	for (uint i = 0; i<numberedChannels.size(); i++)
	{
		allChannels.insert( allChannels.end(), numberedChannels[i]);
	}
	for (uint i = 0; i<unnumberedChannels.size(); i++)
	{
		allChannels.insert( allChannels.end(), unnumberedChannels[i]);
	}

	for ( uint i=0; i<allChannels.size(); i++)
	{
		if (tvChannelsFind( allChannels[i]->OnidSid()) == tvChannelsEnd())
		{
			channel* chan = copyChannelByOnidSid( allChannels[i]->OnidSid());
			chan->chan_nr = tvChanNr++;
			remainChannels->addService( chan);
		}
	}

	allChannels.clear();
	numberedChannels.clear();
	unnumberedChannels.clear();

	for ( map<uint, channel>::iterator it=allchans_radio.begin(); it!=allchans_radio.end(); it++)
	{
		if (it->second.chan_nr > 0)
			numberedChannels.insert( numberedChannels.end(), &(it->second));
		else
			unnumberedChannels.insert( unnumberedChannels.end(), &(it->second));
	}
	sort(numberedChannels.begin(), numberedChannels.end(), CmpChannelByChNr());
	sort(unnumberedChannels.begin(), unnumberedChannels.end(), CmpChannelByChName());

	for (uint i = 0; i<numberedChannels.size(); i++)
	{
		allChannels.insert( allChannels.end(), numberedChannels[i]);
	}
	for (uint i = 0; i<unnumberedChannels.size(); i++)
	{
		allChannels.insert( allChannels.end(), unnumberedChannels[i]);
	}

	for ( uint i=0; i<allChannels.size(); i++)
	{
		if (radioChannelsFind( allChannels[i]->OnidSid()) == radioChannelsEnd())
		{
			channel* chan = copyChannelByOnidSid( allChannels[i]->OnidSid());
			chan->chan_nr = radioChanNr++;
			remainChannels->addService( chan);
		}
	}

	for ( map<uint, channel>::iterator it=allchans_radio.begin(); it!=allchans_radio.end(); it++)
	{
		if (radioChannelsFind( it->second.OnidSid()) == radioChannelsEnd())
		{
			channel* chan = copyChannelByOnidSid( it->second.OnidSid());
			chan->chan_nr = radioChanNr++;
			remainChannels->addService( chan);
		}
	}

	if ((remainChannels->tvChannels.size() == 0) && (remainChannels->radioChannels.size() == 0))
	{
		deleteBouquet(strTitle);
		remainChannels = NULL;
	}
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
			numchans_tv.insert(std::pair<uint, uint>(nChNrTV++, Bouquets[i]->tvChannels[j]->OnidSid()));
		}
		for (uint j=0; j<Bouquets[i]->radioChannels.size(); j++)
		{
			Bouquets[i]->radioChannels[j]->chan_nr = nChNrRadio;
			numchans_radio.insert(std::pair<uint, uint>(nChNrRadio++, Bouquets[i]->radioChannels[j]->OnidSid()));
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
	if (id < Bouquets.size() && Bouquets[id] != remainChannels)
	{
		CBouquet* bouquet = Bouquets[id];

		BouquetList::iterator it;
		uint i;
		for (i=0, it = Bouquets.begin(); i<id; i++, it++);
		Bouquets.erase( it);
		delete bouquet;
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

//
// -- Find Bouquet-Name, if BQ exists   (2002-04-02 rasc)
// -- Return: Bouqet-ID (found: 0..n)  or -1 (Bouquet does not exist)
//
int CBouquetManager::existsBouquet( string name)
{
	uint i;
	for (i=0; (i<Bouquets.size()) && (Bouquets[i]->Name != name); i++);
	return (i<Bouquets.size()) ?(int)i :(int)-1;
}


//
// -- Check if channel exists in BQ   (2002-04-05 rasc)
// -- Return: True/false
//
bool CBouquetManager::existsChannelInBouquet( unsigned int bq_id, unsigned int onid_sid)
{
	bool     status = false;
	channel  *ch = NULL;

	if (bq_id >= 0 && bq_id <= Bouquets.size()) {
		// query TV-Channels  && Radio channels
		ch = Bouquets[bq_id]->getChannelByOnidSid (onid_sid, 0);
		if (ch)  status = true;
	}

	return status;

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

CBouquetManager::tvChannelIterator CBouquetManager::tvChannelsBegin()
{
	uint B=0;
	while ((B<Bouquets.size()) && (Bouquets[B]->tvChannels.size()==0)) B++;
	if (B<Bouquets.size())
		return( tvChannelIterator(this, B));
	else
		return tvChannelsEnd();
}

CBouquetManager::tvChannelIterator CBouquetManager::tvChannelsFind( unsigned int onid_sid)
{
	tvChannelIterator it = tvChannelsBegin();
	while ((it != tvChannelsEnd()) && ((*it)->OnidSid() != onid_sid))
		it++;
	return( it);
}

CBouquetManager::tvChannelIterator CBouquetManager::tvChannelIterator::operator ++(int)
{
	if ((b==-1) && (c==-1))
		return(*this);
	c++;
	if (c >= Owner->Bouquets[b]->tvChannels.size())
	{
		c = 0;
		do
		{
			b++;
		} while ((b < Owner->Bouquets.size()) && (Owner->Bouquets[b]->tvChannels.size()==0));
	}
	if ( b >= Owner->Bouquets.size())
	{
		b=-1; c=-1;
	}
	return(*this);
}

channel* CBouquetManager::tvChannelIterator::operator *()
{
	return( Owner->Bouquets[b]->tvChannels[c]);
}

bool CBouquetManager::tvChannelIterator::operator != (const tvChannelIterator& it) const
{
	return( (b != it.b) || (c != it.c));
}

bool CBouquetManager::tvChannelIterator::operator == (const tvChannelIterator& it) const
{
	return( (b == it.b) && (c == it.c));
}

CBouquetManager::radioChannelIterator CBouquetManager::radioChannelsBegin()
{
	uint B=0;
	while ((B<Bouquets.size()) && (Bouquets[B]->radioChannels.size()==0)) B++;
	if (B<Bouquets.size())
		return( radioChannelIterator(this, B));
	else
		return radioChannelsEnd();
}

CBouquetManager::radioChannelIterator CBouquetManager::radioChannelsFind( unsigned int onid_sid)
{
	radioChannelIterator it = radioChannelsBegin();
	while ((it != radioChannelsEnd()) && ((*it)->OnidSid() != onid_sid))
	{
		it++;
	}
	return( it);
}

CBouquetManager::radioChannelIterator CBouquetManager::radioChannelIterator::operator ++(int)
{
	if ((b==-1) && (c==-1))
		return(*this);
	c++;
	if (c >= Owner->Bouquets[b]->radioChannels.size())
	{
		c = 0;
		do
		{
			b++;
		} while ((b < Owner->Bouquets.size()) && (Owner->Bouquets[b]->radioChannels.size()==0));
	}
	if ( b >= Owner->Bouquets.size())
	{
		b=-1; c=-1;
	}
	return(*this);
}

channel* CBouquetManager::radioChannelIterator::operator *()
{
	return( Owner->Bouquets[b]->radioChannels[c]);
}

bool CBouquetManager::radioChannelIterator::operator != (const radioChannelIterator& it) const
{
	return( (b != it.b) || (c != it.c));
}

bool CBouquetManager::radioChannelIterator::operator == (const radioChannelIterator& it) const
{
	return( (b == it.b) && (c == it.c));
}

