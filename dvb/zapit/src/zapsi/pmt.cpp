/*
 * $Id: pmt.cpp,v 1.49 2007/06/17 18:30:41 dbluelle Exp $
 *
 * (C) 2002 by Andreas Oberritter <obi@tuxbox.org>
 * (C) 2002 by Frank Bormann <happydude@berlios.de>
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
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

/* zapit */
#include <zapit/settings.h>
#include <zapit/descriptors.h>
#include <zapit/dmx.h>
#include <zapit/debug.h>
#include <zapit/pmt.h>

#define PMT_SIZE 1024

#ifndef SKIP_CA_STATUS
#include <eventserver.h>
#include <zapit/client/zapitclient.h>
extern CEventServer *eventServer;
#endif

/*
 * Stream types
 * ------------
 * 0x01 ISO/IEC 11172 Video
 * 0x02 ITU-T Rec. H.262 | ISO/IEC 13818-2 Video or ISO/IEC 11172-2 constrained parameter video stream
 * 0x03 ISO/IEC 11172 Audio
 * 0x04 ISO/IEC 13818-3 Audio
 * 0x05 ITU-T Rec. H.222.0 | ISO/IEC 13818-1 private_sections, e.g. MHP Application signalling stream
 * 0x06 ITU-T Rec. H.222.0 | ISO/IEC 13818-1 PES packets containing private data, e.g. teletext or ac3
 * 0x0b ISO/IEC 13818-6 type B
 * 0x81 User Private (MTV)
 * 0x90 User Private (Premiere Mail, BD_DVB)
 * 0xc0 User Private (Canal+)
 * 0xc1 User Private (Canal+)
 * 0xc6 User Private (Canal+)
 */

unsigned short parse_ES_info(const unsigned char * const buffer, CZapitChannel * const channel, CCaPmt * const caPmt)
{
	unsigned short ES_info_length;
	unsigned short pos;
	unsigned char descriptor_tag;
	unsigned char descriptor_length;
	unsigned char i;

	bool isAc3 = false;
	bool isDts = false;
	bool descramble = false;
	std::string description = "";
	unsigned char componentTag = 0xFF;

	/* elementary stream info for ca pmt */
	CEsInfo *esInfo = new CEsInfo();

	esInfo->stream_type = buffer[0];
	esInfo->reserved1 = buffer[1] >> 5;
	esInfo->elementary_PID = ((buffer[1] & 0x1F) << 8) | buffer[2];
	esInfo->reserved2 = buffer[3] >> 4;

	ES_info_length = ((buffer[3] & 0x0F) << 8) | buffer[4];

	for (pos = 5; pos < ES_info_length + 5; pos += descriptor_length + 2) {
		descriptor_tag = buffer[pos];
		descriptor_length = buffer[pos + 1];

		switch (descriptor_tag) {
			case 0x02:
				video_stream_descriptor(buffer + pos);
				break;

			case 0x03:
				audio_stream_descriptor(buffer + pos);
				break;

			case 0x05:
				if (descriptor_length >= 3)
					if (!strncmp((const char*)&buffer[pos + 2], "DTS", 3))
						isDts = true;
				break;

			case 0x09:
				esInfo->addCaDescriptor(buffer + pos);
#ifndef SKIP_CA_STATUS					
				eventServer->sendEvent(CZapitClient::EVT_ZAP_CA_LOCK, CEventServer::INITID_ZAPIT);
//				INFO("Event_ESINFO: CA_LOCK send");
#endif
				break;

			case 0x0A: /* ISO_639_language_descriptor */
				for (i = 0; i < 3; i++)
					description += buffer[pos + i + 2];
				break;

			case 0x13: /* Defined in ISO/IEC 13818-6 */
				break;

			case 0x0E:
				Maximum_bitrate_descriptor(buffer + pos);
				break;

			case 0x0F:
				Private_data_indicator_descriptor(buffer + pos);
				break;

			case 0x11:
				STD_descriptor(buffer + pos);
				break;

			case 0x45:
				VBI_data_descriptor(buffer + pos);
				break;

			case 0x52: /* stream_identifier_descriptor */
				componentTag = buffer[pos + 2];
				break;

			case 0x56: /* teletext descriptor */
			    unsigned char fieldCount=descriptor_length/5;
			    for (unsigned char fIdx=0;fIdx<fieldCount;fIdx++){
				char tmpLang[4];
				memcpy(tmpLang, &buffer[pos + 5*fIdx + 2], 3);
				tmpLang[3] = '\0';
				unsigned char teletext_type=buffer[pos + 5*fIdx + 5]>> 3;
				unsigned char teletext_magazine_number = buffer[pos + 5*fIdx + 5] & 7;
				unsigned char teletext_page_number=buffer[pos + 5*fIdx + 6];
				if (teletext_type==0x02){
				    // teletex subtitle page
				    channel->addTTXSubtitle(esInfo->elementary_PID,tmpLang,teletext_magazine_number,teletext_page_number);
				} else {
				    if (teletext_type==0x05){
					// teletex subtitle page for hearing impaired
					channel->addTTXSubtitle(esInfo->elementary_PID,tmpLang,teletext_magazine_number,teletext_page_number,true);
				    }
				}
			    } 
			    channel->setTeletextPid(esInfo->elementary_PID);
			    break;

			case 0x59:
			    if (esInfo->stream_type==0x06){
				// private data
				unsigned char fieldCount=descriptor_length/8;
				for (unsigned char fIdx=0;fIdx<fieldCount;fIdx++){
                                    char tmpLang[4];
                                    memcpy(tmpLang,&buffer[pos + 8*fIdx + 2],3);
                                    tmpLang[3] = '\0'; 
				    unsigned char subtitling_type=buffer[pos+8*fIdx+5];
				    unsigned short composition_page_id=
                                        *((unsigned short*)(&buffer[pos + 8*fIdx + 6]));
				    unsigned short ancillary_page_id=
                                        *((unsigned short*)(&buffer[pos + 8*fIdx + 8]));
				    channel->addDVBSubtitle(esInfo->elementary_PID,tmpLang,subtitling_type,composition_page_id,ancillary_page_id);
                                }
			    }
			    subtitling_descriptor(buffer + pos);
			    break;

			case 0x5F:
				private_data_specifier_descriptor(buffer + pos);
				break;

			case 0x66:
				data_broadcast_id_descriptor(buffer + pos);
				break;

			case 0x6A: /* AC3 descriptor */
				isAc3 = true;
				break;

			case 0x6F: /* unknown, Astra 19.2E */
				break;

			case 0x90: /* unknown, Astra 19.2E */
				break;

			case 0xB1: /* unknown, Astra 19.2E */
				break;

			case 0xC0: /* unknown, Astra 19.2E */
				break;

			case 0xC1: /* unknown, Astra 19.2E */
				break;

			case 0xC2: /* User Private descriptor - Canal+ */
#if 0
				DBG("0xC2 dump:");
				for (i = 0; i < descriptor_length; i++) {
					printf("%c", buffer[pos + 2 + i]);
					if (((i+1) % 8) == 0)
						printf("\n");
				}
#endif
				break;

			case 0xC5: /* User Private descriptor - Canal+ Radio */
				for (i = 0; i < 24; i++)
					description += buffer[pos + i + 3];
				break;

			case 0xC6: /* unknown, Astra 19.2E */
				break;

			case 0xFD: /* unknown, Astra 19.2E */
				break;

			case 0xFE: /* unknown, Astra 19.2E */
				break;

			default:
				DBG("descriptor_tag: %02x", descriptor_tag);
				break;
		}
	}

	switch (esInfo->stream_type) {
	case 0x01:
	case 0x02:
		channel->setVideoPid(esInfo->elementary_PID);
		descramble = true;
		break;

	case 0x03:
	case 0x04:
		if (description.empty())
			description = esInfo->elementary_PID;
		channel->addAudioChannel(esInfo->elementary_PID, false, description, componentTag);
		descramble = true;
		break;

	case 0x05:// private section
	{
		int tmp=0;	
		// Houdini: shameless stolen from enigma dvbservices.cpp
		for (pos = 5; pos < ES_info_length + 5; pos += descriptor_length + 2) {
			descriptor_tag = buffer[pos];
			descriptor_length = buffer[pos + 1];
			
			switch (descriptor_tag) {
				case 0x5F: //DESCR_PRIV_DATA_SPEC:
					if ( ((buffer[pos + 2]<<24) | (buffer[pos + 3]<<16) | (buffer[pos + 4]<<8) | (buffer[pos + 5])) == 190 )
						tmp |= 1;
					break;
				case 0x90:
				{
					if ( descriptor_length == 4 && !buffer[pos + 2] && !buffer[pos + 3] && buffer[pos + 4] == 0xFF && buffer[pos + 5] == 0xFF )
						tmp |= 2;
				}
				//break;??
				default:
					break;
			}
		}
		if ( tmp == 3 ) {
			channel->setPrivatePid(esInfo->elementary_PID);
			DBG("channel->setPrivatePid(%x)\n", esInfo->elementary_PID);
		}
		break;
	}

	case 0x06:
		if ((isAc3) || (isDts))
		{
			if (description.empty())
			{
				description = esInfo->elementary_PID;
				if (isAc3)
					description += " (AC3)";
				else if (isDts)
					description += " (DTS)";
			}
			channel->addAudioChannel(esInfo->elementary_PID, true, description, componentTag);
			descramble = true;
		}
		break;

	case 0x0B:
		break;

	case 0x90:
		break;

	case 0x93:
		break;

	case 0xC0:
		break;

	case 0xC1:
		break;

	case 0xC6:
		break;

	default:
		DBG("stream_type: %02x", esInfo->stream_type);
		break;
	}

	if (descramble)
		caPmt->es_info.insert(caPmt->es_info.end(), esInfo);
	else
		delete esInfo;

	return ES_info_length;
}

int parse_pmt(CZapitChannel * const channel)
{
	CDemux dmx;

	unsigned char buffer[PMT_SIZE];

	/* current position in buffer */
	unsigned short i;

	/* length of elementary stream description */
	unsigned short ES_info_length;

	/* TS_program_map_section elements */
	unsigned short section_length;
	unsigned short program_info_length;

	unsigned char filter[DMX_FILTER_SIZE];
	unsigned char mask[DMX_FILTER_SIZE];

	if (channel->getPmtPid() == 0)
		return -1;

	memset(filter, 0x00, DMX_FILTER_SIZE);
	memset(mask, 0x00, DMX_FILTER_SIZE);

	filter[0] = 0x02;	/* table_id */
	filter[1] = channel->getServiceId() >> 8;
	filter[2] = channel->getServiceId();
	filter[3] = 0x01;	/* current_next_indicator */
	filter[4] = 0x00;	/* section_number */
	mask[0] = 0xFF;
	mask[1] = 0xFF;
	mask[2] = 0xFF;
	mask[3] = 0x01;
	mask[4] = 0xFF;

	if ((dmx.sectionFilter(channel->getPmtPid(), filter, mask) < 0) || (dmx.read(buffer, PMT_SIZE) < 0))
		return -1;

	CCaPmt *caPmt = new CCaPmt();

	/* ca pmt */
	caPmt->program_number = (buffer[3] << 8) + buffer[4];
	caPmt->reserved1 = buffer[5] >> 6;
	caPmt->version_number = (buffer[5] >> 1) & 0x1F;
	caPmt->current_next_indicator = buffer[5] & 0x01;
	caPmt->reserved2 = buffer[10] >> 4;

	/* pmt */
	section_length = ((buffer[1] & 0x0F) << 8) + buffer[2];
	channel->setPcrPid(((buffer[8] & 0x1F) << 8) + buffer[9]);
	program_info_length = ((buffer[10] & 0x0F) << 8) | buffer[11];

	if (program_info_length)
		for (i = 12; i < 12 + program_info_length; i += buffer[i + 1] + 2)
			switch (buffer[i]) {
			case 0x09:
				caPmt->addCaDescriptor(buffer + i);
#ifndef SKIP_CA_STATUS
				eventServer->sendEvent(CZapitClient::EVT_ZAP_CA_LOCK, CEventServer::INITID_ZAPIT);
//				INFO("Event_PMT: CA_LOCK send");
#endif
				break;
			default:
				DBG("decriptor_tag: %02x", buffer[i]);
				break;
			}

	//Quick&Dirty Hack to support Premiere's EPG not only on the portal but on the subchannels as well 
	if (channel->getOriginalNetworkId() == 0x0085) {
		if (channel->getTransportStreamId() ==0x0003)
			channel->setPrivatePid(0x0b12);
		if (channel->getTransportStreamId() ==0x0004)
			channel->setPrivatePid(0x0b11);
	}

	/* pmt */
	for (i = 12 + program_info_length; i < section_length - 1; i += ES_info_length + 5)
		ES_info_length = parse_ES_info(buffer + i, channel, caPmt);

	channel->setCaPmt(caPmt);
	channel->setPidsFlag();
	if (channel->getPidsUpdated()) {
		channel->unsetPidsUpdated();
		DBG("PIDs updated, notify clients");
		// ARZKA: TODO Notify to zapitsocket
	}
            
	return 0;
}

int pmt_set_update_filter(CZapitChannel * const channel, int *fd)
{
	struct dmx_sct_filter_params dsfp;

	if (channel->getPmtPid() == 0)
		return -1;

	if ((*fd == -1) && ((*fd = open(DEMUX_DEVICE, O_RDWR)) < 0)) {
		perror(DEMUX_DEVICE);
		return -1;
	}

	bzero(&dsfp, sizeof(struct dmx_sct_filter_params));
	dsfp.filter.filter[0] = 0x02;	/* table_id */
	dsfp.filter.filter[1] = channel->getServiceId() >> 8;
	dsfp.filter.filter[2] = channel->getServiceId();
	dsfp.filter.filter[3] = (channel->getCaPmt()->version_number << 1) | 0x01;
	dsfp.filter.filter[4] = 0x00;	/* section_number */
	dsfp.filter.mask[0] = 0xFF;
	dsfp.filter.mask[1] = 0xFF;
	dsfp.filter.mask[2] = 0xFF;
	dsfp.filter.mask[3] = (0x1F << 1) | 0x01;
	dsfp.filter.mask[4] = 0xFF;
#if HAVE_DVB_API_VERSION >= 3
	dsfp.filter.mode[3] = 0x1F << 1;
#endif
	dsfp.flags = DMX_CHECK_CRC | DMX_IMMEDIATE_START;
	dsfp.pid = channel->getPmtPid();
	dsfp.timeout = 0;

	if (ioctl(*fd, DMX_SET_FILTER, &dsfp) < 0) {
		perror("DMX_SET_FILTER");
		close(*fd);
		return -1;
	}

	return 0;
}

int pmt_stop_update_filter(int *fd)
{
	if ((*fd == -1)) {
		fprintf(stderr,"check your code - nothing to stop\n");
		return 0;
	}
	if (ioctl(*fd, DMX_STOP) < 0) {
		perror("DMX_STOP_FILTER");
	}
	close(*fd);
	return 0;
}

