#ifndef SIEVENTS_HPP
#define SIEVENTS_HPP
//
// $Id: SIevents.hpp,v 1.9 2001/06/11 19:22:54 fnbrd Exp $
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
// Revision 1.9  2001/06/11 19:22:54  fnbrd
// Events haben jetzt mehrere Zeiten, fuer den Fall von NVODs (cinedoms)
//
// Revision 1.8  2001/06/11 01:15:16  fnbrd
// NVOD reference descriptors und Service-Typ
//
// Revision 1.7  2001/06/10 14:55:51  fnbrd
// Kleiner Aenderungen und Ergaenzungen (epgMini).
//
// Revision 1.6  2001/05/20 14:40:15  fnbrd
// Mit parental_rating
//
// Revision 1.5  2001/05/19 20:15:08  fnbrd
// Kleine Aenderungen (und epgXML).
//
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
class SIservices;

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
    // Std-copy
    SIcomponent(const SIcomponent &c) {
      streamContent=c.streamContent;
      componentType=c.componentType;
      componentTag=c.componentTag;
      component=c.component;
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
    int saveXML(FILE *file) const {
      if(fprintf(file, "    <component tag=\"0x%02hhx\" type=\"0x%02hhx\" stream_content=\"0x%02hhx\" />\n", componentTag, componentType, streamContent)<0)
        return 1;
      return 0;
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

// Fuer for_each
struct saveSIcomponentXML : public unary_function<SIcomponent, void>
{
  FILE *f;
  saveSIcomponentXML(FILE *fi) { f=fi;}
  void operator() (const SIcomponent &c) { c.saveXML(f);}
};

typedef multiset <SIcomponent, less<SIcomponent> > SIcomponents;

class SIparentalRating {
  public:
    SIparentalRating(const string &cc, unsigned char rate) {
      rating=rate;
      countryCode=cc;
    }
    // Std-Copy
    SIparentalRating(const SIparentalRating &r) {
      rating=r.rating;
      countryCode=r.countryCode;
    }
    // Der Operator zum sortieren
    bool operator < (const SIparentalRating& c) const {
      return countryCode < c.countryCode;
    }
    void dump(void) const {
      printf("Rating: %s %hhu (+3)\n", countryCode.c_str(), rating);
    }
    int saveXML(FILE *file) const {
      if(fprintf(file, "    <parental_rating country=\"%s\" rating=\"%hhu\" />\n", countryCode.c_str(), rating)<0)
        return 1;
      return 0;
    }
    string countryCode;
    unsigned char rating; // Bei 1-16 -> Minumim Alter = rating +3
};

// Fuer for_each
struct printSIparentalRating : public unary_function<SIparentalRating, void>
{
  void operator() (const SIparentalRating &r) { r.dump();}
};

// Fuer for_each
struct saveSIparentalRatingXML : public unary_function<SIparentalRating, void>
{
  FILE *f;
  saveSIparentalRatingXML(FILE *fi) { f=fi;}
  void operator() (const SIparentalRating &r) { r.saveXML(f);}
};

typedef set <SIparentalRating, less<SIparentalRating> > SIparentalRatings;

class SItime {
  public:
    SItime(time_t s, unsigned d) {
      startzeit=s;
      dauer=d; // in Sekunden, 0 -> time shifted (cinedoms)
    }
    // Std-Copy
    SItime(const SItime &t) {
      startzeit=t.startzeit;
      dauer=t.dauer;
    }
    // Der Operator zum sortieren
    bool operator < (const SItime& t) const {
      return startzeit < t.startzeit;
    }
    void dump(void) const {
      printf("Startzeit: %s", ctime(&startzeit));
      printf("Dauer: %02u:%02u:%02u (%umin, %us)\n", dauer/3600, (dauer%3600)/60, dauer%60, dauer/60, dauer);
    }
    int saveXML(FILE *file) const { // saves the time
      // Ist so noch nicht in Ordnung, das sollte untergliedert werden,
      // da sonst evtl. time,date,duration,time,date,... auftritt
      // und eine rein sequentielle Ordnung finde ich nicht ok.
      struct tm *zeit=localtime(&startzeit);
      fprintf(file, "    <time>%02d:%02d:%02d</time>\n", zeit->tm_hour, zeit->tm_min, zeit->tm_sec);
      fprintf(file, "    <date>%02d.%02d.%04d</date>\n", zeit->tm_mday, zeit->tm_mon+1, zeit->tm_year+1900);
      fprintf(file, "    <duration>%u</duration>\n", dauer);
      return 0;
    }
    time_t startzeit;  // lokale Zeit, 0 -> time shifted (cinedoms)
    unsigned dauer; // in Sekunden, 0 -> time shifted (cinedoms)
};

typedef set <SItime, less<SItime> > SItimes;

// Fuer for_each
struct printSItime : public unary_function<SItime, void>
{
  void operator() (const SItime &t) { t.dump();}
};

// Fuer for_each
struct saveSItimeXML : public unary_function<SItime, void>
{
  FILE *f;
  saveSItimeXML(FILE *fi) { f=fi;}
  void operator() (const SItime &t) { t.saveXML(f);}
};

class SIevent {
  public:
    SIevent(const struct eit_event *);
    // Std-Copy
    SIevent(const SIevent &);
    SIevent(void) {
      serviceID=eventID=0;
//      dauer=0;
//      startzeit=0;
    }
    unsigned short eventID;
    string name; // Name aus dem Short-Event-Descriptor
    string text; // Text aus dem Short-Event-Descriptor
    string itemDescription; // Aus dem Extended Descriptor
    string item; // Aus dem Extended Descriptor
    string extendedText; // Aus dem Extended Descriptor
    string contentClassification; // Aus dem Content Descriptor, als String, da mehrere vorkommen koennen
    string userClassification; // Aus dem Content Descriptor, als String, da mehrere vorkommen koennen
//    time_t startzeit; // lokale Zeit, 0 -> time shifted (cinedoms)
//    unsigned dauer; // in Sekunden, 0 -> time shifted (cinedoms)
    unsigned short serviceID;
    SIcomponents components;
    SIparentalRatings ratings;
    SItimes times;
    // Der Operator zum sortieren
    bool operator < (const SIevent& e) const {
      // Erst nach Service-ID, dann nach Event-ID sortieren
      return serviceID!=e.serviceID ? serviceID < e.serviceID : eventID < e.eventID;
    }
    int saveXML(FILE *file) const { // saves the event
      return saveXML0(file) || saveXML2(file);
    }
    int saveXML(FILE *file, const char *serviceName) const; // saves the event
    void dump(void) const; // dumps the event to stdout
    void dumpSmall(void) const; // dumps the event to stdout (not all information)
    // Liefert das aktuelle EPG des senders mit der uebergebenen serviceID,
    // bei Fehler ist die serviceID des zurueckgelieferten Events 0
    static SIevent readActualEvent(unsigned short serviceID, unsigned timeoutInSeconds=2);
  protected:
    int saveXML0(FILE *f) const;
    int saveXML2(FILE *f) const;
};

// Fuer for_each
struct printSIevent : public unary_function<SIevent, void>
{
  void operator() (const SIevent &e) { e.dump();}
};

// Fuer for_each
struct saveSIeventXML : public unary_function<SIevent, void>
{
  FILE *f;
  saveSIeventXML(FILE *fi) { f=fi;}
  void operator() (const SIevent &e) { e.saveXML(f);}
};

// Fuer for_each
struct saveSIeventXMLwithServiceName : public unary_function<SIevent, void>
{
  FILE *f;
  const SIservices *s;
  saveSIeventXMLwithServiceName(FILE *fi, const SIservices &svs) {f=fi; s=&svs;}
  void operator() (const SIevent &e) {
    SIservices::iterator k=s->find(SIservice(e.serviceID));
    if(k!=s->end()) {
      if(k->serviceName.length())
      e.saveXML(f, k->serviceName.c_str());
    }
    else
      e.saveXML(f);
  }
};

// Fuer for_each
struct printSIeventWithService : public unary_function<SIevent, void>
{
  printSIeventWithService(const SIservices &svs) { s=&svs;}
  void operator() (const SIevent &e) {
    SIservices::iterator k=s->find(SIservice(e.serviceID));
    if(k!=s->end()) {
      char servicename[50];
      strncpy(servicename, k->serviceName.c_str(), sizeof(servicename)-1);
      servicename[sizeof(servicename)-1]=0;
      removeControlCodes(servicename);
      printf("Service-Name: %s\n", servicename);
//      printf("Provider-Name: %s\n", k->providerName.c_str());
    }
//    e.dump();
    e.dumpSmall();
    printf("\n");
  }
  const SIservices *s;
};

//typedef set <SIevent, less<SIevent> > SIevents;

class SIevents : public set <SIevent, less<SIevent> >
{
  public:
    // Entfernt anhand der Services alle time shifted events (Service-Typ 0)
    // und sortiert deren Zeiten in die Events mit dem Text ein.
    void mergeAndRemoveTimeShiftedEvents(const SIservices &);
};

#endif // SIEVENTS_HPP
