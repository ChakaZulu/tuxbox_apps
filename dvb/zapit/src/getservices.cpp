#include "xml/xmltree.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <map>
#include <string>

#include "getservices.h"
#include "descriptors.h"

void ParseTransponder(XMLTreeNode *xmltransponder);
void ParseRoot(XMLTreeNode *root);
void FindTransponder(XMLTreeNode *root);
void LoadSortList(void);
int LoadServices(void);

uint16_t curr_tsid = 0;
uint16_t curr_diseqc = 0;

void nameinsert (std::string name, uint32_t onid_sid, uint sm)
{
	int number = 0;
	char cnumber[3];
	std::string newname = name;

	if (sm == 2)
	{
		while (namechans_radio.count(newname) != 0)
		{
			sprintf(cnumber, "%2d", ++number);
			newname = name + cnumber;
		}
		namechans_radio.insert(std::pair<std::string, uint32_t>(newname, onid_sid));
		allchans_radio.find(onid_sid)->second.name = newname;
	}
	else
	{
		while (namechans_tv.count(newname) != 0)
		{
			sprintf(cnumber, "%2d", ++number);
			newname = name + cnumber;
		}
		namechans_tv.insert(std::pair<std::string, uint32_t>(newname, onid_sid));
		allchans_tv.find(onid_sid)->second.name = newname;
	}
}

void ParseTransponder (XMLTreeNode *xmltransponder)
{
	XMLTreeNode *services;

	char *type;
	uint16_t tmp;

	int sm;
	std::string name;
	uint16_t cnr;
	uint16_t sid;
	uint16_t onid;

	for (services = xmltransponder->GetChild(); services != NULL; services = services->GetNext())
	{
		type = services->GetType();

		if (!strcmp("cable", type))
		{
			if (transponders.count(curr_tsid) == 0)
			{
				printf("[getservices.cpp] no transponder with that tsid found\n");
				return;
			}

			std::map<uint, transponder>::iterator trans = transponders.find(curr_tsid);
			sscanf(services->GetAttributeValue("frequency"),"%u", &trans->second.feparams.Frequency);
			sscanf(services->GetAttributeValue("symbolRate"), "%u", &trans->second.feparams.u.qam.SymbolRate);
			sscanf(services->GetAttributeValue("fec"), "%hu", &tmp);
			trans->second.feparams.u.qam.FEC_inner = getFEC(tmp);
			sscanf(services->GetAttributeValue("modulation"), "%hu", &tmp);
			trans->second.feparams.u.qam.QAM = getModulation(tmp);
		}
		else if (!strcmp("sat", type))
		{
			if (transponders.count(curr_tsid) == 0)
			{
				printf("[getservices.cpp] no transponder with that tsid found\n");
				return;
			}

			std::map<uint, transponder>::iterator trans = transponders.find(curr_tsid);
			sscanf(services->GetAttributeValue("frequency"),"%u", &trans->second.feparams.Frequency);
			sscanf(services->GetAttributeValue("symbolRate"), "%u", &trans->second.feparams.u.qpsk.SymbolRate);
			sscanf(services->GetAttributeValue("fec"), "%hu", &tmp);
			trans->second.feparams.u.qam.FEC_inner = getFEC(tmp);
			sscanf(services->GetAttributeValue("Polarity"), "%hu", &tmp);
			trans->second.polarization = tmp;
			trans->second.DiSEqC = curr_diseqc;
		}
		else if (!strcmp("channel", type))
		{
			sm = atoi(services->GetAttributeValue("serviceType"));

			if ((sm == 1) || (sm == 2) || (sm == 4))
			{
				name = services->GetAttributeValue("Name");
				sscanf(services->GetAttributeValue("channelNR"), "%hd", &cnr);
				sscanf(services->GetAttributeValue("serviceID"), "%hx", &sid);
				sscanf(services->GetAttributeValue("onid"), "%hx", &onid);

				if (sm == 2)
				{
					allchans_radio.insert(std::pair<uint32_t, channel>((onid<<16) | sid, channel(name, 0, 0, 0, 0, 0, sid, curr_tsid, onid, sm, cnr)));

					if (cnr > 0)
						numchans_radio.insert(std::pair<uint16_t, uint32_t>(cnr, (onid<<16) | sid));
					else
						nameinsert(name, (onid<<16) | sid, sm);
				}
				else
				{
					allchans_tv.insert(std::pair<uint32_t, channel>((onid<<16) | sid, channel(name, 0, 0, 0, 0, 0, sid, curr_tsid, onid, sm, cnr)));

					if (cnr > 0)
						numchans_tv.insert(std::pair<uint16_t, uint32_t>(cnr, (onid<<16) | sid));
					else
						nameinsert(name, (onid<<16) | sid, sm);
				}
			}
		}
		else
		{
			printf("[getservices.cpp] not known. skipping %s\n", services->GetType());
		}
	}
	return;
}

void ParseRoot (XMLTreeNode *root)
{
	FrontendParameters feparams;

	for (XMLTreeNode *c=root; c; c=c->GetNext())
	{
		if (!strcasecmp(c->GetType(), "transponder"))
		{
			sscanf(c->GetAttributeValue("transportID"), "%hd", &curr_tsid);
			transponders.insert(std::pair<uint, transponder>(curr_tsid, transponder(curr_tsid, feparams)));
			ParseTransponder(c);
		}
		else
		{
			printf("[getservices.cpp] ignoring %s\n", c->GetType());
		}
	}
}

void FindTransponder (XMLTreeNode *root)
{
	XMLTreeNode *search = root->GetChild();

	while ((strcmp(search->GetType(), "cable")) && (strcmp(search->GetType(), "satellite")))
		search = search->GetChild();

	while (search)
	{
		if (!(strcmp(search->GetType(), "cable")))
		{
			printf("[getservices.cpp] scanning a cable section\n");

			while (strcmp(search->GetType(), "transponder"))
				search = search->GetChild();

			ParseRoot(search);
			search = search->GetParent();
		}
		else if (!(strcmp(search->GetType(), "satellite")))
		{
			printf("[getservices.cpp] scanning a satellite section\n");

			while (!(strcmp(search->GetType(), "satellite")))
			{
				sscanf(search->GetAttributeValue("diseqc"), "%hu", &curr_diseqc);
				printf("[getservices.cpp] going to parse satellite %s\n", search->GetAttributeValue("name"));
				search = search->GetChild();
				ParseRoot(search);
				search = search->GetParent()->GetNext();

				if (!search)
					return;
			}
		}
		search = search->GetNext();
	}
}

int LoadServices(void)
{
	char buf[2048];
	XMLTreeParser *parser;
	FILE *in;
	int done;
	size_t len;

	parser = new XMLTreeParser("ISO-8859-1");
	in = fopen(CONFIGDIR "/zapit/services.xml", "r");

	if (!in)
	{
		perror("[getservices.cpp] " CONFIGDIR "/zapit/services.xml");
		return -23;
	}

	do
	{
		len = fread(buf, 1, sizeof(buf), in);
		done = len < sizeof(buf);

		if (!parser->Parse(buf, len, done))
		{
			printf("[getservices.cpp] parse error: %s at line %d\n", parser->ErrorString(parser->GetErrorCode()), parser->GetCurrentLineNumber());
			fclose(in);
			delete parser;
			return -1;
		}
	}
	while (!done);

	if (parser->RootNode())
		FindTransponder(parser->RootNode());

	fclose(in);
	delete parser;
	return 23;
}

