/*
 * $Id: zapit.cpp,v 1.447 2009/10/12 07:22:55 rhabarber1848 Exp $
 *
 * zapit - d-box2 linux project
 *
 * (C) 2001, 2002 by Philipp Leusmann <faralla@berlios.de>
 * (C) 2002, 2003 by Andreas Oberritter <obi@tuxbox.org>
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

/*
 Options:
 -d	Debugmode: Don't fork, additionally generate debugging messages

 -q     Be quiet.

 Signal handling:
  SIGTERM, SIGHUP: Terminates zapit (gracefully)
  SIGUSR1:         Toggles debug mode
*/

/* system headers */
#include <csignal>
#include <fcntl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if defined(HAVE_DREAMBOX_HARDWARE) || defined(HAVE_IPBOX_HARDWARE) || defined(HAVE_TRIPLEDRAGON)
/* dreambox for fastzap, td for stbgfx ioctl */
#include <sys/ioctl.h>
#endif

/* AudioPIDs per channel are saved here between sessions. */
/* define to /dev/null to disable */
#define AUDIO_CONFIG_FILE "/var/tuxbox/config/zapit/audio.pid"

/* tuxbox headers */
#include <configfile.h>
#include <connection/basicserver.h>

/* zapit headers */
#include <zapit/audio.h>
#ifdef HAVE_DBOX_HARDWARE
#include <zapit/aviaext.h>
#include "irsend/irsend.h"
#endif
#include <zapit/cam.h>
#include <zapit/dmx.h>
#include <zapit/debug.h>
#include <zapit/frontend.h>
#include <zapit/getservices.h>
#include <zapit/pat.h>
#include <zapit/pmt.h>
#include <zapit/scan.h>
#include <zapit/settings.h>
#include <zapit/video.h>
#include <xmltree/xmlinterface.h>
#include <zapit/zapit.h>

#include <zapit/client/msgtypes.h>
#include <controldclient/controldMsg.h>
#include <controldclient/controldclient.h>

#include "controld.h"

#ifdef HAVE_TRIPLEDRAGON
#include <tdgfx/stb04gfx.h>
#endif
/* the conditional access module */
CCam *cam = NULL;
/* the configuration file */
CConfigFile config(',', false);
/* the event server */
CEventServer *eventServer = NULL;
/* the dvb audio device */
CAudio *audioDecoder = NULL;
#ifdef HAVE_DBOX_HARDWARE
/* the aviaEXT device */
CAViAext *aviaExtDriver = NULL;
#endif
/* the dvb frontend device */
CFrontend *frontend = NULL;
/* the dvb video device */
CVideo *videoDecoder = NULL;
/* the current channel */
CZapitChannel *cc = NULL;
/* the transponder scan xml input */
xmlDocPtr scanInputParser = NULL;
/* the bouquet manager */
CBouquetManager *bouquetManager = NULL;
/* the mpeg2 ts->pes demux devices */
CDemux *audioDemux = NULL;
CDemux *pcrDemux = NULL;
CDemux *teletextDemux = NULL;
CDemux *videoDemux = NULL;

/* This associative array holds the last selected AudioPid for each channel */
map<t_channel_id, unsigned short> audio_map;

/* True if we save AudioPIDs between sessions */
bool save_audioPIDs = false;

/* current zapit mode */
enum {
	TV_MODE = 0x01,
	RADIO_MODE = 0x02,
	RECORD_MODE = 0x04
};

int currentMode;
bool playbackStopForced = false;
int debug = 0;

int waitForMotor = 0;
int motorRotationSpeed = 0; //in 0.1 degrees per second
diseqc_t diseqcType;

/* near video on demand */
tallchans nvodchannels;         //  tallchans defined in "bouquets.h"
bool current_is_nvod = false;

/* list of all channels (services) */
tallchans allchans;             //  tallchans defined in "bouquets.h"

/* transponder scan */
transponder_list_t transponders;

pthread_t scan_thread;
extern int found_transponders;
extern int processed_transponders;
extern int found_channels;
extern short curr_sat;
extern short scan_runs;
extern void get_transponder (TP_params *TP);
extern int scan_transponder(TP_params *TP);
extern void scan_clean(void);
extern short abort_scan;


CZapitClient::bouquetMode bouquetMode = CZapitClient::BM_CREATEBOUQUETS;
CZapitClient::scanType scanType = CZapitClient::ST_ALL;

/* the map which stores the wanted cable/satellites */
std::map<uint8_t, std::string> scanProviders;
/* the map which stores the diseqc 1.2 motor positions */
extern std::map<t_satellite_position, uint8_t> motorPositions;
extern std::map<t_satellite_position, uint8_t>::iterator mpos_it;

extern std::map<string, t_satellite_position> satellitePositions;

bool standby = true;

#if HAVE_DVB_API_VERSION < 3
/* on dreambox: use FASTZAP ioctl? */
int fastzap = 1;
#else
int fastzap = 0;
#endif
#ifdef HAVE_TRIPLEDRAGON
int finetune = 2;
#else
int finetune = 0;
#endif

uint32_t lastChannelRadio;
uint32_t lastChannelTV;
uint32_t startChannelRadio;
uint32_t startChannelTV;
bool saveLastChannel = true;

//stolen from scan.cpp - check how to include
void cpy(const char *from, const char *to)
{
	char cmd[256] = "cp -f ";
	strcat(cmd, from);
	strcat(cmd, " ");
	strcat(cmd, to);
	system(cmd);
}
/*
//stolen from zapitools.cpp - check how to include
std::string UTF8_to_UTF8XML(const char * s)
{
	std::string r;
	
	while ((*s) != 0)
	{
		 
		switch (*s)
		{
		case '<':
			r += "&lt;";
			break;
		case '>':
			r += "&gt;";
			break;
		case '&':
			r += "&amp;";
			break;
		case '\"':
			r += "&quot;";
			break;
		case '\'':
			r += "&apos;";
			break;
		default:
			r += *s;
		}
		s++;
	}
	return r;
}
*/
void write_header(FILE * tmp)
{
	fprintf(tmp, 
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<!--\n"
		" This file was automatically generated by zapit/sectionsd.\n"
		" It represents the services being currently signalled \n"
		" by your provider. When you switch through your channels\n"
		" the data will automatically be kept up to date. Please \n"
		" do not modify this file, as your modification will be \n"
		" erased anyway. Please check myservices.xml for adopting\n"
		" your service list to your needs.\n"
		"-->\n"
	       "<zapit>\n");
}

bool write_provider(FILE * tmp, xmlNodePtr provider, const bool start)
{
	std::string frontendType; 
	std::string provider_name;
	std::string diseqc;
	bool is_sat = false;
	
//	unsigned short orbital = 0;
//	unsigned short east_west = 0;
	int position = 0;
	
	frontendType = xmlGetName(provider);
	
	if (start) {
		provider_name = xmlGetAttribute(provider, "name");
	
		if (!strcmp(frontendType.c_str(), "sat")) {
			diseqc = xmlGetAttribute(provider, "diseqc");
			position = xmlGetSignedNumericAttribute(provider, "position", 16);
			if (position == 0)
				fprintf(tmp, "\t<%s name=\"%s\" diseqc=\"%s\">\n", frontendType.c_str(), provider_name.c_str(), diseqc.c_str());
			else {
				//east_west = xmlGetNumericAttribute(provider, "east_west", 16);
				fprintf(tmp, "\t<%s name=\"%s\" position=\"%04x\" diseqc=\"%s\">\n", 
					frontendType.c_str(), 
					provider_name.c_str(),
					position,
					diseqc.c_str());
			}
			is_sat = true;
		}
		else
			fprintf(tmp, "\t<%s name=\"%s\">\n", frontendType.c_str(), provider_name.c_str());
	
	} else {
		if (!strcmp(frontendType.c_str(), "sat")) {
			fprintf(tmp, "\t</sat>\n");
			is_sat = true;
		}
		else
			fprintf(tmp, "\t</cable>\n");
	}
	return is_sat;
}

void write_transponder_node(FILE * tmp, xmlNodePtr transponder, const bool is_sat)
{
	fprintf(tmp, "\t\t<transponder id=\"%04x\" onid=\"%04x\" frequency=\"%d\" inversion=\"%d\" symbol_rate=\"%d\" fec_inner=\"%d\"",
		(int)xmlGetNumericAttribute(transponder, "id", 16),
		(int)xmlGetNumericAttribute(transponder, "onid", 16),
		(int)xmlGetNumericAttribute(transponder, "frequency", 0),
		(int)xmlGetNumericAttribute(transponder, "inversion", 0),
		(int)xmlGetNumericAttribute(transponder, "symbol_rate", 0),
		(int)xmlGetNumericAttribute(transponder, "fec_inner", 0));
	if (is_sat)
		fprintf(tmp, " polarization=\"%d\">\n", (int)xmlGetNumericAttribute(transponder, "polarization", 0));
	else
		fprintf(tmp, " modulation=\"%d\">\n", (int) xmlGetNumericAttribute(transponder, "modulation", 0));
}

void copy_transponder(FILE * tmp, xmlNodePtr transponder, const bool is_sat)
{
	std::string name;
	
//	fprintf(tmp, "Copying TP\n");
	
	write_transponder_node(tmp, transponder, is_sat);
	
	xmlNodePtr node = transponder->xmlChildrenNode;
	
	while (xmlGetNextOccurence(node, "channel") != NULL) {
		
		name = xmlGetAttribute(node, "name");
		fprintf(tmp,
			"\t\t\t<channel service_id=\"%04x\" name=\"%s\" service_type=\"%02x\"/>\n",
			(unsigned int)xmlGetNumericAttribute(node, "service_id", 16),
			convert_UTF8_To_UTF8_XML(name.c_str()).c_str(),
			(unsigned int)xmlGetNumericAttribute(node, "service_type", 16));
		node = node->xmlNextNode;
	}
}

xmlNodePtr getNextNode(xmlNodePtr node, t_service_id sid)
{	
	xmlNodePtr NextNode = NULL;
	while (xmlGetNextOccurence(node, "channel") != NULL) {
		if (xmlGetNumericAttribute(node, "service_id", 16) > sid) {
		 	if (NextNode == NULL)
		 		NextNode = node;
			else {
				if (xmlGetNumericAttribute(node, "service_id", 16) < xmlGetNumericAttribute(NextNode, "service_id", 16))
					NextNode = node;
			}
		}
		node = node->xmlNextNode;
	}
	return NextNode;
}

void merge_transponder(FILE * tmp, xmlNodePtr transponder, xmlNodePtr current_transponder, const bool is_sat)
{
	std::string name;
	bool take_from_current = false;
	bool remove = false;
	
	t_service_id sid;
	t_service_id csid;
	
//	fprintf(tmp, "Merging TP\n");
	
	write_transponder_node(tmp, transponder, is_sat);
	
	xmlNodePtr node = getNextNode(transponder->xmlChildrenNode, 0);
	xmlNodePtr current_node = getNextNode(current_transponder->xmlChildrenNode, 0);
	
	//as long one of the pointers has got services
	while ( (xmlGetNextOccurence(node, "channel") != NULL) || (xmlGetNextOccurence(current_node, "channel") != NULL) ) {
		remove = false;	
		//Both have services
		if ( (xmlGetNextOccurence(node, "channel") != NULL) && (xmlGetNextOccurence(current_node, "channel") != NULL) ) {
			sid = xmlGetNumericAttribute(node, "service_id", 16);
			csid = xmlGetNumericAttribute(current_node, "service_id", 16);
			//Take service from node
			if ( sid < csid )
				take_from_current = false;
			//Replace ro remove service
			else {
				if (sid == csid) {
					if (!strcmp(xmlGetAttribute(current_node, "action"), "remove")) {
						remove = true;
					//printf("Removing\n");
						current_node = getNextNode(current_transponder->xmlChildrenNode, csid);
						node = getNextNode(transponder->xmlChildrenNode, sid);
					}					
					else {
						//Replace
						take_from_current = true;
						node = getNextNode(transponder->xmlChildrenNode, sid);
					}
				}
				//not < or == Add service
				else //if xmlGetNumericAttribute(node, "service_id", 16) > xmlGetNumericAttribute(current_node, "service_id", 16))
					take_from_current = true;
			}
		} 
		//only one has services
		else {
			//There are services in node
			if (xmlGetNextOccurence(node, "channel") != NULL) {
				take_from_current = false;
				sid = xmlGetNumericAttribute(node, "service_id", 16);
				csid = 0;
			}
			//Only current_node has services (can only be action add...)
			else {
				take_from_current = true;
				csid = xmlGetNumericAttribute(current_node, "service_id", 16);
				sid = 0;
			}
		}
		if (!remove) {
			if (take_from_current) {
				name = xmlGetAttribute(current_node, "name");
//				printf("Taking from current: %s\n",name.c_str());
				fprintf(tmp,
					"\t\t\t<channel service_id=\"%04x\" name=\"%s\" service_type=\"%02x\"/>\n",
					csid,
					convert_UTF8_To_UTF8_XML(name.c_str()).c_str(),
					(unsigned int)xmlGetNumericAttribute(current_node, "service_type", 16));
				current_node = getNextNode(current_transponder->xmlChildrenNode, csid);
			} else {
				name = xmlGetAttribute(node, "name");
//				printf("Taking from services: %s\n",name.c_str());
				fprintf(tmp,
					"\t\t\t<channel service_id=\"%04x\" name=\"%s\" service_type=\"%02x\"/>\n",
					sid,
					convert_UTF8_To_UTF8_XML(name.c_str()).c_str(),
					(unsigned int)xmlGetNumericAttribute(node, "service_type", 16));
				node = getNextNode(transponder->xmlChildrenNode, sid);
			}
		}
	}
}

void mergeServices()
{
	FILE * tmp;
	xmlNodePtr provider;
	xmlNodePtr current_provider;
	xmlNodePtr transponder;
	xmlNodePtr current_transponder;
	
	t_transponder_id tsid;
	t_original_network_id onid;
	t_transponder_id current_tsid;
	t_original_network_id current_onid;
	bool is_sat = false;
	bool found = false;
	
	if (!(tmp = fopen(CURRENTSERVICES_XML, "r")))
		return;
		
	fclose(tmp);
	
	xmlDocPtr current_parser = parseXmlFile(CURRENTSERVICES_XML);
	if (current_parser == NULL)
		return;
	
	xmlDocPtr service_parser = parseXmlFile(SERVICES_XML);
	if (service_parser == NULL)
		return;
	
	if (!(tmp = fopen(SERVICES_TMP, "w")))
		return;
	
	provider = xmlDocGetRootElement(service_parser)->xmlChildrenNode;
	write_header(tmp);
	
	while (provider) {		
		transponder = provider->xmlChildrenNode;
		current_provider = xmlDocGetRootElement(current_parser)->xmlChildrenNode;		
		while ( (current_provider) && (strcmp(xmlGetAttribute(provider, "name"), xmlGetAttribute(current_provider, "name"))) )
			current_provider = current_provider->xmlNextNode;
		
		if (current_provider) {			
			is_sat = write_provider(tmp, current_provider, true);
			while (transponder) {
				found = false;
				onid = xmlGetNumericAttribute(transponder, "onid", 16);
				tsid = xmlGetNumericAttribute(transponder, "id", 16);
//				printf("ONID: %04x TSID: %04x from services.xml\n",onid, tsid);
				current_transponder = current_provider->xmlChildrenNode;
				while ( current_transponder && !found ) {
					current_onid = xmlGetNumericAttribute(current_transponder, "onid", 16);
					current_tsid = xmlGetNumericAttribute(current_transponder, "id", 16);
//					printf("ONID: %04x TSID: %04x from currentservices.xml\n",current_onid, current_tsid);
					if ( (tsid == current_tsid) &&
						(onid == current_onid) ) {
						merge_transponder(tmp, transponder, current_transponder, is_sat);	
						found = true;
						//printf("Using merge\n");
					}
					current_transponder = current_transponder->xmlNextNode;
				}	
				if (!found)					
					copy_transponder(tmp, transponder, is_sat);
				fprintf(tmp, "\t\t</transponder>\n");
				transponder = transponder->xmlNextNode;
			}
			//And now check for new transponders vice versa
			current_transponder = current_provider->xmlChildrenNode;			
			while (current_transponder) {
				found = false;
				current_onid = xmlGetNumericAttribute(current_transponder, "onid", 16);
				current_tsid = xmlGetNumericAttribute(current_transponder, "id", 16);
				transponder = provider->xmlChildrenNode;
				while ( transponder && !found ) {
					onid = xmlGetNumericAttribute(transponder, "onid", 16);
					tsid = xmlGetNumericAttribute(transponder, "id", 16);
					if ( (tsid == current_tsid) &&
						(onid == current_onid) )
						found = true;
					transponder = transponder->xmlNextNode;
				}
				if (!found) {
					copy_transponder(tmp, current_transponder, is_sat);
					fprintf(tmp, "\t\t</transponder>\n");
				}
				current_transponder = current_transponder->xmlNextNode;
			}
		} else {
			is_sat = write_provider(tmp, provider, true);
			while (transponder) {
				copy_transponder(tmp, transponder, is_sat);
				fprintf(tmp, "\t\t</transponder>\n");
				transponder = transponder->xmlNextNode;
			}
		}
		write_provider(tmp, provider, false);
		provider = provider->xmlNextNode;
	}
	fprintf(tmp, "</zapit>\n");
	
	fclose(tmp);
	xmlFreeDoc(service_parser);
	xmlFreeDoc(current_parser);
	
	cpy(SERVICES_TMP, SERVICES_XML);
	unlink(SERVICES_TMP);
	unlink(CURRENTSERVICES_XML);
}

static void write_bouquet_xml_header(FILE * fd)
{
	fprintf(fd,
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<!--\n"
		"  This file was modified by zapit.\n"
		"  It contains bouquets as signalled by the providers.\n"
		"  Those bouquets will be automatically maintained if sections scan is active.\n"
		"  Bouquet entries channel can no longer contain name or sat.\n"
		"  Name shall be changed by myservices.xml. What was sat for?\n"
		"  ONID/TSID/SID SHOULD be unique.\n"
		"  The order of services is provided as it came from the provider.\n"
		"  Please encourage your provider to send a useful BAT! Thank you.\n"
		"-->\n"
	       "<zapit>\n");
}

static void write_bouquet_xml_footer(FILE *fd)
{
	fprintf(fd, "</zapit>\n");
}

static void write_bouquet_xml(FILE *fd, xmlNodePtr bouquet)
{
	std::string name;

	name = xmlGetAttribute(bouquet, "name");
	
//	if (xmlGetNumericAttribute(bouquet, "type", 16) == 1) {
//		fprintf(fd, "\t<Bouquet type=\"1\" bouquet_id=\"%04x\" name=\"%s\" hidden=\"0\" locked=\"0\">\n",
//			xmlGetNumericAttribute(bouquet, "bouquet_id", 16),
//			name.c_str());
//	} else {
		fprintf(fd, "\t<Bouquet type=\"%01x\" bouquet_id=\"%04x\" name=\"%s\" hidden=\"%01x\" locked=\"%01x\">\n",
			(unsigned int)xmlGetNumericAttribute(bouquet, "type", 16),
			(unsigned int)xmlGetNumericAttribute(bouquet, "bouquet_id", 16),
			convert_UTF8_To_UTF8_XML(name.c_str()).c_str(),
			(unsigned int)xmlGetNumericAttribute(bouquet, "hidden", 16),
			(unsigned int)xmlGetNumericAttribute(bouquet, "locked", 16));
//	}
	
	xmlNodePtr channel = bouquet->xmlChildrenNode;
	while (xmlGetNextOccurence(channel, "channel") != NULL) {
		if (xmlGetAttribute(channel, "name") != NULL)
			name = xmlGetAttribute(channel, "name");
		else
			name = "";
		if (xmlGetAttribute(channel, "sat") != NULL) {
			fprintf(fd, "\t\t<channel serviceID=\"%04x\" name=\"%s\" tsid=\"%04x\" onid=\"%04x\" sat=\"%03x\"/>\n",
				(unsigned int)xmlGetNumericAttribute(channel, "serviceID", 16),
				convert_UTF8_To_UTF8_XML(name.c_str()).c_str(),
				(unsigned int)xmlGetNumericAttribute(channel, "tsid", 16),
				(unsigned int)xmlGetNumericAttribute(channel, "onid", 16),
				(unsigned int)xmlGetSignedNumericAttribute(channel, "sat", 16));	
		}
		else
			fprintf(fd, "\t\t<channel serviceID=\"%04x\" name=\"%s\" tsid=\"%04x\" onid=\"%04x\"/>\n",
				(unsigned int)xmlGetNumericAttribute(channel, "serviceID", 16),
				convert_UTF8_To_UTF8_XML(name.c_str()).c_str(),
				(unsigned int)xmlGetNumericAttribute(channel, "tsid", 16),
				(unsigned int)xmlGetNumericAttribute(channel, "onid", 16));
		channel = channel->xmlNextNode;
	}
	fprintf(fd, "\t</Bouquet>\n");
}

void mergeBouquets()
{
	FILE * tmp;
	xmlNodePtr bouquet;
	xmlNodePtr current_bouquet;
	t_bouquet_id bouquet_id;
	bool found;
	
	if (!(tmp = fopen(CURRENTBOUQUETS_XML, "r")))
		return;
		
	fclose(tmp);
	
	xmlDocPtr current_parser = parseXmlFile(CURRENTBOUQUETS_XML);
	if (current_parser == NULL)
		return;
	
	xmlDocPtr bouquet_parser = parseXmlFile(BOUQUETS_XML);
	if (bouquet_parser == NULL)
		return;
	
	if (!(tmp = fopen(BOUQUETS_TMP, "w")))
		return;

	bouquet = xmlDocGetRootElement(bouquet_parser)->xmlChildrenNode;	
	write_bouquet_xml_header(tmp);
		
	while (bouquet) {
	
		bouquet_id = xmlGetNumericAttribute(bouquet, "bouquet_id", 16);
		
		if (bouquet_id) {
		
			current_bouquet = xmlDocGetRootElement(current_parser)->xmlChildrenNode;
			found = false;
			
			while ((current_bouquet) && (!found)) {
				if (bouquet_id == xmlGetNumericAttribute(current_bouquet, "bouquet_id", 16)) {
					write_bouquet_xml(tmp, current_bouquet);
					found = true;
				}
				else
					current_bouquet = current_bouquet->xmlNextNode;
			}
			
			if (!found)
				write_bouquet_xml(tmp, bouquet);
				
		}
		else
			write_bouquet_xml(tmp, bouquet);
		bouquet = bouquet->xmlNextNode;
	}
	
	current_bouquet = xmlDocGetRootElement(current_parser)->xmlChildrenNode;
	while (current_bouquet) {
		
		bouquet_id = xmlGetNumericAttribute(current_bouquet, "bouquet_id", 16);
		bouquet = xmlDocGetRootElement(bouquet_parser)->xmlChildrenNode;
	
		found = false;
		
		while ((bouquet) && (!found)) {
			if (bouquet_id == xmlGetNumericAttribute(bouquet, "bouquet_id", 16))
				found = true;
			else
				bouquet = bouquet->xmlNextNode;
		}
		if (!found)
			write_bouquet_xml(tmp, current_bouquet);
		current_bouquet = current_bouquet->xmlNextNode;
	}
	
	write_bouquet_xml_footer(tmp);
	fclose(tmp);
	xmlFreeDoc(bouquet_parser);
	xmlFreeDoc(current_parser);
	
//	cpy(BOUQUETS_TMP, ZAPITCONFIGDIR "/bouquets.txt");
	cpy(BOUQUETS_TMP, BOUQUETS_XML);
	unlink(BOUQUETS_TMP);
	unlink(CURRENTBOUQUETS_XML);
}

void saveSettings(bool write)
{
	if (cc) {
		// now save the lowest channel number with the current channel_id
		int c = ((currentMode & RADIO_MODE) ? bouquetManager->radioChannelsBegin() : bouquetManager->tvChannelsBegin()).getLowestChannelNumberWithChannelID(cc->getChannelID());

		if (c >= 0)
		{
			if ((currentMode & RADIO_MODE))
				lastChannelRadio = c;
			else
				lastChannelTV = c;
		}
	}

	if (write) {
		config.setBool("saveLastChannel", saveLastChannel);
		config.setInt32("lastChannelMode", (currentMode & RADIO_MODE) ? 1 : 0);
		config.setInt32("lastChannelRadio", lastChannelRadio);
		config.setInt32("lastChannelTV", lastChannelTV);
		config.setInt32("startChannelRadio", startChannelRadio);
		config.setInt32("startChannelTV", startChannelTV);
		config.setBool("saveAudioPIDs", save_audioPIDs);
		config.setBool("makeRemainingChannelsBouquet", bouquetManager->remainingChannelsBouquet);

		config.setInt32("lastSatellitePosition", frontend->getCurrentSatellitePosition());
		config.setInt32("diseqcRepeats", frontend->getDiseqcRepeats());
		config.setInt32("diseqcType", frontend->getDiseqcType());

		if (config.getModifiedFlag())
			config.saveConfig(CONFIGFILE);
		if (save_audioPIDs) {
			FILE *audio_config_file = fopen(AUDIO_CONFIG_FILE, "w");
			if (audio_config_file) {
				for (map<t_channel_id, unsigned short>::iterator audio_map_it = audio_map.begin();
						audio_map_it != audio_map.end();
						audio_map_it++) {
					fwrite(&(audio_map_it->first), sizeof(t_channel_id), 1, audio_config_file);
					fwrite(&(audio_map_it->second), sizeof(unsigned short), 1, audio_config_file);
				}
				fclose(audio_config_file);
			}
		}
	}
}

CZapitClient::responseGetLastChannel load_settings(void)
{
	CZapitClient::responseGetLastChannel lastchannel;

	if (currentMode & RADIO_MODE)
		lastchannel.mode = 'r';
	else
		lastchannel.mode = 't';

	lastchannel.channelNumber = (currentMode & RADIO_MODE) ? lastChannelRadio : lastChannelTV;

	return lastchannel;
}


/*
 * - find transponder
 * - stop teletext, video, audio, pcr
 * - tune
 * - set up pids
 * - start pcr, audio, video, teletext
 * - start descrambler
 *
 * return 0 on success
 * return -1 otherwise
 *
 */

static transponder_id_t tuned_transponder_id = TRANSPONDER_ID_NOT_TUNED;
static int pmt_update_fd = -1;
static bool update_pmt = false;
static bool auto_fec = false;

void
remember_selected_audio()
{
	if (save_audioPIDs && cc) {
		if (cc->getAudioChannelCount() > 1) {
			audio_map[cc->getServiceId()] = cc->getAudioPid();
			DBG("*** Remembering apid = %d for channel (service-id) = %d",  cc->getAudioPid(), cc->getServiceId());
		} else {
			audio_map.erase(cc->getServiceId());
			DBG("*** Not Remembering apid = %d for channel (service-id) = %d",  cc->getAudioPid(), cc->getServiceId());
		}
	}
}

int zapit(const t_channel_id channel_id, bool in_nvod, transponder_id_t transponder_id)
{
	bool transponder_change;
	tallchans_iterator cit;
	transponder_id_t current_transponder_id;
	remember_selected_audio();

	if (pmt_update_fd >= 0)
	{
		pmt_stop_update_filter(&pmt_update_fd);
		pmt_update_fd = -1;
	}

	eventServer->sendEvent(CZapitClient::EVT_ZAP_CA_CLEAR, CEventServer::INITID_ZAPIT);
//	INFO("Event: CA_CLEAR send");

#if HAVE_DVB_API_VERSION < 3
	int retry = false;
 again:
#endif
	WARN("tuned_transponder_id: " PRINTF_TRANSPONDER_ID_TYPE, tuned_transponder_id);

	if (transponder_id == TRANSPONDER_ID_NOT_TUNED)	/* usual zap */
	{
		if (in_nvod) {
			current_is_nvod = true;
			cit = nvodchannels.find(channel_id);

			if (cit == nvodchannels.end()) {
				DBG("channel_id " PRINTF_CHANNEL_ID_TYPE_NO_LEADING_ZEROS " not found", channel_id);
				return -1;
			}
		}
		else {
			current_is_nvod = false;
			cit = allchans.find(channel_id);

			if (currentMode & RADIO_MODE) {
				if ((cit == allchans.end()) || (cit->second.getServiceType() != ST_DIGITAL_RADIO_SOUND_SERVICE)) {
					DBG("channel_id " PRINTF_CHANNEL_ID_TYPE_NO_LEADING_ZEROS " not found", channel_id);
					return -1;
				}
			}
			else {
				if (cit == allchans.end() || (cit->second.getServiceType() == ST_DIGITAL_RADIO_SOUND_SERVICE)) {
					DBG("channel_id " PRINTF_CHANNEL_ID_TYPE_NO_LEADING_ZEROS " not found", channel_id);
					return -1;
				}
			}
		}

		/* store the new channel */
		if (!cc || channel_id != cc->getChannelID())
		{
			/* do not only take a reference to the channel list,
			   copy the entry instead. Otherwise we will die if
			   the channellist is deleted, since channel members
			   might get modified. this happened in stop_scan() */
			if (cc)
				delete(cc);
			cc = new CZapitChannel(cit->second);
		}

		current_transponder_id = cc->getTransponderId();
	}
	else /* nvod subservice zap */
	{
		current_transponder_id = transponder_id;
	}

	stopPlayBack();
#ifdef HAVE_TRIPLEDRAGON
	if (cam)
		delete cam;
	cam = new CCam();
#endif

	/* have motor move satellite dish to satellite's position if necessary */
	if ((diseqcType == DISEQC_1_2) && (motorPositions[cc->getSatellitePosition()] != 0))
	{
		if ((frontend->getCurrentSatellitePosition() != cc->getSatellitePosition()))
		{
			printf("[zapit] currentSatellitePosition = %d, satellitePosition = %d\n", frontend->getCurrentSatellitePosition(), cc->getSatellitePosition());
			printf("[zapit] motorPosition = %d\n", motorPositions[cc->getSatellitePosition()]);
			frontend->positionMotor(motorPositions[cc->getSatellitePosition()]);

			waitForMotor = abs(cc->getSatellitePosition() - frontend->getCurrentSatellitePosition()) / motorRotationSpeed; //assuming 1.8 degrees/second motor rotation speed for the time being...
			printf("[zapit] waiting %d seconds for motor to turn satellite dish.\n", waitForMotor);
			eventServer->sendEvent(CZapitClient::EVT_ZAP_MOTOR, CEventServer::INITID_ZAPIT, &waitForMotor, sizeof(waitForMotor));
			sleep(waitForMotor);

			frontend->setCurrentSatellitePosition(cc->getSatellitePosition());
		}
	}

	/* if channel's transponder does not match the transponder tuned before ... */
	if (current_transponder_id != tuned_transponder_id) {

		/* ... tune to it if not in record mode ...
		   if tuned_transponder_id == 0, we are not tuned
		   => recording will not work anyway, let's retune at least... */
		if (tuned_transponder_id && (currentMode & RECORD_MODE))
			return -1;

		transponder_list_t::iterator t;
		t = transponders.find(current_transponder_id);
		if (t == transponders.end())
			return -1;

		int diff = frontend->setParameters(
				&t->second.feparams, t->second.polarization, t->second.DiSEqC);
		switch (diff) {
		case -1:
			WARN("tuning failed\n");
			tuned_transponder_id = TRANSPONDER_ID_NOT_TUNED;
			return -1;
		case 0:
			break;
		default:
			printf("[zapit] tuned frequency does not match request. difference: %d\n", diff);
			break;
		}

		transponder_change = true;

		tuned_transponder_id = current_transponder_id;
	}
	else {
		transponder_change = false;
	}

	CZapitChannel *thisChannel;

	if (transponder_id == TRANSPONDER_ID_NOT_TUNED)
	{
		thisChannel = cc;
		if (thisChannel->getServiceType() == ST_NVOD_REFERENCE_SERVICE) {
			current_is_nvod = true;
			saveSettings(false);
			return 0;
		}
	}
	else
	{
		thisChannel = new CZapitChannel(cc->getName(),
						channel_id & 0xffff,
						GET_TRANSPORT_STREAM_ID_FROM_TRANSPONDER_ID(transponder_id),
						GET_ORIGINAL_NETWORK_ID_FROM_TRANSPONDER_ID(transponder_id),
						1,
						frontend->getDiseqcPosition(),
						cc->getSatellitePosition(),
						GET_FREQUENCY_FROM_TRANSPONDER_ID(transponder_id)
						);
	}

	/* search pids if they are unknown */
#ifdef USE_PID_CACHE
	if (thisChannel->getPidsFlag() == false)
#endif
	{
		bool failed = false;
		unsigned char audioChannel = thisChannel->getAudioChannelIndex();

		thisChannel->resetPids();

		DBG("looking up pids for channel_id " PRINTF_CHANNEL_ID_TYPE, thisChannel->getChannelID());

		/* get program map table pid from program association table */
		if (thisChannel->getPmtPid() == NONE)
			if (parse_pat(thisChannel) < 0) {
				printf("[zapit] pat parsing failed\n");
#if HAVE_DVB_API_VERSION < 3
/* again, a workaround where i don't exactly know why it is needed.
   sometimes, tuning fails on the first try, and for a second try i
   need to go through a full tuning cycle again. Happened with DMAX
   on a dreambox 500S... */
				if (!retry) {
					retry = true;
					printf("[zapit] trying again...\n");
					// force a full tuning cycle
					tuned_transponder_id = TRANSPONDER_ID_NOT_TUNED;
					goto again;
				}
#endif
				failed = true;
			}

		/* parse program map table and store pids */
		if ((!failed) && (parse_pmt(thisChannel) < 0)) {
			WARN("pmt parsing failed");
			failed = true;
		}

		parse_static_pids(thisChannel);

		thisChannel->setAudioChannel(audioChannel);

		if ((!failed) && (thisChannel->getAudioPid() == NONE) && (thisChannel->getVideoPid() == NONE)) {
			WARN("neither audio nor video pid found");
			failed = true;
		}

		if (failed) {
			thisChannel->resetPids();
			if (cc != thisChannel)
				delete thisChannel;
			return -1;
		}
	}

	if (transponder_change == true)
		thisChannel->getCaPmt()->ca_pmt_list_management = 0x03;
	else
		thisChannel->getCaPmt()->ca_pmt_list_management = 0x04;
	if (save_audioPIDs) {
		DBG("***Now trying to get audio right:  %d\t%d\t%d",
			thisChannel->getAudioChannelCount(),
			thisChannel->getAudioChannel(0)->pid,
			thisChannel->getServiceId());
		if (audio_map.find(thisChannel->getServiceId()) != audio_map.end()) {
			DBG("*************** Searching *** %d **** %d ******\n", thisChannel->getServiceId(), audio_map[thisChannel->getServiceId()]);
			for (int i = 0; i < thisChannel->getAudioChannelCount(); i++) {
				if (thisChannel->getAudioChannel(i)->pid == audio_map[thisChannel->getServiceId()]) {
					//printf("[zapit-audiopids]: Restoring previous audiopid to %d\n", thisChannel->getAudioChannel(i)->pid);
					thisChannel->setAudioChannel(i);
				}
			}
		} //else 
			//printf("[zapit-audiopids]: No previous audiopid for this channel stored\n");
	}

	startPlayBack(thisChannel);
	cam->setCaPmt(thisChannel->getCaPmt());
	saveSettings(false);

	if (update_pmt)
		pmt_set_update_filter(thisChannel, &pmt_update_fd);

	if (cc != thisChannel)
		delete thisChannel;

	return 0;
}

int select_nvod_subservice_num(int num)
{
	t_original_network_id original_network_id;
	t_transport_stream_id transport_stream_id;
	t_service_id service_id;

	if (!cc || (cc->getServiceType() != ST_NVOD_REFERENCE_SERVICE) || (num < 0))
		return -1;

	if (tuned_transponder_id != cc->getTransponderId())
		zapit(cc->getChannelID(), false, TRANSPONDER_ID_NOT_TUNED);

	if (nvod_service_ids(cc->getTransportStreamId(), cc->getOriginalNetworkId(), cc->getServiceId(),
				num, &transport_stream_id, &original_network_id, &service_id) < 0)
		return -1;

	DBG("tsid: %04x, onid: %04x, sid: %04x\n", transport_stream_id, original_network_id, service_id);

	return zapit(CREATE_CHANNEL_ID, false, CREATE_TRANSPONDER_ID_FROM_FREQUENCY_SATELLITEPOSITION_ORIGINALNETWORK_TRANSPORTSTREAM_ID(cc->getFrequency(), cc->getSatellitePosition(), original_network_id, transport_stream_id));
}

int change_audio_pid(uint8_t index)
{
	if (!audioDemux || !audioDecoder || !cc)
		return -1;

	/* stop demux filter */
	if (audioDemux->stop() < 0)
		return -1;

	/* stop audio playback */
	if (audioDecoder->stop() < 0)
		return -1;

	/* update current channel */
	cc->setAudioChannel(index);
	remember_selected_audio();

	/* set bypass mode */
	CZapitAudioChannel *currentAudioChannel = cc->getAudioChannel();

	if (!currentAudioChannel) {
		WARN("No current audio channel");
		return -1;
	}

	if (currentAudioChannel->isAc3)
		audioDecoder->enableBypass();
	else
		audioDecoder->disableBypass();

	/* set demux filter */
	if (audioDemux->pesFilter(cc->getAudioPid(), DMX_OUT_DECODER, DMX_PES_AUDIO) < 0)
		return -1;

	/* start audio playback */
	if (audioDecoder->start() < 0)
		return -1;

	/* start demux filter */
	if (audioDemux->start() < 0)
		return -1;

	return 0;
}

int change_subtitle(uint index)
{
	if (!cc) return -1;
	cc->setChannelSub(index);
	return 0;
}

void setRadioMode(void)
{
	currentMode |= RADIO_MODE;
	currentMode &= ~TV_MODE;
}

void setTVMode(void)
{
	currentMode |= TV_MODE;
	currentMode &= ~RADIO_MODE;
}

int getMode(void)
{
	if (standby)
		return CZapitClient::MODE_STANDBY;
	if (currentMode & TV_MODE)
		return CZapitClient::MODE_TV;
	if (currentMode & RADIO_MODE)
		return CZapitClient::MODE_RADIO;
	return 0;
}

void setRecordMode(void)
{
	currentMode |= RECORD_MODE;
	eventServer->sendEvent(CZapitClient::EVT_RECORDMODE_ACTIVATED, CEventServer::INITID_ZAPIT );
}

void unsetRecordMode(void)
{
	currentMode &= ~RECORD_MODE;
	eventServer->sendEvent(CZapitClient::EVT_RECORDMODE_DEACTIVATED, CEventServer::INITID_ZAPIT );
}

int prepare_channels(fe_type_t frontendType, diseqc_t dType)
{
	// for the case this function is NOT called for the first time (by main())
	// we clear all cannel lists, they are refilled
	// by LoadServices() and LoadBouquets()
	transponders.clear();
	bouquetManager->clearAll();
	allchans.clear();  // <- this invalidates all bouquets, too!
	if (LoadServices(frontendType, dType, false) < 0)
		return -1;

	INFO("LoadServices: success");
	bouquetManager->loadBouquets();

	return 0;
}


void parseScanInputXml(void)
{
	std::string filename;
	std::string complete_filename;
	struct stat buf;

	switch (frontend->getInfo()->type) {
		case FE_QPSK:
			filename = SATELLITES_XML;
			break;

		case FE_QAM:
			filename = CABLES_XML;
			break;

		case FE_OFDM:
			filename = TERRESTRIAL_XML;
			break;

		default:
			WARN("Unknown type %d", frontend->getInfo()->type);
			return;
	}

	complete_filename = (std::string)ZAPITCONFIGDIR + "/" + filename;
	if ((stat(complete_filename.c_str(), &buf) == -1) && (errno == ENOENT)) {
		DBG("file %s does not exist", complete_filename.c_str());
		complete_filename = (std::string)DATADIR + "/" + filename;
	}
	DBG("complete_filename: %s", complete_filename.c_str());
	scanInputParser = parseXmlFile(complete_filename.c_str());
}

/*
 * return 0 on success
 * return -1 otherwise
 */
int start_scan(CZapitMessages::startScan msg)
{
	if (!scanInputParser) {
		parseScanInputXml();
		if (!scanInputParser) {
			WARN("scan not configured");
			return -1;
		}
	}

	bouquetManager->clearAll();
	stopPlayBack();
	if (pmt_update_fd>=0) {
		pmt_stop_update_filter(&pmt_update_fd);
		pmt_update_fd = -1;
	}
	tuned_transponder_id = TRANSPONDER_ID_NOT_TUNED;
	found_transponders = 0;
	found_channels = 0;
	scan_runs = 1;

	if ((errno=pthread_create(&scan_thread, 0, start_scanthread,  (void*)&msg))) {
		ERROR("pthread_create");
		scan_runs = 0;
		return -1;
	}
	/* This message is a hack: if the variable "msg" is not used, apparently the
	   compiler might optimize it out, so that it never reaches the scantread...
	   At least that's my conclusion from the strange failures I did encounter. */
	WARN("started scanthread with scan_mode: %d diseqc: %d", msg.scan_mode, msg.diseqc);

	return 0;
}

bool parse_command(CBasicMessage::Header &rmsg, int connfd)
{
	DBG("cmd %d (version %d) received", rmsg.cmd, rmsg.version);

	if ((int)rmsg.cmd > 128) // hack, old controld commands are > 1024 now
	{
		switch (rmsg.cmd)
		{
		case CControldMsg::CMD_SAVECONFIG:
			controldSaveSettings();
			break;
		case CControldMsg::CMD_SETVOLUME:
			CControldMsg::commandVolume msg_commandVolume;
			CBasicServer::receive_data(connfd, &msg_commandVolume, sizeof(msg_commandVolume));
			if (msg_commandVolume.type == CControld::TYPE_UNKNOWN)
				msg_commandVolume.type = settings.volume_type;
			else
				settings.volume_type = msg_commandVolume.type;

			if (msg_commandVolume.type != CControld::TYPE_LIRC)
			{
				settings.volume = msg_commandVolume.volume;
				if (settings.volume_type == CControld::TYPE_OST)
					controldconfig->setInt32("volume", settings.volume);
				else
					controldconfig->setInt32("volume_avs", settings.volume);
				audioDecoder->setVolume(msg_commandVolume.volume);
			}
#ifdef ENABLE_LIRC
			else if (msg_commandVolume.type == CControld::TYPE_LIRC)
			{
				if (msg_commandVolume.volume > 50)
				{
					CIRSend irs("volplus");
					irs.Send();
				}
				else if (msg_commandVolume.volume < 50)
				{
					CIRSend irs("volminus");
					irs.Send();
				}
			}
#endif
			else
				printf("[controld] msg_commandVolume.type == %d not supported on this box!\n", msg_commandVolume.type);
			eventServer->sendEvent(CControldClient::EVT_VOLUMECHANGED, CEventServer::INITID_CONTROLD);
			break;

		case CControldMsg::CMD_SETMUTE:
			CControldMsg::commandMute msg_commandMute;
			CBasicServer::receive_data(connfd, &msg_commandMute, sizeof(msg_commandMute));
			//printf("[controld] CControldMsg::CMD_SETMUTE: %d\n", msg_commandMute.mute);
			if (msg_commandMute.type == CControld::TYPE_UNKNOWN)
				msg_commandMute.type = settings.volume_type;
			else
				settings.volume_type = msg_commandMute.type;			

			if (msg_commandMute.type != CControld::TYPE_LIRC)
			{
				settings.mute = msg_commandMute.mute;
				controldconfig->setBool("mute", settings.mute);
				if (settings.mute)
					audioDecoder->mute();
				else
					audioDecoder->unmute();
			}
#ifdef ENABLE_LIRC
			else if (msg_commandMute.type == CControld::TYPE_LIRC)
			{
				CIRSend irs("mute");
				irs.Send();
			}
#else
			else
				printf("[controld] msg_commandMute.type == %d not supported on this box.\n", msg_commandMute.type);
#endif
			eventServer->sendEvent(CControldClient::EVT_MUTECHANGED, CEventServer::INITID_CONTROLD, &msg_commandMute, sizeof(msg_commandMute));
			break;

		case CControldMsg::CMD_GETVOLUME:
			CControldMsg::commandVolume msg_responseVolume;
			CBasicServer::receive_data(connfd, &msg_responseVolume, sizeof(msg_responseVolume));
			if (msg_responseVolume.type == CControld::TYPE_UNKNOWN)
				msg_responseVolume.type = settings.volume_type;
			if (msg_responseVolume.type != CControld::TYPE_LIRC)
				msg_responseVolume.volume = (unsigned char)settings.volume;
#ifdef ENABLE_LIRC
			else if (msg_responseVolume.type == CControld::TYPE_LIRC)
				msg_responseVolume.volume = 50; //we donnot really know...
#else
			else
				printf("[controld] msg_responseVolume.type == %d not supported on this box.\n", msg_responseVolume.type);
#endif
			CBasicServer::send_data(connfd, &msg_responseVolume, sizeof(msg_responseVolume));
			break;

		case CControldMsg::CMD_GETMUTESTATUS:
			CControldMsg::commandMute msg_responseMute;
			CBasicServer::receive_data(connfd, &msg_responseMute, sizeof(msg_responseMute));
			if (msg_responseMute.type == CControld::TYPE_UNKNOWN)
				msg_responseMute.type = settings.volume_type;
			if (msg_responseMute.type != CControld::TYPE_LIRC)
				msg_responseMute.mute = settings.mute;
#ifdef ENABLE_LIRC
			else if (msg_responseMute.type == CControld::TYPE_LIRC)
				msg_responseMute.mute = false; //we donnot really know...
#else
			else
				printf("[controld msg_responseMute.type == %d not supported on this box.\n", msg_responseMute.type);
#endif
			CBasicServer::send_data(connfd, &msg_responseMute, sizeof(msg_responseMute));
			break;

		case CControldMsg::CMD_SETVIDEOFORMAT:
			//printf("[controld] set videoformat\n");
			CControldMsg::commandVideoFormat msg2;
			CBasicServer::receive_data(connfd, &msg2, sizeof(msg2));
			settings.videoformat = msg2.format;
			videoDecoder->setVideoFormat(msg2.format);
			break;
		case CControldMsg::CMD_SETVIDEOOUTPUT:
			//printf("[controld] set videooutput\n");
			CControldMsg::commandVideoOutput msg3;
			CBasicServer::receive_data(connfd, &msg3, sizeof(msg3));
			setvideooutput((CControld::video_format)msg3.output);
			break;
		case CControldMsg::CMD_SETBOXTYPE:
			//printf("[controld] set boxtype\n");    //-------------------dummy!!!!!!!!!!
			CControldMsg::commandBoxType msg4;
			CBasicServer::receive_data(connfd, &msg4, sizeof(msg4));
			setBoxType();
			break;
		case CControldMsg::CMD_SETSCARTMODE:
			//printf("[controld] set scartmode\n");
			CControldMsg::commandScartMode msg5;
			CBasicServer::receive_data(connfd, &msg5, sizeof(msg5));
			setScartMode(msg5.mode);
			break;
		case CControldMsg::CMD_GETSCARTMODE:
			//printf("[controld] get scartmode\n");
			CControldMsg::responseScartMode msg51;
			msg51.mode = settings.vcr;
			CBasicServer::send_data(connfd, &msg51, sizeof(CControldMsg::responseScartMode));
			break;
		case CControldMsg::CMD_SETVIDEOPOWERDOWN:
			//printf("[controld] set scartmode\n");
			CControldMsg::commandVideoPowerSave msg10;
			CBasicServer::receive_data(connfd, &msg10, sizeof(msg10));
			disableVideoOutput(msg10.powerdown);
			break;

		case CControldMsg::CMD_GETVIDEOPOWERDOWN:
			//printf("[controld] CMD_GETVIDEOPOWERDOWN\n");
			CControldMsg::responseVideoPowerSave msg101;
			msg101.videoPowerSave = settings.videoOutputDisabled;
			CBasicServer::send_data(connfd, &msg101, sizeof(msg101));
			break;

		case CControldMsg::CMD_GETVIDEOFORMAT:
			//printf("[controld] get videoformat (fnc)\n");
			CControldMsg::responseVideoFormat msg8;
			msg8.format = settings.videoformat;
			CBasicServer::send_data(connfd,&msg8,sizeof(msg8));
			break;
		case CControldMsg::CMD_GETASPECTRATIO:
			//printf("[controld] get videoformat (fnc)\n");
			CControldMsg::responseAspectRatio msga;
			if (settings.vcr)
				msga.aspectRatio = settings.aspectRatio_vcr;
			else
				msga.aspectRatio = settings.aspectRatio_dvb;
			CBasicServer::send_data(connfd,&msga,sizeof(msga));
			break;
		case CControldMsg::CMD_GETVIDEOOUTPUT:
			//printf("[controld] get videooutput (fblk)\n");
			CControldMsg::responseVideoOutput msg9;
			msg9.output = settings.videooutput;
			CBasicServer::send_data(connfd,&msg9,sizeof(msg9));
			break;
		case CControldMsg::CMD_GETBOXTYPE:
			//printf("[controld] get boxtype\n");
			CControldMsg::responseBoxType msg0;
			msg0.boxtype = settings.boxtype;
			CBasicServer::send_data(connfd,&msg0,sizeof(msg0));
			break;

		case CControldMsg::CMD_SETCSYNC:
			CControldMsg::commandCsync msg11;
			CBasicServer::receive_data(connfd, &msg11, sizeof(msg11));
			setRGBCsync(msg11.csync);
			break;
		case CControldMsg::CMD_GETCSYNC:
			CControldMsg::commandCsync msg12;
			msg12.csync = getRGBCsync();
			CBasicServer::send_data(connfd, &msg12, sizeof(msg12));
			break;

		case CControldMsg::CMD_REGISTEREVENT:
			eventServer->registerEvent(connfd);
			break;
		case CControldMsg::CMD_UNREGISTEREVENT:
			eventServer->unRegisterEvent(connfd);
			break;

		case CControldMsg::CMD_SETVCROUTPUT:
			//printf("[controld] set vcroutput\n");
			CControldMsg::commandVCROutput msg13;
			CBasicServer::receive_data(connfd, &msg13, sizeof(msg13));
			setvcroutput((CControld::video_format)msg13.vcr_output);
			break;
		case CControldMsg::CMD_GETVCROUTPUT:
			//printf("[controld] get vcroutput\n");
			CControldMsg::responseVCROutput msg14;
			msg14.vcr_output = settings.vcroutput;
			CBasicServer::send_data(connfd,&msg14,sizeof(msg14));
			break;

		default:
			printf("[controld] unknown command %d\n", (int)rmsg.cmd);
		}
		return true;
	}
	// no more controld, only zapit here...
	if ((standby) && 
			((rmsg.cmd != CZapitMessages::CMD_SET_STANDBY) &&
			(rmsg.cmd != CZapitMessages::CMD_SHUTDOWN) &&
#ifdef HAVE_DBOX_HARDWARE
			(rmsg.cmd != CZapitMessages::CMD_SET_AE_IEC_ON) &&
			(rmsg.cmd != CZapitMessages::CMD_SET_AE_IEC_OFF) &&
			(rmsg.cmd != CZapitMessages::CMD_GET_AE_IEC_STATE) &&
			(rmsg.cmd != CZapitMessages::CMD_GET_AE_PLAYBACK_STATE) &&
#endif
#if HAVE_DVB_API_VERSION < 3
/* on the dreambox, we need zapit to set the volume also in movie- or audioplayer mode */
			(rmsg.cmd != CZapitMessages::CMD_SET_VOLUME) &&
			(rmsg.cmd != CZapitMessages::CMD_MUTE) &&
/* without SET_DISPLAY_FORMAT, controld cannot correct the aspect ratio in movieplayer */
//			(rmsg.cmd != CZapitMessages::CMD_SET_DISPLAY_FORMAT) &&
			(rmsg.cmd != CZapitMessages::CMD_SB_GET_PLAYBACK_ACTIVE) &&
#endif
#ifdef HAVE_TRIPLEDRAGON
			(rmsg.cmd != CZapitMessages::CMD_VID_IOCTL) &&
			(rmsg.cmd != CZapitMessages::CMD_SET_ZOOMLEVEL) &&
			(rmsg.cmd != CZapitMessages::CMD_GET_ZOOMLEVEL) &&
#endif
			(rmsg.cmd != CZapitMessages::CMD_GET_MODE) &&
			(rmsg.cmd != CZapitMessages::CMD_GETPIDS))) {
		fprintf(stderr, "[zapit] cmd %d refused in standby mode\n", rmsg.cmd);
		return true;
	}

	switch (rmsg.cmd) {
	case CZapitMessages::CMD_SHUTDOWN:
		return false;

	case CZapitMessages::CMD_SAVECONFIG:
		saveSettings(true);
		break;

	case CZapitMessages::CMD_ZAPTO:
	{
		CZapitMessages::commandZapto msgZapto;
		CBasicServer::receive_data(connfd, &msgZapto, sizeof(msgZapto)); // bouquet & channel number are already starting at 0!
		zapTo(msgZapto.bouquet, msgZapto.channel);
		break;
	}

	case CZapitMessages::CMD_ZAPTO_CHANNELNR:
	{
		CZapitMessages::commandZaptoChannelNr msgZaptoChannelNr;
		CBasicServer::receive_data(connfd, &msgZaptoChannelNr, sizeof(msgZaptoChannelNr)); // bouquet & channel number are already starting at 0!
		zapTo(msgZaptoChannelNr.channel);
		break;
	}

	case CZapitMessages::CMD_ZAPTO_SERVICEID:
	case CZapitMessages::CMD_ZAPTO_SUBSERVICEID:
	{
		CZapitMessages::commandZaptoServiceID msgZaptoServiceID;
		CZapitMessages::responseZapComplete msgResponseZapComplete;
		CBasicServer::receive_data(connfd, &msgZaptoServiceID, sizeof(msgZaptoServiceID));
		/* if nonblocking, send reply before zapping... */
		if (msgZaptoServiceID.nowait && !fastzap)
			CBasicServer::send_data(connfd, &msgResponseZapComplete, sizeof(msgResponseZapComplete));
		msgResponseZapComplete.zapStatus = zapTo_ChannelID(msgZaptoServiceID.channel_id, (rmsg.cmd == CZapitMessages::CMD_ZAPTO_SUBSERVICEID));
		/* else send it after zapping... */
		if (!msgZaptoServiceID.nowait || fastzap)
			CBasicServer::send_data(connfd, &msgResponseZapComplete, sizeof(msgResponseZapComplete));
		break;
	}

#ifndef NO_DEPRECATED_ZAPTO_NOWAIT
	case CZapitMessages::CMD_ZAPTO_SERVICEID_NOWAIT:
	case CZapitMessages::CMD_ZAPTO_SUBSERVICEID_NOWAIT:
	{
		ERROR("CMD_ZAPTO_SERVICEID_NOWAIT and CMD_ZAPTO_SUBSERVICEID_NOWAIT are deprecated!");
		CZapitMessages::commandZaptoServiceID msgZaptoServiceID;
		CBasicServer::receive_data(connfd, &msgZaptoServiceID, sizeof(msgZaptoServiceID));
		zapTo_ChannelID(msgZaptoServiceID.channel_id, (rmsg.cmd == CZapitMessages::CMD_ZAPTO_SUBSERVICEID_NOWAIT));
		break;
	}
#endif

	case CZapitMessages::CMD_GET_LAST_CHANNEL:
	{
		CZapitClient::responseGetLastChannel responseGetLastChannel;
		responseGetLastChannel = load_settings();
		CBasicServer::send_data(connfd, &responseGetLastChannel, sizeof(responseGetLastChannel)); // bouquet & channel number are already starting at 0!
		break;
	}

	case CZapitMessages::CMD_GET_CURRENT_SATELLITE_POSITION:
	{
		int32_t currentSatellitePosition = frontend->getCurrentSatellitePosition();
		CBasicServer::send_data(connfd, &currentSatellitePosition, sizeof(currentSatellitePosition));
		break;
	}

	case CZapitMessages::CMD_SET_AUDIOCHAN:
	{
		CZapitMessages::commandSetAudioChannel msgSetAudioChannel;
		CBasicServer::receive_data(connfd, &msgSetAudioChannel, sizeof(msgSetAudioChannel));
		change_audio_pid(msgSetAudioChannel.channel);
		break;
	}

	case CZapitMessages::CMD_SET_SUBTITLE:
	{
		/* Zapit does NOT need this info. This is just a storage */
		CZapitMessages::commandSetSubtitle msgSetSubtitle;
		CBasicServer::receive_data(connfd, &msgSetSubtitle, sizeof(msgSetSubtitle));
		change_subtitle(msgSetSubtitle.index);
		break;
	}
	case CZapitMessages::CMD_SET_MODE:
	{
		CZapitMessages::commandSetMode msgSetMode;
		CBasicServer::receive_data(connfd, &msgSetMode, sizeof(msgSetMode));
		if (msgSetMode.mode == CZapitClient::MODE_TV)
			setTVMode();
		else if (msgSetMode.mode == CZapitClient::MODE_RADIO)
			setRadioMode();
		break;
	}

	case CZapitMessages::CMD_GET_MODE:
	{
		CZapitMessages::responseGetMode msgGetMode;
		msgGetMode.mode = (CZapitClient::channelsMode) getMode();
		CBasicServer::send_data(connfd, &msgGetMode, sizeof(msgGetMode));
		break;
	}

	case CZapitMessages::CMD_GET_CURRENT_SERVICEID:
	{
		CZapitMessages::responseGetCurrentServiceID msgCurrentSID;
		msgCurrentSID.channel_id = (tuned_transponder_id != TRANSPONDER_ID_NOT_TUNED) ? cc->getChannelID() : 0;
		CBasicServer::send_data(connfd, &msgCurrentSID, sizeof(msgCurrentSID));
		break;
	}

	case CZapitMessages::CMD_GET_CURRENT_SERVICEINFO:
	{
		CZapitClient::CCurrentServiceInfo msgCurrentServiceInfo;
		msgCurrentServiceInfo.onid = cc->getOriginalNetworkId();
		msgCurrentServiceInfo.sid = cc->getServiceId();
		msgCurrentServiceInfo.tsid = cc->getTransportStreamId();
		msgCurrentServiceInfo.vpid = cc->getVideoPid();
		msgCurrentServiceInfo.apid = cc->getAudioPid();
		{
			CZapitAbsSub* sub = cc->getChannelSub(-1);
			if (sub) {
				CZapitDVBSub* sd = reinterpret_cast<CZapitDVBSub*>(sub);
				CZapitTTXSub* st = reinterpret_cast<CZapitTTXSub*>(sub);
				if (sub->thisSubType == CZapitAbsSub::DVB) {
					msgCurrentServiceInfo.spid = sd->pId;
					msgCurrentServiceInfo.spage = sd->composition_page_id;
				} else if (sub->thisSubType == CZapitAbsSub::TTX) {
					msgCurrentServiceInfo.spid = cc->getTeletextPid();
					msgCurrentServiceInfo.spage =
						st->teletext_magazine_number * 100 + (st->teletext_page_number >> 4) * 10 + (st->teletext_page_number & 0xf);
				}
			} else {
				msgCurrentServiceInfo.spid = 0;
				msgCurrentServiceInfo.spage = 0;
			}
		}
		msgCurrentServiceInfo.vtxtpid = cc->getTeletextPid();
		msgCurrentServiceInfo.pmtpid = cc->getPmtPid();
		msgCurrentServiceInfo.pcrpid = cc->getPcrPid();
		msgCurrentServiceInfo.tsfrequency = frontend->getFrequency();
		if (frontend->getInfo()->type == FE_QPSK)
			msgCurrentServiceInfo.polarisation = frontend->getPolarization();
		else
			msgCurrentServiceInfo.polarisation = 2;
		msgCurrentServiceInfo.diseqc = cc->getDiSEqC();
		CBasicServer::send_data(connfd, &msgCurrentServiceInfo, sizeof(msgCurrentServiceInfo));
		break;
	}

	case CZapitMessages::CMD_GET_DELIVERY_SYSTEM:
	{
		CZapitMessages::responseDeliverySystem response;
		switch (frontend->getInfo()->type) {
		case FE_QAM:
			response.system = DVB_C;
			break;
		case FE_QPSK:
			response.system = DVB_S;
			break;
		case FE_OFDM:
			response.system = DVB_T;
			break;
		default:
			break;
		}

		CBasicServer::send_data(connfd, &response, sizeof(response));
		break;
	}

	case CZapitMessages::CMD_GET_BOUQUETS:
	{
		CZapitMessages::commandGetBouquets msgGetBouquets;
		CBasicServer::receive_data(connfd, &msgGetBouquets, sizeof(msgGetBouquets));
		sendBouquets(connfd, msgGetBouquets.emptyBouquetsToo, msgGetBouquets.mode); // bouquet & channel number are already starting at 0!
		break;
	}

	case CZapitMessages::CMD_GET_BOUQUET_CHANNELS:
	{
		CZapitMessages::commandGetBouquetChannels msgGetBouquetChannels;
		CBasicServer::receive_data(connfd, &msgGetBouquetChannels, sizeof(msgGetBouquetChannels));
		sendBouquetChannels(connfd, msgGetBouquetChannels.bouquet, msgGetBouquetChannels.mode); // bouquet & channel number are already starting at 0!
		break;
	}

	case CZapitMessages::CMD_GET_CHANNELS:
	{
		CZapitMessages::commandGetChannels msgGetChannels;
		CBasicServer::receive_data(connfd, &msgGetChannels, sizeof(msgGetChannels));
		sendChannels(connfd, msgGetChannels.mode, msgGetChannels.order); // bouquet & channel number are already starting at 0!
		break;
	}

	case CZapitMessages::CMD_GET_CURRENT_TP:
	{
		TP_params TP;
		get_transponder(&TP);
#if HAVE_DVB_API_VERSION < 3
		INFO("current frequency: %lu", (long unsigned int)TP.feparams.Frequency);
#else
		INFO("current frequency: %lu", (long unsigned int)TP.feparams.frequency);
#endif
		CBasicServer::send_data(connfd, &TP, sizeof(TP));
		break;
	}

	case CZapitMessages::CMD_BQ_RESTORE:
	{
		CZapitMessages::responseCmd response;

		bouquetManager->clearAll();
		bouquetManager->loadBouquets();

		response.cmd = CZapitMessages::CMD_READY;
		CBasicServer::send_data(connfd, &response, sizeof(response));
		break;
	}

	case CZapitMessages::CMD_REINIT_CHANNELS:
	{
		// Houdini: save actual channel to restore it later, old version's channel was set to scans.conf initial channel
		t_channel_id cid= cc ? cc->getChannelID() : 0;

		CZapitMessages::responseCmd response;

		/* load configuration or set defaults if no configuration file exists */
		if (!config.loadConfig(CONFIGFILE))
			WARN("%s not found", CONFIGFILE);

		diseqcType = (diseqc_t)config.getInt32("diseqcType", NO_DISEQC);
		prepare_channels(frontend->getInfo()->type, diseqcType);

		tallchans_iterator cit = allchans.find(cid);
		if (cit != allchans.end()) 
		{
			if (cc)
				delete(cc);
			cc = new CZapitChannel(cit->second);
		}

		response.cmd = CZapitMessages::CMD_READY;
		CBasicServer::send_data(connfd, &response, sizeof(response));
		eventServer->sendEvent(CZapitClient::EVT_BOUQUETS_CHANGED, CEventServer::INITID_ZAPIT);
		break;
	}

	case CZapitMessages::CMD_REZAP:
	{
		CZapitMessages::responseZapComplete msgResponseZapComplete;
		if (cc)
		{
			/* force a full tuning cycle */
			tuned_transponder_id = TRANSPONDER_ID_NOT_TUNED;
			msgResponseZapComplete.zapStatus = zapTo_ChannelID(cc->getChannelID(), false);
		}
		else
			msgResponseZapComplete.zapStatus = 0;

		CBasicServer::send_data(connfd, &msgResponseZapComplete, sizeof(msgResponseZapComplete));
		break;
	}

	case CZapitMessages::CMD_SET_STARTCHANNEL_RADIO:
	{
		CZapitMessages::startChannel msg;
		CBasicServer::receive_data(connfd, &msg, sizeof(msg));
		startChannelRadio=(msg.channel);
		break;
	}

	case CZapitMessages::CMD_SET_STARTCHANNEL_TV:
	{
		CZapitMessages::startChannel msg;
		CBasicServer::receive_data(connfd, &msg, sizeof(msg));
		startChannelTV=(msg.channel);
		break;
	}

	case CZapitMessages::CMD_GET_STARTCHANNEL_RADIO:
	{
		CZapitMessages::startChannel msg;
		msg.channel = startChannelRadio;
		CBasicServer::send_data(connfd, &msg, sizeof(msg));
		break;
	}

	case CZapitMessages::CMD_GET_STARTCHANNEL_TV:
	{
		CZapitMessages::startChannel msg;
		msg.channel = startChannelTV;
		CBasicServer::send_data(connfd, &msg, sizeof(msg));
		break;
	}

	case CZapitMessages::CMD_SET_SAVE_LAST_CHANNEL:
	{
		CZapitMessages::commandBoolean msg;
		CBasicServer::receive_data(connfd, &msg, sizeof(msg));
		saveLastChannel = msg.truefalse;
		break;
	}

	case CZapitMessages::CMD_GET_SAVE_LAST_CHANNEL:
	{
		CZapitMessages::commandBoolean msg;
		msg.truefalse = saveLastChannel;
		CBasicServer::send_data(connfd, &msg, sizeof(msg));
		break;
	}

	case CZapitMessages::CMD_SET_SAVE_AUDIO_PIDS:
	{
		CZapitMessages::commandBoolean msg;
		CBasicServer::receive_data(connfd, &msg, sizeof(msg));
		save_audioPIDs = msg.truefalse;
		break;
	}

	case CZapitMessages::CMD_GET_SAVE_AUDIO_PIDS:
	{
		CZapitMessages::commandBoolean msg;
		msg.truefalse = save_audioPIDs;
		CBasicServer::send_data(connfd, &msg, sizeof(msg));
		break;
	}

	case CZapitMessages::CMD_SET_REMAINING_CHANNELS_BOUQUET:
	{
		CZapitMessages::commandBoolean msg;
		CBasicServer::receive_data(connfd, &msg, sizeof(msg));
		bouquetManager->remainingChannelsBouquet = msg.truefalse;
		break;
	}

	case CZapitMessages::CMD_GET_REMAINING_CHANNELS_BOUQUET:
	{
		CZapitMessages::commandBoolean msg;
		msg.truefalse = bouquetManager->remainingChannelsBouquet;
		CBasicServer::send_data(connfd, &msg, sizeof(msg));
		break;
	}

	case CZapitMessages::CMD_RELOAD_CURRENTSERVICES:
	{
		CZapitMessages::responseCmd response;
		t_channel_id cid= cc ? cc->getChannelID() : 0;

		transponders.clear();
		bouquetManager->clearAll();
		allchans.clear();  // <- this invalidates all bouquets, too!
		LoadServices(frontend->getInfo()->type, diseqcType, false); //true for only loading currentservices...
		bouquetManager->loadBouquets();
		tallchans_iterator cit = allchans.find(cid);
		if (cit != allchans.end()) 
		{
			if (cc)
				delete(cc);
			cc = new CZapitChannel(cit->second);
		}
		response.cmd = CZapitMessages::CMD_READY;
		CBasicServer::send_data(connfd, &response, sizeof(response));
		eventServer->sendEvent(CZapitClient::EVT_BOUQUETS_CHANGED, CEventServer::INITID_ZAPIT);
		break;
	}

	case CZapitMessages::CMD_SCANSTART:
	{
		CZapitMessages::startScan msg;
		CBasicServer::receive_data(connfd, &msg, sizeof(msg));

		if (start_scan(msg) == -1)
			eventServer->sendEvent(CZapitClient::EVT_SCAN_FAILED, CEventServer::INITID_ZAPIT);
		break;
	}

	case CZapitMessages::CMD_SCANSTOP:
		if (scan_runs)
		{
			scan_clean();
		}
		break;

	case CZapitMessages::CMD_SCAN_TP:
	{
		TP_params TP;
		t_channel_id save_channel;
		
		if (!cc) break; // otherwise zapit dies
		// save channel info
		save_channel = cc->getChannelID();
		CBasicServer::receive_data(connfd, &TP, sizeof(TP));
// Houdini: now configured/send by neutrino!
//		TP.diseqc=transponder->second.DiSEqC;
		bouquetManager->clearAll();
		allchans.clear();  // <- this invalidates all bouquets, too!
		stopPlayBack();
		if(scan_transponder(&TP))
		{
			DBG("transponder scan ok");
		}
		prepare_channels(frontend->getInfo()->type, diseqcType);
		DBG("channels reinit ok");
		eventServer->sendEvent(CZapitClient::EVT_BOUQUETS_CHANGED, CEventServer::INITID_ZAPIT);
		// restore channel
		// todo zap to restored channel
//		channel = bouquetManager->findChannelByChannelID(save_channel);
//		zapTo_ChannelID(save_channel, false);
				
		break;
	}

	case CZapitMessages::CMD_SCANREADY:
	{
		CZapitMessages::responseIsScanReady msgResponseIsScanReady;
		msgResponseIsScanReady.satellite = curr_sat;
		msgResponseIsScanReady.transponder = found_transponders;
		msgResponseIsScanReady.processed_transponder = processed_transponders;
		msgResponseIsScanReady.services = found_channels;
		if (scan_runs > 0)
			msgResponseIsScanReady.scanReady = false;
		else
			msgResponseIsScanReady.scanReady = true;
		CBasicServer::send_data(connfd, &msgResponseIsScanReady, sizeof(msgResponseIsScanReady));
		break;
	}

	case CZapitMessages::CMD_SCANGETSATLIST:
	{
		if (!scanInputParser)
		{
			parseScanInputXml();
			if (!scanInputParser)
				break;
		}

		uint32_t satlength;
		char * satname;
		xmlNodePtr search = xmlDocGetRootElement(scanInputParser)->xmlChildrenNode;
		const char * frontendname = getFrontendName();
		CZapitClient::responseGetSatelliteList sat;

		if (frontendname != NULL) {
			while ((search = xmlGetNextOccurence(search, frontendname)) != NULL)
			{
				satname = xmlGetAttribute(search, "name");
				strncpy(sat.satName, satname, 29);
				sat.satPosition = satellitePositions[satname];
				sat.motorPosition = motorPositions[sat.satPosition];
				sat.satDiseqc = -1; /* FIXME */
				satlength = sizeof(sat);
				//printf("[zapit] sending %s, %d, %d\n", sat.satName, sat.satPosition, sat.motorPosition);
				CBasicServer::send_data(connfd, &satlength, sizeof(satlength));
				CBasicServer::send_data(connfd, (char *)&sat, satlength);
				search = search->xmlNextNode;
			}
		}
		satlength = SATNAMES_END_MARKER;
		CBasicServer::send_data(connfd, &satlength, sizeof(satlength));
		break;
	}

	case CZapitMessages::CMD_SCANSETSCANSATLIST:
	{
		CZapitClient::commandSetScanSatelliteList sat;
		CZapitMessages::commandInt num;
		scanProviders.clear();
		CBasicServer::receive_data(connfd, &num, sizeof(num));

		while (num.val-- > 0) {
			CBasicServer::receive_data(connfd, &sat, sizeof(sat));
			DBG("adding %s (diseqc %d)", sat.satName, sat.diseqc);
			scanProviders[sat.diseqc] = sat.satName;
		}
		break;
	}

	case CZapitMessages::CMD_SCANSETSCANMOTORPOSLIST:
	{
		CZapitClient::commandSetScanMotorPosList pos;
		CZapitMessages::commandInt num;
		bool changed = false;
		FILE * fd;

		CBasicServer::receive_data(connfd, &num, sizeof(num));
		while (num.val-- > 0) {
			CBasicServer::receive_data(connfd, &pos, sizeof(pos));
			//printf("adding %d (motorPos %d)\n", pos.satPosition, pos.motorPos);
			changed |= (motorPositions[pos.satPosition] != pos.motorPos);
			motorPositions[pos.satPosition] = pos.motorPos;
		}

		if (changed)
		{
			// save to motor.conf
			//printf("[zapit] saving motor.conf\n");
			fd = fopen(MOTORCONFIGFILE, "w");
			for (mpos_it = motorPositions.begin(); mpos_it != motorPositions.end(); mpos_it++)
			{
				//printf("[zapit] saving %d %d\n", mpos_it->first, mpos_it->second);
				fprintf(fd, "%d %d\n", mpos_it->first, mpos_it->second);
			}
			fclose(fd);
			chmod(MOTORCONFIGFILE, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		}
		break;
	}

	case CZapitMessages::CMD_SCANSETDISEQCTYPE:
	{
		CBasicServer::receive_data(connfd, &diseqcType, sizeof(diseqcType));
		frontend->setDiseqcType(diseqcType);
		DBG("set diseqc type %d", diseqcType);
		break;
	}

	case CZapitMessages::CMD_SCANSETDISEQCREPEAT:
	{
		uint32_t repeats;
		CBasicServer::receive_data(connfd, &repeats, sizeof(repeats));
		frontend->setDiseqcRepeats(repeats);
		DBG("set diseqc repeats to %d", repeats);
		break;
	}

	case CZapitMessages::CMD_SCANSETBOUQUETMODE:
		CBasicServer::receive_data(connfd, &bouquetMode, sizeof(bouquetMode));
		break;

	case CZapitMessages::CMD_SCANSETTYPE:
		CBasicServer::receive_data(connfd, &scanType, sizeof(scanType));
		break;

	case CZapitMessages::CMD_SET_RECORD_MODE:
	{
		CZapitMessages::commandSetRecordMode msgSetRecordMode;
		CBasicServer::receive_data(connfd, &msgSetRecordMode, sizeof(msgSetRecordMode));
		if (msgSetRecordMode.activate)
			setRecordMode();
		else
			unsetRecordMode();
		break;
	}

	case CZapitMessages::CMD_GET_RECORD_MODE:
	{
		CZapitMessages::responseGetRecordModeState msgGetRecordModeState;
		msgGetRecordModeState.activated = (currentMode & RECORD_MODE);
		CBasicServer::send_data(connfd, &msgGetRecordModeState, sizeof(msgGetRecordModeState));
		break;
	}

	case CZapitMessages::CMD_SB_GET_PLAYBACK_ACTIVE:
	{
		CZapitMessages::responseGetPlaybackState msgGetPlaybackState;
		if (videoDecoder && videoDecoder->getPlayState() == VIDEO_PLAYING)
			msgGetPlaybackState.activated = 1;
		else
			msgGetPlaybackState.activated = 0;
		CBasicServer::send_data(connfd, &msgGetPlaybackState, sizeof(msgGetPlaybackState));
		break;
	}

	case CZapitMessages::CMD_BQ_ADD_BOUQUET:
	{
		char * name = CBasicServer::receive_string(connfd);
		bouquetManager->addBouquet(name);
		CBasicServer::delete_string(name);
		break;
	}

	case CZapitMessages::CMD_BQ_DELETE_BOUQUET:
	{
		CZapitMessages::commandDeleteBouquet msgDeleteBouquet;
		CBasicServer::receive_data(connfd, &msgDeleteBouquet, sizeof(msgDeleteBouquet)); // bouquet & channel number are already starting at 0!
		bouquetManager->deleteBouquet(msgDeleteBouquet.bouquet);
		break;
	}

	case CZapitMessages::CMD_BQ_RENAME_BOUQUET:
	{
		CZapitMessages::commandRenameBouquet msgRenameBouquet;
		CBasicServer::receive_data(connfd, &msgRenameBouquet, sizeof(msgRenameBouquet)); // bouquet & channel number are already starting at 0!
		char * name = CBasicServer::receive_string(connfd);
		if (msgRenameBouquet.bouquet < bouquetManager->Bouquets.size())
			bouquetManager->Bouquets[msgRenameBouquet.bouquet]->Name = name;
		CBasicServer::delete_string(name);
		break;
	}

	case CZapitMessages::CMD_BQ_EXISTS_BOUQUET:
	{
		CZapitMessages::responseGeneralInteger responseInteger;

		char * name = CBasicServer::receive_string(connfd);
		responseInteger.number = bouquetManager->existsBouquet(name);
		CBasicServer::delete_string(name);

		CBasicServer::send_data(connfd, &responseInteger, sizeof(responseInteger)); // bouquet & channel number are already starting at 0!
		break;
	}

	case CZapitMessages::CMD_BQ_EXISTS_CHANNEL_IN_BOUQUET:
	{
		CZapitMessages::commandExistsChannelInBouquet msgExistsChInBq;
		CZapitMessages::responseGeneralTrueFalse responseBool;
		CBasicServer::receive_data(connfd, &msgExistsChInBq, sizeof(msgExistsChInBq)); // bouquet & channel number are already starting at 0!
		responseBool.status = bouquetManager->existsChannelInBouquet(msgExistsChInBq.bouquet, msgExistsChInBq.channel_id);
		CBasicServer::send_data(connfd, &responseBool, sizeof(responseBool));
		break;
	}

	case CZapitMessages::CMD_BQ_MOVE_BOUQUET:
	{
		CZapitMessages::commandMoveBouquet msgMoveBouquet;
		CBasicServer::receive_data(connfd, &msgMoveBouquet, sizeof(msgMoveBouquet)); // bouquet & channel number are already starting at 0!
		bouquetManager->moveBouquet(msgMoveBouquet.bouquet, msgMoveBouquet.newPos);
		break;
	}

	case CZapitMessages::CMD_BQ_ADD_CHANNEL_TO_BOUQUET:
	{
		CZapitMessages::commandAddChannelToBouquet msgAddChannelToBouquet;
		CBasicServer::receive_data(connfd, &msgAddChannelToBouquet, sizeof(msgAddChannelToBouquet)); // bouquet & channel number are already starting at 0!
		addChannelToBouquet(msgAddChannelToBouquet.bouquet, msgAddChannelToBouquet.channel_id);
		break;
	}

	case CZapitMessages::CMD_BQ_REMOVE_CHANNEL_FROM_BOUQUET:
	{
		CZapitMessages::commandRemoveChannelFromBouquet msgRemoveChannelFromBouquet;
		CBasicServer::receive_data(connfd, &msgRemoveChannelFromBouquet, sizeof(msgRemoveChannelFromBouquet)); // bouquet & channel number are already starting at 0!
		if (msgRemoveChannelFromBouquet.bouquet < bouquetManager->Bouquets.size())
			bouquetManager->Bouquets[msgRemoveChannelFromBouquet.bouquet]->removeService(msgRemoveChannelFromBouquet.channel_id);
		break;
	}

	case CZapitMessages::CMD_BQ_MOVE_CHANNEL:
	{
		CZapitMessages::commandMoveChannel msgMoveChannel;
		CBasicServer::receive_data(connfd, &msgMoveChannel, sizeof(msgMoveChannel)); // bouquet & channel number are already starting at 0!
		if (msgMoveChannel.bouquet < bouquetManager->Bouquets.size())
			bouquetManager->Bouquets[msgMoveChannel.bouquet]->moveService(msgMoveChannel.oldPos, msgMoveChannel.newPos,
					(((currentMode & RADIO_MODE) && msgMoveChannel.mode == CZapitClient::MODE_CURRENT)
					 || (msgMoveChannel.mode==CZapitClient::MODE_RADIO)) ? 2 : 1);
		break;
	}

	case CZapitMessages::CMD_BQ_SET_LOCKSTATE:
	{
		CZapitMessages::commandBouquetState msgBouquetLockState;
		CBasicServer::receive_data(connfd, &msgBouquetLockState, sizeof(msgBouquetLockState)); // bouquet & channel number are already starting at 0!
		if (msgBouquetLockState.bouquet < bouquetManager->Bouquets.size())
			bouquetManager->Bouquets[msgBouquetLockState.bouquet]->bLocked = msgBouquetLockState.state;
		break;
	}

	case CZapitMessages::CMD_BQ_SET_HIDDENSTATE:
	{
		CZapitMessages::commandBouquetState msgBouquetHiddenState;
		CBasicServer::receive_data(connfd, &msgBouquetHiddenState, sizeof(msgBouquetHiddenState)); // bouquet & channel number are already starting at 0!
		if (msgBouquetHiddenState.bouquet < bouquetManager->Bouquets.size())
			bouquetManager->Bouquets[msgBouquetHiddenState.bouquet]->bHidden = msgBouquetHiddenState.state;
		break;
	}

	case CZapitMessages::CMD_BQ_RENUM_CHANNELLIST:
		bouquetManager->renumServices();
		break;

	case CZapitMessages::CMD_BQ_SAVE_BOUQUETS:
	{
		CZapitMessages::responseCmd response;
		CZapitMessages::commandBoolean msgBoolean;

		CBasicServer::receive_data(connfd, &msgBoolean, sizeof(msgBoolean));
		bouquetManager->saveBouquets(msgBoolean.truefalse);
		bouquetManager->renumServices();

		response.cmd = CZapitMessages::CMD_READY;
		CBasicServer::send_data(connfd, &response, sizeof(response));

		eventServer->sendEvent(CZapitClient::EVT_BOUQUETS_CHANGED, CEventServer::INITID_ZAPIT);

		break;
	}

	case CZapitMessages::CMD_SET_PAL:
		videoDecoder->setVideoSystem(PAL);
		break;

	case CZapitMessages::CMD_SET_NTSC:
		videoDecoder->setVideoSystem(NTSC);
		break;

	case CZapitMessages::CMD_SB_START_PLAYBACK:
		playbackStopForced = false;
		/* FIXME: nvod */
		startPlayBack(cc);
		break;

	case CZapitMessages::CMD_SB_STOP_PLAYBACK:
		stopPlayBack();
		playbackStopForced = true;
		break;

#if 0
	case CZapitMessages::CMD_SET_DISPLAY_FORMAT:
	{
		CZapitMessages::commandInt msg;
		CBasicServer::receive_data(connfd, &msg, sizeof(msg));
		videoDecoder->setCroppingMode((video_displayformat_t) msg.val);
		break;
	}
#endif

	case CZapitMessages::CMD_SET_AUDIO_MODE:
	{
		CZapitMessages::commandInt msg;
		CBasicServer::receive_data(connfd, &msg, sizeof(msg));
		audioDecoder->setChannel((audio_channel_select_t) msg.val);
		break;
	}

	case CZapitMessages::CMD_GETPIDS:
	{
		if (cc)
		{
			CZapitClient::responseGetOtherPIDs responseGetOtherPIDs;
			responseGetOtherPIDs.vpid = cc->getVideoPid();
			responseGetOtherPIDs.ecmpid = NONE; // TODO: remove
			responseGetOtherPIDs.vtxtpid = cc->getTeletextPid();
			responseGetOtherPIDs.pcrpid = cc->getPcrPid();
			responseGetOtherPIDs.pmtpid = cc->getPmtPid();
			responseGetOtherPIDs.selected_apid = cc->getAudioChannelIndex();
			responseGetOtherPIDs.selected_sub = cc->getChannelSubIndex();
			responseGetOtherPIDs.privatepid = cc->getPrivatePid();
			CBasicServer::send_data(connfd, &responseGetOtherPIDs, sizeof(responseGetOtherPIDs));
			sendAPIDs(connfd);
			sendSubPIDs(connfd);
		}
		break;
	}

	case CZapitMessages::CMD_GET_FE_SIGNAL:
	{
		CZapitClient::responseFESignal response_FEsig;

		response_FEsig.sig = frontend->getSignalStrength();
		response_FEsig.snr = frontend->getSignalNoiseRatio();
		response_FEsig.ber = frontend->getBitErrorRate();

		CBasicServer::send_data(connfd, &response_FEsig, sizeof(CZapitClient::responseFESignal));
		break;
	}

	case CZapitMessages::CMD_SETSUBSERVICES:
	{
		CZapitClient::commandAddSubServices msgAddSubService;
		CZapitMessages::commandInt numServices;

		CBasicServer::receive_data(connfd, &numServices, sizeof(numServices));
		while (numServices.val-- > 0)
		{
			CBasicServer::receive_data(connfd, &msgAddSubService, sizeof(msgAddSubService));
			t_original_network_id original_network_id = msgAddSubService.original_network_id;
			t_service_id          service_id          = msgAddSubService.service_id;
			transponder_list_t::iterator t;
			frequency_kHz_t frequency = cc->getFrequency();

	/* following lines are because the current_transponder_id(nvod channels) might not have the correct frequency (but the frequency of the 'master' channel),
	  either complete the CSubService struct in remotecontrol.cpp, but frequency doesn't seem to be available there 
	  or look for the correct transponder (search without frequency) */
			if (transponders.find(CREATE_TRANSPONDER_ID_FROM_FREQUENCY_SATELLITEPOSITION_ORIGINALNETWORK_TRANSPORTSTREAM_ID(frequency, cc->getSatellitePosition(), original_network_id, msgAddSubService.transport_stream_id)) == transponders.end())
				for (t = transponders.begin(); t != transponders.end(); t++)
				{
					if((GET_SATELLITEPOSITION_FROM_TRANSPONDER_ID(t->first) == cc->getSatellitePosition()) &&
					   (GET_ORIGINAL_NETWORK_ID_FROM_TRANSPONDER_ID(t->first) == original_network_id) &&
					   (GET_TRANSPORT_STREAM_ID_FROM_TRANSPONDER_ID(t->first) == msgAddSubService.transport_stream_id))
					{
						/* transponder found: (lets hope that it is not a duplicate one) leave for loop */
						frequency = GET_FREQUENCY_FROM_TRANSPONDER_ID(t->first);
						break;
					}
				}
			
			nvodchannels.insert
			(
				std::pair <t_channel_id, CZapitChannel>
				(
					CREATE_CHANNEL_ID_FROM_SERVICE_ORIGINALNETWORK_TRANSPORTSTREAM_ID(msgAddSubService.service_id, msgAddSubService.original_network_id, msgAddSubService.transport_stream_id),
					CZapitChannel
					(
					    "NVOD",
					    service_id,
					    msgAddSubService.transport_stream_id,
					    original_network_id,
					    1,
					    cc->getDiSEqC(),
					    cc->getSatellitePosition(),
//					    channel->getFrequency()
					    frequency
					)
				)
			);
		}
		current_is_nvod = true;
		break;
	}

	case CZapitMessages::CMD_REGISTEREVENTS:
		eventServer->registerEvent(connfd);
		break;

	case CZapitMessages::CMD_UNREGISTEREVENTS:
		eventServer->unRegisterEvent(connfd);
		break;

	case CZapitMessages::CMD_MUTE:
	{
		CZapitMessages::commandBoolean msgBoolean;
		CBasicServer::receive_data(connfd, &msgBoolean, sizeof(msgBoolean));
		if (msgBoolean.truefalse)
			audioDecoder->mute();
		else
			audioDecoder->unmute();
		break;
	}

	case CZapitMessages::CMD_SET_VOLUME:
	{
		CZapitMessages::commandVolume msgVolume;
		CBasicServer::receive_data(connfd, &msgVolume, sizeof(msgVolume));
		audioDecoder->setVolume(msgVolume.left);
		break;
	}

	case CZapitMessages::CMD_SET_STANDBY:
	{
		CZapitMessages::responseCmd response;
		CZapitMessages::commandBoolean msgBoolean;
		CBasicServer::receive_data(connfd, &msgBoolean, sizeof(msgBoolean));
		if (msgBoolean.truefalse)
			enterStandby();
		else
			leaveStandby();
		response.cmd = CZapitMessages::CMD_READY;
		CBasicServer::send_data(connfd, &response, sizeof(response));
		break;
	}

	case CZapitMessages::CMD_NVOD_SUBSERVICE_NUM:
	{
		CZapitMessages::commandInt msg;
		CBasicServer::receive_data(connfd, &msg, sizeof(msg));
		select_nvod_subservice_num(msg.val);
		break;
	}

	case CZapitMessages::CMD_SEND_MOTOR_COMMAND:
	{
		CZapitMessages::commandMotor msgMotor;
		CBasicServer::receive_data(connfd, &msgMotor, sizeof(msgMotor));
		printf("[zapit] received motor command: %x %x %x %x %x %x\n", msgMotor.cmdtype, msgMotor.address, msgMotor.cmd, msgMotor.num_parameters, msgMotor.param1, msgMotor.param2);
		frontend->sendMotorCommand(msgMotor.cmdtype, msgMotor.address, msgMotor.cmd, msgMotor.num_parameters, msgMotor.param1, msgMotor.param2);
		break;
	}

	case CZapitMessages::CMD_GET_CHANNEL_NAME:
	{
		t_channel_id                           requested_channel_id;
		CZapitMessages::responseGetChannelName response;
		CBasicServer::receive_data(connfd, &requested_channel_id, sizeof(requested_channel_id));
		tallchans_iterator it = allchans.find(requested_channel_id);
		if (it == allchans.end())
			response.name[0] = 0;
		else
			strncpy(response.name, it->second.getName().c_str(), 30);

		CBasicServer::send_data(connfd, &response, sizeof(response));
		break;
	}

	case CZapitMessages::CMD_GET_CHANNELNR_NAME:
	{
		CZapitMessages::commandGetChannelNrName msg;
		CZapitMessages::responseGetChannelName response;
		CBasicServer::receive_data(connfd, &msg, sizeof(msg));

		CBouquetManager::ChannelIterator cit = ((msg.mode & RADIO_MODE) ? bouquetManager->radioChannelsBegin() : bouquetManager->tvChannelsBegin()).FindChannelNr(msg.channel);
		if (cit.EndOfChannels())
		{
			response.name[0] = 0;
		}
		else
		{
			strncpy(response.name, (*cit)->getName().c_str(), 30);
		}

		CBasicServer::send_data(connfd, &response, sizeof(response));
		break;
	}

	case CZapitMessages::CMD_IS_TV_CHANNEL:
	{
		t_channel_id                             requested_channel_id;
		CZapitMessages::responseGeneralTrueFalse response;
		CBasicServer::receive_data(connfd, &requested_channel_id, sizeof(requested_channel_id));
		tallchans_iterator it = allchans.find(requested_channel_id);
		if (it == allchans.end())
			/* if in doubt (i.e. unknown channel) answer yes for possible subservices  */
			response.status = true; //true == tv mode
		else
			/* FIXME: the following check is no even remotely accurate */
			response.status = (it->second.getServiceType() != ST_DIGITAL_RADIO_SOUND_SERVICE);

		CBasicServer::send_data(connfd, &response, sizeof(response));
		break;
	}

#ifdef HAVE_DBOX_HARDWARE
	case CZapitMessages::CMD_SET_AE_IEC_ON:
	{
		setIec(1);
		break;
	}

	case CZapitMessages::CMD_SET_AE_IEC_OFF:
	{
		setIec(0);
		break;
	}

	case CZapitMessages::CMD_GET_AE_IEC_STATE:
	{
                CZapitMessages::responseGeneralInteger responseInteger;
                responseInteger.number = aviaExtDriver->iecState();
                CBasicServer::send_data(connfd, &responseInteger, sizeof(responseInteger));
		break;
	}

	case CZapitMessages::CMD_SET_AE_PLAYBACK_PES:
	{
		setDemuxMode(0);
		break;
	}

	case CZapitMessages::CMD_SET_AE_PLAYBACK_SPTS:
	{
		setDemuxMode(1);
		break;
	}

	case CZapitMessages::CMD_GET_AE_PLAYBACK_STATE:
	{
		CZapitMessages::responseGeneralInteger responseInteger;
		responseInteger.number = aviaExtDriver->playbackState();
		CBasicServer::send_data(connfd, &responseInteger, sizeof(responseInteger));
		break;
	}
#endif
#if HAVE_DVB_API_VERSION < 3
	case CZapitMessages::CMD_SET_FASTZAP:
	{
		CZapitMessages::commandBoolean msgBoolean;
		CBasicServer::receive_data(connfd, &msgBoolean, sizeof(msgBoolean));
		setFastZap(msgBoolean.truefalse);
		break;
	}
#endif
#ifdef HAVE_TRIPLEDRAGON
	case CZapitMessages::CMD_SET_ZOOMLEVEL:
	{
		CZapitMessages::commandInt msg;
		CBasicServer::receive_data(connfd, &msg, sizeof(msg));
		if (videoDecoder)
			videoDecoder->setZoom(msg.val);
		break;
	}
	case CZapitMessages::CMD_GET_ZOOMLEVEL:
	{
		CZapitMessages::responseGeneralInteger responseInteger;
		if (videoDecoder)
			responseInteger.number = videoDecoder->getZoom();
		else
			responseInteger.number = 0;
		CBasicServer::send_data(connfd, &responseInteger, sizeof(responseInteger));
		break;
	}
	case CZapitMessages::CMD_SET_PIG:
	{
		CZapitMessages::commandPig msg;
		CBasicServer::receive_data(connfd, &msg, sizeof(msg));
		if (videoDecoder)
			videoDecoder->setPig(msg.x, msg.y, msg.w, msg.h, msg.aspect);
		break;
	}
	case CZapitMessages::CMD_VID_IOCTL:
	{
		CZapitMessages::commandIoctl msg;
		CZapitMessages::responseGeneralInteger responseInteger;
		CBasicServer::receive_data(connfd, &msg, sizeof(msg));
		if (videoDecoder)
			responseInteger.number = videoDecoder->VdecIoctl(msg.request, msg.arg);
		else
			responseInteger.number = -ENODEV;
		CBasicServer::send_data(connfd, &responseInteger, sizeof(responseInteger));
		break;
	}
#endif
	default:
		WARN("unknown command %d (version %d)", rmsg.cmd, CZapitMessages::ACTVERSION);
		break;
	}

	DBG("cmd %d processed", rmsg.cmd);

	return true;
}

/****************************************************************/
/*								*/
/*  functions for new command handling via CZapitClient		*/
/*								*/
/*  these functions should be encapsulated in a class CZapit	*/
/*								*/
/****************************************************************/

void addChannelToBouquet(const unsigned int bouquet, const t_channel_id channel_id)
{
	DBG("addChannelToBouquet(%d, " PRINTF_CHANNEL_ID_TYPE_NO_LEADING_ZEROS ")", bouquet, channel_id);

	CZapitChannel* chan = bouquetManager->findChannelByChannelID(channel_id);

	if (chan != NULL)
		if (bouquet < bouquetManager->Bouquets.size())
			bouquetManager->Bouquets[bouquet]->addService(chan);
		else
			WARN("bouquet not found");
	else
		WARN("channel_id not found in channellist");
}

void sendBouquets(int connfd, const bool emptyBouquetsToo, const CZapitClient::channelsMode mode)
{
	CZapitClient::responseGetBouquets msgBouquet;
	int wantedMode = TV_MODE;

	if (mode == CZapitClient::MODE_CURRENT) {
		if (currentMode & RADIO_MODE) 		wantedMode = RADIO_MODE;
		else if (currentMode & TV_MODE)		wantedMode = TV_MODE;
	} 
	else if (mode == CZapitClient::MODE_RADIO) 	wantedMode = RADIO_MODE;
	else if (mode == CZapitClient::MODE_TV) 	wantedMode = TV_MODE;
	else if (mode == CZapitClient::MODE_ALL) 	wantedMode = TV_MODE | RADIO_MODE;

	for (uint i = 0; i < bouquetManager->Bouquets.size(); i++)
	{
		if (emptyBouquetsToo ||
			((!bouquetManager->Bouquets[i]->bHidden) &&
			(((wantedMode & RADIO_MODE) && !bouquetManager->Bouquets[i]->radioChannels.empty()) ||
			((wantedMode & TV_MODE) && !bouquetManager->Bouquets[i]->tvChannels.empty()))))
		{
// ATTENTION: in RECORD_MODE empty bouquets are not send!
			if ((!(currentMode & RECORD_MODE)) ||
			    ((cc != NULL) &&
			     (((wantedMode & RADIO_MODE) && (bouquetManager->Bouquets[i]->recModeRadioSize(cc->getTransponderId()) > 0)) ||
			      ((wantedMode & TV_MODE)    && (bouquetManager->Bouquets[i]->recModeTVSize   (cc->getTransponderId()) > 0)))))
			{
				msgBouquet.bouquet_nr = i;
				strncpy(msgBouquet.name, bouquetManager->Bouquets[i]->Name.c_str(), 30);
				msgBouquet.name[29]   = '\0'; // so string is zero terminated -> no need to strncopy in neutrino
				msgBouquet.locked     = bouquetManager->Bouquets[i]->bLocked;
				msgBouquet.hidden     = bouquetManager->Bouquets[i]->bHidden;
				msgBouquet.type       = bouquetManager->Bouquets[i]->type;
				msgBouquet.bouquet_id = bouquetManager->Bouquets[i]->bouquet_id;
				if (CBasicServer::send_data(connfd, &msgBouquet, sizeof(msgBouquet)) == false)
				{
					ERROR("could not send any return");
					return;
				}
			}
		}
	}
	msgBouquet.bouquet_nr = RESPONSE_GET_BOUQUETS_END_MARKER;
	if (CBasicServer::send_data(connfd, &msgBouquet, sizeof(msgBouquet)) == false)
	{
		ERROR("could not send end marker");
		return;
	}
}

bool send_data_count(const int connfd, const int data_count)
{
	CZapitMessages::responseGeneralInteger responseInteger;
	responseInteger.number = data_count;
	if (CBasicServer::send_data(connfd, &responseInteger, sizeof(responseInteger)) == false)
	{
		ERROR("could not send any return");
		return false;
	}
	return true;
}

void internalSendChannels(int connfd, ChannelList* channels, const unsigned int first_channel_nr)
{
	int data_count = channels->size();
	if (currentMode & RECORD_MODE)
	{
		for (uint32_t i = 0; i < channels->size(); i++)
			if ((*channels)[i]->getTransponderId() != cc->getTransponderId())
				data_count--;
	}

	if (!send_data_count(connfd, data_count))
		return;

	for (uint32_t i = 0; i < channels->size();i++)
	{
		if ((currentMode & RECORD_MODE) && ((*channels)[i]->getTransponderId() != cc->getTransponderId()))
			continue;

		CZapitClient::responseGetBouquetChannels response;
		strncpy(response.name, ((*channels)[i]->getName()).c_str(), 30);
		response.name[29]   = '\0'; // so string is zero terminated
		response.satellitePosition = (*channels)[i]->getSatellitePosition();
		response.channel_id = (*channels)[i]->getChannelID();
		response.nr = first_channel_nr + i;
		response.service_type = (*channels)[i]->getServiceType();

		if (CBasicServer::send_data(connfd, &response, sizeof(response)) == false)
		{
			ERROR("could not send any return");
			return;
		}
	}
}

void sendAPIDs(int connfd)
{
	if (!send_data_count(connfd, cc->getAudioChannelCount()))
		return;

	for (uint32_t i = 0; i < cc->getAudioChannelCount(); i++)
	{
		CZapitClient::responseGetAPIDs response;
		response.pid = cc->getAudioPid(i);
		strncpy(response.desc, cc->getAudioChannel(i)->description.c_str(), 25);
		response.is_ac3 = cc->getAudioChannel(i)->isAc3;
		response.component_tag = cc->getAudioChannel(i)->componentTag;

		if (CBasicServer::send_data(connfd, &response, sizeof(response)) == false)
		{
			ERROR("could not send any return");
			return;
		}
	}
}

void sendSubPIDs(int connfd)
{
	if (!send_data_count(connfd, cc->getSubtitleCount()))
		return;
	for (int i = 0 ; i < (int)cc->getSubtitleCount() ; ++i) {
		CZapitClient::responseGetSubPIDs response;
		CZapitAbsSub* s = cc->getChannelSub(i);
		CZapitDVBSub* sd = reinterpret_cast<CZapitDVBSub*>(s);
		CZapitTTXSub* st = reinterpret_cast<CZapitTTXSub*>(s);

		response.pid = sd->pId;
		strncpy(response.desc, sd->ISO639_language_code.c_str(), 4);
		if (s->thisSubType == CZapitAbsSub::DVB) {
			response.composition_page = sd->composition_page_id;
			response.ancillary_page = sd->ancillary_page_id;
			if (sd->subtitling_type >= 0x20) {
				response.hearingImpaired = true;
			} else {
				response.hearingImpaired = false;
			}
			if (CBasicServer::send_data(connfd, &response, sizeof(response)) == false) {
				ERROR("could not send any return");
	                        return;
        	        }
		} else if (s->thisSubType == CZapitAbsSub::TTX) {
			response.composition_page = (st->teletext_magazine_number * 100) + ((st->teletext_page_number >> 4) * 10) + (st->teletext_page_number & 0xf);
			response.ancillary_page = 0;
			response.hearingImpaired = st->hearingImpaired;
			if (CBasicServer::send_data(connfd, &response, sizeof(response)) == false) {
				ERROR("could not send any return");
	                        return;
        	        }
		}
	}
}

void sendBouquetChannels(int connfd, const unsigned int bouquet, const CZapitClient::channelsMode mode)
{
	if (bouquet >= bouquetManager->Bouquets.size())
	{
		WARN("invalid bouquet number: %d", bouquet);
		return;
	}

	if (((currentMode & RADIO_MODE) && (mode == CZapitClient::MODE_CURRENT)) || (mode == CZapitClient::MODE_RADIO))
		internalSendChannels(connfd, &(bouquetManager->Bouquets[bouquet]->radioChannels), bouquetManager->radioChannelsBegin().getNrofFirstChannelofBouquet(bouquet));
	else
		internalSendChannels(connfd, &(bouquetManager->Bouquets[bouquet]->tvChannels), bouquetManager->tvChannelsBegin().getNrofFirstChannelofBouquet(bouquet));
}

void sendChannels(int connfd, const CZapitClient::channelsMode mode, const CZapitClient::channelsOrder order)
{
	ChannelList channels;

	if (order == CZapitClient::SORT_BOUQUET)
	{
		CBouquetManager::ChannelIterator cit = (((currentMode & RADIO_MODE) && (mode == CZapitClient::MODE_CURRENT)) || (mode==CZapitClient::MODE_RADIO)) ? bouquetManager->radioChannelsBegin() : bouquetManager->tvChannelsBegin();
		for (; !(cit.EndOfChannels()); cit++)
			channels.push_back(*cit);
	}
	else if (order == CZapitClient::SORT_ALPHA)   // ATTENTION: in this case response.nr does not return the actual number of the channel for zapping!
	{
		if (((currentMode & RADIO_MODE) && (mode == CZapitClient::MODE_CURRENT)) || (mode==CZapitClient::MODE_RADIO))
		{
			for (tallchans_iterator it = allchans.begin(); it != allchans.end(); it++)
				if (it->second.getServiceType() == ST_DIGITAL_RADIO_SOUND_SERVICE)
					channels.push_back(&(it->second));
		}
		else
		{
			for (tallchans_iterator it = allchans.begin(); it != allchans.end(); it++)
				if (it->second.getServiceType() != ST_DIGITAL_RADIO_SOUND_SERVICE)
					channels.push_back(&(it->second));
		}
		sort(channels.begin(), channels.end(), CmpChannelByChName());
	}

	internalSendChannels(connfd, &channels, 0);
}

int startPlayBack(CZapitChannel *thisChannel)
{
	bool have_pcr = false;
	bool have_audio = false;
	bool have_video = false;
	bool have_teletext = false;

	if ((playbackStopForced == true) || (!thisChannel))
		return -1;

	if (thisChannel->getPcrPid() != 0) {
		have_pcr = true;
	}
	if (thisChannel->getAudioPid() != NONE) {
		have_audio = true;
	}
	if ((thisChannel->getVideoPid() != NONE) && (currentMode & TV_MODE)) {
		have_video = true;
	}
	if (thisChannel->getTeletextPid() != 0) {
		have_teletext = true;
	}

	if ((!have_audio) && (!have_video) && (!have_teletext)) {
		return -1;
	}

	/* set demux filters */
	if (have_video) {
		if (!videoDemux)
			videoDemux = new CDemux();
		if (videoDemux->pesFilter(thisChannel->getVideoPid(), DMX_OUT_DECODER, DMX_PES_VIDEO) < 0)
			return -1;
		if (videoDemux->start() < 0)
			return -1;
	}
	if (have_audio) {
		if (!audioDemux)
			audioDemux = new CDemux();
		if (audioDemux->pesFilter(thisChannel->getAudioPid(), DMX_OUT_DECODER, DMX_PES_AUDIO) < 0)
			return -1;
		if (audioDemux->start() < 0)
			return -1;
	}
	if (have_pcr) {
		if (!pcrDemux)
			pcrDemux = new CDemux();
		if (pcrDemux->pesFilter(thisChannel->getPcrPid(), DMX_OUT_DECODER, DMX_PES_PCR) < 0)
			return -1;
		if (pcrDemux->start() < 0)
			return -1;
	}
#ifdef HAVE_DBOX_HARDWARE
/* AFAIK only the dbox2 can reinsert telextext... */
	if (have_teletext) {
		if (!teletextDemux)
			teletextDemux = new CDemux();
		if (teletextDemux->pesFilter(thisChannel->getTeletextPid(), DMX_OUT_DECODER, DMX_PES_TELETEXT) < 0)
			return -1;
		if (teletextDemux->start() < 0)
			return -1;
	}
#endif

	/* those HAVE_TRIPLEDRAGON ifdefs are here for a reason:
	   - the tripledragon drivers sometimes seem to crash when the ordering of starting
	     audio/video demultiplexers and decoders is not as they expect it
	     so strace'd the original software and implemented the ioctl()'s in the same
	     order - no more crashes
	   - the nokia dbox2 with avia500 and SPTS mode makes crackling noise and delays
	     video decoding if audio decoder is started before the video decoder.
	   - the dreambox does not seem to care ;)
	 */
#ifndef HAVE_TRIPLEDRAGON
	/* start video */
	if (have_video) {
		videoDecoder->setSource(VIDEO_SOURCE_DEMUX);
		videoDecoder->start();
	}
#endif

	/* select audio output and start audio */
	if (have_audio) {
		if (thisChannel->getAudioChannel()->isAc3)
			audioDecoder->enableBypass();
		else
			audioDecoder->disableBypass();

		audioDecoder->setSource(AUDIO_SOURCE_DEMUX);
		audioDecoder->start();
	}

#ifdef HAVE_TRIPLEDRAGON
	/* start video */
	if (have_video) {
#ifdef HAVE_TRIPLEDRAGON
		videoDecoder->setBlank(true);
#endif
		videoDecoder->setSource(VIDEO_SOURCE_DEMUX);
		videoDecoder->start();
	}
#endif

	return 0;
}

int stopPlayBack(void)
{
	if (playbackStopForced)
		return -1;

	if (teletextDemux)
		teletextDemux->stop();
	if (videoDemux)
		videoDemux->stop();
	if (audioDemux)
		audioDemux->stop();
	if (pcrDemux)
		pcrDemux->stop();
	if (videoDecoder)
		videoDecoder->stop();
	if (audioDecoder)
		audioDecoder->stop();

	return 0;
}

#ifdef HAVE_DBOX_HARDWARE
void setIec(int iec_active)
{
	if (iec_active == 0)
                aviaExtDriver->iecOff();
	else
                aviaExtDriver->iecOn();
}

void setDemuxMode(int demux_mode)
{
	if (demux_mode == 0)
                aviaExtDriver->playbackPES();
	else
                aviaExtDriver->playbackSPTS();

	if ((videoDecoder->getPlayState() == VIDEO_PLAYING) && cc) {
		//printf("[zapit] restarting playback after changing demux mode\n");
		stopPlayBack();
		playbackStopForced = true;
		sleep(1);
		playbackStopForced = false;
		startPlayBack(cc);
	}
}
#endif

#if HAVE_DVB_API_VERSION < 3
void setFastZap(int mode)
{
#define VIDEO_SET_FASTZAP       _IOW('o', 4, int)
	int mpeg_fd = open("/dev/video", O_WRONLY);
	if (mpeg_fd > -1) {
		printf("[zapit] set VIDEO_SET_FASTZAP %d\n", mode);
		if (ioctl(mpeg_fd, VIDEO_SET_FASTZAP, mode) < 0)
			perror("zapit: VIDEO_SET_FASTZAP");
		close(mpeg_fd);
	} else
		perror("zapit: open /dev/video");

	fastzap = mode;
}
#endif

void enterStandby(void)
{
	if (standby) {
		sleep(1);
		return;
	}

	standby = true;
	
	saveSettings(true);
	stopPlayBack();
	mergeServices();
	mergeBouquets();
	
	if (audioDemux) {
		delete audioDemux;
		audioDemux = NULL;
	}
	if (pcrDemux) {
		delete pcrDemux;
		pcrDemux = NULL;
	}
	if (teletextDemux) {
		delete teletextDemux;
		teletextDemux = NULL;
	}
	if (videoDemux) {
		delete videoDemux;
		videoDemux = NULL;
	}
#ifndef HAVE_DREAMBOX_HARDWARE
	/* on the dreambox (dm500 tested) we need the audio device for setting
	   the volume and it can be opened multiple times. Other drivers (dbox)
	   do not allow multiple open() of the audio device */
	if (audioDecoder) {
		/* audioDecoder is used to set volume also in standby mode */
		audioDecoder->closeDevice();
	}
#endif
	if (cam) {
		delete cam;
		cam = NULL;
	}
	if (frontend) {
		delete frontend;
		frontend = NULL;
	}
#ifdef HAVE_DBOX_HARDWARE
	// needed in standby to correct aspect ration in movieplayer, but not on dbox...
	if (videoDecoder) {
		delete videoDecoder;
		videoDecoder = NULL;
	}
#endif

	tuned_transponder_id = TRANSPONDER_ID_NOT_TUNED;
}

void leaveStandby(void)
{
	if (!cam) {
		cam = new CCam();
	}
	if (!frontend) {
		frontend = new CFrontend(config.getInt32("uncommitted_switch_mode", 0), (int)auto_fec + finetune);
	}
#ifdef HAVE_DBOX_HARDWARE
	if (!aviaExtDriver) {
		aviaExtDriver = new CAViAext();
	}
#endif
	if (!audioDecoder)
		audioDecoder = new CAudio();
	else	// reopen the device...
		audioDecoder->openDevice();

	if (settings.mute)
		audioDecoder->mute();
	else
		audioDecoder->unmute();

	if (!videoDecoder)
		videoDecoder = new CVideo();

	switch (frontend->getInfo()->type) {
		case FE_QPSK:
			frontend->setCurrentSatellitePosition(config.getInt32("lastSatellitePosition", 192));
			frontend->setDiseqcRepeats(config.getInt32("diseqcRepeats", 0));
			motorRotationSpeed = config.getInt32("motorRotationSpeed", 18); // default: 1.8 degrees per second
			diseqcType = (diseqc_t)config.getInt32("diseqcType", NO_DISEQC);
			frontend->setDiseqcType(diseqcType);

			for (unsigned int i = 0; i < MAX_LNBS; i++) {
				char tmp[17]; // "lnb63_OffsetHigh\0"
				sprintf(tmp, "lnb%d_OffsetLow", i);
				frontend->setLnbOffset(false, i, config.getInt32(tmp, 9750000));
				sprintf(tmp, "lnb%d_OffsetHigh", i);
				frontend->setLnbOffset(true, i, config.getInt32(tmp, 10600000));
			}
			break;

		default:
			break;
	}

	if (cc)
		zapit(cc->getChannelID(), current_is_nvod, 0);

	standby = false;
}

unsigned zapTo(const unsigned int bouquet, const unsigned int channel)
{
	if (bouquet >= bouquetManager->Bouquets.size()) {
		WARN("Invalid bouquet %d", bouquet);
		return CZapitClient::ZAP_INVALID_PARAM;
	}

	ChannelList *channels;

	if (currentMode & RADIO_MODE)
		channels = &(bouquetManager->Bouquets[bouquet]->radioChannels);
	else
		channels = &(bouquetManager->Bouquets[bouquet]->tvChannels);

	if (channel >= channels->size()) {
		WARN("Invalid channel %d in bouquet %d", channel, bouquet);
		return CZapitClient::ZAP_INVALID_PARAM;
	}

	return zapTo_ChannelID((*channels)[channel]->getChannelID(), false);
}

unsigned int zapTo_ChannelID(t_channel_id channel_id, bool isSubService)
{
	unsigned int result = 0;

	if (zapit(channel_id, isSubService, 0) < 0)
	{
		eventServer->sendEvent((isSubService ? CZapitClient::EVT_ZAP_SUB_FAILED : CZapitClient::EVT_ZAP_FAILED), CEventServer::INITID_ZAPIT, &channel_id, sizeof(channel_id));
		return result;
	}

	result |= CZapitClient::ZAP_OK;

	if (isSubService)
	{
		eventServer->sendEvent(CZapitClient::EVT_ZAP_SUB_COMPLETE, CEventServer::INITID_ZAPIT, &channel_id, sizeof(channel_id));
	}
	else if (current_is_nvod)
	{
		eventServer->sendEvent(CZapitClient::EVT_ZAP_COMPLETE_IS_NVOD, CEventServer::INITID_ZAPIT, &channel_id, sizeof(channel_id));
		result |= CZapitClient::ZAP_IS_NVOD;
	}
	else
		eventServer->sendEvent(CZapitClient::EVT_ZAP_COMPLETE, CEventServer::INITID_ZAPIT, &channel_id, sizeof(channel_id));

	return result;
}

unsigned zapTo(const unsigned int channel)
{
	CBouquetManager::ChannelIterator cit = ((currentMode & RADIO_MODE) ? bouquetManager->radioChannelsBegin() : bouquetManager->tvChannelsBegin()).FindChannelNr(channel);
	if (!(cit.EndOfChannels()))
		return zapTo_ChannelID((*cit)->getChannelID(), false);
	else
		return 0;
}

void signal_handler(int signum)
{
	switch (signum) {
	case SIGUSR1:
		debug = !debug;
		break;
	default:
                CZapitClient zapit;
                zapit.shutdown();
                break;
	}
}

int main(int argc, char **argv)
{
	fprintf(stdout, "$Id: zapit.cpp,v 1.447 2009/10/12 07:22:55 rhabarber1848 Exp $\n");

	bool check_lock = true;
	int opt;

	while ((opt = getopt(argc, argv, "dqaunl")) > 0) {
		switch (opt) {
		case 'd':
			debug = true;
			break;
		case 'q':
			/* don't say anything */
			int fd;
			close(STDOUT_FILENO);
			if ((fd = open("/dev/null", O_WRONLY)) != STDOUT_FILENO)
				close(fd);
			close(STDERR_FILENO);
			if ((fd = open("/dev/null", O_WRONLY)) != STDERR_FILENO)
				close(fd);
			break;
		case 'a':
			auto_fec = true;
			break;
		case 'u':
			update_pmt = true;
			printf("[zapit] PMT update enabled\n");
			break;
		case 'n':
#ifdef HAVE_TRIPLEDRAGON
			finetune = 0;
#else
			fastzap = 0;
#endif
			break;
		case 'l':
			printf("[zapit] lock loss check disabled\n");
			check_lock = false;
			break;
		default:
			fprintf(stderr,
				"Usage: %s [-d] [-q] [-u] [-l]\n"
				"-d : debug mode\n"
				"-q : quiet mode\n"
				"-u : enable update on PMT change\n"
				"-l : disable checking for lost lock\n"
#if HAVE_DVB_API_VERSION < 3
				"-n : disable FASTZAP\n"
#endif
#ifdef HAVE_TRIPLEDRAGON
				"-n : disable finetuning\n"
#endif
				"\n"
				"Keys in config file "	CONFIGFILE ":\n"
				"saveLastChannel" ", "
				"lastChannelMode" ", "
				"lastChannelRadio" ", "
				"lastChannelTV" ", "
				"lastSatellitePosition" ", "
				"writeChannelsNames" ", "
				"makeRemainingChannelsBouquet" ", "
			        "diseqcRepeats" ", "
				"diseqcType" ", "
				"motorRotationSpeed" ", "
				"traceNukes" ", "
				"ChannelNamesFromBouquet" ", "
				"saveAudioPIDs" ", "
				"lnb0_OffsetLow" ", ..., " "lnb63_OffsetLow" ", "
				"lnb0_OffsetHigh" ", ..., " "lnb63_OffsetHigh" ", "
				"uncommitted_switch_mode (0..2)" "."
				"\n", argv[0]);
			return EXIT_FAILURE;
		}
	}

#ifdef HAVE_TRIPLEDRAGON
	Stb04GFXOsdControl gfxctrl;
	int gfxfd = open("/dev/stb/tdgfx", O_RDONLY);
	if (ioctl(gfxfd, STB04GFX_OSD_ONOFF, 1) < 0)
		WARN("STB04GFX_OSD_ONOFF failed: %m");
	if (ioctl(gfxfd, STB04GFX_OSD_GETCONTROL, &gfxctrl) < 0)
		WARN("STB04GFX_OSD_GETCONTROL failed: %m");
	WARN("x: %d y: %d w: %d h: %d off: %d depth: %d ga: %d use_ga: %d ff: %d 169ad: %d sqp: %d gc: %d undef: %d",
		gfxctrl.x,
		gfxctrl.y,
		gfxctrl.width,
		gfxctrl.height,
		gfxctrl.offset,
		gfxctrl.depth,
		(int)gfxctrl.global_alpha,
		gfxctrl.use_global_alpha,
		gfxctrl.enable_flicker_filter,
		gfxctrl.enable_16_9_adjust,
		gfxctrl.enable_square_pixel_filter,
		gfxctrl.enable_gamma_correction,
		gfxctrl.undefined_Colors_Transparent);
#if 0
//defaults after boot:
//[zapit.cpp:main:2991] x: 0 y: 0 w: 720 h: 576 off: 0 depth: 32 ga: 176 use_ga: 1 ff: 1 169ad: 0 sqp: 0 gc: 0 undef: 0
	gfxctrl.use_global_alpha = 1;
	gfxctrl.global_alpha=176; //default
	gfxctrl.enable_gamma_correction = 1;
#endif
	/* this is needed, otherwise the picture will be shaded
	   unfortunately, this is all pretty much undocumented... */
	// TODO: should this be done in the CVideo class?
	gfxctrl.undefined_Colors_Transparent = 1;
	if (ioctl(gfxfd, STB04GFX_OSD_SETCONTROL, &gfxctrl) < 0)
		WARN("STB04GFX_OSD_SETCONTROL failed: %m");
#endif

	scan_runs = 0;
	found_channels = 0;
	curr_sat = -1;

	/* load configuration or set defaults if no configuration file exists */
	if (!config.loadConfig(CONFIGFILE))
		WARN("%s not found", CONFIGFILE);

	/* create bouquet manager */
	bouquetManager = new CBouquetManager();

	saveLastChannel = config.getBool("saveLastChannel", true);
	lastChannelRadio = config.getInt32("lastChannelRadio", 0);
	lastChannelTV    = config.getInt32("lastChannelTV", 0);
	startChannelRadio = config.getInt32("startChannelRadio", 0);
	startChannelTV    = config.getInt32("startChannelTV", 0);
	bouquetManager->remainingChannelsBouquet = config.getBool("makeRemainingChannelsBouquet", true);

	if (config.getInt32("lastChannelMode", 0))
		setRadioMode();
	else
		setTVMode();

	if (!frontend)
		frontend = new CFrontend(config.getInt32("uncommitted_switch_mode", 0), (int)auto_fec + finetune);

	diseqcType = (diseqc_t)config.getInt32("diseqcType", NO_DISEQC);
	if (prepare_channels(frontend->getInfo()->type, diseqcType) < 0)
		WARN("error parsing services");
	else
		INFO("channels have been loaded succesfully");

	signal(SIGHUP, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGUSR1, signal_handler);

	CBasicServer zapit_server;

	if (!debug)
		switch (fork()) {
		case -1: /* can't fork */
			ERROR("fork");
			return -1;
		case 0: /* child, process becomes a daemon */
			if (setsid() == -1) {
				ERROR("setsid");
				return -1;
			}
			break;
		default: /* parent returns to calling process */
			return 0;
		}

	if (!zapit_server.prepare(ZAPIT_UDS_NAME))
		return -1;

	save_audioPIDs = config.getBool("saveAudioPIDs", false);
	if (save_audioPIDs) {
		FILE *audio_config_file = fopen(AUDIO_CONFIG_FILE, "r");
		if (audio_config_file) {
			t_channel_id chan;
			unsigned short apid;
			while (! feof(audio_config_file)) {
				fread(&chan, sizeof(t_channel_id), 1, audio_config_file);
				fread(&apid, sizeof(unsigned short), 1, audio_config_file);
				DBG("**** Old channelinfo: %d %d\n", (int) chan, (int) apid);
				audio_map[chan] = apid;
			}
			fclose(audio_config_file);
		}
	}

	// create eventServer
	eventServer = new CEventServer;

	controld_main();

	leaveStandby();

	/* this was done by controld before, but now needs to come after leaveStandby() */
#if defined HAVE_DBOX_HARDWARE || defined HAVE_TRIPLEDRAGON
	/* make sure that both volume settings are initialized */
	audioDecoder->setVolume(settings.volume, (int)CControld::TYPE_OST);
	if (settings.volume_type == CControld::TYPE_AVS)
		settings.volume = settings.volume_avs;
#endif
	audioDecoder->setVolume(settings.volume);

	if (settings.mute)
		audioDecoder->mute();
	else
		audioDecoder->unmute();
	videoDecoder->setVideoFormat(settings.videoformat);

	//zap to lastchannel with zapit TEST
#if 0
	CZapitClient::responseGetLastChannel lastchannel;
	lastchannel=load_settings();
	zapTo(lastchannel.channelNumber);
#endif
#if HAVE_DVB_API_VERSION < 3
	setFastZap(fastzap);
#endif

	time_t lastlockcheck = 0;

	if (update_pmt) {
		while (zapit_server.run(parse_command, CZapitMessages::ACTVERSION, true)) {
			struct pollfd pfd;
		 	if (check_lock && !standby && cc && time(NULL) > lastlockcheck &&
			    scan_runs == 0) {
				DBG("checking for lock...");
				if ((frontend->getStatus() & FE_HAS_LOCK) == 0) {
					printf("[zapit] LOCK LOST! trying rezap... channel: '%s'\n", cc->getName().c_str());
					tuned_transponder_id = TRANSPONDER_ID_NOT_TUNED;
					zapit(cc->getChannelID(), current_is_nvod, 0);
				}
				lastlockcheck = time(NULL);
			}
			if (pmt_update_fd != -1) {
				pfd.fd = pmt_update_fd;
				pfd.events = (POLLIN | POLLPRI);
				if (poll(&pfd, 1, 0) > 0) {
					if (pfd.fd == pmt_update_fd)
						zapit(cc->getChannelID(), current_is_nvod, 0);
				}
			}
			/* yuck, don't waste that much cpu time :) */
			usleep(0);
		}
	}
	else {
		zapit_server.run(parse_command, CZapitMessages::ACTVERSION);
	}

	controld_end();

	enterStandby();

	if (scanInputParser)
		xmlFreeDoc(scanInputParser);

	/* TODO: the destructor of bouquetManager should actually do that */
	bouquetManager->clearAll();
	delete bouquetManager;
	delete eventServer;

	INFO("shutdown complete");

	return 0;
}
