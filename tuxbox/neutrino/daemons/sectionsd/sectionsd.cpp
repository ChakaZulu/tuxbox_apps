//
//  $Id: sectionsd.cpp,v 1.33 2001/07/24 15:05:31 fnbrd Exp $
//
//	sectionsd.cpp (network daemon for SI-sections)
//	(dbox-II-project)
//
//	Copyright (C) 2001 by fnbrd
//
//    Homepage: http://dbox2.elxsi.de
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
//  $Log: sectionsd.cpp,v $
//  Revision 1.33  2001/07/24 15:05:31  fnbrd
//  sectionsd benutzt voruebergehend smart pointers der Boost-Lib.
//
//  Revision 1.32  2001/07/23 20:59:48  fnbrd
//  Fehler im Time-Thread behoben.
//
//  Revision 1.31  2001/07/23 09:00:10  fnbrd
//  Fehler behoben.
//
//  Revision 1.30  2001/07/23 02:43:30  fnbrd
//  internal changes.
//
//  Revision 1.29  2001/07/23 00:22:15  fnbrd
//  Many internal changes, nvod-events now functional.
//
//  Revision 1.28  2001/07/20 00:02:47  fnbrd
//  Kleiner Hack fuer besseres Zusammenspiel mit Neutrino.
//
//  Revision 1.26  2001/07/19 14:12:30  fnbrd
//  Noch ein paar Kleinigkeiten verbessert.
//
//  Revision 1.25  2001/07/19 10:33:52  fnbrd
//  Beschleunigt, interne Strukturen geaendert, Ausgaben sortiert.
//
//  Revision 1.24  2001/07/18 13:51:05  fnbrd
//  Datumsfehler behoben.
//
//  Revision 1.23  2001/07/18 03:26:45  fnbrd
//  Speicherloch gefixed.
//
//  Revision 1.22  2001/07/17 14:15:52  fnbrd
//  Kleine Aenderung damit auch static geht.
//
//  Revision 1.21  2001/07/17 13:14:59  fnbrd
//  Noch ne Verbesserung in Bezug auf alte Events.
//
//  Revision 1.19  2001/07/17 02:38:56  fnbrd
//  Fehlertoleranter
//
//  Revision 1.18  2001/07/16 15:57:58  fnbrd
//  Parameter -d fuer debugausgaben
//
//  Revision 1.17  2001/07/16 13:08:34  fnbrd
//  Noch ein Fehler beseitigt.
//
//  Revision 1.16  2001/07/16 12:56:50  fnbrd
//  Noch ein Fehler behoben.
//
//  Revision 1.15  2001/07/16 12:52:30  fnbrd
//  Fehler behoben.
//
//  Revision 1.14  2001/07/16 11:49:31  fnbrd
//  Neuer Befehl, Zeichen fuer codetable aus den Texten entfernt
//
//  Revision 1.13  2001/07/15 15:09:27  fnbrd
//  Informative Ausgabe.
//
//  Revision 1.12  2001/07/15 15:05:09  fnbrd
//  Speichert jetzt alle Events die bis zu 24h in der Zukunft liegen.
//
//  Revision 1.11  2001/07/15 11:58:20  fnbrd
//  Vergangene Zeit in Prozent beim EPG
//
//  Revision 1.10  2001/07/15 04:32:46  fnbrd
//  neuer sectionsd (mit event-liste)
//
//  Revision 1.9  2001/07/14 22:59:58  fnbrd
//  removeOldEvents() in SIevents
//
//  Revision 1.8  2001/07/14 17:36:04  fnbrd
//  Verbindungsthreads sind jetzt detached (kein Mem-leak mehr)
//
//  Revision 1.7  2001/07/14 16:41:44  fnbrd
//  fork angemacht
//
//  Revision 1.6  2001/07/14 16:38:46  fnbrd
//  Mit workaround fuer defektes mktime der glibc
//
//  Revision 1.5  2001/07/14 10:19:26  fnbrd
//  Mit funktionierendem time-thread (mktime der glibc muss aber gefixt werden)
//
//  Revision 1.4  2001/07/12 22:51:25  fnbrd
//  Time-Thread im sectionsd (noch disabled, da prob mit mktime)
//
//  Revision 1.3  2001/07/11 22:08:55  fnbrd
//  wegen gcc 3.0
//
//  Revision 1.2  2001/07/06 10:25:04  fnbrd
//  Debug-Zeug raus.
//
//  Revision 1.1  2001/06/27 11:59:44  fnbrd
//  Angepasst an gcc 3.0
//
//

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <malloc.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>

#include <set>
#include <map>
#include <algorithm>
#include <string>

#include <ost/dmx.h>

// Loki's SmartPointers benutzen SmallObject zwecks Speicherverwaltung kleiner Objekte,
// den SmartPointers. Fuer eine Multithreaded Umgebung muss ich erst noch nachlesen
// wie ich diese SmallObjects anpassen muss
//#include <loki/SmartPtr.h>

// Daher nehmen wir SmartPointers aus der Boost-Lib (www.boost.org)
#include <boost/smart_ptr.hpp>

#include "sectionsdMsg.h"
#include "SIutils.hpp"
#include "SIservices.hpp"
#include "SIevents.hpp"
#include "SIsections.hpp"

#define PORT_NUMBER 1600

// Zeit die fuer die scheduled eit's benutzt wird (in Sekunden)
#define TIME_EIT_SCHEDULED 50

// Zeit die fuer die present/following (und nvods) eit's benutzt wird (in Sekunden)
#define TIME_EIT_PRESENT 15

// Wieviele Sekunden EPG gecached werden sollen
static long secondsToCache=72*60L*60L; // 3 Tage (72h)
// Ab wann ein Event als alt gilt (in Sekunden)
static long oldEventsAre=120*60L; // 2h
static int debug=0;

#define dprintf(fmt, args...) {if(debug) printf(fmt, ## args);}
#define dputs(str) {if(debug) puts(str);}

static pthread_mutex_t eventsLock=PTHREAD_MUTEX_INITIALIZER; // Unsere (fast-)mutex, damit nicht gleichzeitig in die Menge events geschrieben und gelesen wird
static pthread_mutex_t servicesLock=PTHREAD_MUTEX_INITIALIZER; // Unsere (fast-)mutex, damit nicht gleichzeitig in die Menge services geschrieben und gelesen wird

inline void lockServices(void)
{
  pthread_mutex_lock(&servicesLock);
}

inline void unlockServices(void)
{
  pthread_mutex_unlock(&servicesLock);
}

inline void lockEvents(void)
{
  pthread_mutex_lock(&eventsLock);
}

inline void unlockEvents(void)
{
  pthread_mutex_unlock(&eventsLock);
}

static int timeset=0; // = 1 falls Uhrzeit gesetzt
static const SIevent nullEvt; // Null-Event

//------------------------------------------------------------
// Wir verwalten die events in SmartPointers
// und nutzen verschieden sortierte Menge zum Zugriff
//------------------------------------------------------------

// SmartPointer auf SIevent
//typedef Loki::SmartPtr<class SIevent, Loki::RefCounted, Loki::DisallowConversion, Loki::NoCheck>
//  SIeventPtr;
typedef boost::shared_ptr<class SIevent>
  SIeventPtr;

// Mengen mit SIeventPtr sortiert nach Event-ID
typedef std::map<unsigned short, SIeventPtr, std::less<unsigned short> > MySIeventsOrderEventID;
static MySIeventsOrderEventID mySIeventsOrderEventID;

// Mengen mit SIeventPtr sortiert nach Event-ID fuer NVOD-Events (mehrere Zeiten)
static MySIeventsOrderEventID mySIeventsNVODorderEventID;

struct OrderServiceIDFirstStartTimeEventID
{
  bool operator()(const SIeventPtr &p1, const SIeventPtr &p2) {
    return
      p1->serviceID == p2->serviceID ?
      (p1->times.begin()->startzeit == p2->times.begin()->startzeit ? p1->eventID < p2->eventID : p1->times.begin()->startzeit < p2->times.begin()->startzeit )
      :
      (p1->serviceID < p2->serviceID );
  }
};

typedef std::map<const SIeventPtr, SIeventPtr, OrderServiceIDFirstStartTimeEventID > MySIeventsOrderServiceIDFirstStartTimeEventID;
static MySIeventsOrderServiceIDFirstStartTimeEventID mySIeventsOrderServiceIDFirstStartTimeEventID;

struct OrderFirstEndTimeServiceIDEventID
{
  bool operator()(const SIeventPtr &p1, const SIeventPtr &p2) {
    return
      p1->times.begin()->startzeit + (long)p1->times.begin()->dauer == p2->times.begin()->startzeit + (long)p2->times.begin()->dauer ?
      ( p1->serviceID == p2->serviceID ? p1->eventID < p2->eventID : p1->serviceID < p2->serviceID )
        :
	    ( p1->times.begin()->startzeit + (long)p1->times.begin()->dauer < p2->times.begin()->startzeit + (long)p2->times.begin()->dauer ) ;
  }
};

typedef std::map<const SIeventPtr, SIeventPtr, OrderFirstEndTimeServiceIDEventID > MySIeventsOrderFirstEndTimeServiceIDEventID;
static MySIeventsOrderFirstEndTimeServiceIDEventID mySIeventsOrderFirstEndTimeServiceIDEventID;

// Hier landen alle Service-Ids von Meta-Events inkl. der zugehoerigen Event-ID (nvod)
// d.h. key ist Service-Id des Meta-Events und Data ist die Event-ID
typedef std::map<unsigned short, unsigned short, std::less<unsigned short> > MySIeventIDsMetaOrderServiceID;
static MySIeventIDsMetaOrderServiceID mySIeventIDsMetaOrderServiceID;

// Loescht ein Event aus allen Mengen
static void deleteEvent(const unsigned short eventID)
{
  MySIeventsOrderEventID::iterator e=mySIeventsOrderEventID.find(eventID);
  if(e!=mySIeventsOrderEventID.end()) {
    if(e->second->times.size()) {
      mySIeventsOrderFirstEndTimeServiceIDEventID.erase(e->second);
      mySIeventsOrderServiceIDFirstStartTimeEventID.erase(e->second);
    }
    mySIeventsOrderEventID.erase(eventID);
    mySIeventsNVODorderEventID.erase(eventID);
  }
/*
  for(MySIeventIDsMetaOrderServiceID::iterator i=mySIeventIDsMetaOrderServiceID.begin(); i!=mySIeventIDsMetaOrderServiceID.end(); i++)
    if(i->second==eventID)
      mySIeventIDsMetaOrderServiceID.erase(i);
*/
}

// Fuegt ein Event in alle Mengen ein
static void addEvent(const SIevent &e)
{
  SIeventPtr s(new SIevent(e));
  // Damit in den nicht nach Event-ID sortierten Mengen
  // Mehrere Events mit gleicher ID sind, diese vorher loeschen
  deleteEvent(s->eventID);
  // Pruefen ob es ein Meta-Event ist
  MySIeventIDsMetaOrderServiceID::iterator i=mySIeventIDsMetaOrderServiceID.find(s->serviceID);
  if(i!=mySIeventIDsMetaOrderServiceID.end()) {
    // ist ein MetaEvent, d.h. mit Zeiten fuer NVOD-Event
    if(s->times.size()) {
      // D.h. wir fuegen die Zeiten in das richtige Event ein
      MySIeventsOrderEventID::iterator ie=mySIeventsOrderEventID.find(i->second);
      if(ie!=mySIeventsOrderEventID.end()) {
        // Event vorhanden
        // Falls das Event in den beiden Mengen mit Zeiten nicht vorhanden
        // ist, dieses dort einfuegen
	      MySIeventsOrderServiceIDFirstStartTimeEventID::iterator i2=mySIeventsOrderServiceIDFirstStartTimeEventID.find(ie->second);
	      if(i2==mySIeventsOrderServiceIDFirstStartTimeEventID.end()) {
	        // nicht vorhanden -> einfuegen
          mySIeventsOrderServiceIDFirstStartTimeEventID.insert(std::make_pair(ie->second, ie->second));
          mySIeventsOrderFirstEndTimeServiceIDEventID.insert(std::make_pair(ie->second, ie->second));
   	    }
        // Und die Zeiten im Event updaten
        ie->second->times.insert(s->times.begin(), s->times.end());
      }
    }
  }
  else {
    // normales Event
    mySIeventsOrderEventID.insert(std::make_pair(s->eventID, s));
    if(s->times.size()) {
      // diese beiden Mengen enthalten nur Events mit Zeiten
      mySIeventsOrderServiceIDFirstStartTimeEventID.insert(std::make_pair(s, s));
      mySIeventsOrderFirstEndTimeServiceIDEventID.insert(std::make_pair(s, s));
    }
  }
}

static void addNVODevent(const SIevent &e)
{
  SIeventPtr s(new SIevent(e));
  MySIeventsOrderEventID::iterator e2=mySIeventsOrderEventID.find(s->eventID);
  if(e2!=mySIeventsOrderEventID.end()) {
    // bisher gespeicherte Zeiten retten
    s->times.insert(e2->second->times.begin(), e2->second->times.end());
  }
  // Damit in den nicht nach Event-ID sortierten Mengen
  // Mehrere Events mit gleicher ID sind, diese vorher loeschen
  deleteEvent(s->eventID);
  mySIeventsOrderEventID.insert(std::make_pair(s->eventID, s));
  mySIeventsNVODorderEventID.insert(std::make_pair(s->eventID, s));
  if(s->times.size()) {
    // diese beiden Mengen enthalten nur Events mit Zeiten
    mySIeventsOrderServiceIDFirstStartTimeEventID.insert(std::make_pair(s, s));
    mySIeventsOrderFirstEndTimeServiceIDEventID.insert(std::make_pair(s, s));
  }
}

static void removeOldEvents(long seconds)
{
  // Alte events loeschen
  time_t zeit=time(NULL);
  for(MySIeventsOrderFirstEndTimeServiceIDEventID::iterator e=mySIeventsOrderFirstEndTimeServiceIDEventID.begin(); e!=mySIeventsOrderFirstEndTimeServiceIDEventID.end(); e++)
    if(e->first->times.begin()->startzeit+(long)e->first->times.begin()->dauer<zeit-seconds)
      deleteEvent(e->first->eventID);
    else
      break; // sortiert nach Endzeit, daher weiteres Suchen unnoetig
  return;
}

//typedef SmartPtr<class SIservice, RefCounted, DisallowConversion, AssertCheckStrict>
//typedef Loki::SmartPtr<class SIservice, Loki::RefCounted, Loki::DisallowConversion, Loki::NoCheck>
//  SIservicePtr;
typedef boost::shared_ptr<class SIservice>
  SIservicePtr;

// Key ist unsigned short (Sevice-ID), data ist ein SIservicePtr
typedef std::map<unsigned short, SIservicePtr, std::less<unsigned short> > MySIservicesOrderServiceID;
static MySIservicesOrderServiceID mySIservicesOrderServiceID;

// Key ist unsigned short (Sevice-ID), data ist ein SIservicePtr
typedef std::map<unsigned short, SIservicePtr, std::less<unsigned short> > MySIservicesNVODorderServiceID;
static MySIservicesNVODorderServiceID mySIservicesNVODorderServiceID;

struct OrderServiceName
{
  bool operator()(const SIservicePtr &p1, const SIservicePtr &p2) {
    return strcasecmp(p1->serviceName.c_str(), p2->serviceName.c_str()) < 0;
  }
};

typedef std::map<const SIservicePtr, SIservicePtr, OrderServiceName > MySIservicesOrderServiceName;
static MySIservicesOrderServiceName mySIservicesOrderServiceName;

/*
// Loescht ein Event aus allen Mengen
static void deleteService(const unsigned short serviceID)
{
  MySIservicesOrderServiceID::iterator s=mySIservicesOrderServiceID.find(serviceID);
  if(s!=mySIservicesOrderServiceID.end()) {
    mySIservicesOrderServiceName.erase(s->second);
    mySIservicesOrderServiceID.erase(serviceID);
  }
}
*/

// Fuegt ein Event in alle Mengen ein
static void addService(const SIservice &s)
{
  SIservicePtr sptr(new SIservice(s));
  // Controlcodes entfernen
  char servicename[50];
  strncpy(servicename, sptr->serviceName.c_str(), sizeof(servicename)-1);
  servicename[sizeof(servicename)-1]=0;
  removeControlCodes(servicename);
  sptr->serviceName=servicename;
  mySIservicesOrderServiceID.insert(std::make_pair(sptr->serviceID, sptr));
  if(sptr->nvods.size())
    mySIservicesNVODorderServiceID.insert(std::make_pair(sptr->serviceID, sptr));
  mySIservicesOrderServiceName.insert(std::make_pair(sptr, sptr));
}

//------------------------------------------------------------
// other stuff
//------------------------------------------------------------

// Liest n Bytes aus einem Socket per read
// Liefert 0 bei timeout
// und -1 bei Fehler
// ansonsten die Anzahl gelesener Bytes
inline int readNbytes(int fd, char *buf, int n, unsigned timeoutInSeconds)
{
int j;

  timeoutInSeconds*=1000; // in Millisekunden aendern
  for(j=0; j<n;) {
    struct pollfd ufds;
    ufds.fd=fd;
    ufds.events=POLLIN;
    ufds.revents=0;
    int rc=poll(&ufds, 1, timeoutInSeconds);
    if(!rc)
      return 0; // timeout
    else if(rc<0 && errno==EINTR)
      continue; // interuppted
    else if(rc<0) {
      perror ("[sectionsd] poll");
//      printf("errno: %d\n", errno);
      return -1;
    }
    if(!(ufds.revents&POLLIN)) {
      // POLLHUP, beim dmx bedeutet das DMXDEV_STATE_TIMEDOUT
      // kommt wenn ein Timeout im Filter gesetzt wurde
//      dprintf("revents: 0x%hx\n", ufds.revents);
      usleep(200*1000UL); // wir warten 200 Millisekunden bevor wir es nochmal probieren
      if(timeoutInSeconds<=200)
        return 0; // timeout
      timeoutInSeconds-=200;
      continue;
    }
    int r=read (fd, buf, n-j);
    if(r>0) {
      j+=r;
      buf+=r;
    }
    else if(r<=0 && errno!=EINTR) {
//      printf("errno: %d\n", errno);
      perror ("[sectionsd] read");
      return -1;
    }
  }
  return j;
}

//------------------------------------------------------------
// class DMX<>
//------------------------------------------------------------

class DMX {
  public:
    DMX(unsigned char p, unsigned char f1, unsigned char m1, unsigned char f2, unsigned char m2, unsigned short bufferSizeInKB, int nCRC=0) {
      fd=0;
      isScheduled=false;
      lastChanged=0;
      pID=p;
      filter1=f1;
      mask1=m1;
      filter2=f2;
      mask2=m2;
      dmxBufferSizeInKB=bufferSizeInKB;
      noCRC=nCRC;
      pthread_mutex_init(&dmxlock, NULL); // default = fast mutex
    }
    ~DMX() {
      closefd();
      pthread_mutex_destroy(&dmxlock); // ist bei Linux ein dummy
    }
    int start(void); // calls unlock at end
    int read(char *buf, size_t buflength, unsigned timeoutInSeconds) {
      return readNbytes(fd, buf, buflength, timeoutInSeconds);
    }
    void closefd(void) {
      if(fd) {
        close(fd);
        fd=0;
      }
    }
/*
    int stop(void) {
      if(!fd)
        return 1;
      lock();
      closefd();
      return 0;
    }
*/
    int pause(void); // calls lock at begin
    int unpause(void); // calls unlock at end
    int change(void); // locks while changing
    void lock(void) {
      pthread_mutex_lock(&dmxlock);
    }
    void unlock(void) {
      pthread_mutex_unlock(&dmxlock);
    }
    bool isScheduled;
    time_t lastChanged;
    bool isOpen(void) {
      return fd ? true : false;
    }
  private:
    int fd;
    pthread_mutex_t dmxlock;
    unsigned char pID, filter1, mask1, filter2, mask2;
    unsigned short dmxBufferSizeInKB;
    int noCRC; // = 1 -> der 2. Filter hat keine CRC
};

int DMX::start(void)
{
  if(fd)
    return 1;
  if ((fd = open("/dev/ost/demux0", O_RDWR)) == -1) {
    perror ("[sectionsd] DMX: /dev/ost/demux0");
    return 2;
  }
  if(dmxBufferSizeInKB!=256)
    if (ioctl (fd, DMX_SET_BUFFER_SIZE, (unsigned long)(dmxBufferSizeInKB*1024UL)) == -1) {
      closefd();
      perror ("[sectionsd] DMX: DMX_SET_BUFFER_SIZE");
      return 3;
    }
  struct dmxSctFilterParams flt;
  memset (&flt, 0, sizeof (struct dmxSctFilterParams));
//  memset (&flt.filter, 0, sizeof (struct dmxFilter));
  flt.pid              = pID;
  flt.filter.filter[0] = filter1; // current/next
  flt.filter.mask[0]   = mask1; // -> 4e und 4f
  flt.flags            = DMX_IMMEDIATE_START | DMX_CHECK_CRC;

  if (ioctl (fd, DMX_SET_FILTER, &flt) == -1) {
    closefd();
    perror ("[sectionsd] DMX: DMX_SET_FILTER");
    return 4;
  }
  isScheduled=false;
  lastChanged=time(NULL);
  unlock();
  return 0;
}

int DMX::pause(void)
{
  if(!fd)
    return 1;
  lock();
  if (ioctl (fd, DMX_STOP, 0) == -1) {
    closefd();
    perror ("[sectionsd] DMX: DMX_STOP");
    return 2;
  }
  return 0;
}

int DMX::unpause(void)
{
  if(!fd)
    return 1;
  if (ioctl (fd, DMX_START, 0) == -1) {
    closefd();
    perror ("[sectionsd] DMX: DMX_START");
    unlock();
    return 2;
  }
  unlock();
  return 0;
}

int DMX::change(void)
{
  if(!fd)
    return 1;

  if(pID==0x12) // Nur bei EIT
    dprintf("changeDMX -> %s\n", isScheduled ? "current/next" : "scheduled" );
  if(pause()) // -> lock
    return 2;
  struct dmxSctFilterParams flt;
  memset (&flt, 0, sizeof (struct dmxSctFilterParams));
  if(isScheduled) {
    flt.pid              = pID;
    flt.filter.filter[0] = filter1; // current/next
    flt.filter.mask[0]   = mask1; // -> 4e und 4f
    flt.flags            = DMX_IMMEDIATE_START | DMX_CHECK_CRC;
    isScheduled=false;
  }
  else {
    flt.pid              = pID;
    flt.filter.filter[0] = filter2; // schedule
    flt.filter.mask[0]   = mask2; // -> 5x
    flt.flags            = DMX_IMMEDIATE_START;
    if(!noCRC)
      flt.flags|=DMX_CHECK_CRC;
    isScheduled=true;
  }

  if (ioctl (fd, DMX_SET_FILTER, &flt) == -1) {
    closefd();
    perror ("[sectionsd] DMX: DMX_SET_FILTER");
    unlock();
    return 3;
  }
  lastChanged=time(NULL);
  unlock();
  return 0;
}

static DMX dmxEIT(0x12, 0x4e, 0xfe, 0x50, 0xf0, 384);
static DMX dmxSDT(0x11, 0x42, 0xff, 0x42, 0xff, 256);

//------------------------------------------------------------
// misc. functions
//------------------------------------------------------------

// Liefert die ServiceID zu einem Namen
// 0 bei Misserfolg
static unsigned short findServiceIDforServiceName(const char *serviceName)
{
  SIservicePtr s(new SIservice((unsigned short)0));
  s->serviceName=serviceName;
  dprintf("Search for Service '%s'\n", serviceName);
  MySIservicesOrderServiceName::iterator si=mySIservicesOrderServiceName.find(s);
  if(si!=mySIservicesOrderServiceName.end())
    return si->first->serviceID;
  dputs("Service not found");
  return 0;
}

static const SIevent &findActualSIeventForServiceID(const unsigned serviceID, SItime& zeit)
{
  time_t azeit=time(NULL);
  // Event (serviceid) suchen
  int serviceIDfound=0;
  for(MySIeventsOrderServiceIDFirstStartTimeEventID::iterator e=mySIeventsOrderServiceIDFirstStartTimeEventID.begin(); e!=mySIeventsOrderServiceIDFirstStartTimeEventID.end(); e++)
    if(e->first->serviceID==serviceID) {
      serviceIDfound=1;
      for(SItimes::iterator t=e->first->times.begin(); t!=e->first->times.end(); t++)
        if(t->startzeit<=azeit && azeit<=(long)(t->startzeit+t->dauer)) {
          zeit=*t;
          return *(e->first);
        }
    } // if = serviceID
    else if(serviceIDfound)
      break; // sind nach serviceID und startzeit sortiert, daher weiteres Suchen unnoetig
  return nullEvt;
}

static const SIevent &findActualSIeventForServiceName(const char *serviceName, SItime& zeit)
{
  unsigned short serviceID=findServiceIDforServiceName(serviceName);
  if(serviceID)
    return findActualSIeventForServiceID(serviceID, zeit);
  return nullEvt;
}

static const SIevent &findNextSIevent(const unsigned short eventID, SItime &zeit)
{
  MySIeventsOrderEventID::iterator eFirst=mySIeventsOrderEventID.find(eventID);
  if(eFirst!=mySIeventsOrderEventID.end()) {
    if(eFirst->second->times.size()>1) {
      // Wir haben ein NVOD-Event
      // d.h. wir suchen die aktuelle Zeit und nehmen die naechste davon, falls existent
      for(SItimes::iterator t=eFirst->second->times.begin(); t!=eFirst->second->times.end(); t++)
        if(t->startzeit==zeit.startzeit) {
          t++;
          if(t!=eFirst->second->times.end()) {
            zeit=*t;
            return *(eFirst->second);
          }
          break; // ganz normal naechstes Event suchen
        }
    }
    MySIeventsOrderServiceIDFirstStartTimeEventID::iterator eNext=mySIeventsOrderServiceIDFirstStartTimeEventID.find(eFirst->second);
    eNext++;
    if(eNext!=mySIeventsOrderServiceIDFirstStartTimeEventID.end()) {
      if(eNext->second->serviceID==eFirst->second->serviceID) {
        zeit=*(eNext->second->times.begin());
        return *(eNext->second);
      }
      else
        return nullEvt;
    }
  }
  return nullEvt;
}

//*********************************************************************
//			connection-thread
// handles incoming requests
//*********************************************************************
struct connectionData {
  int connectionSocket;
  struct sockaddr_in clientAddr;
};

static void commandDumpAllServices(struct connectionData *client, char *data, unsigned dataLength)
{
  if(dataLength)
    return;
  dputs("Request of service list.\n");
  char *serviceList=new char[65*1024]; // 65kb should be enough and dataLength is unsigned short
  if(!serviceList) {
    fprintf(stderr, "low on memory!\n");
    return;
  }
  *serviceList=0;
  lockServices();
  char daten[200];
  for(MySIservicesOrderServiceName::iterator s=mySIservicesOrderServiceName.begin(); s!=mySIservicesOrderServiceName.end(); s++) {
    sprintf(daten, "%hu %hhu %d %d %d %d %u ",
      s->first->serviceID, s->first->serviceTyp,
      s->first->eitScheduleFlag(), s->first->eitPresentFollowingFlag(),
      s->first->runningStatus(), s->first->freeCAmode(),
      s->first->nvods.size());
    strcat(serviceList, daten);
    strcat(serviceList, "\n");
    strcat(serviceList, s->first->serviceName.c_str());
    strcat(serviceList, "\n");
    strcat(serviceList, s->first->providerName.c_str());
    strcat(serviceList, "\n");
  }
  unlockServices();
  struct msgSectionsdResponseHeader msgResponse;
  msgResponse.dataLength=strlen(serviceList)+1;
  if(msgResponse.dataLength==1)
    msgResponse.dataLength=0;
  write(client->connectionSocket, &msgResponse, sizeof(msgResponse));
  if(msgResponse.dataLength)
    write(client->connectionSocket, serviceList, msgResponse.dataLength);
  delete[] serviceList;
  return;
}

static void commandSetEventsAreOldInMinutes(struct connectionData *client, char *data, unsigned dataLength)
{
  if(dataLength!=2)
    return;
  dprintf("Set events are old after minutes: %hd\n", *((unsigned short*)data));
  oldEventsAre=*((unsigned short*)data)*60L;
  struct msgSectionsdResponseHeader responseHeader;
  responseHeader.dataLength=0;
  write(client->connectionSocket, &responseHeader, sizeof(responseHeader));
  return;
}

static void commandSetHoursToCache(struct connectionData *client, char *data, unsigned dataLength)
{
  if(dataLength!=2)
    return;
  dprintf("Set hours to cache: %hd\n", *((unsigned short*)data));
  secondsToCache=*((unsigned short*)data)*60L*60L;
  struct msgSectionsdResponseHeader responseHeader;
  responseHeader.dataLength=0;
  write(client->connectionSocket, &responseHeader, sizeof(responseHeader));
  return;
}

static void commandAllEventsChannelName(struct connectionData *client, char *data, unsigned dataLength)
{
  data[dataLength-1]=0; // to be sure it has an trailing 0
  dprintf("Request of all events for '%s'\n", data);
  lockServices();
  unsigned short serviceID=findServiceIDforServiceName(data);
  unlockServices();
  char *evtList=new char[65*1024]; // 65kb should be enough and dataLength is unsigned short
  if(!evtList) {
    fprintf(stderr, "low on memory!\n");
    return;
  }
  *evtList=0;
  if(serviceID!=0) {
    // service Found
    if(dmxEIT.pause()) {
      delete[] evtList;
      return;
    }
    lockEvents();
    int serviceIDfound=0;
    for(MySIeventsOrderServiceIDFirstStartTimeEventID::iterator e=mySIeventsOrderServiceIDFirstStartTimeEventID.begin(); e!=mySIeventsOrderServiceIDFirstStartTimeEventID.end(); e++)
      if(e->first->serviceID==serviceID) {
        serviceIDfound=1;
        char strZeit[50];
        struct tm *tmZeit;
        tmZeit=localtime(&(e->first->times.begin()->startzeit));
        sprintf(strZeit, "%02d.%02d %02d:%02d %u ",
          tmZeit->tm_mday, tmZeit->tm_mon+1, tmZeit->tm_hour, tmZeit->tm_min, e->first->times.begin()->dauer/60);
        strcat(evtList, strZeit);
        strcat(evtList, e->first->name.c_str());
        strcat(evtList, "\n");
      } // if = serviceID
      else if(serviceIDfound)
        break; // sind nach serviceID und startzeit sortiert -> nicht weiter suchen
    unlockEvents();
    if(dmxEIT.unpause()) {
      delete[] evtList;
      return;
    }
  }
  struct msgSectionsdResponseHeader responseHeader;
  responseHeader.dataLength=strlen(evtList)+1;
  write(client->connectionSocket, &responseHeader, sizeof(responseHeader));
  if(responseHeader.dataLength)
    write(client->connectionSocket, evtList, responseHeader.dataLength);
  delete[] evtList;
  return;
}

static void commandDumpStatusInformation(struct connectionData *client, char *data, unsigned dataLength)
{
  if(dataLength)
    return;
  dputs("Request of status information");
  lockEvents();
  unsigned anzEvents=mySIeventsOrderEventID.size();
  unsigned anzNVODevents=mySIeventsNVODorderEventID.size();
  unsigned anzMetaServices=mySIeventIDsMetaOrderServiceID.size();
  unlockEvents();
  lockServices();
  unsigned anzServices=mySIservicesOrderServiceID.size();
  unsigned anzNVODservices=mySIservicesNVODorderServiceID.size();
//  unsigned anzServices=services.size();
  unlockServices();
  struct mallinfo speicherinfo=mallinfo();
  time_t zeit=time(NULL);
  char stati[1024];
  sprintf(stati,
    "Current time: %s"
    "Hours to cache: %ld\n"
    "Events are old %ldmin after their end time\n"
    "Number of cached services: %u\n"
    "Number of cached nvod-services: %u\n"
    "Number of cached events: %u\n"
    "Number of cached nvod-events: %u\n"
    "Number of cached meta-services: %u\n"
    "Total size of memory occupied by chunks handed out by malloc: %d\n"
    "Total bytes memory allocated with `sbrk' by malloc, in bytes: %d (%dkb, %.2fMB)\n",
    ctime(&zeit),
    secondsToCache/(60*60L), oldEventsAre/60, anzServices, anzNVODservices, anzEvents, anzNVODevents, anzMetaServices, speicherinfo.uordblks,
    speicherinfo.arena, speicherinfo.arena/1024, (float)speicherinfo.arena/(1024.*1024.)
    );
  struct msgSectionsdResponseHeader responseHeader;
  responseHeader.dataLength=strlen(stati)+1;
  write(client->connectionSocket, &responseHeader, sizeof(responseHeader));
  if(responseHeader.dataLength)
    write(client->connectionSocket, stati, responseHeader.dataLength);
  return;
}

#ifdef NO_ZAPD_NEUTRINO_HACK
static int currentNextWasOk=0;
#endif

// Mostly copied from epgd (something bugfixed ;) )
static void commandCurrentNextInfoChannelName(struct connectionData *client, char *data, unsigned dataLength)
{
  int nResultDataSize=0;
  char* pResultData=0;

  data[dataLength-1]=0; // to be sure it has an trailing 0
  dprintf("Request of current/next information for '%s'\n", data);

  if(dmxEIT.pause()) // -> lock
    return;
  lockEvents();
  lockServices();
  SItime zeitEvt1(0, 0);
  const SIevent &evt=findActualSIeventForServiceName(data, zeitEvt1);
  unlockServices();
  if(evt.serviceID!=0) {//Found
    dprintf("current EPG found.\n");
    SItime zeitEvt2(zeitEvt1);
    const SIevent &nextEvt=findNextSIevent(evt.eventID, zeitEvt2);
    if(nextEvt.serviceID!=0) {
      dprintf("next EPG found.\n");
      // Folgendes ist grauenvoll, habs aber einfach kopiert aus epgd
      // und keine Lust das grossartig zu verschoenern
      nResultDataSize=
        strlen(evt.name.c_str())+1+		//Name + del
        3+2+1+					//std:min + del
        4+1+					//dauer (mmmm) + del
        3+1+					//100 + del
        strlen(nextEvt.name.c_str())+1+		//Name + del
        3+2+1+					//std:min + del
        4+1+1;					//dauer (mmmm) + del + 0
      pResultData = new char[nResultDataSize];
      struct tm *pStartZeit = localtime(&zeitEvt1.startzeit);
      int nSH(pStartZeit->tm_hour), nSM(pStartZeit->tm_min);
      unsigned dauer=zeitEvt1.dauer/60;
      unsigned nProcentagePassed=(unsigned)((float)(time(NULL)-zeitEvt1.startzeit)/(float)zeitEvt1.dauer*100.);

      pStartZeit = localtime(&zeitEvt2.startzeit);
      int nSH2(pStartZeit->tm_hour), nSM2(pStartZeit->tm_min);
      unsigned dauer2=zeitEvt2.dauer/60;

      sprintf(pResultData,
      "%s\n%02d:%02d\n%04u\n%03u\n%s\n%02d:%02d\n%04u\n",
        evt.name.c_str(),
        nSH, nSM, dauer, nProcentagePassed,
        nextEvt.name.c_str(),
        nSH2, nSM2, dauer2);
    }
  }
  unlockEvents();
  dmxEIT.unpause(); // -> unlock

  // response
  struct msgSectionsdResponseHeader pmResponse;
  pmResponse.dataLength=nResultDataSize;
  write(client->connectionSocket, &pmResponse, sizeof(pmResponse));
  if( nResultDataSize > 0 ) {
    write(client->connectionSocket, pResultData, nResultDataSize);
    delete[] pResultData;
#ifdef NO_ZAPD_NEUTRINO_HACK
    currentNextWasOk=1;
#endif
  }
  else
    dprintf("current/next EPG not found!\n");
}

// Mostly copied from epgd (something bugfixed ;) )
static void commandActualEPGchannelName(struct connectionData *client, char *data, unsigned dataLength)
{
  int nResultDataSize=0;
  char* pResultData=0;

  data[dataLength-1]=0; // to be sure it has an trailing 0
  dprintf("Request of actual EPG for '%s'\n", data);

  if(dmxEIT.pause()) // -> lock
    return;
  lockEvents();
  lockServices();
  SItime zeitEvt(0,0);
  const SIevent &evt=findActualSIeventForServiceName(data, zeitEvt);
  unlockServices();
  if(evt.serviceID!=0) { //Found
    dprintf("EPG found.\n");
    nResultDataSize=strlen(evt.name.c_str())+1+		//Name + del
      strlen(evt.text.c_str())+1+		//Text + del
      strlen(evt.extendedText.c_str())+1+	//ext + del
      3+3+4+1+					//dd.mm.yyyy + del
      3+2+1+					//std:min + del
      3+2+1+					//std:min+ del
      3+1+1;					//100 + del + 0
    pResultData = new char[nResultDataSize];
    struct tm *pStartZeit = localtime(&zeitEvt.startzeit);
    int nSDay(pStartZeit->tm_mday), nSMon(pStartZeit->tm_mon+1), nSYear(pStartZeit->tm_year+1900),
     nSH(pStartZeit->tm_hour), nSM(pStartZeit->tm_min);

    long int uiEndTime(zeitEvt.startzeit+zeitEvt.dauer);
    struct tm *pEndeZeit = localtime((time_t*)&uiEndTime);
    int nFH(pEndeZeit->tm_hour), nFM(pEndeZeit->tm_min);

    unsigned nProcentagePassed=(unsigned)((float)(time(NULL)-zeitEvt.startzeit)/(float)zeitEvt.dauer*100.);

    sprintf(pResultData, "%s\xFF%s\xFF%s\xFF%02d.%02d.%04d\xFF%02d:%02d\xFF%02d:%02d\xFF%03u\xFF",
      evt.name.c_str(),
      evt.text.c_str(),
      evt.extendedText.c_str(), nSDay, nSMon, nSYear, nSH, nSM, nFH, nFM, nProcentagePassed );
  }
  else
    dprintf("actual EPG not found!\n");
  unlockEvents();
  dmxEIT.unpause(); // -> unlock

  // response
  struct msgSectionsdResponseHeader pmResponse;
  pmResponse.dataLength=nResultDataSize;
  write(client->connectionSocket, &pmResponse, sizeof(pmResponse));
  if( nResultDataSize > 0 ) {
    write(client->connectionSocket, pResultData, nResultDataSize);
    delete[] pResultData;
  }
}

static void sendEventList(struct connectionData *client, unsigned char serviceTyp1, unsigned char serviceTyp2=0)
{
  char *evtList=new char[65*1024]; // 65kb should be enough and dataLength is unsigned short
  if(!evtList) {
    fprintf(stderr, "low on memory!\n");
    return;
  }
  *evtList=0;
  if(dmxEIT.pause()) { // -> lock
    delete[] evtList;
    return;
  }
  lockServices();
  lockEvents();
  for(MySIservicesOrderServiceName::iterator s=mySIservicesOrderServiceName.begin(); s!=mySIservicesOrderServiceName.end(); s++)
    if(s->first->serviceTyp==serviceTyp1 || (serviceTyp2 && s->first->serviceTyp==serviceTyp2)) {
      SItime zeit(0, 0);
      const SIevent &evt=findActualSIeventForServiceID(s->first->serviceID, zeit);
      strcat(evtList, s->first->serviceName.c_str());
      strcat(evtList, "\n");
      if(evt.serviceID!=0)
        //Found
        strcat(evtList, evt.name.c_str());
      strcat(evtList, "\n");
    } // if TV
  unlockEvents();
  unlockServices();
  dmxEIT.unpause(); // -> unlock
  struct msgSectionsdResponseHeader msgResponse;
  msgResponse.dataLength=strlen(evtList)+1;
  if(msgResponse.dataLength==1)
    msgResponse.dataLength=0;
  write(client->connectionSocket, &msgResponse, sizeof(msgResponse));
  if(msgResponse.dataLength)
    write(client->connectionSocket, evtList, msgResponse.dataLength);
  delete[] evtList;
}

static void commandEventListTV(struct connectionData *client, char *data, unsigned dataLength)
{
  if(dataLength)
    return;
  dputs("Request of TV event list.\n");
  sendEventList(client, 0x01, 0x04);
  return;
}

static void commandEventListRadio(struct connectionData *client, char *data, unsigned dataLength)
{
  if(dataLength)
    return;
  dputs("Request of radio event list.\n");
  sendEventList(client, 0x02);
  return;
}

static void (*connectionCommands[NUMBER_OF_SECTIONSD_COMMANDS]) (struct connectionData *, char *, unsigned)  = {
  commandActualEPGchannelName,
  commandEventListTV,
  commandCurrentNextInfoChannelName,
  commandDumpStatusInformation,
  commandAllEventsChannelName,
  commandSetHoursToCache,
  commandSetEventsAreOldInMinutes,
  commandDumpAllServices,
  commandEventListRadio
};

static void *connectionThread(void *conn)
{
struct connectionData *client=(struct connectionData *)conn;

  dprintf("Connection from %s\n", inet_ntoa(client->clientAddr.sin_addr));
  struct msgSectionsdRequestHeader header;
  memset(&header, 0, sizeof(header));

  if(readNbytes(client->connectionSocket, (char *)&header, sizeof(header) , 2)>0) {
    dprintf("version: %hhd, cmd: %hhd\n", header.version, header.command);
    if(header.version==2 && header.command<NUMBER_OF_SECTIONSD_COMMANDS) {
      dprintf("data length: %hd\n", header.dataLength);
      char *data=new char[header.dataLength+1];
      if(!data)
        fprintf(stderr, "low on memory!\n");
      else {
        int rc=1;
        if(header.dataLength)
	  rc=readNbytes(client->connectionSocket, data, header.dataLength, 2);
        if(rc>0) {
          dprintf("Starting command %hhd\n", header.command);
          connectionCommands[header.command](client, data, header.dataLength);
        }
        delete[] data;
      }
    }
    else
      dputs("Unknow format or version of request!");
  }
  close(client->connectionSocket);
  dprintf("Connection from %s closed!\n", inet_ntoa(client->clientAddr.sin_addr));
  delete client;
#ifdef NO_ZAPD_NEUTRINO_HACK
  if(currentNextWasOk) {
    // Damit nach dem umschalten der camd/pzap usw. schneller anlaeuft.
    currentNextWasOk=0;
    if(dmxEIT.pause()) // -> lock
      return 0;
    if(dmxSDT.pause()) {
      dmxEIT.unpause(); // -> unlock
      return 0;
    }
    int rc=5;
    while(rc)
      rc=sleep(rc);
    dmxSDT.unpause();
    dmxEIT.unpause(); // -> unlock
  }
  else if(dmxEIT.isScheduled) {
    dmxEIT.change(); // auf present/following umschalten
  }
#endif
  return 0;
}

//*********************************************************************
//			sdt-thread
// reads sdt for service list
//*********************************************************************
static void *sdtThread(void *)
{
struct SI_section_header header;
char *buf;
const unsigned timeoutInSeconds=2;

  dprintf("sdt-thread started.\n");
  dmxSDT.lock();
  if(dmxSDT.start()) // -> unlock
    return 0;
  for(;;) {
    dmxSDT.lock();
    int rc=dmxSDT.read((char *)&header, sizeof(header), timeoutInSeconds);
    if(!rc) {
      dmxSDT.unlock();
      dputs("dmxSDT.read timeout");
      continue; // timeout -> kein EPG
    }
    else if(rc<0) {
      dmxSDT.unlock();
      // DMX neu starten
      dmxSDT.pause(); // -> lock
      dmxSDT.unpause(); // -> unlock
      continue;
    }
    buf=new char[sizeof(header)+header.section_length-5];
    if(!buf) {
      dmxSDT.unlock();
      fprintf(stderr, "Not enough memory!\n");
      break;
    }
    // Den Header kopieren
    memcpy(buf, &header, sizeof(header));
    rc=dmxSDT.read(buf+sizeof(header), header.section_length-5, timeoutInSeconds);
    dmxSDT.unlock();
    if(!rc) {
      delete[] buf;
      dputs("dmxSDT.read timeout after header");
      // DMX neu starten, noetig, da bereits der Header gelesen wurde
      dmxSDT.pause(); // -> lock
      dmxSDT.unpause(); // -> unlock
      continue; // timeout -> kein EPG
    }
    else if(rc<0) {
      delete[] buf;
      // DMX neu starten
      dmxSDT.pause(); // -> lock
      dmxSDT.unpause(); // -> unlock
      continue;
    }
    if(header.current_next_indicator) {
      // Wir wollen nur aktuelle sections
      SIsectionSDT sdt(SIsection(sizeof(header)+header.section_length-5, buf));
      lockServices();
      for(SIservices::iterator s=sdt.services().begin(); s!=sdt.services().end(); s++)
        addService(*s);
      unlockServices();
    } // if
    else
      delete[] buf;
  } // for
  dmxSDT.closefd();
  dprintf("sdt-thread ended\n");
  return 0;
}

//*********************************************************************
//			Time-thread
// updates system time according TOT every 30 minutes
//*********************************************************************
struct SI_section_TOT_header {
      unsigned char table_id : 8;
      // 1 byte
      unsigned char section_syntax_indicator : 1;
      unsigned char reserved_future_use : 1;
      unsigned char reserved1 : 2;
      unsigned short section_length : 12;
      // 3 bytes
      unsigned long long UTC_time : 40;
      // 8 bytes
      unsigned char reserved2 : 4;
      unsigned short descriptors_loop_length : 12;
} __attribute__ ((packed)) ; // 10 bytes

struct SI_section_TDT_header {
      unsigned char table_id : 8;
      // 1 byte
      unsigned char section_syntax_indicator : 1;
      unsigned char reserved_future_use : 1;
      unsigned char reserved1 : 2;
      unsigned short section_length : 12;
      // 3 bytes
      unsigned long long UTC_time : 40;
} __attribute__ ((packed)) ; // 10 bytes

/*
// BR schickt falschen Time-Offset, daher per TZ und Rest hier auskommentiert

struct descr_gen_struct {
  unsigned char descriptor_tag : 8;
  unsigned char descriptor_length : 8;
} __attribute__ ((packed)) ;

struct local_time_offset {
  char country_code1 : 8;
  char country_code2 : 8;
  char country_code3 : 8;
  unsigned char country_region_id : 6;
  unsigned char reserved : 1;
  unsigned char local_time_offset_polarity : 1;
  unsigned short local_time_offs : 16;
  unsigned long long time_of_chng : 40;
  unsigned short next_time_offset : 8;
} __attribute__ ((packed)) ;

static int timeOffsetMinutes=0; // minutes
static int timeOffsetFound=0;

static void parseLocalTimeOffsetDescriptor(const char *buf, const char *countryCode)
{
  struct descr_gen_struct *desc=(struct descr_gen_struct *)buf;
  buf+=2;
  while(buf<((char *)desc)+2+desc->descriptor_length-sizeof(struct local_time_offset)) {
    struct local_time_offset *lto=(struct local_time_offset *)buf;
    if(!strncmp(countryCode, buf, 3)) {
      timeOffsetMinutes=(((lto->local_time_offs)>>12)&0x0f)*10*60L+(((lto->local_time_offs)>>8)&0x0f)*60L+
	(((lto->local_time_offs)>>4)&0x0f)*10+((lto->local_time_offs)&0x0f);
      if(lto->local_time_offset_polarity)
        timeOffsetMinutes=-timeOffsetMinutes;
      timeOffsetFound=1;
      break;
    }
//    else
//      printf("Code: %c%c%c\n", lto->country_code1, lto->country_code2, lto->country_code3);
    buf+=sizeof(struct local_time_offset);
  }
}

static void parseDescriptors(const char *des, unsigned len, const char *countryCode)
{
  struct descr_gen_struct *desc;
  while(len>=sizeof(struct descr_gen_struct)) {
    desc=(struct descr_gen_struct *)des;
    if(desc->descriptor_tag==0x58) {
//      printf("Found time descriptor\n");
      parseLocalTimeOffsetDescriptor((const char *)desc, countryCode);
      if(timeOffsetFound)
        break;
    }
    len-=desc->descriptor_length+2;
    des+=desc->descriptor_length+2;
  }
}

*/
static void *timeThread(void *)
{
const unsigned timeoutInSeconds=31;
char *buf;
DMX dmxTOT(0x14, 0x73, 0xff, 0x70, 0xff, 256, 1);

//  pthread_detach(pthread_self());
  dprintf("time-thread started.\n");
  dmxTOT.lock();
  // Zuerst per TDT (schneller)
  if(dmxTOT.start()) // -> unlock
    return 0;
  if(dmxTOT.change()) // von TOT nach TDT wechseln
    return 0;
  struct SI_section_TDT_header tdt_header;
  int rc=dmxTOT.read((char *)&tdt_header, sizeof(tdt_header), timeoutInSeconds);
  if(rc>0) {
    time_t tim=changeUTCtoCtime(((const unsigned char *)&tdt_header)+3);
    if(tim) {
      if(stime(&tim)< 0) {
        perror("[sectionsd] cannot set date");
	dmxTOT.closefd();
        return 0;
      }
      timeset=1;
      time_t t=time(NULL);
      dprintf("local time: %s", ctime(&t));
    }
  }
  if(dmxTOT.change()) // von TDT nach TOT wechseln
    return 0;
  // Jetzt wird die Uhrzeit nur noch per TOT gesetzt (CRC)
  for(;;) {
    if(!dmxTOT.isOpen()) {
      dmxTOT.lock();
      if(dmxTOT.start()) // -> unlock
        return 0;
    }
    struct SI_section_TOT_header header;
    int rc=dmxTOT.read((char *)&header, sizeof(header), timeoutInSeconds);
    if(!rc) {
      dputs("dmxTOT.read timeout");
      continue; // timeout -> keine Zeit
    }
    else if(rc<0) {
      dmxTOT.closefd();
      break;
    }
    buf=new char[sizeof(header)+header.section_length-5];
    if(!buf) {
      fprintf(stderr, "Not enough memory!\n");
      dmxTOT.closefd();
      break;
    }
    // Den Header kopieren
    memcpy(buf, &header, sizeof(header));
    rc=dmxTOT.read(buf+sizeof(header), header.section_length-5, timeoutInSeconds);
    delete[] buf;
    if(!rc) {
      dputs("dmxTOT.read timeout after header");
      // DMX neu starten, noetig, da bereits der Header gelesen wurde
      dmxTOT.pause(); // -> lock
      dmxTOT.unpause(); // -> unlock
      continue; // timeout -> kein TDT
    }
    else if(rc<0) {
      dmxTOT.closefd();
      break;
    }
    time_t tim=changeUTCtoCtime(((const unsigned char *)&header)+3);
    if(tim) {
//      timeOffsetFound=0;
//      parseDescriptors(buf+sizeof(struct SI_section_TOT_header), ((struct SI_section_TOT_header *)buf)->descriptors_loop_length, "DEU");
//      printf("local time: %s", ctime(&tim));
//      printf("Time offset %d", timeOffsetMinutes);
//      if(timeOffsetFound)
//        tim+=timeOffsetMinutes*60L;
      if(stime(&tim)< 0) {
        perror("[sectionsd] cannot set date");
    	dmxTOT.closefd();
	break;
      }
      timeset=1;
      time_t t=time(NULL);
      dprintf("local time: %s", ctime(&t));
    }
    dmxTOT.closefd();
    if(timeset)
      rc=60*30;  // sleep 30 minutes
    else
      rc=60;  // sleep 1 minute
    while(rc)
      rc=sleep(rc);
  } // for
  dprintf("time-thread ended\n");
  return 0;
}

//*********************************************************************
//			EIT-thread
// reads EPG-datas
//*********************************************************************
static void *eitThread(void *)
{
struct SI_section_header header;
char *buf;
const unsigned timeoutInSeconds=2;

  dprintf("eit-thread started.\n");
  dmxEIT.lock();
  if(dmxEIT.start()) // -> unlock
    return 0;
  for(;;) {
    time_t zeit=time(NULL);
    if(dmxEIT.isScheduled) {
      if(zeit>dmxEIT.lastChanged+TIME_EIT_SCHEDULED)
        dmxEIT.change(); // -> lock, unlock
    }
    else if(zeit>dmxEIT.lastChanged+TIME_EIT_PRESENT)
      dmxEIT.change(); // -> lock, unlock
    dmxEIT.lock();
    int rc=dmxEIT.read((char *)&header, sizeof(header), timeoutInSeconds);
    if(!rc) {
      dmxEIT.unlock();
      dputs("dmxEIT.read timeout");
      continue; // timeout -> kein EPG
    }
    else if(rc<0) {
      dmxEIT.unlock();
      // DMX neu starten
      dmxEIT.pause();
      dmxEIT.unpause();
      continue;
    }
    buf=new char[sizeof(header)+header.section_length-5];
    if(!buf) {
      dmxEIT.closefd();
      dmxEIT.unlock();
      fprintf(stderr, "Not enough memory!\n");
      break;
    }
    // Den Header kopieren
    memcpy(buf, &header, sizeof(header));
    rc=dmxEIT.read(buf+sizeof(header), header.section_length-5, timeoutInSeconds);
    dmxEIT.unlock();
    if(!rc) {
      delete[] buf;
      dputs("dmxEIT.read timeout after header");
      // DMX neu starten, noetig, da bereits der Header gelesen wurde
      dmxEIT.pause(); // -> lock
      dmxEIT.unpause(); // -> unlock
      continue; // timeout -> kein EPG
    }
    else if(rc<0) {
      delete[] buf;
      // DMX neu starten
      dmxEIT.pause(); // -> lock
      dmxEIT.unpause(); // -> unlock
      continue;
    }
    if(header.current_next_indicator) {
      // Wir wollen nur aktuelle sections
      SIsectionEIT eit(SIsection(sizeof(header)+header.section_length-5, buf));
      zeit=time(NULL);
      // Nicht alle Events speichern
      for(SIevents::iterator e=eit.events().begin(); e!=eit.events().end(); e++)
        if(e->times.size()>0) {
          if(e->times.begin()->startzeit < zeit+secondsToCache &&
            e->times.begin()->startzeit+(long)e->times.begin()->dauer > zeit-oldEventsAre
            ) {
            lockEvents();
            addEvent(*e);
            unlockEvents();
          }
        }
        else {
          // pruefen ob nvod event
          lockServices();
          MySIservicesNVODorderServiceID::iterator si=mySIservicesNVODorderServiceID.find(e->serviceID);
          if(si!=mySIservicesNVODorderServiceID.end()) {
            // Ist ein nvod-event
            lockEvents();
            for(SInvodReferences::iterator i=si->second->nvods.begin(); i!=si->second->nvods.end(); i++)
              mySIeventIDsMetaOrderServiceID.insert(std::make_pair(i->serviceID, e->eventID));
            unlockServices();
            addNVODevent(*e);
            unlockEvents();
          }
          else
            unlockServices();
        }
    } // if
    else
      delete[] buf;
  } // for
  dprintf("eit-thread ended\n");
  return 0;
}

//*********************************************************************
//			housekeeping-thread
// does cleaning on fetched datas
//*********************************************************************
static void *houseKeepingThread(void *)
{
  dprintf("housekeeping-thread started.\n");
  for(;;) {
    int rc=5*60;  // sleep 2 minutes
    while(rc)
      rc=sleep(rc);
    dprintf("housekeeping.\n");
/*
    if(stopDMXeit())
      return 0;
    if(stopDMXsdt())
      return 0;
*/
    if(debug) {
      // Speicher-Info abfragen
      struct mallinfo speicherinfo=mallinfo();
      dprintf("total size of memory occupied by chunks handed out by malloc: %d\n", speicherinfo.uordblks);
      dprintf("total bytes memory allocated with `sbrk' by malloc, in bytes: %d (%dkb, %.2fMB)\n",speicherinfo.arena, speicherinfo.arena/1024, (float)speicherinfo.arena/(1024.*1024));
    }
    lockEvents();
//    unsigned anzEventsAlt=events.size();
/*
    lockServices();
    events.mergeAndRemoveTimeShiftedEvents(services);
    unlockServices();
    if(events.size()!=anzEventsAlt)
      printf("Removed %d time-shifted events.\n", anzEventsAlt-events.size());
*/
    unsigned anzEventsAlt=mySIeventsOrderEventID.size();
    removeOldEvents(oldEventsAre); // alte Events
    if(mySIeventsOrderEventID.size()!=anzEventsAlt)
      dprintf("Removed %d old events.\n", anzEventsAlt-mySIeventsOrderEventID.size());
    dprintf("Number of sptr events (event-ID): %u\n", mySIeventsOrderEventID.size());
    dprintf("Number of sptr events (service-id, start time, event-id): %u\n", mySIeventsOrderServiceIDFirstStartTimeEventID.size());
    dprintf("Number of sptr events (end time, service-id, event-id): %u\n", mySIeventsOrderFirstEndTimeServiceIDEventID.size());
    dprintf("Number of sptr nvod events (event-ID): %u\n", mySIeventsNVODorderEventID.size());
    dprintf("Number of cached meta-services: %u\n", mySIeventIDsMetaOrderServiceID.size());
    unlockEvents();
    if(debug) {
      lockServices();
      dprintf("Number of services: %u\n", mySIservicesOrderServiceID.size());
      dprintf("Number of cached nvod-services: %u\n", mySIservicesNVODorderServiceID.size());
//      dprintf("Number of services: %u\n", services.size());
      unlockServices();
    }
/*
    if(startDMXsdt())
      return 0;
    if(startDMXeit())
      return 0;
*/
    if(debug) {
      // Speicher-Info abfragen
      struct mallinfo speicherinfo=mallinfo();
      dprintf("total size of memory occupied by chunks handed out by malloc: %d\n", speicherinfo.uordblks);
      dprintf("total bytes memory allocated with `sbrk' by malloc, in bytes: %d (%dkb, %.2fMB)\n",speicherinfo.arena, speicherinfo.arena/1024, (float)speicherinfo.arena/(1024.*1024));
    }
  } // for endlos
}

static void printHelp(void)
{
    printf("\nUsage: sectionsd [-d]\n\n");
}

int main(int argc, char **argv)
{
pthread_t threadTOT, threadEIT, threadSDT, threadHouseKeeping;
int rc;
int listenSocket;
struct sockaddr_in serverAddr;

  printf("$Id: sectionsd.cpp,v 1.33 2001/07/24 15:05:31 fnbrd Exp $\n");

  if(argc!=1 && argc!=2) {
    printHelp();
    return 1;
  }
  if(argc==2) {
    if(!strcmp(argv[1], "-d"))
      debug=1;
    else {
      printHelp();
      return 1;
    }
  }
  printf("caching %ld hours\n", secondsToCache/(60*60L));
  printf("events are old %ldmin after their end time\n", oldEventsAre/60);
  tzset(); // TZ auswerten

  if( fork()!= 0 ) // switching to background
    return 0;

  // from here on forked

  // den Port für die Clients öffnen
  listenSocket = socket(AF_INET, SOCK_STREAM, 0);
  memset( &serverAddr, 0, sizeof(serverAddr) );
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serverAddr.sin_port = htons(PORT_NUMBER);
  if(bind(listenSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr) )) {
    perror("[sectionsd] bind");
    return 2;
  }
  if(listen(listenSocket, 5)) { // max. 5 Clients
    perror("[sectionsd] listen");
    return 3;
  }

  // SDT-Thread starten

  rc=pthread_create(&threadSDT, 0, sdtThread, 0);
  if(rc) {
    fprintf(stderr, "failed to create sdt-thread (rc=%d)\n", rc);
    return 1;
  }

  // EIT-Thread starten
  rc=pthread_create(&threadEIT, 0, eitThread, 0);
  if(rc) {
    fprintf(stderr, "failed to create eit-thread (rc=%d)\n", rc);
    return 1;
  }
  // time-Thread starten
  rc=pthread_create(&threadTOT, 0, timeThread, 0);
  if(rc) {
    fprintf(stderr, "failed to create time-thread (rc=%d)\n", rc);
    return 1;
  }

  // housekeeping-Thread starten
  rc=pthread_create(&threadHouseKeeping, 0, houseKeepingThread, 0);
  if(rc) {
    fprintf(stderr, "failed to create houskeeping-thread (rc=%d)\n", rc);
    return 1;
  }

  pthread_attr_t conn_attrs;
  pthread_attr_init(&conn_attrs);
  pthread_attr_setdetachstate(&conn_attrs, PTHREAD_CREATE_DETACHED);
  // Unsere Endlosschliefe
  socklen_t clientInputLen = sizeof(serverAddr);
  for(;;) {
    // wir warten auf eine Verbindung
    struct connectionData *client=new connectionData; // Wird vom Thread freigegeben
    do {
      client->connectionSocket = accept(listenSocket, (struct sockaddr *)&(client->clientAddr), &clientInputLen);
    } while(client->connectionSocket == -1);
    pthread_t threadConnection;
    rc=pthread_create(&threadConnection, &conn_attrs, connectionThread, client);
    if(rc) {
      fprintf(stderr, "failed to create connection-thread (rc=%d)\n", rc);
      return 4;
    }
  }
  printf("sectionsd ended\n");
  return 0;
}
