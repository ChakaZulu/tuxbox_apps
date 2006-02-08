/*
 * $Id: bouquets.cpp,v 1.108 2006/02/08 21:19:35 houdini Exp $
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

#include <sys/stat.h>
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
extern std::map<std::string, t_satellite_position> satellitePositions;
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

size_t CBouquet::recModeRadioSize(const transponder_id_t transponder_id)
{
	size_t size = 0;

	for (size_t i = 0; i < radioChannels.size(); i++)
		if (transponder_id == radioChannels[i]->getTransponderId())
			size++;

	return size;
}

size_t CBouquet::recModeTVSize(const transponder_id_t transponder_id)
{
	size_t size = 0;

	for (size_t i = 0; i < tvChannels.size(); i++)
		if (transponder_id == tvChannels[i]->getTransponderId())
			size++;

	return size;
}

static const char * const printf_string_with_names[2] =
{
	"\t\t<channel serviceID=\"%04x\" name=\"%s\" tsid=\"%04x\" onid=\"%04x\"/>\n",
	"\t\t<channel serviceID=\"%04x\" name=\"%s\" tsid=\"%04x\" onid=\"%04x\" sat=\"%hd\"/>\n"
};

static const char * const printf_string_without_names[2] =
{
	"\t\t<channel serviceID=\"%04x\" tsid=\"%04x\" onid=\"%04x\"/>\n",
	"\t\t<channel serviceID=\"%04x\" tsid=\"%04x\" onid=\"%04x\" sat=\"%hd\"/>\n"
};

void writeChannelList(FILE * const bouq_fd, const ChannelList & list, const bool write_names, const char * const channel_printf_string)
{
	for (ChannelList::const_iterator it = list.begin(); it != list.end(); it++)
	{
		const CZapitChannel * c = *it;
		if (write_names)
			{
				fprintf(bouq_fd, channel_printf_string,
					c->getServiceId(),
					convert_UTF8_To_UTF8_XML(c->getName().c_str()).c_str(),
					c->getTransportStreamId(),
					c->getOriginalNetworkId(),
					c->getSatellitePosition());
			}
			else
			{
				fprintf(bouq_fd, channel_printf_string,
					c->getServiceId(),
					c->getTransportStreamId(),
					c->getOriginalNetworkId(),
					c->getSatellitePosition());
			}
	}
}

/**** class CBouquetManager *************************************************/
void CBouquetManager::saveBouquets(void)
{
	FILE *       bouq_fd;
	bool         write_names           = config.getBool("writeChannelsNames", true);
	unsigned int string_number         = (strcmp(getFrontendName(), "sat") == 0) ? 1 : 0;
	const char * channel_printf_string = write_names ? printf_string_with_names[string_number] : printf_string_without_names[string_number];
	
	bouq_fd = fopen(BOUQUETS_XML, "w");
		
	fprintf(bouq_fd, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<zapit>\n");

	for (BouquetList::const_iterator it = Bouquets.begin(); it != Bouquets.end(); it++)
	{
		// TODO: use locales
		if (((*it) != remainChannels) && (strncmp((*it)->Name.c_str(),"Neue Sender",11) != 0))
		{
			//fprintf(bouq_fd, "\t<Bouquet name=\"%s\" hidden=\"%d\" locked=\"%d\">\n",
			fprintf(bouq_fd, "\t<Bouquet type=\"%01x\" bouquet_id=\"%04x\" name=\"%s\" hidden=\"%01x\" locked=\"%01x\">\n",
				(*it)->type,
				(*it)->bouquet_id,
				convert_UTF8_To_UTF8_XML((*it)->Name.c_str()).c_str(),
				(*it)->bHidden ? 1 : 0,
				(*it)->bLocked ? 1 : 0);

			writeChannelList(bouq_fd, (*it)->tvChannels   , write_names, channel_printf_string);
			writeChannelList(bouq_fd, (*it)->radioChannels, write_names, channel_printf_string);

			fprintf(bouq_fd, "\t</Bouquet>\n");
		}
	}
	
	fprintf(bouq_fd, "</zapit>\n");
	fclose(bouq_fd);

	chmod(BOUQUETS_XML, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
}

void CBouquetManager::saveBouquets(const CZapitClient::bouquetMode bouquetMode, const char * const providerName)
{
/* //	printf("[zapit] b size %d\n", Bouquets.size());
	if (bouquetMode == CZapitClient::BM_CREATESATELLITEBOUQUET)
	{
//	printf("[zapit] b mode sat \n");
		while (Bouquets.size() > 1)
		{
			BouquetList::iterator it = Bouquets.begin() + 1;
			Bouquets[0]->tvChannels.insert(Bouquets[0]->tvChannels.end(), (*it)->tvChannels.begin(), (*it)->tvChannels.end());
			Bouquets[0]->radioChannels.insert(Bouquets[0]->radioChannels.end(), (*it)->radioChannels.begin(), (*it)->radioChannels.end());
			delete (*it);
			Bouquets.erase(it);
		}
//	printf("[zapit] b mode sat \n");
		if(Bouquets.size() > 0)
		Bouquets[0]->Name = providerName;
	}
*/	
//	printf("[zapit] b mode sat \n");
	if ((bouquetMode == CZapitClient::BM_UPDATEBOUQUETS) || (bouquetMode == CZapitClient::BM_CREATESATELLITEBOUQUET))
	{
		BouquetList storedBouquets;

		storedBouquets = Bouquets;
		Bouquets.clear();
		remainChannels = NULL;
		
		LoadServices(frontend->getInfo()->type, frontend->getDiseqcType(), false);
		
		loadBouquets();
		
		deleteBouquet(remainChannels);
		remainChannels = NULL;
		
		for (unsigned int i = 0; i < Bouquets.size(); i++)
		{
			unsigned int j;
			for (j = 0; j < Bouquets[i]->tvChannels.size(); j++)
				Bouquets[i]->tvChannels[j] = new CZapitChannel(*(Bouquets[i]->tvChannels[j]));
			for (j = 0; j < Bouquets[i]->radioChannels.size(); j++)
				Bouquets[i]->radioChannels[j] = new CZapitChannel(*(Bouquets[i]->radioChannels[j]));
		}
		
		allchans.clear();
		
		while (!(storedBouquets.empty()))
		{
			int dest = existsBouquet(storedBouquets[0]->Name.c_str());
			if (dest != -1)
			{
				while (!(storedBouquets[0]->tvChannels.empty()))
				{
					if (!(existsChannelInBouquet(dest, storedBouquets[0]->tvChannels[0]->getChannelID())))
					{
						Bouquets[dest]->addService(storedBouquets[0]->tvChannels[0]);
						storedBouquets[0]->removeService(storedBouquets[0]->tvChannels[0]);
					}
					else
					{
						delete storedBouquets[0]->tvChannels[0];
						storedBouquets[0]->tvChannels.erase(storedBouquets[0]->tvChannels.begin());
					}
				}
				while (!(storedBouquets[0]->radioChannels.empty()))
				{
					if (!(existsChannelInBouquet(dest, storedBouquets[0]->radioChannels[0]->getChannelID())))
					{
						Bouquets[dest]->addService(storedBouquets[0]->radioChannels[0]);
						storedBouquets[0]->removeService(storedBouquets[0]->radioChannels[0]);
					}
					else
					{
						delete storedBouquets[0]->radioChannels[0];
						storedBouquets[0]->radioChannels.erase(storedBouquets[0]->radioChannels.begin());
					}
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

void CBouquetManager::parseBouquetsXml(const xmlNodePtr root)
{
	extern CConfigFile config;
	bool channel_names_from_bouquet = config.getBool("ChannelNamesFromBouquet", false);

	xmlNodePtr search=root->xmlChildrenNode;
	xmlNodePtr channel_node;
	

	if (search)
	{
		t_original_network_id original_network_id;
		t_service_id          service_id;
		t_transport_stream_id transport_stream_id;
		t_satellite_position  satellitePosition;

		INFO("reading bouquets");

		while ((search = xmlGetNextOccurence(search, "Bouquet")) != NULL)
		{
			CBouquet* newBouquet = addBouquet(xmlGetAttribute(search, "name"));
			char* hidden = xmlGetAttribute(search, "hidden");
			char* locked = xmlGetAttribute(search, "locked");
			newBouquet->type = xmlGetNumericAttribute(search, "type", 16);
			newBouquet->bouquet_id = xmlGetNumericAttribute(search, "bouquet_id", 16);
			newBouquet->bHidden = hidden ? (strcmp(hidden, "1") == 0) : false;
			newBouquet->bLocked = locked ? (strcmp(locked, "1") == 0) : false;
			channel_node = search->xmlChildrenNode;

			while ((channel_node = xmlGetNextOccurence(channel_node, "channel")) != NULL)
			{
				GET_ATTR(channel_node, "serviceID", SCANF_SERVICE_ID_TYPE, service_id);
				GET_ATTR(channel_node, "onid", SCANF_ORIGINAL_NETWORK_ID_TYPE, original_network_id);
				GET_ATTR(channel_node, "sat", SCANF_SATELLITE_POSITION_TYPE, satellitePosition);
				GET_ATTR(channel_node, "tsid", SCANF_TRANSPORT_STREAM_ID_TYPE, transport_stream_id);

				CZapitChannel* chan = findChannelByChannelID(CREATE_CHANNEL_ID);

				if (chan != NULL) {
					if (channel_names_from_bouquet)
						chan->setName(xmlGetAttribute(channel_node, "name"));
					newBouquet->addService(chan);
				}

				channel_node = channel_node->xmlNextNode;
			}

			search = search->xmlNextNode;
		}
	
		INFO("found %d bouquets", Bouquets.size());
	}

}

void CBouquetManager::makeBouquetfromCurrentservices(const xmlNodePtr root)
{
	xmlNodePtr provider = root->xmlChildrenNode;
	
	// TODO: use locales
	CBouquet* newBouquet = addBouquet("Neue Sender");
			newBouquet->bHidden = false;
			newBouquet->bLocked = false;
			
	t_original_network_id original_network_id;
	t_service_id          service_id;
	t_transport_stream_id transport_stream_id;
	t_satellite_position  satellitePosition;
	
	while (provider) {
		
		xmlNodePtr transponder = provider->xmlChildrenNode;
		
		while (xmlGetNextOccurence(transponder, "transponder") != NULL) {
			
			xmlNodePtr channel_node = transponder->xmlChildrenNode;
			
			while (xmlGetNextOccurence(channel_node, "channel") != NULL) {
				
				if (strncmp(xmlGetAttribute(channel_node, "action"), "remove", 6)) {
					
					GET_ATTR(provider, "position", SCANF_SATELLITE_POSITION_TYPE, satellitePosition);
					GET_ATTR(transponder, "onid", SCANF_ORIGINAL_NETWORK_ID_TYPE, original_network_id);
					GET_ATTR(transponder, "id", SCANF_TRANSPORT_STREAM_ID_TYPE, transport_stream_id);
					GET_ATTR(channel_node, "service_id", SCANF_SERVICE_ID_TYPE, service_id);
								
					CZapitChannel* chan = findChannelByChannelID(CREATE_CHANNEL_ID);

					if (chan != NULL)
						newBouquet->addService(chan);
				}
			
				channel_node = channel_node->xmlNextNode;
			}
			transponder = transponder->xmlNextNode;
		}
		provider = provider->xmlNextNode;
	}
}

void CBouquetManager::loadBouquets(bool ignoreBouquetFile)
{
	xmlDocPtr parser;

	if (ignoreBouquetFile == false)
	{
		parser = parseXmlFile(BOUQUETS_XML);

		if (parser != NULL)
		{
			parseBouquetsXml(xmlDocGetRootElement(parser));
			xmlFreeDoc(parser);
		}
		
		parser = parseXmlFile(CURRENTSERVICES_XML);
		
		if (parser != NULL)
		{
			makeBouquetfromCurrentservices(xmlDocGetRootElement(parser));
			xmlFreeDoc(parser);
		}
	}
	renumServices();
}

void CBouquetManager::makeRemainingChannelsBouquet(void)
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

CBouquet* CBouquetManager::addBouquet(const std::string & name)
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
int CBouquetManager::existsBouquet(char const * const name)
{
	unsigned int i;

	for (i = 0; i < Bouquets.size(); i++)
	{
		if (Bouquets[i]->Name == name)
			return (int)i;
	}

	return -1;
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
