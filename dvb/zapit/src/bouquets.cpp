/*
 * $Id: bouquets.cpp,v 1.39 2002/08/29 18:21:57 obi Exp $
 *
 * BouquetManager for zapit - d-box2 linux project
 *
 * (C) 2002 by Simplex <simplex@berlios.de>,
 *	       rasc    <rasc@berlios.de>
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

#include <map>

#include "bouquets.h"

extern std::map <unsigned int, CZapitChannel> allchans_tv;
extern std::map <unsigned int, unsigned int> numchans_tv;
extern std::map <std::string, unsigned int> namechans_tv;
extern std::map <unsigned int, CZapitChannel> allchans_radio;
extern std::map <unsigned int, unsigned int> numchans_radio;
extern std::map <std::string, unsigned int> namechans_radio;

/**** class CBouquet ********************************************************/
CBouquet::CBouquet (const CBouquet& bouquet)
{
	Name = bouquet.Name;
	bHidden = bouquet.bHidden;
	bLocked = bouquet.bLocked;
	for (unsigned int i=0; i< bouquet.tvChannels.size(); i++)
		addService( new CZapitChannel(*(bouquet.tvChannels[i])));
	for (unsigned int i=0; i< bouquet.radioChannels.size(); i++)
		addService( new CZapitChannel(*(bouquet.radioChannels[i])));
}

CBouquet::~CBouquet()
{
	for (unsigned int i=0; i<tvChannels.size(); i++)
		delete tvChannels[i];
	for (unsigned int i=0; i<radioChannels.size(); i++)
		delete radioChannels[i];
}

CZapitChannel* CBouquet::getChannelByName (char* serviceName, unsigned char serviceType)
{
	CZapitChannel* result = NULL;

	ChannelList* channels = &tvChannels;
	switch (serviceType)
	{
		case 0:
		case 1:
		case 4: channels = &tvChannels; break;
		case 2: channels = &radioChannels; break;
	}

	unsigned int i;
	for (i=0; i<channels->size(), (*channels)[i]->getName() != string(serviceName); i++);

	if (i<channels->size())
		result = (*channels)[i];

	if ((serviceType==0) && (result==NULL))
		result = getChannelByName(serviceName, 2);

	return( result);
}

//
// -- servicetype 0 queries TV and Radio Channels
//

CZapitChannel* CBouquet::getChannelByOnidSid (unsigned int onidSid, unsigned char serviceType)
{
	CZapitChannel* result = NULL;

	ChannelList* channels = &tvChannels;
	switch (serviceType)
	{
		case 0:
		case 1:
		case 4: channels = &tvChannels; break;
		case 2: channels = &radioChannels; break;
	}

	unsigned int i;
	for (i=0; (i<channels->size()) && ((*channels)[i]->getOnidSid() != onidSid); i++);

	if (i<channels->size())
		result = (*channels)[i];

	if ((serviceType==0) && (result==NULL))
	{
		result = getChannelByOnidSid(onidSid, 2);
	}

	return( result);
}

void CBouquet::addService (CZapitChannel* newChannel)
{
	switch (newChannel->getServiceType())
	{
		case 1:
		case 4:
			tvChannels.push_back(newChannel);
		break;
		case 2:
			radioChannels.push_back(newChannel);
		break;
	}
}

void CBouquet::removeService (CZapitChannel* oldChannel)
{
	if (oldChannel != NULL)
	{
		ChannelList* channels = &tvChannels;
		switch (oldChannel->getServiceType())
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

void CBouquet::moveService (char* serviceName, unsigned int newPosition, unsigned char serviceType)
{
	ChannelList* channels = &tvChannels;
	switch (serviceType)
	{
		case 1:
		case 4: channels = &tvChannels; break;
		case 2: channels = &radioChannels; break;
	}

	unsigned int i=0;
	ChannelList::iterator it = channels->begin();
	while ((it<=channels->end()) && ((*it)->getName() != string(serviceName)))
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
void CBouquet::moveService(  unsigned int onidSid, unsigned int newPosition)
{
}
*/

void CBouquet::moveService (unsigned int oldPosition, unsigned int newPosition, unsigned char serviceType)
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
		unsigned int i;
		for (i=0, itOld = channels->begin(); i<oldPosition; i++, itOld++);
		for (i=0, itNew = channels->begin(); i<newPosition; i++, itNew++);

		CZapitChannel* tmp = (*channels)[oldPosition];
		channels->erase( itOld);
		channels->insert( itNew, tmp);
	}
}

int CBouquet::recModeRadioSize (unsigned int tsid)
{
	int size = 0;
	for ( unsigned int i=0; i< tvChannels.size(); i++)
	{
		if ( tsid == tvChannels[i]->getTsidOnid())
			size++;
	}
	return(size);
}

int CBouquet::recModeTVSize( unsigned int tsid)
{
	int size = 0;
	for ( unsigned int i=0; i< radioChannels.size(); i++)
	{
		if ( tsid == radioChannels[i]->getTsidOnid())
			size++;
	}
	return(size);
}


/**** class CBouquetManager *************************************************/

string CBouquetManager::convertForXML( string s)
{
	string r;
	unsigned int i;
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
			r += s[i];
			break;
#if 0
			// comparison is always true due to limited range of data type
			if ((s[i]>=32) && (s[i]<128))
				r += s[i];

			// comparison is always false due to limited range of data type
			else if (s[i] > 128)
			{
				char val[5];
				sprintf(val, "%d", s[i]);
				r = r + "&#" + val + ";";
			}
#endif
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

	for (unsigned int i=0; i<Bouquets.size(); i++)
	{
		if (Bouquets[i] != remainChannels)
		{
			fprintf(bouq_fd, "\t<Bouquet name=\"%s\" hidden=\"%d\" locked=\"%d\">\n",
				convertForXML(Bouquets[i]->Name).c_str(),
				Bouquets[i]->bHidden ? 1 : 0,
				Bouquets[i]->bLocked ? 1 : 0);
			for ( unsigned int j=0; j<Bouquets[i]->tvChannels.size(); j++)
			{
				fprintf(bouq_fd, "\t\t<channel serviceID=\"%04x\" name=\"%s\" onid=\"%04x\"/>\n",
						Bouquets[i]->tvChannels[j]->getServiceId(),
						convertForXML(Bouquets[i]->tvChannels[j]->getName()).c_str(),
						Bouquets[i]->tvChannels[j]->getOriginalNetworkId());
			}
			for ( unsigned int j=0; j<Bouquets[i]->radioChannels.size(); j++)
			{
				fprintf(bouq_fd, "\t\t<channel serviceID=\"%04x\" name=\"%s\" onid=\"%04x\"/>\n",
						Bouquets[i]->radioChannels[j]->getServiceId(),
						convertForXML(Bouquets[i]->radioChannels[j]->getName()).c_str(),
						Bouquets[i]->radioChannels[j]->getOriginalNetworkId());
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

		unsigned int onid, sid;

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

				CZapitChannel* chan = copyChannelByOnidSid( (onid << 16) + sid);

				if ( chan != NULL)
				{
					switch (chan->getServiceType())
					{
						case 1:
						case 4:
							chan->setChannelNumber(nChNrTV);
							newBouquet->addService(chan);
							numchans_tv.insert(std::pair<unsigned int, unsigned int>(nChNrTV++, (onid<<16)+sid));
						break;
						case 2:
							chan->setChannelNumber(nChNrRadio);
							newBouquet->addService(chan);
							numchans_radio.insert(std::pair<unsigned int, unsigned int>(nChNrRadio++, (onid<<16)+sid));
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

void CBouquetManager::loadBouquets(bool ignoreBouquetFile)
{
	FILE * in;
	XMLTreeParser * parser;
	
	if (ignoreBouquetFile == false)
	{
		char buf[2048];
		int done;

		in = fopen(CONFIGDIR "/zapit/bouquets.xml", "r");

		if (!in)
		{
			perror("[zapit] " CONFIGDIR "/zapit/bouquets.xml");
		}
		else
		{
			parser = new XMLTreeParser("ISO-8859-1");

			do
			{
				unsigned int len = fread(buf, 1, sizeof(buf), in);
				done = len < sizeof(buf);

				if (!parser->Parse(buf, len, done))
				{
					printf("[zapit] parse error: %s at line %d\n",
					parser->ErrorString(parser->GetErrorCode()),
					parser->GetCurrentLineNumber());
					fclose(in);
					delete parser;
					return;
				}
			}
			while (!done);

			if (parser->RootNode())
				parseBouquetsXml(parser->RootNode());

			delete parser;

			fclose(in);
		}
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
		storedBouquets.push_back(new CBouquet( *Bouquets[i]));
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
		Bouquets.push_back(new CBouquet( *storedBouquets[i]));
	}
}

void CBouquetManager::makeRemainingChannelsBouquet( unsigned int tvChanNr, unsigned int radioChanNr, string strTitle )
{
	ChannelList allChannels;
	ChannelList numberedChannels;
	ChannelList unnumberedChannels;

	deleteBouquet(remainChannels);
	remainChannels = addBouquet(strTitle);

	for ( map<unsigned int, CZapitChannel>::iterator it=allchans_tv.begin(); it!=allchans_tv.end(); it++)
	{
		if (it->second.getChannelNumber() > 0)
			numberedChannels.push_back(&(it->second));
		else
			unnumberedChannels.push_back(&(it->second));
	}
	sort(numberedChannels.begin(), numberedChannels.end(), CmpChannelByChNr());
	sort(unnumberedChannels.begin(), unnumberedChannels.end(), CmpChannelByChName());

	for (unsigned int i = 0; i<numberedChannels.size(); i++)
		allChannels.push_back(numberedChannels[i]);
	for (unsigned int i = 0; i<unnumberedChannels.size(); i++)
		allChannels.push_back(unnumberedChannels[i]);

	for ( unsigned int i=0; i<allChannels.size(); i++)
	{
		if (tvChannelsFind( allChannels[i]->getOnidSid()).EndOfChannels())
		{
			CZapitChannel* chan = copyChannelByOnidSid( allChannels[i]->getOnidSid());
			chan->setChannelNumber(tvChanNr++);
			remainChannels->addService( chan);
		}
	}

	allChannels.clear();
	numberedChannels.clear();
	unnumberedChannels.clear();

	for ( map<unsigned int, CZapitChannel>::iterator it=allchans_radio.begin(); it!=allchans_radio.end(); it++)
	{
		if (it->second.getChannelNumber() > 0)
			numberedChannels.push_back(&(it->second));
		else
			unnumberedChannels.push_back(&(it->second));
	}
	sort(numberedChannels.begin(), numberedChannels.end(), CmpChannelByChNr());
	sort(unnumberedChannels.begin(), unnumberedChannels.end(), CmpChannelByChName());

	for (unsigned int i = 0; i<numberedChannels.size(); i++)
		allChannels.push_back(numberedChannels[i]);
	for (unsigned int i = 0; i<unnumberedChannels.size(); i++)
		allChannels.push_back(unnumberedChannels[i]);

	for ( unsigned int i=0; i<allChannels.size(); i++)
	{
		if (radioChannelsFind( allChannels[i]->getOnidSid()).EndOfChannels())
		{
			CZapitChannel* chan = copyChannelByOnidSid( allChannels[i]->getOnidSid());
			chan->setChannelNumber(radioChanNr++);
			remainChannels->addService( chan);
		}
	}

	for ( map<unsigned int, CZapitChannel>::iterator it=allchans_radio.begin(); it!=allchans_radio.end(); it++)
	{
		if (radioChannelsFind( it->second.getOnidSid()).EndOfChannels())
		{
			CZapitChannel* chan = copyChannelByOnidSid( it->second.getOnidSid());
			chan->setChannelNumber(radioChanNr++);
			remainChannels->addService( chan);
		}
	}

	if ((remainChannels->tvChannels.size() == 0) && (remainChannels->radioChannels.size() == 0))
	{
		deleteBouquet(remainChannels);
		remainChannels = NULL;
	}
}

void CBouquetManager::renumServices()
{
	int nChNrRadio = 1;
	int nChNrTV = 1;

	numchans_tv.clear();
	numchans_radio.clear();

	for (unsigned int i=0; i<Bouquets.size(); i++)
	{
		for (unsigned int j=0; j<Bouquets[i]->tvChannels.size(); j++)
		{
			Bouquets[i]->tvChannels[j]->setChannelNumber(nChNrTV);
			numchans_tv.insert(std::pair<unsigned int, unsigned int>(nChNrTV++, Bouquets[i]->tvChannels[j]->getOnidSid()));
		}
		for (unsigned int j=0; j<Bouquets[i]->radioChannels.size(); j++)
		{
			Bouquets[i]->radioChannels[j]->setChannelNumber(nChNrRadio);
			numchans_radio.insert(std::pair<unsigned int, unsigned int>(nChNrRadio++, Bouquets[i]->radioChannels[j]->getOnidSid()));
		}
	}

	map<unsigned int, unsigned int>::iterator	numit;
	map<std::string, unsigned int>::iterator nameit;
	map<unsigned int, CZapitChannel>::iterator     cit;

	extern map<unsigned int, unsigned int> allnumchannels_tv;
	extern map<unsigned int, unsigned int> allnumchannels_radio;
	extern map<std::string, unsigned int> allnamechannels_tv;
	extern map<std::string, unsigned int> allnamechannels_radio;


	int number = 1;
	allnumchannels_tv.clear();
	allnamechannels_tv.clear();
	allnumchannels_radio.clear();
	allnamechannels_radio.clear();
	for (numit = numchans_tv.begin(); numit != numchans_tv.end(); numit++)
	{
		cit = allchans_tv.find(numit->second);
		cit->second.setChannelNumber(number);
		allnumchannels_tv.insert(std::pair<unsigned int,unsigned int>(number++, cit->second.getOnidSid()));
		allnamechannels_tv.insert(std::pair<std::string, unsigned int>(cit->second.getName(), cit->second.getOnidSid()));
	}
	numchans_tv.clear();

	for (nameit = namechans_tv.begin(); nameit != namechans_tv.end(); nameit++)
	{
		cit = allchans_tv.find(nameit->second);
		cit->second.setChannelNumber(number);
		allnumchannels_tv.insert(std::pair<unsigned int, unsigned int>(number++, cit->second.getOnidSid()));
		allnamechannels_tv.insert(std::pair<std::string, unsigned int>(nameit->first, cit->second.getOnidSid()));
	}
	namechans_tv.clear();

	number = 1;
	for (numit = numchans_radio.begin(); numit != numchans_radio.end(); numit++)
	{
		cit = allchans_radio.find(numit->second);
		cit->second.setChannelNumber(number);
		allnumchannels_radio.insert(std::pair<unsigned int,unsigned int>(number++, cit->second.getOnidSid()));
		allnamechannels_radio.insert(std::pair<std::string, unsigned int>(cit->second.getName(), cit->second.getOnidSid()));
	}
	numchans_radio.clear();

	for (nameit = namechans_radio.begin(); nameit != namechans_radio.end(); nameit++)
	{
		cit = allchans_radio.find(nameit->second);
		cit->second.setChannelNumber(number);
		allnumchannels_radio.insert(std::pair<unsigned int, unsigned int>(number++, cit->second.getOnidSid()));
		allnamechannels_radio.insert(std::pair<std::string, unsigned int>(nameit->first, cit->second.getOnidSid()));
	}
	namechans_radio.clear();
}

CBouquet* CBouquetManager::addBouquet( string name)
{
	CBouquet* newBouquet = new CBouquet(name);
	Bouquets.push_back(newBouquet);
	return( newBouquet);
}

void CBouquetManager::deleteBouquet(const unsigned int id)
{
	if (id < Bouquets.size() && Bouquets[id] != remainChannels)
		deleteBouquet(Bouquets[id]);
}

void CBouquetManager::deleteBouquet(const CBouquet* bouquet)
{
	if (bouquet != NULL)
	{
		BouquetList::iterator it = find(Bouquets.begin(), Bouquets.end(), bouquet);

		if (it != Bouquets.end())
		{
			Bouquets.erase(it);
			delete bouquet;
		}
	}
}

//
// -- Find Bouquet-Name, if BQ exists   (2002-04-02 rasc)
// -- Return: Bouqet-ID (found: 0..n)  or -1 (Bouquet does not exist)
//
int CBouquetManager::existsBouquet( string name)
{
	unsigned int i;
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
	CZapitChannel  *ch = NULL;

	if (bq_id >= 0 && bq_id <= Bouquets.size()) {
		// query TV-Channels  && Radio channels
		ch = Bouquets[bq_id]->getChannelByOnidSid (onid_sid, 0);
		if (ch)  status = true;
	}

	return status;

}


void CBouquetManager::moveBouquet( unsigned int oldId, unsigned int newId)
{
	if ((oldId < Bouquets.size()) && (newId < Bouquets.size()))
	{
		BouquetList::iterator itOld, itNew;
		unsigned int i;
		for (i=0, itOld = Bouquets.begin(); i<oldId; i++, itOld++);
		for (i=0, itNew = Bouquets.begin(); i<newId; i++, itNew++);

		CBouquet* tmp = Bouquets[oldId];
		Bouquets.erase( itOld);
		Bouquets.insert( itNew, tmp);
	}
}

void CBouquetManager::saveAsLast( unsigned int BouquetId, unsigned int channelNr)
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

void CBouquetManager::getLast( unsigned int* BouquetId, unsigned int* channelNr)
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
	for (unsigned int i=0; i<Bouquets.size(); i++)
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

CZapitChannel* CBouquetManager::copyChannelByOnidSid( unsigned int onid_sid)
{
	CZapitChannel* chan = NULL;
	map<unsigned int, CZapitChannel>::iterator itChannel = allchans_tv.find(onid_sid);
	if (itChannel != allchans_tv.end())
	{
		chan = new CZapitChannel(itChannel->second);
	}
	else
	{
		itChannel = allchans_radio.find( onid_sid);
		if (itChannel != allchans_radio.end())
		{
			chan = new CZapitChannel(itChannel->second);
		}
	}
	return( chan);
}

CBouquetManager::ChannelIterator CBouquetManager::ChannelIterator::operator ++(int)
{
	if ((c != -2) || (b != 0))   // we can add if it's not the end marker
	{
		c++;
		if ((unsigned int) c >= getBouquet()->size())
		{
			for (b++; b < Owner->Bouquets.size(); b++)
				if (getBouquet()->size() != 0)
				{
					c = 0;
					goto end;
				}
			b = 0; c = -2;
		}
	}
 end:
	return(*this);
}

CZapitChannel* CBouquetManager::ChannelIterator::operator *()
{
	return (*getBouquet())[c];               // returns junk if we are an end marker !!
}

CBouquetManager::ChannelIterator CBouquetManager::ChannelIterator::FindChannelNr(const unsigned int channel)
{
	c = channel;
	for (b = 0; b < Owner->Bouquets.size(); b++)
		if (getBouquet()->size() > (unsigned int)c)
			goto end;
		else
			c -= getBouquet()->size();
	b = 0; c = -2;
 end:
	return (*this);
}

CBouquetManager::ChannelIterator CBouquetManager::tvChannelsFind(const unsigned int onid_sid)
{
	ChannelIterator it = tvChannelsBegin();
	while ((!it.EndOfChannels()) && ((*it)->getOnidSid() != onid_sid))
		it++;
	return it;
}

CBouquetManager::ChannelIterator CBouquetManager::radioChannelsFind(const unsigned int onid_sid)
{
	ChannelIterator it = radioChannelsBegin();
	while ((!it.EndOfChannels()) && ((*it)->getOnidSid() != onid_sid))
		it++;
	return it;
}
