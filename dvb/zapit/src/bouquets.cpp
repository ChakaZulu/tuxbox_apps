/*
 * $Id: bouquets.cpp,v 1.91 2003/09/17 10:49:52 thegoodguy Exp $
 *
 * BouquetManager for zapit - d-box2 linux project
 *
 * (C) 2002 by Simplex    <simplex@berlios.de>,
 *             rasc       <rasc@berlios.de>,
 *             thegoodguy <thegoodguy@berlios.de>
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
#include <set>

#include <unistd.h>

/* tuxbox headers */
#include <configfile.h>

#include <zapit/bouquets.h>
#include <zapit/debug.h>
#include <zapit/getservices.h> /* LoadServices */
#include <zapit/sdt.h>
#include <zapit/settings.h>
#include <zapit/xmlinterface.h>
#include <zapit/frontend.h>

extern tallchans allchans;   //  defined in zapit.cpp
extern CConfigFile config;   //  defined in zapit.cpp
extern std::map<std::string, int16_t>satellitePositions;
extern CFrontend *frontend;

char *getFrontendName(void);
void cp(char * from, char * to);


#define GET_ATTR(node, name, fmt, arg)                                  \
        do {                                                            \
                char * ptr = xmlGetAttribute(node, name);               \
                if ((ptr == NULL) || (sscanf(ptr, fmt, &arg) <= 0))     \
                        arg = 0;                                        \
        }                                                               \
        while (0)


/**** class CBouquet ********************************************************/

//
// -- servicetype 0 queries TV and Radio Channels
//

CZapitChannel* CBouquet::getChannelByChannelID(const t_channel_id channel_id, const unsigned char serviceType)
{
	CZapitChannel* result = NULL;

	ChannelList* channels = &tvChannels;
	
	switch (serviceType)
	{
		case ST_RESERVED: // ?
		case ST_DIGITAL_TELEVISION_SERVICE:
		case ST_NVOD_REFERENCE_SERVICE:
		case ST_NVOD_TIME_SHIFTED_SERVICE:
			channels = &tvChannels;
			break;
				
		case ST_DIGITAL_RADIO_SOUND_SERVICE:
			channels = &radioChannels;
			break;
	}

	unsigned int i;
	for (i=0; (i<channels->size()) && ((*channels)[i]->getChannelID() != channel_id); i++);

	if (i<channels->size())
		result = (*channels)[i];

	if ((serviceType == ST_RESERVED) && (result == NULL))
	{
		result = getChannelByChannelID(channel_id, ST_DIGITAL_RADIO_SOUND_SERVICE);
	}

	return result;
}

void CBouquet::addService(CZapitChannel* newChannel)
{
	switch (newChannel->getServiceType())
	{
		case ST_DIGITAL_TELEVISION_SERVICE:
		case ST_NVOD_REFERENCE_SERVICE:
		case ST_NVOD_TIME_SHIFTED_SERVICE:
			tvChannels.push_back(newChannel);
			break;
			
		case ST_DIGITAL_RADIO_SOUND_SERVICE:
			radioChannels.push_back(newChannel);
			break;
	}
}

void CBouquet::removeService(CZapitChannel* oldChannel)
{
	if (oldChannel != NULL)
	{
		ChannelList* channels = &tvChannels;
		switch (oldChannel->getServiceType())
		{
			case ST_DIGITAL_TELEVISION_SERVICE:
			case ST_NVOD_REFERENCE_SERVICE:
			case ST_NVOD_TIME_SHIFTED_SERVICE:
				channels = &tvChannels;
				break;

			case ST_DIGITAL_RADIO_SOUND_SERVICE:
				channels = &radioChannels;
				break;
		}
		(*channels).erase(remove(channels->begin(), channels->end(), oldChannel), channels->end());
	}
}

void CBouquet::moveService(const unsigned int oldPosition, const unsigned int newPosition, const unsigned char serviceType)
{
	ChannelList* channels = &tvChannels;
	switch (serviceType)
	{
		case ST_DIGITAL_TELEVISION_SERVICE:
		case ST_NVOD_REFERENCE_SERVICE:
		case ST_NVOD_TIME_SHIFTED_SERVICE:
			channels = &tvChannels;
			break;
			
		case ST_DIGITAL_RADIO_SOUND_SERVICE:
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
	return size;
}

int CBouquet::recModeTVSize( unsigned int tsid)
{
	int size = 0;
	for ( unsigned int i=0; i< radioChannels.size(); i++)
	{
		if ( tsid == radioChannels[i]->getTsidOnid())
			size++;
	}
	return size;
}

void copy_to_bouquet(FILE * fd, FILE * fd1, const char * bouquetName)
{
	//copies bouquets from previous bouquets.xml file from start up to the bouquet that is being written...
	char buffer[256] = "";
	
	//printf("[bouquets] copy_to_bouquet: %s\n", bouquetName);
	//look for bouqet to be written... or end of file
	fgets(buffer, 255, fd1);
	while(!feof(fd1) && !((strstr(buffer, "Bouquet name") && strstr(buffer, bouquetName)) || strstr(buffer, "</zapit>")))
	{
		fputs(buffer, fd);
		fgets(buffer, 255, fd1);
	}
	
	// if not end of file
	if (!feof(fd1) && !strstr(buffer, "</zapit>"))
		// skip to end of bouquet
		while (!feof(fd1) && !strstr(buffer, "</Bouquet>"))
			fgets(buffer, 255, fd1);
}

void copy_to_end(FILE * fd, FILE * fd1)
{
	//copies the services from previous services.xml file from the end of sat being scanned to the end of the file...
	//printf("[bouquets] copying to end...\n");
	char buffer[256] ="";
	
	fgets(buffer, 255, fd1);
	while(!feof(fd1) && !strstr(buffer, "</zapit>"))
	{
		fputs(buffer, fd);
		fgets(buffer, 255, fd1);
	}
	fclose(fd1);
	unlink(BOUQUETS_TMP);
}

void CBouquetManager::writeBouquetHeader(FILE * bouq_fd, uint i, const char * bouquetName)
{
	//printf("[bouquets] writing bouquet header: %s\n", bouquetName);
	fprintf(bouq_fd, "\t<Bouquet name=\"%s\" hidden=\"%d\" locked=\"%d\">\n",
		bouquetName,
		Bouquets[i]->bHidden ? 1 : 0,
		Bouquets[i]->bLocked ? 1 : 0);	
}

void CBouquetManager::writeBouquetFooter(FILE * bouq_fd)
{
	fprintf(bouq_fd, "\t</Bouquet>\n");
}

void CBouquetManager::writeBouquetChannels(FILE * bouq_fd, uint i)
{
	if (!strcmp(getFrontendName(), "sat"))
	{
		for ( unsigned int j=0; j<Bouquets[i]->tvChannels.size(); j++)
		{
			fprintf(bouq_fd, "\t\t<channel serviceID=\"%04x\" name=\"%s\" tsid=\"%04x\" onid=\"%04x\" sat_position=\"%hd\"/>\n",
					Bouquets[i]->tvChannels[j]->getServiceId(),
					convert_UTF8_To_UTF8_XML(Bouquets[i]->tvChannels[j]->getName()).c_str(),
					Bouquets[i]->tvChannels[j]->getTransportStreamId(), 
					Bouquets[i]->tvChannels[j]->getOriginalNetworkId(),
					Bouquets[i]->tvChannels[j]->getSatellitePosition());
		}
		for ( unsigned int j=0; j<Bouquets[i]->radioChannels.size(); j++)
		{
			fprintf(bouq_fd, "\t\t<channel serviceID=\"%04x\" name=\"%s\" tsid=\"%04x\" onid=\"%04x\" sat_position=\"%hd\"/>\n",
					Bouquets[i]->radioChannels[j]->getServiceId(),
					convert_UTF8_To_UTF8_XML(Bouquets[i]->radioChannels[j]->getName()).c_str(),
					Bouquets[i]->radioChannels[j]->getTransportStreamId(), 
					Bouquets[i]->radioChannels[j]->getOriginalNetworkId(), 
					Bouquets[i]->radioChannels[j]->getSatellitePosition());
		}
	}
	else
	{
		for ( unsigned int j=0; j<Bouquets[i]->tvChannels.size(); j++)
		{
			fprintf(bouq_fd, "\t\t<channel serviceID=\"%04x\" name=\"%s\" onid=\"%04x\"/>\n",
					Bouquets[i]->tvChannels[j]->getServiceId(),
					convert_UTF8_To_UTF8_XML(Bouquets[i]->tvChannels[j]->getName()).c_str(),
					Bouquets[i]->tvChannels[j]->getOriginalNetworkId());
		}
		for ( unsigned int j=0; j<Bouquets[i]->radioChannels.size(); j++)
		{
			fprintf(bouq_fd, "\t\t<channel serviceID=\"%04x\" name=\"%s\" onid=\"%04x\"/>\n",
					Bouquets[i]->radioChannels[j]->getServiceId(),
					convert_UTF8_To_UTF8_XML(Bouquets[i]->radioChannels[j]->getName()).c_str(),
					Bouquets[i]->radioChannels[j]->getOriginalNetworkId());
		}
	}
}


/**** class CBouquetManager *************************************************/
void CBouquetManager::saveBouquets(void)
{
	FILE * bouq_fd;
	
	bouq_fd = fopen(BOUQUETS_XML, "w");
		
	fprintf(bouq_fd, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<zapit>\n");

	for (unsigned int i = 0; i < Bouquets.size(); i++)
	{
		if (Bouquets[i] != remainChannels)
		{
			writeBouquetHeader(bouq_fd, i, convert_UTF8_To_UTF8_XML(Bouquets[i]->Name).c_str());
			writeBouquetChannels(bouq_fd, i);
			writeBouquetFooter(bouq_fd);
		}
	}
	
	fprintf(bouq_fd, "</zapit>\n");
	fclose(bouq_fd);
}

void CBouquetManager::saveBouquets(CZapitClient::bouquetMode bouquetMode, char * providerName)
{
	diseqc_t diseqcType;
	char * frontendType;
	FILE * bouq_fd = NULL;
	FILE * fd2 = NULL;
	
	//printf("[bouquets] creating new bouquets for %s\n", providerName);
	
	diseqcType = frontend->getDiseqcType();
	frontendType = getFrontendName();

/*	if (diseqcType != DISEQC_1_2) */
	if (bouquetMode != CZapitClient::BM_CREATESATELLITEBOUQUET)
	{
		// not diseqc 1.2 or (diseqc 1.2 and no bouquets.xml file exists)

		if (bouquetMode == CZapitClient::BM_UPDATEBOUQUETS)
		{
			storeBouquets();
			clearAll();

			LoadServices(frontend->getInfo()->type, diseqcType);

			loadBouquets();

			deleteBouquet(remainChannels);
			remainChannels = NULL;

			for (unsigned int i = 0; i < Bouquets.size(); i++)
			{
				unsigned int j;
				for (j = 0; j < Bouquets[i]->tvChannels.size(); j++)
					Bouquets[i]->tvChannels[j] = new CZapitChannel(*(Bouquets[i]->tvChannels[j]));
				for (j = 0; j < Bouquets[i]->radioChannels.size(); j++)
					Bouquets[i]->radioChannels[j] = new CZapitChannel(*(Bouquets[i]->tvChannels[j]));
			}
				
			allchans.clear();

			while (!(storedBouquets.empty()))
			{
				int dest = existsBouquet(storedBouquets[0]->Name);
				if (dest != -1)
				{
					while (!(storedBouquets[0]->tvChannels.empty()))
					{
						if (!(existsChannelInBouquet(dest, storedBouquets[0]->tvChannels[0]->getChannelID())))
							Bouquets[dest]->addService(storedBouquets[0]->tvChannels[0]);
						storedBouquets[0]->removeService(storedBouquets[0]->tvChannels[0]);
					}
					while (!(storedBouquets[0]->radioChannels.empty()))
					{
						if (!(existsChannelInBouquet(dest, storedBouquets[0]->radioChannels[0]->getChannelID())))
							Bouquets[dest]->addService(storedBouquets[0]->radioChannels[0]);
						storedBouquets[0]->removeService(storedBouquets[0]->radioChannels[0]);
					}
					delete storedBouquets[0];
				}
				else
				{
					Bouquets.push_back(storedBouquets[0]);
				}
				storedBouquets.erase(storedBouquets.begin());
			}
		}

		saveBouquets();
	}
	else
	{
/*
		//diseqc 1.2
		//just replace existings bouquets
		
		if (bouquetMode == CZapitClient::BM_CREATEBOUQUETS)
		{
			for (unsigned int i = 0; i < Bouquets.size(); i++)
			{
				//printf("[bouquets] creating bouquet: %s\n", convert_UTF8_To_UTF8_XML(Bouquets[i]->Name).c_str());
				if (Bouquets[i] != remainChannels)
				{
					cp(BOUQUETS_XML, BOUQUETS_TMP);
					fd2 = fopen(BOUQUETS_TMP, "r");
					bouq_fd = fopen(BOUQUETS_XML, "w");
					writeBouquetHeader(bouq_fd, i, providerName);
					copy_to_bouquet(bouq_fd, fd2, convert_UTF8_To_UTF8_XML(Bouquets[i]->Name).c_str());
					writeBouquetChannels(bouq_fd, i);
					writeBouquetFooter(bouq_fd);
					copy_to_end(bouq_fd, fd2);
					fprintf(bouq_fd, "</zapit>\n");
					fclose(bouq_fd);
				}
			}
		}
		else
*/
		if (bouquetMode == CZapitClient::BM_CREATESATELLITEBOUQUET)
		{
			if (providerName != NULL)
			{
				//create satellite bouquet
				//printf("[bouquets] creating satellite bouquet for %s\n", providerName);
				cp(BOUQUETS_XML, BOUQUETS_TMP);
				fd2 = fopen(BOUQUETS_TMP, "r");
				bouq_fd = fopen(BOUQUETS_XML, "w");
				copy_to_bouquet(bouq_fd, fd2, providerName);
				uint i = 1;
				writeBouquetHeader(bouq_fd, i, providerName);
				for (unsigned int i = 0; i < Bouquets.size(); i++)
				{
					if (Bouquets[i] != remainChannels)
					{
						writeBouquetChannels(bouq_fd, i);
					}
				}
				writeBouquetFooter(bouq_fd);
				copy_to_end(bouq_fd, fd2);
				fprintf(bouq_fd, "</zapit>\n");
				fclose(bouq_fd);
			}
		}
		else
			printf("[bouquets] unsupported bouquet mode.\n");
	}
}

void CBouquetManager::parseBouquetsXml(const xmlNodePtr root)
{
	xmlNodePtr search=root->xmlChildrenNode;
	xmlNodePtr channel_node;
	

	if (search)
	{
		t_original_network_id original_network_id;
		t_service_id service_id;
		t_transport_stream_id transport_stream_id;
		int16_t satellitePosition;

		INFO("reading bouquets");

		while ((search = xmlGetNextOccurence(search, "Bouquet")) != NULL)
		{
			CBouquet* newBouquet = addBouquet(xmlGetAttribute(search, "name"));
			char* hidden = xmlGetAttribute(search, "hidden");
			char* locked = xmlGetAttribute(search, "locked");
			newBouquet->bHidden = hidden ? (strcmp(hidden, "1") == 0) : false;
			newBouquet->bLocked = locked ? (strcmp(locked, "1") == 0) : false;
			channel_node = search->xmlChildrenNode;

			while ((channel_node = xmlGetNextOccurence(channel_node, "channel")) != NULL)
			{
				GET_ATTR(channel_node, "serviceID", SCANF_SERVICE_ID_TYPE, service_id);
				GET_ATTR(channel_node, "onid", SCANF_ORIGINAL_NETWORK_ID_TYPE, original_network_id);
				GET_ATTR(channel_node, "sat_position", SCANF_SATELLITE_POSITION_TYPE, satellitePosition);
				GET_ATTR(channel_node, "tsid", SCANF_TRANSPORT_STREAM_ID_TYPE, transport_stream_id);

				CZapitChannel* chan = findChannelByChannelID(CREATE_CHANNEL_ID);

				if (chan != NULL)
					newBouquet->addService(chan);

				channel_node = channel_node->xmlNextNode;
			}

			search = search->xmlNextNode;
		}
	
		INFO("found %d bouquets", Bouquets.size());
	}

}

void CBouquetManager::loadBouquets(bool ignoreBouquetFile)
{
	xmlDocPtr parser;

	if (ignoreBouquetFile == false)
	{
		parser = parseXmlFile(string(BOUQUETS_XML));

		if (parser != NULL)
		{
			parseBouquetsXml(xmlDocGetRootElement(parser));
			xmlFreeDoc(parser);
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

void CBouquetManager::makeRemainingChannelsBouquet()
{
	ChannelList unusedChannels;
	set<t_channel_id> chans_processed;

	for (vector<CBouquet*>::const_iterator it = Bouquets.begin(); it != Bouquets.end(); it++)
	{
		for (vector<CZapitChannel*>::iterator jt = (*it)->tvChannels.begin(); jt != (*it)->tvChannels.end(); jt++)
			chans_processed.insert((*jt)->getChannelID());
		for (vector<CZapitChannel*>::iterator jt = (*it) ->radioChannels.begin(); jt != (*it)->radioChannels.end(); jt++)
			chans_processed.insert((*jt)->getChannelID());
	}

	// TODO: use locales
	remainChannels = addBouquet((Bouquets.size() == 0) ? "Alle Kan\xC3\xA4le" : "Andere"); // UTF-8 encoded

	for (tallchans::iterator it = allchans.begin(); it != allchans.end(); it++)
		if (chans_processed.find(it->first) == chans_processed.end())
			unusedChannels.push_back(&(it->second));

	sort(unusedChannels.begin(), unusedChannels.end(), CmpChannelByChName());

	for (ChannelList::const_iterator it = unusedChannels.begin(); it != unusedChannels.end(); it++)
		remainChannels->addService(findChannelByChannelID((*it)->getChannelID()));

	if ((remainChannels->tvChannels.empty()) && (remainChannels->radioChannels.empty()))
	{
		deleteBouquet(remainChannels);
		remainChannels = NULL;
	}
}

void CBouquetManager::renumServices()
{
	deleteBouquet(remainChannels);
	remainChannels = NULL;
	
	if (config.getBool("makeRemainingChannelsBouquet", true))
		makeRemainingChannelsBouquet();
}

CBouquet* CBouquetManager::addBouquet( string name)
{
	CBouquet* newBouquet = new CBouquet(name);
	Bouquets.push_back(newBouquet);
	return newBouquet;
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
bool CBouquetManager::existsChannelInBouquet( unsigned int bq_id, const t_channel_id channel_id)
{
	bool     status = false;
	CZapitChannel  *ch = NULL;

	if (bq_id <= Bouquets.size()) {
		// query TV-Channels  && Radio channels
		ch = Bouquets[bq_id]->getChannelByChannelID(channel_id, 0);
		if (ch)  status = true;
	}

	return status;

}


void CBouquetManager::moveBouquet(const unsigned int oldId, const unsigned int newId)
{
	if ((oldId < Bouquets.size()) && (newId < Bouquets.size()))
	{
		BouquetList::iterator it = Bouquets.begin();

		advance(it, oldId);
		CBouquet* tmp = *it;
		Bouquets.erase(it);

		advance(it, newId - oldId);
		Bouquets.insert(it, tmp);
	}
}

void CBouquetManager::clearAll()
{
	for (unsigned int i=0; i<Bouquets.size(); i++)
		delete Bouquets[i];

	Bouquets.clear();
	remainChannels = NULL;
}

CZapitChannel* CBouquetManager::findChannelByChannelID(const t_channel_id channel_id)
{
	tallchans_iterator itChannel = allchans.find(channel_id);
	if (itChannel != allchans.end())
		return &(itChannel->second);

	return NULL;
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

int CBouquetManager::ChannelIterator::getLowestChannelNumberWithChannelID(const t_channel_id channel_id)
{
	int i = 0;

	for (b = 0; b < Owner->Bouquets.size(); b++)
		for (c = 0; (unsigned int) c < getBouquet()->size(); c++, i++)
			if ((**this)->getChannelID() == channel_id)
			    return i;
	return -1; // not found
}


int CBouquetManager::ChannelIterator::getNrofFirstChannelofBouquet(const unsigned int bouquet_nr)
{
	if (bouquet_nr >= Owner->Bouquets.size())
		return -1;  // not found

	int i = 0;

	for (b = 0; b < bouquet_nr; b++)
		i += getBouquet()->size();

	return i;
}
