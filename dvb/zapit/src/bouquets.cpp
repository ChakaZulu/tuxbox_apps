/*
 * $Id: bouquets.cpp,v 1.52 2002/09/09 08:51:32 thegoodguy Exp $
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
#include <ext/hash_set>

#include <zapsi/sdt.h>

#include "bouquets.h"

extern tallchans allchans_tv, allchans_radio;   //  defined in zapit.cpp

/**** class CBouquet ********************************************************/
CBouquet::CBouquet(const CBouquet& bouquet)
{
        Name = bouquet.Name;
        bHidden = bouquet.bHidden;
        bLocked = bouquet.bLocked;
        for(unsigned int i = 0; i < bouquet.tvChannels.size(); i++)
                addService(new CZapitChannel(*(bouquet.tvChannels[i])));
        for(unsigned int i = 0; i < bouquet.radioChannels.size(); i++)
                addService(new CZapitChannel(*(bouquet.radioChannels[i])));
}

CBouquet::~CBouquet()
{
	for (unsigned int i=0; i<tvChannels.size(); i++)
		delete tvChannels[i];
	for (unsigned int i=0; i<radioChannels.size(); i++)
		delete radioChannels[i];
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
		case RESERVED: // ?
		case DIGITAL_TELEVISION_SERVICE:
		case NVOD_REFERENCE_SERVICE:
		case NVOD_TIME_SHIFTED_SERVICE:
			channels = &tvChannels;
			break;
				
		case DIGITAL_RADIO_SOUND_SERVICE:
			channels = &radioChannels;
			break;
	}

	unsigned int i;
	for (i=0; (i<channels->size()) && ((*channels)[i]->getOnidSid() != onidSid); i++);

	if (i<channels->size())
		result = (*channels)[i];

	if ((serviceType == RESERVED) && (result == NULL))
	{
		result = getChannelByOnidSid(onidSid, 2);
	}

	return( result);
}

void CBouquet::addService (CZapitChannel* newChannel)
{
	switch (newChannel->getServiceType())
	{
		case DIGITAL_TELEVISION_SERVICE:
		case NVOD_REFERENCE_SERVICE:
		case NVOD_TIME_SHIFTED_SERVICE:
			tvChannels.push_back(newChannel);
			break;
			
		case DIGITAL_RADIO_SOUND_SERVICE:
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
			case DIGITAL_TELEVISION_SERVICE:
			case NVOD_REFERENCE_SERVICE:
			case NVOD_TIME_SHIFTED_SERVICE:
				channels = &tvChannels;
				break;

			case DIGITAL_RADIO_SOUND_SERVICE:
				channels = &radioChannels;
				break;
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
		case DIGITAL_TELEVISION_SERVICE:
		case NVOD_REFERENCE_SERVICE:
		case NVOD_TIME_SHIFTED_SERVICE:
			channels = &tvChannels;
			break;
			
		case DIGITAL_RADIO_SOUND_SERVICE:
			channels = &radioChannels;
			break;
	}
	if ((oldPosition < channels->size()) && (newPosition < channels->size()))
	{
		ChannelList::iterator it = channels->begin();

		advance(it, oldPosition);
		CZapitChannel* tmp = *it;
		channels->erase(it);

		advance(it, newPosition - oldPosition);
		channels->insert(it, tmp);
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

void CBouquetManager::parseBouquetsXml(const XMLTreeNode *root)
{
	XMLTreeNode *search=root->GetChild();
	XMLTreeNode *channel_node;

	if (search)
	{
		while (strcmp(search->GetType(), "Bouquet"))
		{
			search = search->GetNext();
		}

		unsigned int onid, sid;

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

				if (chan != NULL)
					newBouquet->addService(chan);

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
	renumServices();
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

void CBouquetManager::makeRemainingChannelsBouquet(unsigned int tvChanNr, unsigned int radioChanNr, __gnu_cxx::hash_set<uint32_t> *tvchans_processed, __gnu_cxx::hash_set<uint32_t> *radiochans_processed, const string strTitle)
//void CBouquetManager::makeRemainingChannelsBouquet(unsigned int tvChanNr, unsigned int radioChanNr, const string strTitle)
{
	ChannelList unnumberedChannels;

	deleteBouquet(remainChannels);
	remainChannels = addBouquet(strTitle);

	for (tallchans_iterator it=allchans_tv.begin(); it!=allchans_tv.end(); it++)
		if (tvchans_processed->find(it->second.getOnidSid()) == tvchans_processed->end())  // not found == not yet processed
			unnumberedChannels.push_back(&(it->second));

	sort(unnumberedChannels.begin(), unnumberedChannels.end(), CmpChannelByChName());

	for (unsigned int i = 0; i < unnumberedChannels.size(); i++)
	{
		CZapitChannel* chan = copyChannelByOnidSid(unnumberedChannels[i]->getOnidSid());
		allchans_tv.find(chan->getOnidSid())->second.setChannelNumber(tvChanNr);         // necessary?
		chan->setChannelNumber(tvChanNr++);
		remainChannels->addService(chan);
	}

	unnumberedChannels.clear();

	for (tallchans_iterator it = allchans_radio.begin(); it != allchans_radio.end(); it++)
		if (radiochans_processed->find(it->second.getOnidSid()) == radiochans_processed->end())  // not found == not yet processed
			unnumberedChannels.push_back(&(it->second));

	sort(unnumberedChannels.begin(), unnumberedChannels.end(), CmpChannelByChName());

	for (unsigned int i = 0; i < unnumberedChannels.size(); i++)
	{
		CZapitChannel* chan = copyChannelByOnidSid(unnumberedChannels[i]->getOnidSid());
		allchans_radio.find(chan->getOnidSid())->second.setChannelNumber(radioChanNr);         // necessary?
		chan->setChannelNumber(radioChanNr++);
		remainChannels->addService(chan);
	}

	if ((remainChannels->tvChannels.size() == 0) && (remainChannels->radioChannels.size() == 0))
	{
		deleteBouquet(remainChannels);
		remainChannels = NULL;
	}
	printf("[zapit:bouquets.cpp:makeRemainingChannelsBouquet] TV Channels #: %d, Radio Channels #: %d.\n", tvChanNr, radioChanNr);
}

void CBouquetManager::renumServices()
{
	__gnu_cxx::hash_set <uint32_t> tvchans_processed, radiochans_processed;
	int tvChanNr = 1;
	int radioChanNr = 1;

	for (unsigned int i = 0; i < Bouquets.size(); i++)
	{
		for (unsigned int j = 0; j < Bouquets[i]->tvChannels.size(); j++)
		{
			uint32_t OnidSid = Bouquets[i]->tvChannels[j]->getOnidSid();
			allchans_tv.find(OnidSid)->second.setChannelNumber(tvChanNr);         // necessary?
			Bouquets[i]->tvChannels[j]->setChannelNumber(tvChanNr++);
			tvchans_processed.insert(OnidSid);
		}
		for (unsigned int j = 0; j < Bouquets[i]->radioChannels.size(); j++)
		{
			uint32_t OnidSid = Bouquets[i]->radioChannels[j]->getOnidSid();
			allchans_radio.find(OnidSid)->second.setChannelNumber(radioChanNr);         // necessary?
			Bouquets[i]->radioChannels[j]->setChannelNumber(radioChanNr++);
			radiochans_processed.insert(OnidSid);
		}
	}

	printf("[zapit:bouquets.cpp:renumServices] In Bouquets: TV Channels #: %d, Radio Channels #: %d.\n", tvChanNr, radioChanNr);

	makeRemainingChannelsBouquet(tvChanNr, radioChanNr, &tvchans_processed, &radiochans_processed, (Bouquets.size() == 0) ? "Alle Kanäle" : "Andere");  // TODO: use locales

	storeBouquets();
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
	tallchans_iterator itChannel = allchans_tv.find(onid_sid);
	if (itChannel != allchans_tv.end())
	{
		chan = new CZapitChannel(itChannel->second);
		chan->setIsCopy(true);
	}
	else
	{
		itChannel = allchans_radio.find( onid_sid);
		if (itChannel != allchans_radio.end())
		{
			chan = new CZapitChannel(itChannel->second);
			chan->setIsCopy(true);
		}
	}
	return( chan);
}

CBouquetManager::ChannelIterator::ChannelIterator(CBouquetManager* owner, const bool TV)
{
	Owner = owner;
	tv = TV;
	if (Owner->Bouquets.size() == 0)
		c = -2;
	else
	{
		b = 0;
		c = -1; 
		(*this)++;
	}
}

CBouquetManager::ChannelIterator CBouquetManager::ChannelIterator::operator ++(int)
{
	if (c != -2)  // we can add if it's not the end marker
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
			c = -2;
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
	c = -2;
 end:
	return (*this);
}

int CBouquetManager::ChannelIterator::getLowestChannelNumberWithOnidSid(const unsigned int onid_sid)
{
	int i = 0;

	for (b = 0; b < Owner->Bouquets.size(); b++)
		for (c = 0; (unsigned int) c < getBouquet()->size(); c++, i++)
			if ((**this)->getOnidSid() == onid_sid)
			    return i;
	return -1; // not found
}
