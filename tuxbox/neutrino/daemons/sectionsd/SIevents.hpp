#ifndef SIEVENTS_HPP
#define SIEVENTS_HPP
//
// $Id: SIevents.hpp,v 1.2 2001/05/17 01:53:35 fnbrd Exp $
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


class SIevent {
  public:
    SIevent(const struct eit_event *e) {
      eventID=e->event_id;
      startzeit=changeUTCtoCtime(((const unsigned char *)e)+2);
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
    }
    unsigned short eventID;
    string name; // Name aus dem Short-Event-Descriptor
    string text; // Text aus dem Short-Event-Descriptor
    time_t startzeit; // lokale Zeit
    unsigned dauer; // in Sekunden
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
      if(name.length())
        printf("Name: %s\n", name.c_str());
      if(text.length())
        printf("Text: %s\n", text.c_str());
      printf("Startzeit: %s", ctime(&startzeit));
      printf("Dauer: %02u:%02u:%02u (%umin, %us)\n\n", dauer/3600, (dauer%3600)/60, dauer%60, dauer/60, dauer);
    }
    void dumpSmall(void) const {
      if(name.length())
        printf("Name: %s\n", name.c_str());
      if(text.length())
        printf("Text: %s\n", text.c_str());
      printf("Startzeit: %s", ctime(&startzeit));
      printf("Dauer: %02u:%02u:%02u (%umin, %us)\n\n", dauer/3600, (dauer%3600)/60, dauer%60, dauer/60, dauer);
    }
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
