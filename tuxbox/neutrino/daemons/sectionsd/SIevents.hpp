#ifndef SIEVENTS_HPP
#define SIEVENTS_HPP
//
// $Id: SIevents.hpp,v 1.4 2001/05/18 20:31:04 fnbrd Exp $
//
// classes SIevent and SIevents (dbox-II-project)
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
// $Log: SIevents.hpp,v $
// Revision 1.4  2001/05/18 20:31:04  fnbrd
// Aenderungen fuer -Wall
//
// Revision 1.3  2001/05/18 13:11:46  fnbrd
// Fast komplett, fehlt nur noch die Auswertung der time-shifted events
// (Startzeit und Dauer der Cinedoms).
//
// Revision 1.2  2001/05/17 01:53:35  fnbrd
// Jetzt mit lokaler Zeit.
//
// Revision 1.1  2001/05/16 15:23:47  fnbrd
// Alles neu macht der Mai.
//
//

// forward references
class SIservice;

struct eit_event {
  unsigned short event_id : 16;
  unsigned long long start_time : 40;
  unsigned int duration : 24;
  unsigned char running_status : 3;
  unsigned char free_CA_mode : 1;
  unsigned short descriptors_loop_length : 12;
} __attribute__ ((packed)) ;


struct descr_component_header {
  unsigned char descriptor_tag : 8;
  unsigned char descriptor_length : 8;
  unsigned char reserved_future_use : 4;
  unsigned char stream_content : 4;
  unsigned char component_type : 8;
  unsigned char component_tag : 8;
  unsigned iso_639_2_language_code : 24;
} __attribute__ ((packed)) ;

class SIcomponent {
  public:
    SIcomponent(const struct descr_component_header *comp) {
      streamContent=comp->stream_content;
      componentType=comp->component_type;
      componentTag=comp->component_tag;
      if(comp->descriptor_length>sizeof(struct descr_component_header)-2)
        component=string(((const char *)comp)+sizeof(struct descr_component_header), comp->descriptor_length-(sizeof(struct descr_component_header)-2));
    }
    // Der Operator zum sortieren
    bool operator < (const SIcomponent& c) const {
      return streamContent < c.streamContent;
//      return component < c.component;
    }
    void dump(void) const {
      if(component.length())
        printf("Component: %s\n", component.c_str());
      printf("Stream Content: 0x%02hhx\n", streamContent);
      printf("Component type: 0x%02hhx\n", componentType);
      printf("Component tag: 0x%02hhx\n", componentTag);
    }
    string component; // Text aus dem Component Descriptor
    unsigned char componentType; // Component Descriptor
    unsigned char componentTag; // Component Descriptor
    unsigned char streamContent; // Component Descriptor
};

// Fuer for_each
struct printSIcomponent : public unary_function<SIcomponent, void>
{
  void operator() (const SIcomponent &c) { c.dump();}
};


typedef multiset <SIcomponent, less<SIcomponent> > SIcomponents;

class SIevent {
  public:
    SIevent(const struct eit_event *e) {
      eventID=e->event_id;
      startzeit=changeUTCtoCtime(((const unsigned char *)e)+2);
      if(e->duration==0xffffff)
        dauer=0; // keine Dauer
      else
        dauer=((e->duration)>>20)*10*3600L+(((e->duration)>>16)&0x0f)*3600L+
          (((e->duration)>>12)&0x0f)*10*60L+(((e->duration)>>8)&0x0f)*60L+
	  (((e->duration)>>4)&0x0f)*10+((e->duration)&0x0f);
      serviceID=0;
    }
    // Std-Copy
    SIevent(const SIevent &e) {
      eventID=e.eventID;
      name=e.name;
      text=e.text;
      startzeit=e.startzeit;
      dauer=e.dauer;
      serviceID=e.serviceID;
      itemDescription=e.itemDescription;
      item=e.item;
      extendedText=e.extendedText;
      contentClassification=e.contentClassification;
      userClassification=e.userClassification;
      components=e.components;
    }
    unsigned short eventID;
    string name; // Name aus dem Short-Event-Descriptor
    string text; // Text aus dem Short-Event-Descriptor
    string itemDescription; // Aus dem Extended Descriptor
    string item; // Aus dem Extended Descriptor
    string extendedText; // Aus dem Extended Descriptor
    string contentClassification; // Aus dem Content Descriptor, als String, da mehrere vorkommen koennen
    string userClassification; // Aus dem Content Descriptor, als String, da mehrere vorkommen koennen
    time_t startzeit; // lokale Zeit, 0 -> time shifted (cinedoms)
    unsigned dauer; // in Sekunden, 0 -> time shifted (cinedoms)
    unsigned short serviceID;
    // Der Operator zum sortieren
    bool operator < (const SIevent& e) const {
      // Erst nach Service-ID, dann nach Event-ID sortieren
      return serviceID!=e.serviceID ? serviceID < e.serviceID : eventID < e.eventID;
    }
    void dump(void) const {
      if(serviceID)
        printf("Service-ID: %hu\n", serviceID);
      printf("Event-ID: %hu\n", eventID);
      if(item.length())
        printf("Item: %s\n", item.c_str());
      if(itemDescription.length())
        printf("Item-Description: %s\n", itemDescription.c_str());
      if(name.length())
        printf("Name: %s\n", name.c_str());
      if(text.length())
        printf("Text: %s\n", text.c_str());
      if(extendedText.length())
        printf("Extended-Text: %s\n", extendedText.c_str());
      if(contentClassification.length()) {
        printf("Content classification:");
        for(unsigned i=0; i<contentClassification.length(); i++)
          printf(" 0x%02hhx", contentClassification[i]);
        printf("\n");
      }
      if(userClassification.length()) {
        printf("User classification:");
        for(unsigned i=0; i<userClassification.length(); i++)
          printf(" 0x%02hhx", userClassification[i]);
        printf("\n");
      }
      if(startzeit)
        printf("Startzeit: %s", ctime(&startzeit));
      if(dauer)
        printf("Dauer: %02u:%02u:%02u (%umin, %us)\n", dauer/3600, (dauer%3600)/60, dauer%60, dauer/60, dauer);
      printf("\n");
      for_each(components.begin(), components.end(), printSIcomponent());
    }
    void dumpSmall(void) const {
      if(name.length())
        printf("Name: %s\n", name.c_str());
      if(text.length())
        printf("Text: %s\n", text.c_str());
      if(extendedText.length())
        printf("Extended-Text: %s\n", extendedText.c_str());
      if(startzeit)
        printf("Startzeit: %s", ctime(&startzeit));
      if(dauer)
        printf("Dauer: %02u:%02u:%02u (%umin, %us)\n", dauer/3600, (dauer%3600)/60, dauer%60, dauer/60, dauer);
      printf("\n");
    }
    SIcomponents components;
};

// Fuer for_each
struct printSIevent : public unary_function<SIevent, void>
{
  void operator() (const SIevent &e) { e.dump();}
};

// Fuer for_each
struct printSIeventWithService : public unary_function<SIevent, void>
{
  printSIeventWithService(const SIservices &svs) { s=&svs;}
  void operator() (const SIevent &e) {
    SIservices::iterator k=s->find(SIservice(e.serviceID));
    if(k!=s->end())
      printf("%s\n", k->serviceName.c_str());
    e.dumpSmall();
  }
  const SIservices *s;
};

typedef set <SIevent, less<SIevent> > SIevents;

#endif // SIEVENTS_HPP
