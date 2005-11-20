//
// $Id: SIsections.cpp,v 1.41 2005/11/20 15:11:40 mogway Exp $
//
// classes for SI sections (dbox-II-project)
//
//    Homepage: http://dbox2.elxsi.de
//
//    Copyright (C) 2001 fnbrd (fnbrd@gmx.de)
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
//

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/poll.h> // fuer poll()

#include <set>
#include <algorithm>
#include <string>

#include "SIutils.hpp"
#include "SIservices.hpp"
#include "SIevents.hpp"
#include "SIbouquets.hpp"
#include "SInetworks.hpp"
#include "SIsections.hpp"
#include <dmxapi.h>
#include <zapit/dvbstring.h>

//#define DEBUG

struct descr_generic_header {
	unsigned descriptor_tag			: 8;
	unsigned descriptor_length		: 8;
} __attribute__ ((packed)) ;

struct descr_short_event_header {
	unsigned descriptor_tag			: 8;
	unsigned descriptor_length		: 8;
	unsigned language_code_hi		: 8;
	unsigned language_code_mid		: 8;
	unsigned language_code_lo		: 8;
	unsigned event_name_length		: 8;
} __attribute__ ((packed)) ;

struct descr_service_header {
	unsigned descriptor_tag			: 8;
	unsigned descriptor_length		: 8;
	unsigned service_typ			: 8;
	unsigned service_provider_name_length	: 8;
} __attribute__ ((packed)) ;

struct descr_extended_event_header {
	unsigned descriptor_tag			: 8;
	unsigned descriptor_length		: 8;
	unsigned descriptor_number		: 4;
	unsigned last_descriptor_number		: 4;
	unsigned iso_639_2_language_code_hi	: 8;
	unsigned iso_639_2_language_code_mid	: 8;
	unsigned iso_639_2_language_code_lo	: 8;
	unsigned length_of_items		: 8;
} __attribute__ ((packed)) ;

struct service_list_entry {
	unsigned service_id_hi			: 8;
	unsigned service_id_lo			: 8;
	unsigned service_type			: 8;
} __attribute__ ((packed)) ;

inline unsigned min(unsigned a, unsigned b)
{
	return b < a ? b : a;
}


//-----------------------------------------------------------------------
// Da es vorkommen kann das defekte Packete empfangen werden
// sollte hier alles ueberprueft werden.
// Leider ist das noch nicht bei allen Descriptoren so.
//-----------------------------------------------------------------------
void SIsectionEIT::parseLinkageDescriptor(const char *buf, SIevent &e, unsigned maxlen)
{
  if(maxlen>=sizeof(struct descr_linkage_header))
  {
    SIlinkage l((const struct descr_linkage_header *)buf);
    e.linkage_descs.insert(e.linkage_descs.end(), l);
//    printf("LinkName: %s\n", l.name.c_str());
  }
}

void SIsectionEIT::parseComponentDescriptor(const char *buf, SIevent &e, unsigned maxlen)
{
  if(maxlen>=sizeof(struct descr_component_header))
    e.components.insert(SIcomponent((const struct descr_component_header *)buf));
}

void SIsectionEIT::parseContentDescriptor(const char *buf, SIevent &e, unsigned maxlen)
{
  struct descr_generic_header *cont=(struct descr_generic_header *)buf;
  if(cont->descriptor_length+sizeof(struct descr_generic_header)>maxlen)
    return; // defekt
  const char *classification=buf+sizeof(struct descr_generic_header);
  while(classification<=buf+sizeof(struct descr_generic_header)+cont->descriptor_length-2) {
    e.contentClassification+=std::string(classification, 1);
//    printf("Content: 0x%02hhx\n", *classification);
    e.userClassification+=std::string(classification+1, 1);
//    printf("User: 0x%02hhx\n", *(classification+1));
    classification+=2;
  }
}

void SIsectionEIT::parseParentalRatingDescriptor(const char *buf, SIevent &e, unsigned maxlen)
{
  struct descr_generic_header *cont=(struct descr_generic_header *)buf;
  if(cont->descriptor_length+sizeof(struct descr_generic_header)>maxlen)
    return; // defekt
  const char *s=buf+sizeof(struct descr_generic_header);
  while(s<buf+sizeof(struct descr_generic_header)+cont->descriptor_length-4) {
    e.ratings.insert(SIparentalRating(std::string(s, 3), *(s+3)));
    s+=4;
  }
}

void SIsectionEIT::parseExtendedEventDescriptor(const char *buf, SIevent &e, unsigned maxlen)
{
  struct descr_extended_event_header *evt=(struct descr_extended_event_header *)buf;
  if((evt->descriptor_length+sizeof(descr_generic_header)>maxlen) || (evt->descriptor_length<sizeof(struct descr_extended_event_header)-sizeof(descr_generic_header)))
    return; // defekt
  unsigned char *items=(unsigned char *)(buf+sizeof(struct descr_extended_event_header));
  while(items<(unsigned char *)(buf+sizeof(struct descr_extended_event_header)+evt->length_of_items)) {
    if(*items) {
      if(*(items+1) < 0x06) { // other code table
		  // 21.07.2005 - collect all extended events in one
		  // string, delimit multiple entries with a newline
		  e.itemDescription.append(std::string((const char *)(items+2), min(maxlen-((const char *)items+2-buf), (*items)-1)));
		  e.itemDescription.append("\n");
	  }
      else {
		  // 21.07.2005 - collect all extended events in one
		  // string, delimit multiple entries with a newline
		  e.itemDescription.append(std::string((const char *)(items+1), min(maxlen-((const char *)items+1-buf), *items)));
		  e.itemDescription.append("\n");
	  }
    }
    items+=1+*items;
    if(*items) {
		// 21.07.2005 - collect all extended events in one
		// string, delimit multiple entries with a newline
        e.item.append(std::string((const char *)(items+1), min(maxlen-((const char *)items+1-buf), *items)));
        e.item.append("\n");
    }
    items+=1+*items;
  }
//  if (0 != e.itemDescription.length()) {
//	printf("Item Description: %s\n", e.itemDescription.c_str());
//	printf("Item: %s\n", e.item.c_str());
//  }
  if(*items) {
    if(*(items+1) < 0x06) // other code table
      e.extendedText+=std::string((const char *)(items+2), min(maxlen-((const char *)items+2-buf), (*items)-1));
    else
      e.extendedText+=std::string((const char *)(items+1), min(maxlen-((const char *)items+1-buf), *items));
//    printf("Extended Text: %s\n", e.extendedText.c_str());
  }
}

void SIsectionEIT::parseShortEventDescriptor(const char *buf, SIevent &e, unsigned maxlen)
{
  struct descr_short_event_header *evt=(struct descr_short_event_header *)buf;
  if((evt->descriptor_length+sizeof(descr_generic_header)>maxlen) || (evt->descriptor_length<sizeof(struct descr_short_event_header)-sizeof(descr_generic_header)))
    return; // defekt
  buf+=sizeof(struct descr_short_event_header);
  if(evt->event_name_length) {
    if(*buf < 0x06) // other code table
      e.name=std::string(buf+1, evt->event_name_length-1);
    else
      e.name=std::string(buf, evt->event_name_length);
  }
  buf+=evt->event_name_length;
  unsigned char textlength=*((unsigned char *)buf);
  if(textlength > 2) {
    if(*(buf+1) < 0x06) // other code table
      e.text=std::string((++buf)+1, textlength-1);
    else
      e.text=std::string(++buf, textlength);
  }

//  printf("Name: %s\n", e.name.c_str());
//  printf("Text: %s\n", e.text.c_str());

}

void SIsectionEIT::parseDescriptors(const char *des, unsigned len, SIevent &e)
{
  struct descr_generic_header *desc;
  while(len>=sizeof(struct descr_generic_header)) {
    desc=(struct descr_generic_header *)des;
//    printf("Type: %s\n", decode_descr(desc->descriptor_tag));
    if(desc->descriptor_tag==0x4D)
      parseShortEventDescriptor((const char *)desc, e, len);
    else if(desc->descriptor_tag==0x4E)
      parseExtendedEventDescriptor((const char *)desc, e, len);
    else if(desc->descriptor_tag==0x54)
      parseContentDescriptor((const char *)desc, e, len);
    else if(desc->descriptor_tag==0x50)
      parseComponentDescriptor((const char *)desc, e, len);
    else if(desc->descriptor_tag==0x55)
      parseParentalRatingDescriptor((const char *)desc, e, len);
    else if(desc->descriptor_tag==0x4A)
      parseLinkageDescriptor((const char *)desc, e, len);
    if((unsigned)(desc->descriptor_length+2)>len)
      break;
    len-=desc->descriptor_length+2;
    des+=desc->descriptor_length+2;
  }
}

// Die infos aus dem Puffer holen
void SIsectionEIT::parse(void)
{
	const char *actPos;
	struct eit_event *evt;
	unsigned short descriptors_loop_length;

	if (!buffer || parsed)
		return;

	if (bufferLength < sizeof(SI_section_EIT_header) + sizeof(struct eit_event)) {
		delete [] buffer;
		buffer=0;
		bufferLength=0;
		return;
	}

	actPos = &buffer[sizeof(SI_section_EIT_header)];

	while (actPos < &buffer[bufferLength - sizeof(struct eit_event)]) {
		evt = (struct eit_event *) actPos;
		SIevent e(evt);
		e.service_id = service_id();
		e.original_network_id = original_network_id();
		e.transport_stream_id = transport_stream_id();
		descriptors_loop_length = (evt->descriptors_loop_length_hi << 8) | evt->descriptors_loop_length_lo;
		parseDescriptors(((const char *)evt) + sizeof(struct eit_event), min((unsigned)(buffer + bufferLength - actPos), descriptors_loop_length), e);
		evts.insert(e);
		actPos += sizeof(struct eit_event) + descriptors_loop_length;
	}

	parsed = 1;
}


//-----------------------------------------------------------------------
// Da es vorkommen kann das defekte Packete empfangen werden
// sollte hier alles ueberprueft werden.
// Leider ist das noch nicht bei allen Descriptoren so.
//-----------------------------------------------------------------------
void SIsectionPPT::parseLinkageDescriptor(const char *buf, SIevent &e, unsigned maxlen)
{
  if(maxlen>=sizeof(struct descr_linkage_header))
  {
    SIlinkage l((const struct descr_linkage_header *)buf);
    e.linkage_descs.insert(e.linkage_descs.end(), l);
//    printf("LinkName: %s\n", l.name.c_str());
  }
}

void SIsectionPPT::parseComponentDescriptor(const char *buf, SIevent &e, unsigned maxlen)
{
  if(maxlen>=sizeof(struct descr_component_header))
    e.components.insert(SIcomponent((const struct descr_component_header *)buf));
}

void SIsectionPPT::parseContentDescriptor(const char *buf, SIevent &e, unsigned maxlen)
{
  struct descr_generic_header *cont=(struct descr_generic_header *)buf;
  if(cont->descriptor_length+sizeof(struct descr_generic_header)>maxlen)
    return; // defekt
  const char *classification=buf+sizeof(struct descr_generic_header);
  while(classification<=buf+sizeof(struct descr_generic_header)+cont->descriptor_length-2) {
    e.contentClassification+=std::string(classification, 1);
//    printf("Content: 0x%02hhx\n", *classification);
    e.userClassification+=std::string(classification+1, 1);
//    printf("User: 0x%02hhx\n", *(classification+1));
    classification+=2;
  }
}

void SIsectionPPT::parseParentalRatingDescriptor(const char *buf, SIevent &e, unsigned maxlen)
{
  struct descr_generic_header *cont=(struct descr_generic_header *)buf;
  if(cont->descriptor_length+sizeof(struct descr_generic_header)>maxlen)
    return; // defekt
  const char *s=buf+sizeof(struct descr_generic_header);
  while(s<buf+sizeof(struct descr_generic_header)+cont->descriptor_length-4) {
    e.ratings.insert(SIparentalRating(std::string(s, 3), *(s+3)));
    s+=4;
  }
}

void SIsectionPPT::parseExtendedEventDescriptor(const char *buf, SIevent &e, unsigned maxlen)
{
  struct descr_extended_event_header *evt=(struct descr_extended_event_header *)buf;
  if((evt->descriptor_length+sizeof(descr_generic_header)>maxlen) || (evt->descriptor_length<sizeof(struct descr_extended_event_header)-sizeof(descr_generic_header)))
    return; // defekt
  unsigned char *items=(unsigned char *)(buf+sizeof(struct descr_extended_event_header));
  while(items<(unsigned char *)(buf+sizeof(struct descr_extended_event_header)+evt->length_of_items)) {
    if(*items) {
      if(*(items+1) < 0x06) // other code table
        e.itemDescription=std::string((const char *)(items+2), min(maxlen-((const char *)items+2-buf), (*items)-1));
      else
        e.itemDescription=std::string((const char *)(items+1), min(maxlen-((const char *)items+1-buf), *items));
//      printf("Item Description: %s\n", e.itemDescription.c_str());
    }
    items+=1+*items;
    if(*items) {
      e.item=std::string((const char *)(items+1), min(maxlen-((const char *)items+1-buf), *items));
//    printf("Item: %s\n", e.item.c_str());
    }
    items+=1+*items;
  }
  if(*items) {
    if(*(items+1) < 0x06) // other code table
      e.extendedText+=std::string((const char *)(items+2), min(maxlen-((const char *)items+2-buf), (*items)-1));
    else
      e.extendedText+=std::string((const char *)(items+1), min(maxlen-((const char *)items+1-buf), *items));
//    printf("Extended Text: %s\n", e.extendedText.c_str());
  }
}

void SIsectionPPT::parseShortEventDescriptor(const char *buf, SIevent &e, unsigned maxlen)
{
  struct descr_short_event_header *evt=(struct descr_short_event_header *)buf;
  if((evt->descriptor_length+sizeof(descr_generic_header)>maxlen) || (evt->descriptor_length<sizeof(struct descr_short_event_header)-sizeof(descr_generic_header)))
    return; // defekt
  buf+=sizeof(struct descr_short_event_header);
  if(evt->event_name_length) {
    if(*buf < 0x06) // other code table
      e.name=std::string(buf+1, evt->event_name_length-1);
    else
      e.name=std::string(buf, evt->event_name_length);
  }
  
  buf+=evt->event_name_length;
  unsigned char textlength=*((unsigned char *)buf);
  if(textlength > 2) {
    if(*(buf+1) < 0x06) // other code table
      e.text=std::string((++buf)+1, textlength-1);
    else
      e.text=std::string(++buf, textlength);
  }

//  printf("Name: %s\n", e.name.c_str());
//  printf("Text: %s\n", e.text.c_str());

}

void SIsectionPPT::parsePrivateContentOrderDescriptor(const char *buf, SIevent &e, unsigned maxlen)
{
  struct descr_short_event_header *evt=(struct descr_short_event_header *)buf;
  if((evt->descriptor_length+sizeof(descr_generic_header)>maxlen) || (evt->descriptor_length<sizeof(struct descr_short_event_header)-sizeof(descr_generic_header)))
    return; // defekt
    
#if 0
// to be done    
    unsigned char Order_number_length;
    char Order_number[Order_number_length];
    unsigned char Order_price_length;
    char Order_price[Order_price_length];
    unsigned char Order_phone_number_length;
    char Order_phone_number[Order_phone_number_length];
    unsigned char SMS_order_information_length;
    char SMS_order_information[SMS_order_information_length];
    unsigned char URL_order_information_length;
    char URL_order_information[URL_order_information_length];
#endif    
}

void SIsectionPPT::parsePrivateParentalInformationDescriptor(const char *buf, SIevent &e, unsigned maxlen)
{
  struct descr_short_event_header *evt=(struct descr_short_event_header *)buf;
  if((evt->descriptor_length+sizeof(descr_generic_header)>maxlen) || (evt->descriptor_length<sizeof(struct descr_short_event_header)-sizeof(descr_generic_header)))
    return; // defekt

  buf+=sizeof(struct descr_generic_header);
    
  if (sizeof(struct descr_generic_header)+1 < evt->descriptor_length) {
    e.ratings.insert(SIparentalRating(std::string("", 0), *(buf+1)));
  }
#if 0    
    unsigned char rating;
    unsigned char Controll_time_t1[3]; // BCD coded
    unsigned char Controll_time_t2[3]; // BCD coded
    unsigned char Parental_information_length;
    unsigned char Parental_information[Parental_information_length];
#endif    
}
void SIsectionPPT::parsePrivateContentTransmissionDescriptor(const char *buf, SIevent &e, unsigned maxlen)
{
  unsigned short starttime_loop_length = 0;
  unsigned char tm_buf[6];
  int i;  
  
  struct descr_short_event_header *evt=(struct descr_short_event_header *)buf;
  if((evt->descriptor_length+sizeof(descr_generic_header)>maxlen) || (evt->descriptor_length<sizeof(struct descr_short_event_header)-sizeof(descr_generic_header)))
    return; // defekt

//printf("parsePrivateContentTransmissionDescriptor\n");
  const char *p=buf+sizeof(struct descr_generic_header);
  if (sizeof(struct descr_generic_header)+1 <= maxlen) e.transport_stream_id = ((*p)<<8) | (*(p+1));
  if (sizeof(struct descr_generic_header)+3 <= maxlen) e.original_network_id = ((*(p+2))<<8) | (*(p+3));
  if (sizeof(struct descr_generic_header)+5 <= maxlen) e.service_id = ((*(p+4))<<8) | (*(p+5));
  
  p += 6;
  while(p+6 <= buf + evt->descriptor_length + sizeof(struct descr_generic_header)) {// at least one startdate/looplength/time entry
	tm_buf[0] = *(p);
	tm_buf[1] = *(p+1);
	starttime_loop_length = (*(p+2))/3;
	for (i=0;i<starttime_loop_length; i++) {
  		tm_buf[2] = *(p+3*i+3);
  		tm_buf[3] = *(p+3*i+4);
  		tm_buf[4] = *(p+3*i+5);
		e.times.insert(SItime(changeUTCtoCtime(tm_buf), duration()));
	}
	p+=3 + 3*starttime_loop_length; // goto next starttime entry
  }
  
  // fake linkage !?
  SIlinkage l;
  l.linkageType = 0; // no linkage descriptor
  l.transportStreamId = e.transport_stream_id;
  l.originalNetworkId = e.original_network_id;
  l.serviceId = e.service_id;
  e.linkage_descs.insert(e.linkage_descs.end(), l);
}

void SIsectionPPT::parseDescriptors(const char *des, unsigned len, SIevent &e)
{
  struct descr_generic_header *desc;
  bool linkage_alreadyseen = false;
  
  while(len>=sizeof(struct descr_generic_header)) {
    desc=(struct descr_generic_header *)des;
 
//    printf("Type: %s\n", decode_descr(desc->descriptor_tag));
    if(desc->descriptor_tag==0x4D)
      parseShortEventDescriptor((const char *)desc, e, len);
    else if(desc->descriptor_tag==0x4E)
      parseExtendedEventDescriptor((const char *)desc, e, len);
    else if(desc->descriptor_tag==0x54)
      parseContentDescriptor((const char *)desc, e, len);
    else if(desc->descriptor_tag==0x50)
      parseComponentDescriptor((const char *)desc, e, len);
    else if(desc->descriptor_tag==0x55)
      parseParentalRatingDescriptor((const char *)desc, e, len);
    else if(desc->descriptor_tag==0x4A)
      parseLinkageDescriptor((const char *)desc, e, len);
    else if(desc->descriptor_tag==0xF0)
      parsePrivateContentOrderDescriptor((const char *)desc, e, len);
    else if(desc->descriptor_tag==0xF1)
      parsePrivateParentalInformationDescriptor((const char *)desc, e, len);
    else if(desc->descriptor_tag==0xF2) {
      if (linkage_alreadyseen) {
      	// Private EPG can have two linkage descriptors with their own date/time parameters for one event
	// not sure if current event system supports it therefore:
	// Generate additional Event(s) if there are more than one linkage descriptor (for repeated transmission)
	SIevent e2(e);
	e2.linkage_descs.clear();
	e2.times.clear();
	parsePrivateContentTransmissionDescriptor((const char *)desc, e2, len);
	evts.insert(e2);
      } else {
	parsePrivateContentTransmissionDescriptor((const char *)desc, e, len);
	linkage_alreadyseen = true;
      }
    }
    if((unsigned)(desc->descriptor_length+2)>len)
      break;
    len-=desc->descriptor_length+2;
    des+=desc->descriptor_length+2;
  }
}

// Die infos aus dem Puffer holen
void SIsectionPPT::parse(void)
{
	const char *actPos;
	unsigned short descriptors_loop_length;

	if (!buffer || parsed)
		return;

	if (bufferLength < sizeof(SI_section_PPT_header)) {
		delete [] buffer;
		buffer=0;
		bufferLength=0;
		return;
	}

	actPos = &buffer[sizeof(SI_section_PPT_header)];
	
	/*while (actPos < &buffer[bufferLength])*/ {
		SIevent e;
		descriptors_loop_length = (((SI_section_PPT_header*)buffer)->descriptor_section_length_hi << 8) | ((SI_section_PPT_header*)buffer)->descriptor_section_length_lo;
		e.eventID = (unsigned short)(content_id()); // ??
		parseDescriptors(actPos, min((unsigned)(buffer + bufferLength - actPos), descriptors_loop_length), e);
		evts.insert(e);
		actPos += descriptors_loop_length;
	}

	parsed = 1;
}
/********************/

void SIsectionSDT::parseNVODreferenceDescriptor(const char *buf, SIservice &s)
{
  struct descr_generic_header *hdr=(struct descr_generic_header *)buf;
  unsigned short *spointer=(unsigned short *)(buf+sizeof(struct descr_generic_header));
  while((const char *)spointer<=buf+sizeof(struct descr_generic_header)+hdr->descriptor_length-2) {
    unsigned short transportStreamID=*spointer++;
    unsigned short originalNetworkID=*spointer++;
    unsigned short sID=*spointer++;
    s.nvods.insert(SInvodReference(transportStreamID, originalNetworkID, sID));
  }
}

void SIsectionSDT::parseServiceDescriptor(const char *buf, SIservice &s)
{
  struct descr_service_header *sv=(struct descr_service_header *)buf;
  buf+=sizeof(struct descr_service_header);
  s.serviceTyp=sv->service_typ;
  if(sv->service_provider_name_length) {
    if(*buf < 0x06) // other code table
      s.providerName=std::string(buf+1, sv->service_provider_name_length-1);
    else
      s.providerName=std::string(buf, sv->service_provider_name_length);
  }
  buf+=sv->service_provider_name_length;
  unsigned char servicenamelength=*((unsigned char *)buf);
  if(servicenamelength) {
    //if(*(buf+1) < 0x06) // other code table
    	s.serviceName  = CDVBString((const char *)((++buf)), servicenamelength).getContent();
	//printf("%s\n", s.serviceName.c_str());
      //s.serviceName=std::string((++buf)+1, servicenamelength-1);
    //else
      //s.serviceName=std::string(++buf, servicenamelength);
  }
  //s.serviceName = Latin1_to_UTF8(s.serviceName.c_str());
//  printf("Provider-Name: %s\n", s.providerName.c_str());
//  printf("Service-Name: %s\n", s.serviceName.c_str());
}

void SIsectionSDT::parseDescriptors(const char *des, unsigned len, SIservice &s)
{
  struct descr_generic_header *desc;
  while(len>=sizeof(struct descr_generic_header)) {
    desc=(struct descr_generic_header *)des;
//    printf("Type: %s\n", decode_descr(desc->descriptor_tag));
//    printf("Length: %hhu\n", desc->descriptor_length);
    if(desc->descriptor_tag==0x48) {
//      printf("Found service descriptor\n");
      parseServiceDescriptor((const char *)desc, s);
    }
    else if(desc->descriptor_tag==0x4b) {
//      printf("Found NVOD reference descriptor\n");
      parseNVODreferenceDescriptor((const char *)desc, s);
    }
    len-=desc->descriptor_length+2;
    des+=desc->descriptor_length+2;
  }
}

// Die infos aus dem Puffer holen
void SIsectionSDT::parse(void)
{
	const char *actPos;
	struct sdt_service *sv;
	unsigned short descriptors_loop_length;

	if (!buffer || parsed)
		return;

	if (bufferLength < sizeof(SI_section_SDT_header) + sizeof(struct sdt_service)) {
		delete [] buffer;
		buffer=0;
		bufferLength=0;
		return;
	}

	actPos = &buffer[sizeof(SI_section_SDT_header)];

	while (actPos <= &buffer[bufferLength - sizeof(struct sdt_service)]) {
		sv = (struct sdt_service *)actPos;
		SIservice s(sv);
		s.original_network_id = original_network_id();
		s.transport_stream_id = transport_stream_id();
		descriptors_loop_length = (sv->descriptors_loop_length_hi << 8) | sv->descriptors_loop_length_lo;
		//printf("actpos: %p buf+bl: %p sid: %hu desclen: %hu\n", actPos, buffer+bufferLength, sv->service_id, sv->descriptors_loop_length);
		parseDescriptors(((const char *)sv) + sizeof(struct sdt_service), descriptors_loop_length, s);
		svs.insert(s);
		actPos += sizeof(struct sdt_service) + descriptors_loop_length;
	}

	parsed = 1;
}
/************************************/

//Within the Service List all Channels of a bouquet are specified
void SIsectionBAT::parseServiceListDescriptor(const char *buf, SIbouquet &s)
{
  struct descr_generic_header *sv=(struct descr_generic_header *)buf;
  buf+=sizeof(struct descr_generic_header);
  unsigned short len = sv->descriptor_length;
  while(len >= sizeof(struct service_list_entry)) {
  	struct service_list_entry *sl=(struct service_list_entry *)buf;
	buf+=sizeof(struct service_list_entry);
	SIbouquet bs(s);
	bs.service_id=(sl->service_id_hi << 8) | sl->service_id_lo;
	bs.serviceTyp=sl->service_type;
	bsv.insert(bs);
	len -= sizeof(struct service_list_entry);
  }
}

void SIsectionBAT::parseBouquetNameDescriptor(const char *buf, SIbouquet &s)
{
  struct descr_generic_header *sv=(struct descr_generic_header *)buf;
  buf+=sizeof(struct descr_generic_header);
  if(sv->descriptor_length) {
    if(*buf < 0x06) // other code table
      s.bouquetName=std::string(buf+1, sv->descriptor_length-1);
    else
      s.bouquetName=std::string(buf, sv->descriptor_length);
  }
  //printf("Bouquet-Name: %s\n", s.bouquetName.c_str());
}

void SIsectionBAT::parseDescriptors(const char *des, unsigned len, SIbouquet &s)
{
  struct descr_generic_header *desc;
  while(len>=sizeof(struct descr_generic_header)) {
    desc=(struct descr_generic_header *)des;
//    printf("Type: %s\n", decode_descr(desc->descriptor_tag));
//    printf("Length: %hhu\n", desc->descriptor_length);
    if(desc->descriptor_tag==0x41) {
//      printf("Found service list descriptor\n");
      parseServiceListDescriptor((const char *)desc, s);
    }
    else if(desc->descriptor_tag==0x47) {
//      printf("Found bouquet name descriptor\n");
      parseBouquetNameDescriptor((const char *)desc, s);
    }
    len-=desc->descriptor_length+2;
    des+=desc->descriptor_length+2;
  }
}

// Die infos aus dem Puffer holen
void SIsectionBAT::parse(void)
{
	const char *actPos;
	struct bat_service *sv;
//	struct bouquet_ident bi;
	struct SI_section_BAT_header *bh;
	unsigned short descriptors_loop_length;
	unsigned short descriptors_length;
	struct loop_len *ll;
//	std::string bouquetName = "";

	if (!buffer || parsed)
		return;

	if (bufferLength < sizeof(SI_section_BAT_header) + sizeof(struct bat_service)) {
		printf("BAT fix?\n");	//No Services possible - length too short
		delete [] buffer;
		buffer=0;
		bufferLength=0;
		return;
	}

	actPos = &buffer[0];				// We need Bouquet ID and bouquet descriptor length from header
	bh = (struct SI_section_BAT_header *)actPos;	// Header
//	printf("hi: %hu lo: %hu\n", bh->bouquet_descriptors_length_hi, bh->bouquet_descriptors_length_lo);
	descriptors_loop_length = (bh->bouquet_descriptors_length_hi << 8) | bh->bouquet_descriptors_length_lo;
	SIbouquet s((bh->bouquet_id_hi << 8) | bh->bouquet_id_lo);	//Create a new Bouquet entry
//	printf("ident: %hu actpos: %p buf+bl: %p desclen: %hu\n", bi.bouquet_id, actPos, buffer+bufferLength, descriptors_loop_length);
	parseDescriptors(((const char *)bh) + sizeof(SI_section_BAT_header), descriptors_loop_length, s); //Fill out Bouquet Name
	actPos += sizeof(SI_section_BAT_header) + descriptors_loop_length;
	
	ll = (struct loop_len *)actPos;
	descriptors_loop_length = (ll->descriptors_loop_length_hi << 8) | ll->descriptors_loop_length_lo; 	//len is not used at the moment
//	printf("desclen: %hu\n", descriptors_loop_length);
	actPos += sizeof(loop_len);
	
	while (actPos <= &buffer[bufferLength - sizeof(struct bat_service)]) {
		sv = (struct bat_service *)actPos;
		s.transport_stream_id = (sv->transport_stream_id_hi << 8) | sv->transport_stream_id_lo;
		s.original_network_id = (sv->original_network_id_hi << 8) | sv->original_network_id_lo;
		descriptors_length = (sv->descriptors_loop_length_hi << 8) | sv->descriptors_loop_length_lo;
		parseDescriptors(((const char *)sv) + sizeof(struct bat_service), descriptors_length, s); // Transport Stream Loop
		actPos += sizeof(struct bat_service) + descriptors_length;
	}
	parsed = 1;
}

void SIsectionNIT::copyDeliveryDescriptor(const char *buf, SInetwork &s)
{
  //struct descr_generic_header *sv=(struct descr_generic_header *)buf;
  buf+=sizeof(struct descr_generic_header);
  memcpy(s.delivery_descriptor, buf, sizeof(struct satellite_delivery_descriptor));  //same size as cable...
  //printf("Bouquet-Name: %s\n", s.bouquetName.c_str());
}

void SIsectionNIT::parseDescriptors(const char *des, unsigned len, SInetwork &s)
{
//  struct satellite_delivery_descriptor *sdd;
//  const char *ddp;
//  t_transport_stream_id tsid;
//  t_original_network_id onid;
//  unsigned short orbital_pos;

  struct descr_generic_header *desc;
  while(len>=sizeof(struct descr_generic_header)) {
    desc=(struct descr_generic_header *)des;
//    printf("Type: %s\n", decode_descr(desc->descriptor_tag));
//    printf("Length: %hhu\n", desc->descriptor_length);
    if ( (desc->descriptor_tag==0x43) || (desc->descriptor_tag==0x44) ) {
      s.delivery_type = desc->descriptor_tag;
//      printf("Found satellite_delivery_system_descriptor\n");
      copyDeliveryDescriptor((const char *)desc, s);
//      ddp = &s.delivery_descriptor[0];
//      sdd = (struct satellite_delivery_descriptor *)ddp;
//      tsid = s.transport_stream_id;
//      onid = s.original_network_id;
//      orbital_pos = (sdd->orbital_pos_hi << 8) | sdd->orbital_pos_lo;
//      printf("ONID: %04x TSID: %04x Orbital Position: %d\n", onid, tsid, orbital_pos);
    }
    len-=desc->descriptor_length+2;
    des+=desc->descriptor_length+2;
  }
}

// Die infos aus dem Puffer holen
void SIsectionNIT::parse(void)
{

	const char *actPos;
	struct nit_transponder *tp;
	struct SI_section_NIT_header *nh;
	unsigned short descriptors_loop_length;
	unsigned short descriptors_length;
	struct loop_len *ll;

	if (!buffer || parsed)
		return;

	if (bufferLength < sizeof(SI_section_NIT_header) + sizeof(struct nit_transponder)) {
		printf("NIT fix?\n");	//No Services possible - length too short
		delete [] buffer;
		buffer=0;
		bufferLength=0;
		return;
	}
	
	actPos = &buffer[0];				// We need Bouquet ID and bouquet descriptor length from header
	nh = (struct SI_section_NIT_header *)actPos;	// Header
//	printf("hi: %hu lo: %hu\n", bh->bouquet_descriptors_length_hi, bh->bouquet_descriptors_length_lo);
	descriptors_loop_length = (nh->network_descriptors_length_hi << 8) | nh->network_descriptors_length_lo;
//	SIbouquet s((bh->bouquet_id_hi << 8) | bh->bouquet_id_lo);	//Create a new Bouquet entry
//	printf("ident: %hu actpos: %p buf+bl: %p desclen: %hu\n", bi.bouquet_id, actPos, buffer+bufferLength, descriptors_loop_length);
//	parseDescriptors(((const char *)bh) + sizeof(SI_section_BAT_header), descriptors_loop_length, s); //Fill out Bouquet Name
	actPos += sizeof(SI_section_NIT_header) + descriptors_loop_length;
	
	ll = (struct loop_len *)actPos;
	descriptors_loop_length = (ll->descriptors_loop_length_hi << 8) | ll->descriptors_loop_length_lo; 	//len is not used at the moment
//	printf("desclen: %hu\n", descriptors_loop_length);
	actPos += sizeof(loop_len);
	
	while (actPos <= &buffer[bufferLength - sizeof(struct nit_transponder)]) {
		tp = (struct nit_transponder *)actPos;
		SInetwork s(tp);
		s.transport_stream_id = (tp->transport_stream_id_hi << 8) | tp->transport_stream_id_lo;
		s.original_network_id = (tp->original_network_id_hi << 8) | tp->original_network_id_lo;
		descriptors_length = (tp->descriptors_loop_length_hi << 8) | tp->descriptors_loop_length_lo;
		parseDescriptors(((const char *)tp) + sizeof(struct nit_transponder), descriptors_length, s); // Transport Stream Loop
		ntw.insert(s);
		actPos += sizeof(struct nit_transponder) + descriptors_length;
	}
	parsed = 1;
}

#ifndef DO_NOT_INCLUDE_STUFF_NOT_NEEDED_FOR_SECTIONSD
// Liest n Bytes aus einem Socket per read
// Liefert 0 bei timeout
// und -1 bei Fehler
// ansonsten die Anzahl gelesener Bytes
inline int readNbytes(int fd, char *buf, int n, unsigned timeoutInSeconds)
{
int j;
  for(j=0; j<n;) {
    struct pollfd ufds;
//    memset(&ufds, 0, sizeof(ufds));
    ufds.fd=fd;
    ufds.events=POLLIN|POLLPRI;
    ufds.revents=0;
    int rc=poll(&ufds, 1, timeoutInSeconds*1000);
    if(!rc)
      return 0; // timeout
    else if(rc<0) {
      perror ("poll");
      return -1;
    }
    int r=read (fd, buf, n-j);
    if(r<=0) {
      perror ("read");
      return -1;
    }
    j+=r;
    buf+=r;
  }
  return j;
}

//
// Beachtung der Stuffing tables (ST) fehlt noch
//
int SIsections :: readSections(const unsigned short pid, const unsigned char filter, const unsigned char mask, int readNext, unsigned timeoutInSeconds)
{
	int fd;
	struct SI_section_header header;
	unsigned long long firstKey=(unsigned long long)-1;
	SIsections missingSections;
	char *buf;
	unsigned short section_length;

	if ((fd = open(DEMUX_DEVICE, O_RDWR)) == -1) {
		perror(DEMUX_DEVICE);
		return 1;
	}

	if (!setfilter(fd, pid, filter, mask, DMX_IMMEDIATE_START | DMX_CHECK_CRC)) {
		close(fd);
		return 2;
	}

	time_t szeit = time(NULL);

	// Erstes Segment lesen

	do {
		if (time(NULL) > szeit + (long)timeoutInSeconds) {
			close(fd);
			return 0; // timeout -> kein EPG
		}

		int rc = readNbytes(fd, (char *)&header, sizeof(header), timeoutInSeconds);

		if(!rc) {
			close(fd);
			return 0; // timeout -> kein EPG
		}

		else if(rc<0) {
			close(fd);
			//perror ("read header");
			return 3;
		}

		section_length = (header.section_length_hi << 8) | header.section_length_lo;

		buf = new char[sizeof(header) + section_length - 5];

		if (!buf) {
			close(fd);
			fprintf(stderr, "Not enough memory!\n");
			return 4;
		}

		// Den Header kopieren
		memcpy(buf, &header, sizeof(header));

		rc = readNbytes(fd, &buf[sizeof(header)], section_length - 5, timeoutInSeconds);

		if (!rc) {
			close(fd);
			delete[] buf;
			return 0; // timeout -> kein EPG
		}

		else if (rc<0) {
			close(fd);
			//perror ("read section");
			delete[] buf;
			return 5;
		}

		if ((readNext) || (header.current_next_indicator)) {
			// Wir wollen nur aktuelle sections
			insert(SIsection(sizeof(header) + section_length - 5, buf));
			firstKey = SIsection::key(&header);

			// Sonderfall: Nur eine Section
			// d.h. wir sind fertig
			if ((!header.section_number) && (!header.last_section_number)) {
				close(fd);
				return 0;
			}
		}

		else {
			delete[] buf;
		}

	} while (firstKey == (unsigned long long) -1);

	// Die restlichen Segmente lesen
	szeit = time(NULL);

	for (;;) {
		if (time(NULL) > szeit + (long)timeoutInSeconds)
			break; // timeout

		int rc = readNbytes(fd, (char *)&header, sizeof(header), timeoutInSeconds);

		if(!rc)
			break; // timeout

		else if(rc<0) {
			close(fd);
			//perror ("read header");
			return 6;
		}

		if (firstKey==SIsection::key(&header))
			// Wir haben die 1. section wieder gefunden
			break;

		section_length = (header.section_length_hi << 8) | header.section_length_lo;

		buf = new char[sizeof(header) + section_length - 5];

		if (!buf) {
			close(fd);
			fprintf(stderr, "Not enough memory!\n");
			return 7;
		}

		// Den Header kopieren (evtl. malloc und realloc nehmen)
		memcpy(buf, &header, sizeof(header));

		// den Rest der Section einlesen
		rc = readNbytes(fd, &buf[sizeof(header)], section_length - 5, timeoutInSeconds);

		if (!rc) {
			delete[] buf;
			break; // timeout
		}

		else if (rc < 0) {
			close(fd);
			delete[] buf;
			//perror ("read section");
			return 8;
		}

		if ((readNext) || (header.current_next_indicator))
			insert(SIsection(sizeof(header) + section_length - 5, buf));
		else
			delete[] buf;
	}

	close(fd);

#ifdef DEBUG
	// Die Sections ausgeben
	printf("----------------Found sections-----------------------\n");
	//  for_each(begin(), end(), printSIsection());
	for_each(begin(), end(), printSIsectionEIT());
	printf("-----------------------------------------------------\n");
#endif // DEBUG


	// Jetzt erstellen wir eine Liste der fehlenden Sections
	unsigned short actualTableIDextension = (unsigned short) -1;
	unsigned char actualTableID = (unsigned char) -1;
	unsigned char maxNr = 0;
	unsigned char lastNr = 0;

	for (SIsections::iterator k = begin(); k != end(); k++) {
		if ((k->tableIDextension() != actualTableIDextension) || (k->tableID() != actualTableID)) {
			// Neue Table-ID-Extension
			maxNr = k->lastSectionNumber();
			actualTableIDextension = k->tableIDextension();
			actualTableID = k->tableID();
		}

		else if (k->sectionNumber() != (unsigned char)(lastNr + 1)) {
			// Es fehlen Sections
			for (unsigned l = lastNr + 1; l < k->sectionNumber(); l++) {
				//printf("Debug: t: 0x%02x s: %u nr: %u last: %u max: %u l: %u\n", actualTableID, actualTableIDextension, k->sectionNumber(), lastNr, maxNr, l);

				struct SI_section_header h;
				memcpy(&h, k->header(), sizeof(struct SI_section_header));
				h.section_number = l;
				missingSections.insert(SIsection(&h));
			}
		}

		lastNr = k->sectionNumber();
	}

#ifdef DEBUG
	printf("Sections read: %d\n\n", size());
#endif // DEBUG

	if (!missingSections.size())
		return 0;

#ifdef DEBUG
	printf("----------------Missing sections---------------------\n");
	for_each(missingSections.begin(), missingSections.end(), printSmallSIsectionHeader());
	printf("-----------------------------------------------------\n");
	printf("Sections read: %d\n\n", size());
	printf("Sections misssing: %d\n", missingSections.size());
	printf("Searching missing sections\n");
#endif // DEBUG

	szeit = time(NULL);

	if ((fd = open(DEMUX_DEVICE, O_RDWR)) == -1) {
		perror(DEMUX_DEVICE);
		return 9;
	}

	if (!setfilter(fd, pid, filter, mask, DMX_IMMEDIATE_START | DMX_CHECK_CRC)) {
		close(fd);
		return 10;
	}

	// Jetzt lesen wir die fehlenden Sections ein
	for(;;) {
		if (time(NULL) > szeit + (long)timeoutInSeconds)
			break; // Timeout

		int rc = readNbytes(fd, (char *)&header, sizeof(header), timeoutInSeconds);

		if(!rc)
			break; // timeout

		else if (rc < 0) {
			close(fd);
			//perror ("read header");
			return 11;

		}

		section_length = (header.section_length_hi << 8) | header.section_length_lo;
		
		buf = new char[sizeof(header) + section_length - 5];

		if (!buf) {
			close(fd);
			fprintf(stderr, "Not enough memory!\n");
			return 12;
		}

		// Den Header kopieren (evtl. malloc und realloc nehmen)
		memcpy(buf, &header, sizeof(header));
		// den Rest der Section einlesen
		rc = readNbytes(fd, &buf[sizeof(header)], section_length - 5, timeoutInSeconds);

		if (!rc) {
			delete[] buf;
			break; // timeout
		}

		else if (rc < 0) {
			close(fd);
			delete[] buf;
			//perror ("read section");
			return 13;
		}

		if (missingSections.find(SIsection(&header)) != missingSections.end()) {
#ifdef DEBUG
			printf("Find missing section:");
			SIsection::dumpSmallSectionHeader(&header);
#endif  // DEBUG
			// War bisher vermisst
			// In die Menge einfuegen
			insert(SIsection(sizeof(header) + section_length - 5, buf));

			// Und aus der vermissten Menge entfernen
			missingSections.erase(SIsection(&header));
#ifdef DEBUG
			printf("Sections misssing: %d\n", missingSections.size());
#endif // DEBUG
		}

		else {
			// Puffer wieder loeschen
			delete[] buf;
		}
	}

	close(fd);
	return 0;
}
#endif
