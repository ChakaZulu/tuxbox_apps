//
//  $Id: sectionsd.cpp,v 1.135 2002/10/04 16:31:25 thegoodguy Exp $
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
//  Revision 1.135  2002/10/04 16:31:25  thegoodguy
//  Bugfix: In the event of a timeout with part of the section read, the eitthread should use the correct commands to restart the dmx, too
//
//  Revision 1.134  2002/10/02 21:00:15  thegoodguy
//  Only read 3 bytes of eit table first, fixed memory leak & locking problem
//
//  Revision 1.133  2002/10/02 17:52:49  thegoodguy
//  Make driver developers happy :)
//
//  Revision 1.132  2002/10/02 17:45:30  thegoodguy
//  Bugfixes: Fix handling of "small" sections & implement own table_id filter (joint effort with the sedulous alexw)
//
//  Revision 1.131  2002/09/25 23:27:48  thegoodguy
//  Tiny code cleanup
//
//  Revision 1.130  2002/09/24 22:29:06  thegoodguy
//  Code cleanup (kick out onid_sid)
//
//  Revision 1.129  2002/09/20 00:05:55  thegoodguy
//  Fixed potential deadlock
//
//  Revision 1.128  2002/08/27 19:00:45  obi
//  use devfs device names
//
//  Revision 1.127  2002/08/17 06:27:25  obi
//  - no more compiler warnings
//  - formatted sourcecode, it's almost readable now...
//
//  Revision 1.126  2002/05/15 10:00:55  dirch
//  timerd removed
//
//  Revision 1.125  2002/05/04 00:14:51  rasc
//  -- default cache 21 days, processor load should be no problem
//
//  Revision 1.124  2002/04/27 10:48:07  field
//  Geschindigkeit gefixt
//
//  Revision 1.123  2002/04/25 16:17:10  field
//  Updates
//
//  Revision 1.122  2002/04/24 10:56:46  field
//  Kleinigkeiten
//
//  Revision 1.121  2002/04/19 07:04:44  field
//  Zeit-setzen verbessert
//
//  Revision 1.120  2002/04/18 13:09:53  field
//  Sectionsd auf clientlib umgestellt :)
//
//  Revision 1.119  2002/04/18 10:43:56  field
//  Clientlib
//
//  Revision 1.118  2002/04/17 15:58:24  field
//  Anpassungen
//
//  Revision 1.117  2002/04/17 13:07:06  field
//  Bugfix (current-event)
//
//  Revision 1.116  2002/04/16 13:03:26  field
//  stime in neutrino verschoben
//
//  Revision 1.115  2002/04/16 11:23:50  field
//  Timeout-Handling umgestellt
//
//  Revision 1.114  2002/04/15 12:33:44  field
//  Wesentlich verbessertes Paket-Handling (CPU-Last sollte viel besser sein
//  *g*)
//
//  Revision 1.113  2002/04/12 15:47:27  field
//  laestigen Bug in der glibc2.2.5 umschifft
//
//  Revision 1.112  2002/04/10 16:40:46  field
//  Timeset verändert, timerd (einstweilen) auskommentiert
//
//  Revision 1.111  2002/04/08 18:46:06  Simplex
//  checks availability of timerd
//
//  Revision 1.110  2002/04/06 20:01:50  Simplex
//  - made EVT_NEXTPROGRAM work
//  - communication with timerd should be changed to eventserver
//
//  Revision 1.109  2002/03/29 15:47:18  obi
//  use setsid()
//  field, mcclean: i guess that is what you wanted instead of catching signals ;)
//
//  Revision 1.108  2002/03/28 08:49:14  obi
//  sigkill cannot be caught
//  exit on all signals except sighup
//
//  Revision 1.107  2002/03/22 17:12:06  field
//  Weitere Updates, compiliert wieder
//
//  Revision 1.104  2002/03/18 16:55:16  field
//  Bugfix
//
//  Revision 1.102  2002/03/12 16:12:55  field
//  Bugfixes
//
//  Revision 1.101  2002/03/07 18:33:43  field
//  ClientLib angegangen, Events angefangen
//
//  Revision 1.100  2002/02/28 01:52:21  field
//  Verbessertes Umschalt-Handling
//
//  Revision 1.99  2002/02/23 21:08:43  field
//  Bugfix des Bugfixes :)
//
//  Revision 1.97  2002/02/23 20:23:23  McClean
//  fix up
//
//  Revision 1.96  2002/02/14 14:59:24  field
//  CD-Fix
//
//  Revision 1.94  2002/02/08 17:50:05  field
//  Updates - neues Format bei sendEPG
//
//  Revision 1.93  2002/02/04 14:43:42  field
//  Start/Stop/Restart u. Threading veraendert ;)
//
//  Revision 1.92  2002/01/31 17:02:35  field
//  Buffergroesse
//
//  Revision 1.91  2002/01/30 13:35:02  field
//  Verbesserungen
//
//  Revision 1.90  2002/01/29 23:23:57  field
//  Mehr Details in ListAll
//
//  Revision 1.89  2002/01/06 18:23:44  McClean
//  better busybox-handling
//
//  Revision 1.88  2002/01/06 03:03:13  McClean
//  busybox 0.60 workarround
//
//  Revision 1.87  2001/11/22 13:19:00  field
//  Liefert nun auch nur NEXT Epg ab
//
//  Revision 1.85  2001/11/15 13:25:58  field
//  Bugfix, zeigt endlich auch die Events im gleichen Bouquet
//
//  Revision 1.84  2001/11/08 13:50:32  field
//  Debug-Out verbessert
//
//  Revision 1.83  2001/11/07 23:49:45  field
//  Current/Next Umschaltung funktioniert wieder
//
//  Revision 1.82  2001/11/05 17:12:05  field
//  Versuch zu Wiederholungen
//
//  Revision 1.81  2001/11/03 15:39:57  field
//  Deadlock behoben, Perspektiven
//
//  Revision 1.80  2001/11/03 03:13:53  field
//  Auf Perspektiven vorbereitet
//
//  Revision 1.79  2001/10/31 18:04:46  field
//  dmxTOT wird bei scan nicht gestoppt
//
//  Revision 1.78  2001/10/31 12:38:30  field
//  Timethread auch gepaust beim scanning
//
//  Revision 1.77  2001/10/30 21:13:11  field
//  bugfix
//
//  Revision 1.76  2001/10/29 17:57:09  field
//  Problem, dass Events mit hoeherer Nummer vorgehen, behoben
//
//  Revision 1.75  2001/10/25 12:24:29  field
//  verbeserte Behandlung von unvollstaendigen EPGs
//
//  Revision 1.72  2001/10/24 17:03:42  field
//  Deadlock behoben, Geschwindigkeit gesteigert
//
//  Revision 1.71  2001/10/22 14:27:24  field
//  Kleinigkeiten
//
//  Revision 1.70  2001/10/22 11:41:09  field
//  nvod-zeiten
//
//  Revision 1.69  2001/10/21 13:04:40  field
//  nvod-zeiten funktionieren
//
//  Revision 1.68  2001/10/18 23:57:18  field
//  Gesamtliste massiv beschleunigt
//
//  Revision 1.66  2001/10/10 14:56:30  fnbrd
//  tsid wird bei nvod mitgeschickt
//
//  Revision 1.65  2001/10/10 02:53:47  fnbrd
//  Neues kommando (noch nicht voll funktionsfaehig).
//
//  Revision 1.64  2001/10/05 02:37:00  fnbrd
//  Removed forgotten comment.
//
//  Revision 1.63  2001/10/04 20:12:59  fnbrd
//  Removed duplicate code.
//
//  Revision 1.62  2001/10/04 19:25:59  fnbrd
//  Neues Kommando allEventsChannelID.
//
//  Revision 1.61  2001/10/02 16:18:53  fnbrd
//  Fehler behoben.
//
//  Revision 1.59  2001/09/26 09:54:50  field
//  Neues Kommando (fuer Tontraeger-Auswahl)
//
//  Revision 1.58  2001/09/20 19:22:10  fnbrd
//  Changed format of eventlists with IDs
//
//  Revision 1.57  2001/09/20 12:53:22  fnbrd
//  More speed with event list.More speed with event list.More speed with event list.More speed with event list.More speed with event list.More speed with event list.More speed with event list.
//
//  Revision 1.56  2001/09/20 11:24:47  fnbrd
//  Fehler behoben.
//
//  Revision 1.55  2001/09/20 11:01:59  fnbrd
//  Format bei currentNextinformationChannelID geaendert.
//
//  Revision 1.54  2001/09/18 18:15:27  fnbrd
//  2 new commands.
//
//  Revision 1.53  2001/09/10 02:58:00  fnbrd
//  Cinedom-Hack
//
//  Revision 1.52  2001/08/29 22:41:51  fnbrd
//  Fix error with pause.
//
//  Revision 1.51  2001/08/17 16:37:28  fnbrd
//  Some mor informational output with -d and cache decrease
//
//  Revision 1.50  2001/08/17 16:21:06  fnbrd
//  Intelligent cache decrease to prevent dmx buffer overflows.
//
//  Revision 1.49  2001/08/17 14:33:15  fnbrd
//  Added a reopen of DMX connections after 15 timeouts to fix possible buffer overflows.
//
//  Revision 1.48  2001/08/16 13:13:12  fnbrd
//  New commands.
//
//  Revision 1.47  2001/08/16 10:55:41  fnbrd
//  Actual event list only with channels with event.
//
//  Revision 1.46  2001/08/16 01:35:23  fnbrd
//  internal changes.
//
//  Revision 1.45  2001/08/09 23:36:26  fnbrd
//  Pause command for the grabbers, internal changes.
//
//  Revision 1.43  2001/07/26 01:38:35  fnbrd
//  All events shows now all nvod-times.
//
//  Revision 1.42  2001/07/26 01:12:46  fnbrd
//  Removed a warning.
//
//  Revision 1.41  2001/07/26 00:58:09  fnbrd
//  Fixed one bug when time was not set.
//
//  Revision 1.40  2001/07/25 20:46:21  fnbrd
//  Neue Kommandos, kleine interne Verbesserungen.
//
//  Revision 1.39  2001/07/25 16:46:46  fnbrd
//  Added unique-keys to all commands.
//
//  Revision 1.37  2001/07/25 15:06:18  fnbrd
//  Added exception-handlers for better bug hunting.
//
//  Revision 1.36  2001/07/25 11:42:15  fnbrd
//  Added support for 'unique keys' in services and events.
//
//  Revision 1.35  2001/07/24 20:26:46  fnbrd
//  Fixed some nasty bugs introduced with (too) quickly implemented writeNBytes().
//
//  Revision 1.34  2001/07/24 18:19:06  fnbrd
//  Scheint stabil zu laufen, daher cache jetzt 5 Tage (und ein paar kleinere interne Aenderungen).
//
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
#include <semaphore.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <malloc.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h> 
//#include <sys/resource.h> // getrusage
#include <set>
#include <map>
#include <algorithm>
#include <string>

#include <ost/dmx.h>

#include <sys/wait.h>
#include <sys/time.h>

// Daher nehmen wir SmartPointers aus der Boost-Lib (www.boost.org)
#include <boost/smart_ptr.hpp>

#include "sectionsdMsg.h"
#include "SIutils.hpp"
#include "SIservices.hpp"
#include "SIevents.hpp"
#include "SIsections.hpp"

#include "eventserver.h"
#include "sectionsdclient.h"

//#include "timerdclient.h"
//#include "../timermanager.h"

// 60 Minuten Zyklus...
#define TIME_EIT_SCHEDULED_PAUSE 60* 60 
// Zeit die fuer die gewartet wird, bevor der Filter weitergeschaltet wird, falls es automatisch nicht klappt
#define TIME_EIT_SKIPPING 30

// 12h Pause für SDT
#define TIME_SDT_SCHEDULED_PAUSE 12* 60* 60
#define TIME_SDT_SKIPPING 5


// Timeout bei tcp/ip connections in ms
#define TIMEOUT_CONNECTIONS 2000

// Gibt die Anzahl Timeouts an, nach der die Verbindung zum DMX neu gestartet wird (wegen evtl. buffer overflow)
#define RESTART_DMX_AFTER_TIMEOUTS 5

// Gibt die Anzahl Timeouts an, nach der überprüft wird, ob die Timeouts von einem Sender ohne EIT kommen oder nicht
#define CHECK_RESTART_DMX_AFTER_TIMEOUTS 3

// Wieviele Sekunden EPG gecached werden sollen
//static long secondsToCache=4*24*60L*60L; // 4 Tage - weniger Prozessorlast?!
static long secondsToCache = 21*24*60L*60L; // 21 Tage - Prozessorlast <3% (rasc)
// Ab wann ein Event als alt gilt (in Sekunden)
static long oldEventsAre = 60*60L; // 1h
static int debug = 0;
static int scanning = 1;


// EVENTS...

CEventServer *eventServer;
//CTimerdClient   *timerdClient;
//bool            timerd = false;

#define dprintf(fmt, args...) { if (debug) { printf(fmt, ## args); fflush(stdout); } }
#define dputs(str) { if (debug) { puts(str); fflush(stdout); } }

static pthread_mutex_t eventsLock = PTHREAD_MUTEX_INITIALIZER; // Unsere (fast-)mutex, damit nicht gleichzeitig in die Menge events geschrieben und gelesen wird
static pthread_mutex_t servicesLock = PTHREAD_MUTEX_INITIALIZER; // Unsere (fast-)mutex, damit nicht gleichzeitig in die Menge services geschrieben und gelesen wird
static pthread_mutex_t messagingLock = PTHREAD_MUTEX_INITIALIZER;

inline void lockServices(void)
{
	pthread_mutex_lock(&servicesLock);
}

inline void unlockServices(void)
{
	pthread_mutex_unlock(&servicesLock);
}

inline void lockMessaging(void)
{
	pthread_mutex_lock(&messagingLock);
}

inline void unlockMessaging(void)
{
	pthread_mutex_unlock(&messagingLock);
}

inline void lockEvents(void)
{
	pthread_mutex_lock(&eventsLock);
}

inline void unlockEvents(void)
{
	pthread_mutex_unlock(&eventsLock);
}


static long long last_profile_call;

void showProfiling( string text )
{

	struct timeval tv;

	gettimeofday( &tv, NULL );
	long long now = (long long) tv.tv_usec + (long long)((long long) tv.tv_sec * (long long) 1000000);


	dprintf("--> '%s' %f\n", text.c_str(), (now - last_profile_call) / 1000.);
	last_profile_call = now;
}

static int timeset = 0; // = 1 falls Uhrzeit gesetzt
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

// Mengen mit SIeventPtr sortiert nach UniqueKey
typedef std::map<unsigned long long, SIeventPtr, std::less<unsigned long long> > MySIeventsOrderUniqueKey;
static MySIeventsOrderUniqueKey mySIeventsOrderUniqueKey;

// Mengen mit SIeventPtr sortiert nach Event-ID fuer NVOD-Events (mehrere Zeiten)
static MySIeventsOrderUniqueKey mySIeventsNVODorderUniqueKey;

struct OrderServiceUniqueKeyFirstStartTimeEventUniqueKey
{
	bool operator()(const SIeventPtr &p1, const SIeventPtr &p2)
	{
		return
		    SIservice::makeUniqueKey(p1->originalNetworkID, p1->serviceID) == SIservice::makeUniqueKey(p2->originalNetworkID, p2->serviceID) ?
		    (p1->times.begin()->startzeit == p2->times.begin()->startzeit ? p1->eventID < p2->eventID : p1->times.begin()->startzeit < p2->times.begin()->startzeit )
				    :
				    (SIservice::makeUniqueKey(p1->originalNetworkID, p1->serviceID) < SIservice::makeUniqueKey(p2->originalNetworkID, p2->serviceID) );
	}
};

typedef std::map<const SIeventPtr, SIeventPtr, OrderServiceUniqueKeyFirstStartTimeEventUniqueKey > MySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey;
static MySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey mySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey;

struct OrderFirstEndTimeServiceIDEventUniqueKey
{
	bool operator()(const SIeventPtr &p1, const SIeventPtr &p2)
	{
		return
		    p1->times.begin()->startzeit + (long)p1->times.begin()->dauer == p2->times.begin()->startzeit + (long)p2->times.begin()->dauer ?
		    //      ( p1->serviceID == p2->serviceID ? p1->uniqueKey() < p2->uniqueKey() : p1->serviceID < p2->serviceID )
		    ( p1->serviceID == p2->serviceID ? p1->uniqueKey() > p2->uniqueKey() : p1->serviceID < p2->serviceID )
				    :
				    ( p1->times.begin()->startzeit + (long)p1->times.begin()->dauer < p2->times.begin()->startzeit + (long)p2->times.begin()->dauer ) ;
	}
};

typedef std::map<const SIeventPtr, SIeventPtr, OrderFirstEndTimeServiceIDEventUniqueKey > MySIeventsOrderFirstEndTimeServiceIDEventUniqueKey;
static MySIeventsOrderFirstEndTimeServiceIDEventUniqueKey mySIeventsOrderFirstEndTimeServiceIDEventUniqueKey;

// Hier landen alle Service-Ids von Meta-Events inkl. der zugehoerigen Event-ID (nvod)
// d.h. key ist der Unique Service-Key des Meta-Events und Data ist der unique Event-Key
typedef std::map<unsigned, unsigned long long, std::less<unsigned> > MySIeventUniqueKeysMetaOrderServiceUniqueKey;
static MySIeventUniqueKeysMetaOrderServiceUniqueKey mySIeventUniqueKeysMetaOrderServiceUniqueKey;

/*
class NvodSubEvent {
  public:
    NvodSubEvent() {
      uniqueServiceID=0;
      uniqueEventID=0;
    }
    NvodSubEvent(const NvodSubEvent &n) {
      uniqueServiceID=n.uniqueServiceID;
      uniqueEventID=n.uniqueEventID;
    }
    unsigned uniqueServiceID; // zum zappen per onid+sid
    unsigned long long uniqueMetaEventID; // ID des Meta-Events
    unsigned long long uniqueMetaEventID; // ID des eigentlichen Events
};
 
// Menge sortiert nach Meta-ServiceIDs (NVODs)
typedef std::multimap<unsigned, class NvodSubEvent *, std::less<unsigned> > nvodSubEvents;
*/

// Loescht ein Event aus allen Mengen
static bool deleteEvent(const unsigned long long uniqueKey)
{
	MySIeventsOrderUniqueKey::iterator e = mySIeventsOrderUniqueKey.find(uniqueKey);

	if (e != mySIeventsOrderUniqueKey.end())
	{
		if (e->second->times.size())
		{
			mySIeventsOrderFirstEndTimeServiceIDEventUniqueKey.erase(e->second);
			mySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey.erase(e->second);
		}

		mySIeventsOrderUniqueKey.erase(uniqueKey);
		mySIeventsNVODorderUniqueKey.erase(uniqueKey);
		return true;
	}
	else
		return false;

	/*
	  for(MySIeventIDsMetaOrderServiceID::iterator i=mySIeventIDsMetaOrderServiceID.begin(); i!=mySIeventIDsMetaOrderServiceID.end(); i++)
	    if(i->second==eventID)
	      mySIeventIDsMetaOrderServiceID.erase(i);
	*/
}

// Fuegt ein Event in alle Mengen ein
static void addEvent(const SIevent &evt)
{
	SIevent *eptr = new SIevent(evt);

	if (!eptr)
	{
		printf("[sectionsd::addEvent] new SIevent failed.\n");
		throw std::bad_alloc();
	}

	SIeventPtr e(eptr);

	// Damit in den nicht nach Event-ID sortierten Mengen
	// Mehrere Events mit gleicher ID sind, diese vorher loeschen
	deleteEvent(e->uniqueKey());

	// Pruefen ob es ein Meta-Event ist
	MySIeventUniqueKeysMetaOrderServiceUniqueKey::iterator i = mySIeventUniqueKeysMetaOrderServiceUniqueKey.find(SIservice::makeUniqueKey(e->originalNetworkID, e->serviceID));

	if (i != mySIeventUniqueKeysMetaOrderServiceUniqueKey.end())
	{
		// ist ein MetaEvent, d.h. mit Zeiten fuer NVOD-Event

		if (e->times.size())
		{
			// D.h. wir fuegen die Zeiten in das richtige Event ein
			MySIeventsOrderUniqueKey::iterator ie = mySIeventsOrderUniqueKey.find(i->second);

			if (ie != mySIeventsOrderUniqueKey.end())
			{
				// Event vorhanden
				// Falls das Event in den beiden Mengen mit Zeiten nicht vorhanden
				// ist, dieses dort einfuegen
				MySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey::iterator i2 = mySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey.find(ie->second);

				if (i2 == mySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey.end())
				{
					// nicht vorhanden -> einfuegen
					mySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey.insert(std::make_pair(ie->second, ie->second));
					mySIeventsOrderFirstEndTimeServiceIDEventUniqueKey.insert(std::make_pair(ie->second, ie->second));

				}

				// Und die Zeiten im Event updaten
				ie->second->times.insert(e->times.begin(), e->times.end());
			}
		}
	}

	// normales Event
	mySIeventsOrderUniqueKey.insert(std::make_pair(e->uniqueKey(), e));

	if (e->times.size())
	{
		// diese beiden Mengen enthalten nur Events mit Zeiten
		mySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey.insert(std::make_pair(e, e));
		mySIeventsOrderFirstEndTimeServiceIDEventUniqueKey.insert(std::make_pair(e, e));

		/*      auskommentiert - das ist ein Performance-WAHNSINN
				*g* da muss eine bessere Lösung her (und die ist schon unterwegs)
		 
		            if (timerd)
		            {
		            	CTimerEvent_NextProgram::EventInfo evInfo;
		                evInfo.uniqueKey = e->uniqueKey();
		                evInfo.onidSid = (e->originalNetworkID << 16) + e->serviceID;
		                strncpy( evInfo.name, evt.name.c_str(), 50);
		                evInfo.fsk = evt.getFSK();
		 
		                for (SItimes::iterator it = evt.times.begin(); it != evt.times.end(); it++)
		                {
		                	time_t actTime_t;
		                	::time(&actTime_t);
		                	if (it->startzeit + it->dauer > actTime_t)
		                	{
		                		time_t kurzvorStartzeit = it->startzeit; // - 60;
		                		struct tm* startTime = localtime( &(kurzvorStartzeit));
		                		timerdClient->addTimerEvent(
		                			CTimerdClient::TIMER_NEXTPROGRAM,
		                			&evInfo,
		                			startTime->tm_min,
		                			startTime->tm_hour,
		                			startTime->tm_mday,
		                			startTime->tm_mon);
		                	}
		                }
		            }
		*/

	}
}

static void addNVODevent(const SIevent &evt)
{
	SIevent *eptr = new SIevent(evt);

	if (!eptr)
	{
		printf("[sectionsd::addNVODevent] new SIevent failed.\n");
		throw std::bad_alloc();
	}

	SIeventPtr e(eptr);

	MySIeventsOrderUniqueKey::iterator e2 = mySIeventsOrderUniqueKey.find(e->uniqueKey());

	if (e2 != mySIeventsOrderUniqueKey.end())
	{
		// bisher gespeicherte Zeiten retten
		e->times.insert(e2->second->times.begin(), e2->second->times.end());
	}

	// Damit in den nicht nach Event-ID sortierten Mengen
	// mehrere Events mit gleicher ID sind, diese vorher loeschen
	deleteEvent(e->uniqueKey());

	mySIeventsOrderUniqueKey.insert(std::make_pair(e->uniqueKey(), e));

	mySIeventsNVODorderUniqueKey.insert(std::make_pair(e->uniqueKey(), e));

	if (e->times.size())
	{
		// diese beiden Mengen enthalten nur Events mit Zeiten
		mySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey.insert(std::make_pair(e, e));
		mySIeventsOrderFirstEndTimeServiceIDEventUniqueKey.insert(std::make_pair(e, e));
	}
}

#if 0
static void removeNewEvents(void)
{
	// Alte events loeschen
	time_t zeit = time(NULL);

	// Mal umgekehrt wandern

	for (MySIeventsOrderFirstEndTimeServiceIDEventUniqueKey::iterator e = mySIeventsOrderFirstEndTimeServiceIDEventUniqueKey.begin(); e != mySIeventsOrderFirstEndTimeServiceIDEventUniqueKey.end(); e++)
		if (e->first->times.begin()->startzeit > zeit + secondsToCache)
			deleteEvent(e->first->uniqueKey());

	return ;
}
#endif

static void removeOldEvents(const long seconds)
{
	// Alte events loeschen
	time_t zeit = time(NULL);

	for (MySIeventsOrderFirstEndTimeServiceIDEventUniqueKey::iterator e = mySIeventsOrderFirstEndTimeServiceIDEventUniqueKey.begin(); e != mySIeventsOrderFirstEndTimeServiceIDEventUniqueKey.end(); e++)
		if (e->first->times.begin()->startzeit + (long)e->first->times.begin()->dauer < zeit - seconds)
			deleteEvent(e->first->uniqueKey());
		else
			break; // sortiert nach Endzeit, daher weiteres Suchen unnoetig

	return ;
}

//  SIservicePtr;
typedef boost::shared_ptr<class SIservice>
SIservicePtr;

// Key ist unsigned  (Unique Service-ID), data ist ein SIservicePtr
typedef std::map<unsigned, SIservicePtr, std::less<unsigned> > MySIservicesOrderUniqueKey;
static MySIservicesOrderUniqueKey mySIservicesOrderUniqueKey;

// Key ist unsigned (Unique Sevice-ID), data ist ein SIservicePtr
typedef std::map<unsigned, SIservicePtr, std::less<unsigned> > MySIservicesNVODorderUniqueKey;
static MySIservicesNVODorderUniqueKey mySIservicesNVODorderUniqueKey;

// Hier sollte man die hash-funktion fuer strings der stl benutzen
// Muss mal schauen ob es die auch fuer 'ignore_case' gibt

struct OrderServiceName
{
	bool operator()(const SIservicePtr &p1, const SIservicePtr &p2)
	{
		return strcasecmp(p1->serviceName.c_str(), p2->serviceName.c_str()) < 0;
	}
};

typedef std::map<const SIservicePtr, SIservicePtr, OrderServiceName > MySIservicesOrderServiceName;
static MySIservicesOrderServiceName mySIservicesOrderServiceName;

// Fuegt ein Service in alle Mengen ein
static void addService(const SIservice &s)
{
	SIservice *sp = new SIservice(s);

	if (!sp)
	{
		printf("[sectionsd::addService] new SIservice failed.\n");
		throw std::bad_alloc();
	}

	SIservicePtr sptr(sp);

	// Controlcodes entfernen
	char servicename[50];

	strncpy(servicename, sptr->serviceName.c_str(), sizeof(servicename) - 1);

	servicename[sizeof(servicename) - 1] = 0;

	removeControlCodes(servicename);

	sptr->serviceName = servicename;

	mySIservicesOrderUniqueKey.insert(std::make_pair(sptr->uniqueKey(), sptr));

	if (sptr->nvods.size())
		mySIservicesNVODorderUniqueKey.insert(std::make_pair(sptr->uniqueKey(), sptr));

	//  if(sptr->serviceID==0x01 || sptr->serviceID==0x02 || sptr->serviceID==0x04)
	mySIservicesOrderServiceName.insert(std::make_pair(sptr, sptr));
}

//------------------------------------------------------------
// other stuff
//------------------------------------------------------------

// Liest n Bytes aus einem Socket per read
// Liefert 0 bei timeout
// und -1 bei Fehler
// ansonsten die Anzahl gelesener Bytes
/* inline */
int readNbytes(int fd, char *buf, const size_t n, unsigned timeoutInMSeconds)
{
	size_t j;

	//	timeoutInSeconds*; // in Millisekunden aendern

	for (j = 0; j < n;)
	{

		struct pollfd ufds;
		ufds.fd = fd;
		ufds.events = POLLIN;
		ufds.revents = 0;
		int rc = poll(&ufds, 1, timeoutInMSeconds);

		if (!rc)
			return 0; // timeout
		else if (rc < 0 && errno == EINTR)
			continue; // interuppted
		else if (rc < 0)
		{
			perror ("[sectionsd] poll");
			//printf("errno: %d\n", errno);
			return -1;
		}

		if (!(ufds.revents&POLLIN))
		{
			// POLLHUP, beim dmx bedeutet das DMXDEV_STATE_TIMEDOUT
			// kommt wenn ein Timeout im Filter gesetzt wurde
			// dprintf("revents: 0x%hx\n", ufds.revents);

			usleep(100*1000UL); // wir warten 100 Millisekunden bevor wir es nochmal probieren

			if (timeoutInMSeconds <= 200000)
				return 0; // timeout

			timeoutInMSeconds -= 200000;

			continue;
		}

		int r = read (fd, buf, n - j);

		if (r > 0)
		{
			j += r;
			buf += r;
		}
		else if (r <= 0 && errno != EINTR)
		{
			//printf("errno: %d\n", errno);
			perror ("[sectionsd] read");
			return -1;
		}
	}

	return j;
}

// Schreibt n Bytes in einen Socket per write
// Liefert 0 bei timeout
// und -1 bei Fehler
// ansonsten die Anzahl geschriebener Bytes
/* inline */ int writeNbytes(int fd,  const char *buf,  const size_t numberOfBytes,  unsigned timeoutInSeconds)
{
	// Timeouthandling usw fehlt noch
	int n = numberOfBytes;

	while (n)
	{
		fd_set readfds, writefds, exceptfds;
		FD_ZERO(&readfds);
		FD_ZERO(&writefds);
		FD_ZERO(&exceptfds);
		FD_SET(fd, &writefds);
		timeval tv;
		tv.tv_sec = timeoutInSeconds;
		tv.tv_usec = 0;
		int f = select(fd + 1, &readfds, &writefds, &exceptfds, &tv);

		if (!f)
		{
			//      dputs("select: timeout");
			return 0; // timeout
		}
		else if (f == -1 || (f > 0 && fd == -1))
		{
			//      dputs("select: fehler");
			return -1; // Fehler
		}

		int rc = write(fd, buf, n);

		if (!rc)
			continue;
		else if (rc < 0)
		{
			if (errno == EINTR)
				continue;
			else
				return -1;
		}
		else
			n -= rc;
	}

	return numberOfBytes;
}

//------------------------------------------------------------
// class DMX
//------------------------------------------------------------

class DMX
{

public:
	DMX(unsigned char p, unsigned short bufferSizeInKB, int nCRC = 0)
	{
		fd = 0;
		lastChanged = 0;
		filter_index = 0;
		pID = p;
		dmxBufferSizeInKB = bufferSizeInKB;
		noCRC = nCRC;
		pthread_mutex_init(&pauselock, NULL); // default = fast mutex
		pthread_mutex_init(&start_stop_mutex, NULL); // default = fast mutex
		pthread_cond_init( &change_cond, NULL );
		pauseCounter = 0;
		real_pauseCounter = 0;
	}

	~DMX()
	{
		closefd();
		pthread_mutex_destroy(&pauselock); // ist bei Linux ein dummy
		pthread_mutex_destroy(&start_stop_mutex); // ist bei Linux ein dummy
		pthread_cond_destroy(&change_cond);
	}

	int start(void); // calls unlock at end
	int read(char *buf, const size_t buflength, unsigned timeoutMInSeconds)
	{
		return readNbytes(fd, buf, buflength, timeoutMInSeconds);
	}

	void closefd(void)
	{
		if (fd)
		{
			close(fd);
			fd = 0;
		}
	}

	void addfilter(unsigned char filter, unsigned char mask)
	{
		s_filters	tmp;
		tmp.filter = filter;
		tmp.mask = mask;
		filters.insert(filters.end(), tmp);
	}

	int stop(void)
	{
		if (!fd)
			return 1;

		pthread_mutex_lock( &start_stop_mutex );

		if (real_pauseCounter == 0)
			closefd();

		pthread_mutex_unlock( &start_stop_mutex );

		return 0;
	}

	int pause(void); // increments pause_counter
	int unpause(void); // decrements pause_counter

	int real_pause(void);
	int real_unpause(void);

	int request_pause(void);
	int request_unpause(void);

	int change(int new_filter_index); // locks while changing

	void lock (void)
	{
		pthread_mutex_lock( &start_stop_mutex );
	}

	void unlock(void)
	{
		pthread_mutex_unlock( &start_stop_mutex );
	}

	struct s_filters
	{
		unsigned char filter;
		unsigned char mask;
	};

	std::vector<s_filters> filters;
	int	filter_index;
	time_t	lastChanged;

	bool isOpen(void)
	{
		return fd ? true : false;
	}

	int pauseCounter;
	int real_pauseCounter;
	pthread_cond_t	change_cond;
	pthread_mutex_t	start_stop_mutex;

private:
	int fd;
	pthread_mutex_t pauselock;
	unsigned char pID;
	unsigned short dmxBufferSizeInKB;
	int noCRC; // = 1 -> der 2. Filter hat keine CRC

};

int DMX::start(void)
{
	if (fd)
	{
		return 1;
	}

	pthread_mutex_lock( &start_stop_mutex );

	if (real_pauseCounter != 0)
	{
		pthread_mutex_unlock( &start_stop_mutex );
		return 0;
	}

	if ((fd = open("/dev/dvb/card0/demux0", O_RDWR)) == -1)
	{
		perror ("[sectionsd] DMX: /dev/dvb/card0/demux0");
		pthread_mutex_unlock( &start_stop_mutex );
		return 2;
	}

	if (dmxBufferSizeInKB != 256)
		if (ioctl (fd, DMX_SET_BUFFER_SIZE, (unsigned long)(dmxBufferSizeInKB*1024UL)) == -1)
		{
			closefd();
			perror ("[sectionsd] DMX: DMX_SET_BUFFER_SIZE");
			pthread_mutex_unlock( &start_stop_mutex );
			return 3;
		}

	struct dmxSctFilterParams flt;

	memset (&flt, 0, sizeof (struct dmxSctFilterParams));

	//  memset (&flt.filter, 0, sizeof (struct dmxFilter));
	flt.pid = pID;

	flt.filter.filter[0] = filters[filter_index].filter; // current/next

	flt.filter.mask[0] = filters[filter_index].mask; // -> 4e und 4f

	flt.flags = DMX_IMMEDIATE_START | DMX_CHECK_CRC;

	if (ioctl (fd, DMX_SET_FILTER, &flt) == -1)
	{
		closefd();
		perror ("[sectionsd] DMX: DMX_SET_FILTER");
		pthread_mutex_unlock( &start_stop_mutex );
		return 4;
	}

	//  	if(timeset) // Nur wenn ne richtige Uhrzeit da ist
	//    	lastChanged=time(NULL);
	pthread_mutex_unlock( &start_stop_mutex );

	return 0;
}

int DMX::real_pause(void)
{
	if (!fd)
		return 1;

	pthread_mutex_lock( &start_stop_mutex );

	if (real_pauseCounter == 0)
	{
		if (ioctl (fd, DMX_STOP, 0) == -1)
		{
			closefd();
			perror ("[sectionsd] DMX: DMX_STOP");
			pthread_mutex_unlock( &start_stop_mutex );
			return 2;
		}
	}

	//dprintf("real_pause: %d\n", real_pauseCounter);
	pthread_mutex_unlock( &start_stop_mutex );

	return 0;
}

int DMX::real_unpause(void)
{
	if (!fd)
		return 1;

	pthread_mutex_lock( &start_stop_mutex );

	if (real_pauseCounter == 0)
	{
		if (ioctl (fd, DMX_START, 0) == -1)
		{
			closefd();
			perror ("[sectionsd] DMX: DMX_START");
			pthread_mutex_unlock( &start_stop_mutex );
			return 2;
		}

		//dprintf("real_unpause DONE: %d\n", real_pauseCounter);
	}

	//    else
	//dprintf("real_unpause NOT DONE: %d\n", real_pauseCounter);


	pthread_mutex_unlock( &start_stop_mutex );

	return 0;
}

int DMX::request_pause(void)
{
	real_pause(); // unlocked

	pthread_mutex_lock( &start_stop_mutex );
	//dprintf("request_pause: %d\n", real_pauseCounter);

	real_pauseCounter++;
	pthread_mutex_unlock( &start_stop_mutex );

	return 0;
}


int DMX::request_unpause(void)
{
	pthread_mutex_lock( &start_stop_mutex );
	//dprintf("request_unpause: %d\n", real_pauseCounter);
	--real_pauseCounter;
	pthread_mutex_unlock( &start_stop_mutex );

	real_unpause(); // unlocked

	return 0;
}


int DMX::pause(void)
{
	if (!fd)
		return 1;

	pthread_mutex_lock(&pauselock);

	//dprintf("lock from pc: %d\n", pauseCounter);
	pauseCounter++;

	pthread_mutex_unlock(&pauselock);

	return 0;
}

int DMX::unpause(void)
{
	if (!fd)
		return 1;

	pthread_mutex_lock(&pauselock);

	//dprintf("unlock from pc: %d\n", pauseCounter);
	--pauseCounter;

	pthread_mutex_unlock(&pauselock);

	return 0;
}

int DMX::change(int new_filter_index)
{
	if (!fd)
		return 1;

	if (new_filter_index == filter_index)
	{
		// do wakeup only... - no change neccessary
		pthread_cond_signal( &change_cond );
		return 0;
	}

	showProfiling("before pthread_mutex_lock( &start_stop_mutex );");

	pthread_mutex_lock( &start_stop_mutex );
	showProfiling("after pthread_mutex_lock( &start_stop_mutex );");

	if (real_pauseCounter > 0)
	{
		pthread_mutex_unlock( &start_stop_mutex );
		return 0;	// läuft nicht (zB streaming)
	}

	//	if(pID==0x12) // Nur bei EIT
	dprintf("changeDMX [%x]-> %s (%d)\n", pID, (new_filter_index == 0) ? "current/next" : "scheduled", new_filter_index );

	if (ioctl (fd, DMX_STOP, 0) == -1)
	{
		closefd();
		perror ("[sectionsd] DMX: DMX_STOP");
		pthread_mutex_unlock( &start_stop_mutex );
		return 2;
	}

	struct dmxSctFilterParams flt;

	memset (&flt, 0, sizeof (struct dmxSctFilterParams));

	flt.pid = pID;

	flt.filter.filter[0] = filters[new_filter_index].filter;

	flt.filter.mask[0] = filters[new_filter_index].mask;

	//dprintf("changeDMX newfilter %x %x\n", flt.filter.filter[0], flt.filter.mask[0]);
	flt.flags = DMX_IMMEDIATE_START;

	if ( ( new_filter_index != 0 ) && (!noCRC) )
		flt.flags |= DMX_CHECK_CRC;

	filter_index = new_filter_index;

	pthread_cond_signal( &change_cond );

	if (ioctl (fd, DMX_SET_FILTER, &flt) == -1)
	{
		closefd();
		perror ("[sectionsd] DMX: DMX_SET_FILTER");
		pthread_mutex_unlock( &start_stop_mutex );
		return 3;
	}

	showProfiling("after DMX_SET_FILTER, &");

	if (timeset) // Nur wenn ne richtige Uhrzeit da ist
		lastChanged = time(NULL);

	pthread_mutex_unlock( &start_stop_mutex );

	return 0;
}

// k.A. ob volatile im Kampf gegen Bugs trotz mutex's was bringt,
// falsch ist es zumindest nicht
/*
static DMX dmxEIT(0x12, 0x4f, (0xff- 0x01), 0x50, (0xff- 0x0f), 384);
static DMX dmxSDT(0x11, 0x42, 0xff, 0x42, 0xff, 256);
static DMX dmxTOT(0x14, 0x73, 0xff, 0x70, (0xff- 0x03), 256, 1);
*/
static DMX dmxEIT(0x12, 384);
static DMX dmxSDT(0x11, 256);
static DMX dmxTOT(0x14, 256, 1);

//------------------------------------------------------------
// misc. functions
//------------------------------------------------------------

static unsigned findServiceUniqueKeyforServiceName(const char *serviceName)
{
	SIservice *sp = new SIservice((unsigned short)0, (unsigned short)0);

	if (!sp)
	{
		printf("[sectionsd::findServiceUniqueKeyforServiceName] new SIservice failed.\n");
		throw std::bad_alloc();
	}

	SIservicePtr s(sp);

	s->serviceName = serviceName;

	dprintf("Search for Service '%s'\n", serviceName);

	MySIservicesOrderServiceName::iterator si = mySIservicesOrderServiceName.find(s);

	if (si != mySIservicesOrderServiceName.end())
		return si->first->uniqueKey();

	dputs("Service not found");

	return 0;
}

static const SIevent& findSIeventForEventUniqueKey(const long long& eventUniqueKey)
{
	// Event (eventid) suchen
	MySIeventsOrderUniqueKey::iterator e = mySIeventsOrderUniqueKey.find(eventUniqueKey);

	if (e != mySIeventsOrderUniqueKey.end())
		return *(e->second);

	return nullEvt;
}

static const SIevent& findActualSIeventForServiceUniqueKey(const unsigned serviceUniqueKey, SItime& zeit, long plusminus = 0, unsigned *flag = 0)
{
	time_t azeit = time(NULL);

	if (flag != 0)
		*flag = 0;

	for (MySIeventsOrderFirstEndTimeServiceIDEventUniqueKey::iterator e = mySIeventsOrderFirstEndTimeServiceIDEventUniqueKey.begin(); e != mySIeventsOrderFirstEndTimeServiceIDEventUniqueKey.end(); e++)
		if (SIservice::makeUniqueKey(e->first->originalNetworkID, e->first->serviceID) == serviceUniqueKey)
		{
			if (flag != 0)
				*flag |= sectionsd::epgflags::has_anything; // überhaupt was da...

			for (SItimes::reverse_iterator t = e->first->times.rend(); t != e->first->times.rbegin(); t--)
				if ((long)(azeit + plusminus) < (long)(t->startzeit + t->dauer))
				{
					if (flag != 0)
						*flag |= sectionsd::epgflags::has_later; // spätere events da...

					if (t->startzeit <= (long)(azeit + plusminus))
					{
						//printf("azeit %d, startzeit+t->dauer %d \n", azeit, (long)(t->startzeit+t->dauer) );

						if (flag != 0)
							*flag |= sectionsd::epgflags::has_current; // aktuelles event da...

						zeit = *t;

						return *(e->first);
					}
				}
		}

	return nullEvt;
}

static const SIevent& findNextSIeventForServiceUniqueKey(const unsigned serviceUniqueKey, SItime& zeit)
{
	time_t azeit = time(NULL);

	for (MySIeventsOrderFirstEndTimeServiceIDEventUniqueKey::iterator e = mySIeventsOrderFirstEndTimeServiceIDEventUniqueKey.begin(); e != mySIeventsOrderFirstEndTimeServiceIDEventUniqueKey.end(); e++)
		if (SIservice::makeUniqueKey(e->first->originalNetworkID, e->first->serviceID) == serviceUniqueKey)
		{
			for (SItimes::iterator t = e->first->times.begin(); t != e->first->times.end(); t++)
				if ((long)(azeit) < (long)(t->startzeit + t->dauer))
				{
					zeit = *t;
					return *(e->first);
				}
		}

	return nullEvt;
}


static const SIevent &findActualSIeventForServiceName(const char *serviceName, SItime& zeit)
{
	unsigned serviceUniqueKey = findServiceUniqueKeyforServiceName(serviceName);

	if (serviceUniqueKey)
		return findActualSIeventForServiceUniqueKey(serviceUniqueKey, zeit);

	return nullEvt;
}



// Sucht das naechste Event anhand unique key und Startzeit
static const SIevent &findNextSIevent(const unsigned long long uniqueKey, SItime &zeit)
{
	MySIeventsOrderUniqueKey::iterator eFirst = mySIeventsOrderUniqueKey.find(uniqueKey);

	if (eFirst != mySIeventsOrderUniqueKey.end())
	{
		if (eFirst->second->times.size() > 1)
		{
			// Wir haben ein NVOD-Event
			// d.h. wir suchen die aktuelle Zeit und nehmen die naechste davon, falls existent

			for (SItimes::iterator t = eFirst->second->times.begin(); t != eFirst->second->times.end(); t++)
				if (t->startzeit == zeit.startzeit)
				{
					t++;

					if (t != eFirst->second->times.end())
					{
						zeit = *t;
						return *(eFirst->second);
					}

					break; // ganz normal naechstes Event suchen
				}
		}

		MySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey::iterator eNext = mySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey.find(eFirst->second);
		eNext++;

		if (eNext != mySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey.end())
		{
			if (SIservice::makeUniqueKey(eNext->second->originalNetworkID, eNext->second->serviceID) == SIservice::makeUniqueKey(eFirst->second->originalNetworkID, eFirst->second->serviceID))
			{
				zeit = *(eNext->second->times.begin());
				return *(eNext->second);
			}
			else
				return nullEvt;
		}
	}

	return nullEvt;
}

// Sucht das naechste UND vorhergehende Event anhand unique key und Startzeit
static void findPrevNextSIevent(const unsigned long long uniqueKey, SItime &zeit, SIevent &prev, SItime &prev_zeit, SIevent &next, SItime &next_zeit)
{
	prev = nullEvt;
	next = nullEvt;
	bool prev_ok = false;
	bool next_ok = false;

	MySIeventsOrderUniqueKey::iterator eFirst = mySIeventsOrderUniqueKey.find(uniqueKey);

	if (eFirst != mySIeventsOrderUniqueKey.end())
	{
		if (eFirst->second->times.size() > 1)
		{
			// Wir haben ein NVOD-Event
			// d.h. wir suchen die aktuelle Zeit und nehmen die naechste davon, falls existent

			for (SItimes::iterator t = eFirst->second->times.begin(); t != eFirst->second->times.end(); t++)
				if (t->startzeit == zeit.startzeit)
				{
					if (t != eFirst->second->times.begin())
					{
						t--;
						prev_zeit = *t;
						prev = *(eFirst->second);
						prev_ok = true;
						t++;
					}

					t++;

					if (t != eFirst->second->times.end())
					{
						next_zeit = *t;
						next = *(eFirst->second);
						next_ok = true;
					}

					if ( prev_ok && next_ok )
						return ; // beide gefunden...
					else
						break;
				}
		}

		MySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey::iterator eNext = mySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey.find(eFirst->second);

		if ( (!prev_ok) && (eNext != mySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey.begin() ) )
		{
			eNext--;

			if (SIservice::makeUniqueKey(eNext->second->originalNetworkID, eNext->second->serviceID) == SIservice::makeUniqueKey(eFirst->second->originalNetworkID, eFirst->second->serviceID))
			{
				prev_zeit = *(eNext->second->times.begin());
				prev = *(eNext->second);
			}

			eNext++;
		}

		eNext++;

		if ( (!next_ok) && (eNext != mySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey.end()) )
		{
			if (SIservice::makeUniqueKey(eNext->second->originalNetworkID, eNext->second->serviceID) == SIservice::makeUniqueKey(eFirst->second->originalNetworkID, eFirst->second->serviceID))
			{
				next_zeit = *(eNext->second->times.begin());
				next = *(eNext->second);
			}
		}

		// printf("evt_id >%llx<, time %x - evt_id >%llx<, time %x\n", prev.uniqueKey(), prev_zeit.startzeit, next.uniqueKey(), next_zeit.startzeit);
	}
}

//---------------------------------------------------------------------
//			connection-thread
// handles incoming requests
//---------------------------------------------------------------------

struct connectionData
{
	int connectionSocket;

	struct sockaddr_in clientAddr;
};

static void commandPauseScanning(struct connectionData *client, char *data, const unsigned dataLength)
{
	if (dataLength != 4)
		return ;

	int pause = *(int *)data;

	if (pause && pause != 1)
		return ;

	dprintf("Request of %s scanning.\n", pause ? "stop" : "continue" );

	if (scanning && pause)
	{
		dmxEIT.request_pause();
		dmxSDT.request_pause();
		//dmxTOT.real_pause();
		scanning = 0;
	}
	else if (!pause && !scanning)
	{
		dmxSDT.request_unpause();
		dmxEIT.request_unpause();
		//dmxTOT.real_unpause();
		scanning = 1;
	}

	struct sectionsd::msgResponseHeader msgResponse;

	msgResponse.dataLength = 0;

	writeNbytes(client->connectionSocket, (const char *)&msgResponse, sizeof(msgResponse), TIMEOUT_CONNECTIONS);

	return ;
}

static void commandPauseSorting(struct connectionData *client, char *data, const unsigned dataLength)
{
	if (dataLength != 4)
		return ;

	int pause = *(int *)data;

	if (pause && pause != 1)
		return ;

	dprintf("Request of %s sorting events.\n", pause ? "stop" : "continue" );

	if (pause)
		dmxEIT.pause();
	else
		dmxEIT.unpause();

	struct sectionsd::msgResponseHeader msgResponse;

	msgResponse.dataLength = 0;

	writeNbytes(client->connectionSocket, (const char *)&msgResponse, sizeof(msgResponse), TIMEOUT_CONNECTIONS);

	return ;
}

static void commandDumpAllServices(struct connectionData *client, char *data, const unsigned dataLength)
{
	if (dataLength)
		return ;

	dputs("Request of service list.\n");

	char *serviceList = new char[65*1024]; // 65kb should be enough and dataLength is unsigned short

	if (!serviceList)
	{
		fprintf(stderr, "low on memory!\n");
		return ;
	}

	*serviceList = 0;
	lockServices();
	char daten[200];

	for (MySIservicesOrderServiceName::iterator s = mySIservicesOrderServiceName.begin(); s != mySIservicesOrderServiceName.end(); s++)
	{
		sprintf(daten, "%08x %hu %hhu %d %d %d %d %u ",
		        s->first->uniqueKey(),
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

	struct sectionsd::msgResponseHeader msgResponse;
	msgResponse.dataLength = strlen(serviceList) + 1;

	if (msgResponse.dataLength == 1)
		msgResponse.dataLength = 0;

	if (writeNbytes(client->connectionSocket, (const char *)&msgResponse, sizeof(msgResponse), TIMEOUT_CONNECTIONS) > 0)
	{
		if (msgResponse.dataLength)
			writeNbytes(client->connectionSocket, serviceList, msgResponse.dataLength, TIMEOUT_CONNECTIONS);
	}
	else
		dputs("[sectionsd] Fehler/Timeout bei write");

	delete[] serviceList;

	return ;
}

static void commandSetEventsAreOldInMinutes(struct connectionData *client, char *data, const unsigned dataLength)
{
	if (dataLength != 2)
		return ;

	dprintf("Set events are old after minutes: %hd\n", *((unsigned short*)data));

	oldEventsAre = *((unsigned short*)data)*60L;

	struct sectionsd::msgResponseHeader responseHeader;

	responseHeader.dataLength = 0;

	writeNbytes(client->connectionSocket, (const char *)&responseHeader, sizeof(responseHeader), TIMEOUT_CONNECTIONS);

	return ;
}

static void commandSetHoursToCache(struct connectionData *client, char *data, const unsigned dataLength)
{
	if (dataLength != 2)
		return ;

	dprintf("Set hours to cache: %hd\n", *((unsigned short*)data));

	secondsToCache = *((unsigned short*)data)*60L*60L;

	struct sectionsd::msgResponseHeader responseHeader;

	responseHeader.dataLength = 0;

	writeNbytes(client->connectionSocket, (const char *)&responseHeader, sizeof(responseHeader), TIMEOUT_CONNECTIONS);

	return ;
}

static void sendAllEvents(struct connectionData *client, unsigned serviceUniqueKey, bool oldFormat = true )
{
	char *evtList = new char[65*1024]; // 65kb should be enough and dataLength is unsigned short

	if (!evtList)
	{
		fprintf(stderr, "low on memory!\n");
		return ;
	}

	dprintf("sendAllEvents for %x\n", serviceUniqueKey);
	*evtList = 0;
	char *liste = evtList;

	if (serviceUniqueKey != 0)
	{
		// service Found

		if ( dmxEIT.pause() )
		{
			delete[] evtList;
			return ;
		}

		lockEvents();
		int serviceIDfound = 0;

		for (MySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey::iterator e = mySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey.begin(); e != mySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey.end(); e++)
		{
			if (SIservice::makeUniqueKey(e->first->originalNetworkID, e->first->serviceID) == serviceUniqueKey)
			{
				serviceIDfound = 1;

				for (SItimes::iterator t = e->first->times.begin(); t != e->first->times.end(); t++)
				{
					if ( oldFormat )
					{
						char strZeit[50];
						sprintf(strZeit, "%012llx ", e->first->uniqueKey());
						strcat(liste, strZeit);

						struct tm *tmZeit;
						tmZeit = localtime(&(t->startzeit));
						sprintf(strZeit, "%02d.%02d %02d:%02d %u ",
						        tmZeit->tm_mday, tmZeit->tm_mon + 1, tmZeit->tm_hour, tmZeit->tm_min, e->first->times.begin()->dauer / 60);
						strcat(liste, strZeit);
						strcat(liste, e->first->name.c_str());
						strcat(liste, "\n");
					}
					else
					{
						*((unsigned long long *)liste) = e->first->uniqueKey();
						liste += 8;
						*((unsigned *)liste) = t->startzeit;
						liste += 4;
						*((unsigned *)liste) = t->dauer;
						liste += 4;
						strcpy(liste, e->first->name.c_str());
						liste += strlen(liste);
						liste++;

						if (e->first->text == "" )
						{
							strcpy(liste, e->first->extendedText.substr(0, 40).c_str());
							liste += strlen(liste);
						}
						else
						{
							strcpy(liste, e->first->text.c_str());
							liste += strlen(liste);
						}

						liste++;
					}
				}
			} // if = serviceID
			else if ( serviceIDfound )
				break; // sind nach serviceID und startzeit sortiert -> nicht weiter suchen
		}

		unlockEvents();

		if (dmxEIT.unpause())
		{
			delete[] evtList;
			return ;
		}
	}

	struct sectionsd::msgResponseHeader responseHeader;

	responseHeader.dataLength = liste - evtList;

	dprintf("[sectionsd] all events - response-size: 0x%x\n", responseHeader.dataLength);

	if ( responseHeader.dataLength == 1 )
		responseHeader.dataLength = 0;

	if (writeNbytes(client->connectionSocket, (const char *)&responseHeader, sizeof(responseHeader), TIMEOUT_CONNECTIONS) > 0)
	{
		if (responseHeader.dataLength)
			writeNbytes(client->connectionSocket, evtList, responseHeader.dataLength, TIMEOUT_CONNECTIONS);
	}
	else
		dputs("[sectionsd] Fehler/Timeout bei write");

	delete[] evtList;

	return ;
}

static void commandAllEventsChannelName(struct connectionData *client, char *data, const unsigned dataLength)
{
	data[dataLength - 1] = 0; // to be sure it has an trailing 0
	dprintf("Request of all events for '%s'\n", data);
	lockServices();
	unsigned uniqueServiceKey = findServiceUniqueKeyforServiceName(data);
	unlockServices();
	sendAllEvents(client, uniqueServiceKey);
	return ;
}

static void commandAllEventsChannelID(struct connectionData *client, char *data, const unsigned dataLength)
{
	if (dataLength != 4)
		return ;

	unsigned serviceUniqueKey = *(unsigned *)data;

	dprintf("Request of all events for 0x%x\n", serviceUniqueKey);

	sendAllEvents(client, serviceUniqueKey, false);

	return ;
}

static void commandDumpStatusInformation(struct connectionData *client, char *data, const unsigned dataLength)
{
	if (dataLength)
		return ;

	dputs("Request of status information");

	lockEvents();

	unsigned anzEvents = mySIeventsOrderUniqueKey.size();

	unsigned anzNVODevents = mySIeventsNVODorderUniqueKey.size();

	unsigned anzMetaServices = mySIeventUniqueKeysMetaOrderServiceUniqueKey.size();

	unlockEvents();

	lockServices();

	unsigned anzServices = mySIservicesOrderUniqueKey.size();

	unsigned anzNVODservices = mySIservicesNVODorderUniqueKey.size();

	//  unsigned anzServices=services.size();
	unlockServices();

	struct mallinfo speicherinfo = mallinfo();

	//  struct rusage resourceUsage;
	//  getrusage(RUSAGE_CHILDREN, &resourceUsage);
	//  getrusage(RUSAGE_SELF, &resourceUsage);
	time_t zeit = time(NULL);

	char stati[2024];

	sprintf(stati,
	        "$Id: sectionsd.cpp,v 1.135 2002/10/04 16:31:25 thegoodguy Exp $\n"
	        "Current time: %s"
	        "Hours to cache: %ld\n"
	        "Events are old %ldmin after their end time\n"
	        "Number of cached services: %u\n"
	        "Number of cached nvod-services: %u\n"
	        "Number of cached events: %u\n"
	        "Number of cached nvod-events: %u\n"
	        "Number of cached meta-services: %u\n"
	        //    "Resource-usage: maxrss: %ld ixrss: %ld idrss: %ld isrss: %ld\n"
	        "Total size of memory occupied by chunks handed out by malloc: %d\n"
	        "Total bytes memory allocated with `sbrk' by malloc, in bytes: %d (%dkb, %.2fMB)\n",
	        ctime(&zeit),
	        secondsToCache / (60*60L), oldEventsAre / 60, anzServices, anzNVODservices, anzEvents, anzNVODevents, anzMetaServices,
	        //    resourceUsage.ru_maxrss, resourceUsage.ru_ixrss, resourceUsage.ru_idrss, resourceUsage.ru_isrss,
	        speicherinfo.uordblks,
	        speicherinfo.arena, speicherinfo.arena / 1024, (float)speicherinfo.arena / (1024.*1024.)
	       );

	struct sectionsd::msgResponseHeader responseHeader;

	responseHeader.dataLength = strlen(stati) + 1;

	if (writeNbytes(client->connectionSocket, (const char *)&responseHeader, sizeof(responseHeader), TIMEOUT_CONNECTIONS) > 0)
	{
		if (responseHeader.dataLength)
			writeNbytes(client->connectionSocket, stati, responseHeader.dataLength, TIMEOUT_CONNECTIONS);
	}
	else
		dputs("[sectionsd] Fehler/Timeout bei write");

	return ;
}

static void commandCurrentNextInfoChannelName(struct connectionData *client, char *data, const unsigned dataLength)
{
	int nResultDataSize = 0;
	char* pResultData = 0;

	data[dataLength - 1] = 0; // to be sure it has an trailing 0
	dprintf("Request of current/next information for '%s'\n", data);

	if (dmxEIT.pause()) // -> lock
		return ;

	lockServices();

	lockEvents();

	SItime zeitEvt1(0, 0);

	const SIevent &evt = findActualSIeventForServiceName(data, zeitEvt1);

	unlockServices();

	if (evt.serviceID != 0)
	{ //Found
		dprintf("current EPG found.\n");
		SItime zeitEvt2(zeitEvt1);
		const SIevent &nextEvt = findNextSIevent(evt.uniqueKey(), zeitEvt2);

		if (nextEvt.serviceID != 0)
		{
			dprintf("next EPG found.\n");
			// Folgendes ist grauenvoll, habs aber einfach kopiert aus epgd
			// und keine Lust das grossartig zu verschoenern
			nResultDataSize =
			    12 + 1 + 					// Unique-Key + del
			    strlen(evt.name.c_str()) + 1 + 		//Name + del
			    3 + 2 + 1 + 					//std:min + del
			    4 + 1 + 					//dauer (mmmm) + del
			    3 + 1 + 					//100 + del
			    12 + 1 + 					// Unique-Key + del
			    strlen(nextEvt.name.c_str()) + 1 + 		//Name + del
			    3 + 2 + 1 + 					//std:min + del
			    4 + 1 + 1;					//dauer (mmmm) + del + 0
			pResultData = new char[nResultDataSize];

			if (!pResultData)
			{
				fprintf(stderr, "low on memory!\n");
				unlockEvents();
				dmxEIT.unpause();
				return ;
			}

			struct tm *pStartZeit = localtime(&zeitEvt1.startzeit);

			int nSH(pStartZeit->tm_hour), nSM(pStartZeit->tm_min);

			unsigned dauer = zeitEvt1.dauer / 60;

			unsigned nProcentagePassed = (unsigned)((float)(time(NULL) - zeitEvt1.startzeit) / (float)zeitEvt1.dauer * 100.);

			pStartZeit = localtime(&zeitEvt2.startzeit);

			int nSH2(pStartZeit->tm_hour), nSM2(pStartZeit->tm_min);

			unsigned dauer2 = zeitEvt2.dauer / 60;

			sprintf(pResultData,
			        "%012llx\n%s\n%02d:%02d\n%04u\n%03u\n%012llx\n%s\n%02d:%02d\n%04u\n",
			        evt.uniqueKey(),
			        evt.name.c_str(),
			        nSH, nSM, dauer, nProcentagePassed,
			        nextEvt.uniqueKey(),
			        nextEvt.name.c_str(),
			        nSH2, nSM2, dauer2);
		}
	}

	unlockEvents();
	dmxEIT.unpause(); // -> unlock

	// response

	struct sectionsd::msgResponseHeader pmResponse;
	pmResponse.dataLength = nResultDataSize;
	int rc = writeNbytes(client->connectionSocket, (const char *)&pmResponse, sizeof(pmResponse), TIMEOUT_CONNECTIONS);

	if ( nResultDataSize > 0 )
	{
		if (rc > 0)
			writeNbytes(client->connectionSocket, pResultData, nResultDataSize, TIMEOUT_CONNECTIONS);
		else
			dputs("[sectionsd] Fehler/Timeout bei write");

		delete[] pResultData;
	}
	else
	{
		dprintf("current/next EPG not found!\n");
	}

	return ;
}

static void commandComponentTagsUniqueKey(struct connectionData *client, char *data, const unsigned dataLength)
{
	int nResultDataSize = 0;
	char* pResultData = 0;

	if (dataLength != 8)
		return ;

	unsigned long long uniqueKey = *(unsigned long long*)data;

	dprintf("Request of ComponentTags for 0x%llx\n", uniqueKey);

	if (dmxEIT.pause()) // -> lock
		return ;

	lockEvents();

	nResultDataSize = sizeof(int);    // num. Component-Tags

	MySIeventsOrderUniqueKey::iterator eFirst = mySIeventsOrderUniqueKey.find(uniqueKey);

	if (eFirst != mySIeventsOrderUniqueKey.end())
	{
		//Found
		dprintf("ComponentTags found.\n");
		dprintf("components.size %d \n", eFirst->second->components.size());

		for (SIcomponents::iterator cmp = eFirst->second->components.begin(); cmp != eFirst->second->components.end(); cmp++)
		{
			dprintf(" %s \n", cmp->component.c_str());
			nResultDataSize += strlen(cmp->component.c_str()) + 1 +  	// name
			                   sizeof(unsigned char) +  //componentType
			                   sizeof(unsigned char) +  //componentTag
			                   sizeof(unsigned char); //streamContent
		}
	}

	pResultData = new char[nResultDataSize];

	if (!pResultData)
	{
		fprintf(stderr, "low on memory!\n");
		unlockEvents();
		dmxEIT.unpause();
		return ;
	}

	char *p = pResultData;

	if (eFirst != mySIeventsOrderUniqueKey.end())
	{
		*((int *)p) = eFirst->second->components.size();
		p += sizeof(int);

		for (SIcomponents::iterator cmp = eFirst->second->components.begin(); cmp != eFirst->second->components.end(); cmp++)
		{

			strcpy(p, cmp->component.c_str());
			p += strlen(cmp->component.c_str()) + 1;
			*((unsigned char *)p) = cmp->componentType;
			p += sizeof(unsigned char);
			*((unsigned char *)p) = cmp->componentTag;
			p += sizeof(unsigned char);
			*((unsigned char *)p) = cmp->streamContent;
			p += sizeof(unsigned char);
		}
	}
	else
	{
		*((int *)p) = 0;
		p += sizeof(int);
	}

	unlockEvents();
	dmxEIT.unpause(); // -> unlock

	struct sectionsd::msgResponseHeader responseHeader;
	responseHeader.dataLength = nResultDataSize;

	if (writeNbytes(client->connectionSocket, (const char *)&responseHeader, sizeof(responseHeader), TIMEOUT_CONNECTIONS) > 0)
	{
		if (responseHeader.dataLength)
			writeNbytes(client->connectionSocket, pResultData, responseHeader.dataLength, TIMEOUT_CONNECTIONS);
	}
	else
		dputs("[sectionsd] Fehler/Timeout bei write");

	delete[] pResultData;

	return ;
}

static void commandLinkageDescriptorsUniqueKey(struct connectionData *client, char *data, const unsigned dataLength)
{
	int nResultDataSize = 0;
	char* pResultData = 0;

	if (dataLength != 8)
		return ;

	unsigned long long uniqueKey = *(unsigned long long*)data;

	dprintf("Request of LinkageDescriptors for 0x%llx\n", uniqueKey);

	if (dmxEIT.pause()) // -> lock
		return ;

	lockEvents();

	nResultDataSize = sizeof(int);    // num. Component-Tags

	int countDescs = 0;

	MySIeventsOrderUniqueKey::iterator eFirst = mySIeventsOrderUniqueKey.find(uniqueKey);

	if (eFirst != mySIeventsOrderUniqueKey.end())
	{
		//Found
		dprintf("LinkageDescriptors found.\n");
		dprintf("linkage_descs.size %d \n", eFirst->second->linkage_descs.size());


		for (SIlinkage_descs::iterator linkage_desc = eFirst->second->linkage_descs.begin(); linkage_desc != eFirst->second->linkage_descs.end(); linkage_desc++)
		{
			if (linkage_desc->linkageType == 0xB0)
			{
				countDescs++;
				dprintf(" %s \n", linkage_desc->name.c_str());
				nResultDataSize += strlen(linkage_desc->name.c_str()) + 1 +  	// name
				                   sizeof(unsigned short) +  //transportStreamId
				                   sizeof(unsigned short) +  //originalNetworkId
				                   sizeof(unsigned short); //serviceId
			}
		}
	}

	pResultData = new char[nResultDataSize];

	if (!pResultData)
	{
		fprintf(stderr, "low on memory!\n");
		unlockEvents();
		dmxEIT.unpause();
		return ;
	}

	char *p = pResultData;

	*((int *)p) = countDescs;
	p += sizeof(int);

	if (eFirst != mySIeventsOrderUniqueKey.end())
	{
		for (SIlinkage_descs::iterator linkage_desc = eFirst->second->linkage_descs.begin(); linkage_desc != eFirst->second->linkage_descs.end(); linkage_desc++)
		{
			if (linkage_desc->linkageType == 0xB0)
			{
				strcpy(p, linkage_desc->name.c_str());
				p += strlen(linkage_desc->name.c_str()) + 1;
				*((unsigned short *)p) = linkage_desc->transportStreamId;
				p += sizeof(unsigned short);
				*((unsigned short *)p) = linkage_desc->originalNetworkId;
				p += sizeof(unsigned short);
				*((unsigned short *)p) = linkage_desc->serviceId;
				p += sizeof(unsigned short);
			}
		}
	}

	unlockEvents();
	dmxEIT.unpause(); // -> unlock

	struct sectionsd::msgResponseHeader responseHeader;
	responseHeader.dataLength = nResultDataSize;

	if (writeNbytes(client->connectionSocket, (const char *)&responseHeader, sizeof(responseHeader), TIMEOUT_CONNECTIONS) > 0)
	{
		if (responseHeader.dataLength)
			writeNbytes(client->connectionSocket, pResultData, responseHeader.dataLength, TIMEOUT_CONNECTIONS);
	}
	else
		dputs("[sectionsd] Fehler/Timeout bei write");

	delete[] pResultData;

	return ;
}

static unsigned	messaging_current_servicekey = 0;
std::vector<long long> messaging_skipped_sections_ID [0x22];		// 0x4e .. 0x6f
static long long messaging_sections_max_ID [0x22];			// 0x4e .. 0x6f
static int messaging_sections_got_all [0x22];			// 0x4e .. 0x6f
std::vector<long long>	messaging_sdt_skipped_sections_ID [2];		// 0x42, 0x46
static long long messaging_sdt_sections_max_ID [2];			// 0x42, 0x46
static int messaging_sdt_sections_got_all [2];			// 0x42, 0x46

static bool	messaging_wants_current_next_Event = false;
static time_t messaging_last_requested = time(NULL);
static bool	messaging_neutrino_sets_time = false;

static void commandserviceChanged(struct connectionData *client, char *data, const unsigned dataLength)
{
	//int nResultDataSize=0;
	//char* pResultData=0;

	if (dataLength != 8)
		return ;

	unsigned* uniqueServiceKey = (unsigned *)data;

	data += 4;

	bool* requestCN_Event = (bool *)data;

	bool doWakeUp = false;

	dprintf("[sectionsd] Service changed to 0x%x\n", *uniqueServiceKey);

	showProfiling("before messaging lock");

	time_t zeit = time(NULL);

	lockMessaging();

	if ( ( messaging_current_servicekey != *uniqueServiceKey ) ||
	        ( zeit > ( messaging_last_requested + 5 ) ) )
	{
		messaging_current_servicekey = *uniqueServiceKey;

		for ( int i = 0x4e; i <= 0x6f; i++)
		{
			messaging_skipped_sections_ID[i - 0x4e].clear();
			messaging_sections_max_ID[i - 0x4e] = -1;
			messaging_sections_got_all[i - 0x4e] = false;
		}

		for ( int i = 0; i <= 1; i++)
		{
			messaging_sdt_skipped_sections_ID[i].clear();
			messaging_sdt_sections_max_ID[i] = -1;
			messaging_sdt_sections_got_all[i] = false;
		}

		doWakeUp = true;
	}



	if ( ( !doWakeUp ) && ( messaging_sections_got_all[0] ) && ( *requestCN_Event ) )
	{
		messaging_wants_current_next_Event = false;
		eventServer->sendEvent(CSectionsdClient::EVT_GOT_CN_EPG, CEventServer::INITID_SECTIONSD, &messaging_current_servicekey, sizeof(messaging_current_servicekey) );
	}
	else
	{
		messaging_wants_current_next_Event = *requestCN_Event;

		if (messaging_wants_current_next_Event)
			dprintf("[sectionsd] requesting current_next event...\n");
	}

	showProfiling("before wakeup");
	messaging_last_requested = zeit;

	if ( doWakeUp )
	{
		// nur wenn lange genug her, oder wenn anderer Service :)
		dmxEIT.change( 0 );
		dmxSDT.change( 0 );
	}
	else
		dprintf("[sectionsd] ignoring wakeup request...\n");

	unlockMessaging();

	showProfiling("after doWakeup");

	struct sectionsd::msgResponseHeader msgResponse;

	msgResponse.dataLength = 0;

	writeNbytes(client->connectionSocket, (const char *)&msgResponse, sizeof(msgResponse), TIMEOUT_CONNECTIONS);

	return ;
}

static void commandCurrentNextInfoChannelID(struct connectionData *client, char *data, const unsigned dataLength)
{
	int nResultDataSize = 0;
	char* pResultData = 0;

	if (dataLength != 4)
		return ;

	unsigned* uniqueServiceKey = (unsigned *)data;

	dprintf("[sectionsd] Request of current/next information for 0x%x\n", *uniqueServiceKey);

	if (dmxEIT.pause()) // -> lock
		return ;

	lockServices();

	lockEvents();

	SItime zeitEvt1(0, 0);

	unsigned flag = 0;

	const SIevent &evt = findActualSIeventForServiceUniqueKey(*uniqueServiceKey, zeitEvt1, 0, &flag);

	if (evt.serviceID == 0)
	{
		MySIservicesOrderUniqueKey::iterator si = mySIservicesOrderUniqueKey.end();
		si = mySIservicesOrderUniqueKey.find(*uniqueServiceKey);

		if (si != mySIservicesOrderUniqueKey.end())
		{
			dprintf("[sectionsd] current service has%s scheduled events, and has%s present/following events\n", si->second->eitScheduleFlag() ? "" : " no", si->second->eitPresentFollowingFlag() ? "" : " no" );

			if ( /*( !si->second->eitScheduleFlag() ) || */
			    ( !si->second->eitPresentFollowingFlag() ) )
			{
				flag |= sectionsd::epgflags::not_broadcast;
			}
		}
	}

	//dprintf("[sectionsd] current flag %d\n", flag);

	unlockServices();

	SIevent nextEvt;

	SItime zeitEvt2(zeitEvt1);

	if (evt.serviceID != 0)
	{ //Found
		dprintf("[sectionsd] current EPG found.\n");

		for (unsigned int i = 0; i < evt.linkage_descs.size(); i++)
			if (evt.linkage_descs[i].linkageType == 0xB0)
			{
				flag |= sectionsd::epgflags::current_has_linkagedescriptors;
				break;
			}


		nextEvt = findNextSIevent(evt.uniqueKey(), zeitEvt2);
	}
	else
		if ( flag & sectionsd::epgflags::has_anything )
		{

			nextEvt = findNextSIeventForServiceUniqueKey(*uniqueServiceKey, zeitEvt2);

			if (nextEvt.serviceID != 0)
			{
				MySIeventsOrderUniqueKey::iterator eFirst = mySIeventsOrderUniqueKey.find(*uniqueServiceKey);

				if (eFirst != mySIeventsOrderUniqueKey.end())
				{
					eFirst--;

					if (eFirst != mySIeventsOrderUniqueKey.begin())
					{
						time_t azeit = time(NULL);

						if ( ( eFirst->second->times.begin()->startzeit < azeit ) &&
						        ( eFirst->second->uniqueKey() == (nextEvt.uniqueKey() - 1) ) )
							flag |= sectionsd::epgflags::has_no_current;
					}
				}
			}
		}

	if (nextEvt.serviceID != 0)
	{
		dprintf("[sectionsd] next EPG found.\n");
		flag |= sectionsd::epgflags::has_next;
	}

	nResultDataSize =
	    sizeof(unsigned long long) +        	// Unique-Key
	    sizeof(sectionsd::sectionsdTime) +  	// zeit
	    strlen(evt.name.c_str()) + 1 + 	  		// name + 0
	    sizeof(unsigned long long) +        	// Unique-Key
	    sizeof(sectionsd::sectionsdTime) +  	// zeit
	    strlen(nextEvt.name.c_str()) + 1 +    	// name + 0
	    sizeof(unsigned) + 					// flags
	    1									// CurrentFSK
	    ;

	pResultData = new char[nResultDataSize];

	if (!pResultData)
	{
		fprintf(stderr, "low on memory!\n");
		unlockEvents();
		dmxEIT.unpause();
		return ;
	}

	char *p = pResultData;
	*((unsigned long long *)p) = evt.uniqueKey();
	p += sizeof(unsigned long long);
	sectionsd::sectionsdTime zeit;
	zeit.startzeit = zeitEvt1.startzeit;
	zeit.dauer = zeitEvt1.dauer;
	*((sectionsd::sectionsdTime *)p) = zeit;
	p += sizeof(sectionsd::sectionsdTime);
	strcpy(p, evt.name.c_str());
	p += strlen(evt.name.c_str()) + 1;
	*((unsigned long long *)p) = nextEvt.uniqueKey();
	p += sizeof(unsigned long long);
	zeit.startzeit = zeitEvt2.startzeit;
	zeit.dauer = zeitEvt2.dauer;
	*((sectionsd::sectionsdTime *)p) = zeit;
	p += sizeof(sectionsd::sectionsdTime);
	strcpy(p, nextEvt.name.c_str());
	p += strlen(nextEvt.name.c_str()) + 1;
	*((unsigned*)p) = flag;
	p += sizeof(unsigned);
	*p = evt.getFSK();
	//int x= evt.getFSK();
	p++;

	unlockEvents();
	dmxEIT.unpause(); // -> unlock

	// response

	struct sectionsd::msgResponseHeader pmResponse;
	pmResponse.dataLength = nResultDataSize;
	int rc = writeNbytes(client->connectionSocket, (const char *)&pmResponse, sizeof(pmResponse), TIMEOUT_CONNECTIONS);

	if ( nResultDataSize > 0 )
	{
		if (rc > 0)
			writeNbytes(client->connectionSocket, pResultData, nResultDataSize, TIMEOUT_CONNECTIONS);
		else
			dputs("[sectionsd] Fehler/Timeout bei write");

		delete[] pResultData;
	}
	else
	{
		dprintf("[sectionsd] current/next EPG not found!\n");
	}

	return ;
}

// Sendet ein EPG, unlocked die events, unpaused dmxEIT

static void sendEPG(struct connectionData *client, const SIevent& e, const SItime& t, int shortepg = 0)
{

	struct sectionsd::msgResponseHeader responseHeader;

	if (!shortepg)
	{
		// new format - 0 delimiters
		responseHeader.dataLength =
		    sizeof(unsigned long long) +        // Unique-Key
		    strlen(e.name.c_str()) + 1 + 		// Name + del
		    strlen(e.text.c_str()) + 1 + 		// Text + del
		    strlen(e.extendedText.c_str()) + 1 + 	// ext + del
		    strlen(e.contentClassification.c_str()) + 1 + 		// Text + del
		    strlen(e.userClassification.c_str()) + 1 + 	// ext + del
		    1 +                                   // fsk
		    sizeof(sectionsd::sectionsdTime); // zeit
	}
	else
		responseHeader.dataLength =
		    strlen(e.name.c_str()) + 1 + 		// Name + del
		    strlen(e.text.c_str()) + 1 + 		// Text + del
		    strlen(e.extendedText.c_str()) + 1 + 1; // ext + del + 0

	char* msgData = new char[responseHeader.dataLength];

	if (!msgData)
	{
		fprintf(stderr, "low on memory!\n");
		unlockEvents();
		dmxEIT.unpause(); // -> unlock
		return ;
	}

	if (!shortepg)
	{
		char *p = msgData;
		*((unsigned long long *)p) = e.uniqueKey();
		p += sizeof(unsigned long long);

		strcpy(p, e.name.c_str());
		p += strlen(e.name.c_str()) + 1;
		strcpy(p, e.text.c_str());
		p += strlen(e.text.c_str()) + 1;
		strcpy(p, e.extendedText.c_str());
		p += strlen(e.extendedText.c_str()) + 1;
		strcpy(p, e.contentClassification.c_str());
		p += strlen(e.contentClassification.c_str()) + 1;
		strcpy(p, e.userClassification.c_str());
		p += strlen(e.userClassification.c_str()) + 1;
		*p = e.getFSK();
		p++;

		sectionsd::sectionsdTime zeit;
		zeit.startzeit = t.startzeit;
		zeit.dauer = t.dauer;
		*((sectionsd::sectionsdTime *)p) = zeit;
		p += sizeof(sectionsd::sectionsdTime);

	}
	else
		sprintf(msgData,
		        "%s\xFF%s\xFF%s\xFF",
		        e.name.c_str(),
		        e.text.c_str(),
		        e.extendedText.c_str()
		       );

	unlockEvents();

	dmxEIT.unpause(); // -> unlock

	int rc = writeNbytes(client->connectionSocket, (const char *)&responseHeader, sizeof(responseHeader), TIMEOUT_CONNECTIONS);

	if (rc > 0)
		writeNbytes(client->connectionSocket, msgData, responseHeader.dataLength, TIMEOUT_CONNECTIONS);
	else
		dputs("[sectionsd] Fehler/Timeout bei write");

	delete[] msgData;
}

static void commandGetNextEPG(struct connectionData *client, char *data, const unsigned dataLength)
{
	if (dataLength != 8 + 4)
		return ;

	unsigned long long *uniqueEventKey = (unsigned long long *)data;

	time_t *starttime = (time_t *)(data + 8);

	dprintf("Request of next epg for 0x%llx %s", *uniqueEventKey, ctime(starttime));

	if (dmxEIT.pause()) // -> lock
		return ;

	lockEvents();

	SItime zeit(*starttime, 0);

	const SIevent &nextEvt = findNextSIevent(*uniqueEventKey, zeit);

	if (nextEvt.serviceID != 0)
	{
		dprintf("next epg found.\n");
		sendEPG(client, nextEvt, zeit);
	}
	else
	{
		unlockEvents();
		dmxEIT.unpause(); // -> unlock
		dprintf("next epg not found!\n");

		struct sectionsd::msgResponseHeader responseHeader;
		responseHeader.dataLength = 0;
		writeNbytes(client->connectionSocket, (const char *)&responseHeader, sizeof(responseHeader), TIMEOUT_CONNECTIONS);
	}

	return ;
}

static void commandActualEPGchannelID(struct connectionData *client, char *data, const unsigned dataLength)
{
	if (dataLength != 4)
		return ;

	unsigned* uniqueServiceKey = (unsigned *)data;

	dprintf("Request of actual EPG for 0x%x\n", * uniqueServiceKey);

	if (dmxEIT.pause()) // -> lock
		return ;

	lockEvents();

	SItime zeit(0, 0);

	const SIevent &evt = findActualSIeventForServiceUniqueKey(*uniqueServiceKey, zeit);

	if (evt.serviceID != 0)
	{
		dprintf("EPG found.\n");
		sendEPG(client, evt, zeit);
	}
	else
	{
		unlockEvents();
		dmxEIT.unpause(); // -> unlock
		dprintf("EPG not found!\n");

		struct sectionsd::msgResponseHeader responseHeader;
		responseHeader.dataLength = 0;
		writeNbytes(client->connectionSocket, (const char *)&responseHeader, sizeof(responseHeader), TIMEOUT_CONNECTIONS);
	}

	return ;
}

static void commandGetEPGPrevNext(struct connectionData *client, char *data, const unsigned dataLength)
{
	if (dataLength != 8 + 4)
		return ;

	unsigned long long *uniqueEventKey = (unsigned long long *)data;

	time_t *starttime = (time_t *)(data + 8);

	dprintf("Request of Prev/Next EPG for 0x%llx %s", *uniqueEventKey, ctime(starttime));

	if (dmxEIT.pause()) // -> lock
		return ;

	lockEvents();

	SItime zeit(*starttime, 0);

	SItime prev_zeit(0, 0);

	SItime next_zeit(0, 0);

	SIevent prev_evt;

	SIevent next_evt;

	findPrevNextSIevent(*uniqueEventKey, zeit, prev_evt, prev_zeit, next_evt, next_zeit);

	struct sectionsd::msgResponseHeader responseHeader;

	responseHeader.dataLength =
	    12 + 1 + 				// Unique-Key + del
	    8 + 1 + 				// start time + del
	    12 + 1 + 				// Unique-Key + del
	    8 + 1 + 1;				// start time + del

	char* msgData = new char[responseHeader.dataLength];

	if (!msgData)
	{
		fprintf(stderr, "low on memory!\n");
		unlockEvents();
		dmxEIT.unpause(); // -> unlock
		return ;
	}

	sprintf(msgData, "%012llx\xFF%08lx\xFF%012llx\xFF%08lx\xFF",
	        prev_evt.uniqueKey(),
	        prev_zeit.startzeit,
	        next_evt.uniqueKey(),
	        next_zeit.startzeit
	       );
	unlockEvents();
	dmxEIT.unpause(); // -> unlock

	int rc = writeNbytes(client->connectionSocket, (const char *)&responseHeader, sizeof(responseHeader), TIMEOUT_CONNECTIONS);

	if (rc > 0)
		writeNbytes(client->connectionSocket, msgData, responseHeader.dataLength, TIMEOUT_CONNECTIONS);
	else
		dputs("[sectionsd] Fehler/Timeout bei write");

	delete[] msgData;

	return ;
}

// Mostly copied from epgd (something bugfixed ;) )

static void commandActualEPGchannelName(struct connectionData *client, char *data, const unsigned dataLength)
{
	int nResultDataSize = 0;
	char* pResultData = 0;

	data[dataLength - 1] = 0; // to be sure it has an trailing 0
	dprintf("Request of actual EPG for '%s'\n", data);

	if (dmxEIT.pause()) // -> lock
		return ;

	lockServices();

	lockEvents();

	SItime zeitEvt(0, 0);

	const SIevent &evt = findActualSIeventForServiceName(data, zeitEvt);

	unlockServices();

	if (evt.serviceID != 0)
	{ //Found
		dprintf("EPG found.\n");
		nResultDataSize =
		    12 + 1 + 					// Unique-Key + del
		    strlen(evt.name.c_str()) + 1 + 		//Name + del
		    strlen(evt.text.c_str()) + 1 + 		//Text + del
		    strlen(evt.extendedText.c_str()) + 1 + 	//ext + del
		    3 + 3 + 4 + 1 + 					//dd.mm.yyyy + del
		    3 + 2 + 1 + 					//std:min + del
		    3 + 2 + 1 + 					//std:min+ del
		    3 + 1 + 1;					//100 + del + 0
		pResultData = new char[nResultDataSize];

		if (!pResultData)
		{
			fprintf(stderr, "low on memory!\n");
			unlockEvents();
			dmxEIT.unpause();
			return ;
		}

		struct tm *pStartZeit = localtime(&zeitEvt.startzeit);

		int nSDay(pStartZeit->tm_mday), nSMon(pStartZeit->tm_mon + 1), nSYear(pStartZeit->tm_year + 1900),
		nSH(pStartZeit->tm_hour), nSM(pStartZeit->tm_min);

		long int uiEndTime(zeitEvt.startzeit + zeitEvt.dauer);

		struct tm *pEndeZeit = localtime((time_t*) & uiEndTime);

		int nFH(pEndeZeit->tm_hour), nFM(pEndeZeit->tm_min);

		unsigned nProcentagePassed = (unsigned)((float)(time(NULL) - zeitEvt.startzeit) / (float)zeitEvt.dauer * 100.);

		sprintf(pResultData, "%012llx\xFF%s\xFF%s\xFF%s\xFF%02d.%02d.%04d\xFF%02d:%02d\xFF%02d:%02d\xFF%03u\xFF",
		        evt.uniqueKey(),
		        evt.name.c_str(),
		        evt.text.c_str(),
		        evt.extendedText.c_str(), nSDay, nSMon, nSYear, nSH, nSM, nFH, nFM, nProcentagePassed );
	}
	else
		dprintf("actual EPG not found!\n");

	unlockEvents();

	dmxEIT.unpause(); // -> unlock

	// response

	struct sectionsd::msgResponseHeader pmResponse;

	pmResponse.dataLength = nResultDataSize;

	int rc = writeNbytes(client->connectionSocket, (const char *)&pmResponse, sizeof(pmResponse), TIMEOUT_CONNECTIONS);

	if ( nResultDataSize > 0 )
	{
		if (rc > 0)
			writeNbytes(client->connectionSocket, pResultData, nResultDataSize, TIMEOUT_CONNECTIONS);
		else
			dputs("[sectionsd] Fehler/Timeout bei write");

		delete[] pResultData;
	}
}

static void sendEventList(struct connectionData *client, const unsigned char serviceTyp1, const unsigned char serviceTyp2 = 0, int sendServiceName = 1)
{
	char *evtList = new char[128* 1024]; // 256kb..? should be enough and dataLength is unsigned short

	if (!evtList)
	{
		fprintf(stderr, "low on memory!\n");
		return ;
	}

	*evtList = 0;

	if (dmxEIT.pause())
	{
		delete[] evtList;
		return ;
	}

	if (dmxSDT.pause())
	{
		delete[] evtList;
		dmxEIT.unpause();
		return ;
	}

	char *liste = evtList;
	lockServices();
	lockEvents();

	unsigned uniqueNow = 0;
	unsigned uniqueOld = 0;
	bool found_already = false;
	time_t azeit = time(NULL);
	std::string sname;

	for (MySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey::iterator e = mySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey.begin(); e != mySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey.end(); e++)
	{
		uniqueNow = SIservice::makeUniqueKey(e->first->originalNetworkID, e->first->serviceID);

		if ( uniqueNow != uniqueOld )
		{
			found_already = true;

			// new service, check service- type
			MySIservicesOrderUniqueKey::iterator s = mySIservicesOrderUniqueKey.find(uniqueNow);

			if (s != mySIservicesOrderUniqueKey.end())
			{
				if (s->second->serviceTyp == serviceTyp1 || (serviceTyp2 && s->second->serviceTyp == serviceTyp2))
				{
					sname = s->second->serviceName;
					found_already = false;
				}
			}
			else
			{
				// wenn noch nie hingetuned wurde, dann gibts keine Info über den ServiceTyp...
				// im Zweifel mitnehmen
				found_already = false;
			}

			uniqueOld = uniqueNow;
		}

		if ( !found_already )
		{
			for (SItimes::iterator t = e->first->times.begin(); t != e->first->times.end(); t++)
				if (t->startzeit <= azeit && azeit <= (long)(t->startzeit + t->dauer))
				{
					if (sendServiceName)
					{
						sprintf(liste, "%012llx\n", e->first->uniqueKey());
						liste += 13;
						strcpy(liste, sname.c_str());
						liste += strlen(sname.c_str());
						*liste = '\n';
						liste++;
						strcpy(liste, e->first->name.c_str());
						liste += strlen(e->first->name.c_str());
						*liste = '\n';
						liste++;
					} // if sendServiceName
					else
					{
						*((unsigned long long *)liste) = e->first->uniqueKey();
						liste += 8;
						*((unsigned *)liste) = t->startzeit;
						liste += 4;
						*((unsigned *)liste) = t->dauer;
						liste += 4;
						strcpy(liste, e->first->name.c_str());
						liste += strlen(liste);
						liste++;

						if (e->first->text == "" )
						{
							strcpy(liste, e->first->extendedText.substr(0, 40).c_str());
							liste += strlen(liste);
						}
						else
						{
							strcpy(liste, e->first->text.c_str());
							liste += strlen(liste);
						}

						liste++;
					} // else !sendServiceName

					found_already = true;

					break;
				}
		}
	}

	if (sendServiceName)
	{
		*liste = 0;
		liste++;
	}

	unlockEvents();
	unlockServices();

	dmxSDT.unpause();
	dmxEIT.unpause();

	struct sectionsd::msgResponseHeader msgResponse;
	msgResponse.dataLength = liste - evtList;
	dprintf("[sectionsd] all channels - response-size: 0x%x\n", msgResponse.dataLength);

	if ( msgResponse.dataLength == 1 )
		msgResponse.dataLength = 0;

	if (writeNbytes(client->connectionSocket, (const char *)&msgResponse, sizeof(msgResponse), TIMEOUT_CONNECTIONS) > 0)
	{
		if (msgResponse.dataLength)
			writeNbytes(client->connectionSocket, evtList, msgResponse.dataLength, TIMEOUT_CONNECTIONS);
	}
	else
		dputs("[sectionsd] Fehler/Timeout bei write");

	delete[] evtList;
}

// Sendet ein short EPG, unlocked die events, unpaused dmxEIT

static void sendShort(struct connectionData *client, const SIevent& e, const SItime& t)
{

	struct sectionsd::msgResponseHeader responseHeader;

	responseHeader.dataLength =
	    12 + 1 + 				// Unique-Key + del
	    strlen(e.name.c_str()) + 1 + 		// name + del
	    8 + 1 + 				// start time + del
	    8 + 1 + 1;				// duration + del + 0
	char* msgData = new char[responseHeader.dataLength];

	if (!msgData)
	{
		fprintf(stderr, "low on memory!\n");
		unlockEvents();
		dmxEIT.unpause(); // -> unlock
		return ;
	}

	sprintf(msgData,
	        "%012llx\n%s\n%08lx\n%08x\n",
	        e.uniqueKey(),
	        e.name.c_str(),
	        t.startzeit,
	        t.dauer
	       );
	unlockEvents();
	dmxEIT.unpause(); // -> unlock

	int rc = writeNbytes(client->connectionSocket, (const char *)&responseHeader, sizeof(responseHeader), TIMEOUT_CONNECTIONS);

	if (rc > 0)
		writeNbytes(client->connectionSocket, msgData, responseHeader.dataLength, TIMEOUT_CONNECTIONS);
	else
		dputs("[sectionsd] Fehler/Timeout bei write");

	delete[] msgData;
}

static void commandGetNextShort(struct connectionData *client, char *data, const unsigned dataLength)
{
	if (dataLength != 8 + 4)
		return ;

	unsigned long long *uniqueEventKey = (unsigned long long *)data;

	time_t *starttime = (time_t *)(data + 8);

	dprintf("Request of next short for 0x%llx %s", *uniqueEventKey, ctime(starttime));

	if (dmxEIT.pause()) // -> lock
		return ;

	lockEvents();

	SItime zeit(*starttime, 0);

	const SIevent &nextEvt = findNextSIevent(*uniqueEventKey, zeit);

	if (nextEvt.serviceID != 0)
	{
		dprintf("next short found.\n");
		sendShort(client, nextEvt, zeit);
	}
	else
	{
		unlockEvents();
		dmxEIT.unpause(); // -> unlock
		dprintf("next short not found!\n");

		struct sectionsd::msgResponseHeader responseHeader;
		responseHeader.dataLength = 0;
		writeNbytes(client->connectionSocket, (const char *)&responseHeader, sizeof(responseHeader), TIMEOUT_CONNECTIONS);
	}

	return ;
}

static void commandEventListTV(struct connectionData *client, char *data, const unsigned dataLength)
{
	if (dataLength)
		return ;

	dputs("Request of TV event list.\n");

	sendEventList(client, 0x01, 0x04);

	return ;
}

static void commandEventListTVids(struct connectionData *client, char *data, const unsigned dataLength)
{
	if (dataLength)
		return ;

	dputs("Request of TV event list (IDs).\n");

	sendEventList(client, 0x01, 0x04, 0);

	return ;
}

static void commandEventListRadio(struct connectionData *client, char *data, const unsigned dataLength)
{
	if (dataLength)
		return ;

	dputs("Request of radio event list.\n");

	sendEventList(client, 0x02);

	return ;
}

static void commandEventListRadioIDs(struct connectionData *client, char *data, const unsigned dataLength)
{
	if (dataLength)
		return ;

	dputs("Request of radio event list (IDs).\n");

	sendEventList(client, 0x02, 0, 0);

	return ;
}

static void commandEPGepgID(struct connectionData *client, char *data, const unsigned dataLength)
{
	if (dataLength != 8 + 4)
		return ;

	unsigned long long* epgID = (unsigned long long*)data;

	time_t* startzeit = (time_t *)(data + 8);

	dprintf("Request of actual EPG for 0x%llx 0x%lx\n", *epgID, *startzeit);

	if (dmxEIT.pause()) // -> lock
		return ;

	lockEvents();

	const SIevent& evt = findSIeventForEventUniqueKey(*epgID);

	if (evt.serviceID != 0)
	{ // Event found
		SItimes::iterator t = evt.times.begin();

		for (; t != evt.times.end(); t++)
			if (t->startzeit == *startzeit)
				break;

		if (t == evt.times.end())
		{
			dputs("EPG not found!");
			unlockEvents();
			dmxEIT.unpause(); // -> unlock
		}
		else
		{
			dputs("EPG found.");
			// Sendet ein EPG, unlocked die events, unpaused dmxEIT
			sendEPG(client, evt, *t);
		}
	}
	else
	{
		dputs("EPG not found!");
		unlockEvents();
		dmxEIT.unpause(); // -> unlock
		// response

		struct sectionsd::msgResponseHeader pmResponse;
		pmResponse.dataLength = 0;

		if (writeNbytes(client->connectionSocket, (const char *)&pmResponse, sizeof(pmResponse), TIMEOUT_CONNECTIONS) <= 0)
			dputs("[sectionsd] Fehler/Timeout bei write");
	}
}

static void commandEPGepgIDshort(struct connectionData *client, char *data, const unsigned dataLength)
{
	if (dataLength != 8)
		return ;

	unsigned long long* epgID = (unsigned long long*)data;

	dprintf("Request of actual EPG for 0x%llx\n", *epgID);

	if (dmxEIT.pause()) // -> lock
		return ;

	lockEvents();

	const SIevent& evt = findSIeventForEventUniqueKey(*epgID);

	if (evt.serviceID != 0)
	{ // Event found
		dputs("EPG found.");
		sendEPG(client, evt, SItime(0, 0), 1);
	}
	else
	{
		dputs("EPG not found!");
		unlockEvents();
		dmxEIT.unpause(); // -> unlock
		// response

		struct sectionsd::msgResponseHeader pmResponse;
		pmResponse.dataLength = 0;

		if (writeNbytes(client->connectionSocket, (const char *)&pmResponse, sizeof(pmResponse), TIMEOUT_CONNECTIONS) <= 0)
			dputs("[sectionsd] Fehler/Timeout bei write");
	}
}

static void commandTimesNVODservice(struct connectionData *client, char *data, const unsigned dataLength)
{
	if (dataLength != 4)
		return ;

	unsigned uniqueServiceKey = *(unsigned *)data;

	dprintf("Request of NVOD times for 0x%x\n", uniqueServiceKey);

	if (dmxEIT.pause()) // -> lock
		return ;

	lockServices();

	lockEvents();

	MySIservicesNVODorderUniqueKey::iterator si = mySIservicesNVODorderUniqueKey.find(uniqueServiceKey);

	char *msgData = 0;

	struct sectionsd::msgResponseHeader responseHeader;

	responseHeader.dataLength = 0;

	if (si != mySIservicesNVODorderUniqueKey.end())
	{
		dprintf("NVODServices: %u\n", si->second->nvods.size());

		if (si->second->nvods.size())
		{
			responseHeader.dataLength = (sizeof(t_service_id) + sizeof(t_original_network_id) + sizeof(t_transport_stream_id) + 4 + 4) * si->second->nvods.size();
			msgData = new char[responseHeader.dataLength];

			if (!msgData)
			{
				fprintf(stderr, "low on memory!\n");
				unlockEvents();
				unlockServices();
				dmxEIT.unpause(); // -> unlock
				return ;
			}

			char *p = msgData;
			//      time_t azeit=time(NULL);

			for (SInvodReferences::iterator ni = si->second->nvods.begin(); ni != si->second->nvods.end(); ni++)
			{
				// Zeiten sind erstmal dummy, d.h. pro Service eine Zeit
				*(t_service_id          *)p = ni->serviceID;			p += sizeof(t_service_id);
				*(t_original_network_id *)p = ni->originalNetworkID;		p += sizeof(t_original_network_id);
				*(t_transport_stream_id *)p = ni->transportStreamID;		p += sizeof(t_transport_stream_id);

				SItime zeitEvt1(0, 0);
				//        const SIevent &evt=
				findActualSIeventForServiceUniqueKey(ni->uniqueKey(), zeitEvt1, 15*60);
				*(time_t *)p = zeitEvt1.startzeit;
				p += 4;
				*(unsigned *)p = zeitEvt1.dauer;
				p += 4;

				/*        MySIeventUniqueKeysMetaOrderServiceUniqueKey::iterator ei=mySIeventUniqueKeysMetaOrderServiceUniqueKey.find(ni->uniqueKey());
				        if(ei!=mySIeventUniqueKeysMetaOrderServiceUniqueKey.end())
				        {
				            dprintf("found NVod - Service: %0llx\n", ei->second);
				            MySIeventsOrderUniqueKey::iterator e=mySIeventsOrderUniqueKey.find(ei->second);
				            if(e!=mySIeventsOrderUniqueKey.end())
				            {
				                // ist ein MetaEvent, d.h. mit Zeiten fuer NVOD-Event
				                for(SItimes::iterator t=e->second->times.begin(); t!=e->second->times.end(); t++)
				                if(t->startzeit<=azeit && azeit<=(long)(t->startzeit+t->dauer))
				                {
				                    *(time_t *)p=t->startzeit;
				                    break;
				                }
				            }
				        }
				*/

			}
		}
	}

	dprintf("data bytes: %u\n", responseHeader.dataLength);
	int rc = writeNbytes(client->connectionSocket, (const char *)&responseHeader, sizeof(responseHeader), TIMEOUT_CONNECTIONS);

	if (rc > 0)
	{
		if (responseHeader.dataLength)
		{
			writeNbytes(client->connectionSocket, msgData, responseHeader.dataLength, TIMEOUT_CONNECTIONS);
			delete[] msgData;
		}
	}
	else
		dputs("[sectionsd] Fehler/Timeout bei write");

	unlockEvents();

	unlockServices();

	dmxEIT.unpause(); // -> unlock
}


static void commandGetIsTimeSet(struct connectionData *client, char *data, const unsigned dataLength)
{
	if (dataLength)
		return ;

	sectionsd::responseIsTimeSet rmsg;

	rmsg.IsTimeSet = (timeset != 0);

	dprintf("Request of Time-Is-Set %d\n", rmsg.IsTimeSet);

	struct sectionsd::msgResponseHeader responseHeader;

	responseHeader.dataLength = sizeof(rmsg);

	if (writeNbytes(client->connectionSocket, (const char *)&responseHeader, sizeof(responseHeader), TIMEOUT_CONNECTIONS) > 0)
	{
		writeNbytes(client->connectionSocket, (const char *)&rmsg, responseHeader.dataLength, TIMEOUT_CONNECTIONS);
	}
	else
		dputs("[sectionsd] Fehler/Timeout bei write");

	return ;
}



static void (*connectionCommands[sectionsd::numberOfCommands]) (struct connectionData *, char *, const unsigned) =
    {
        commandActualEPGchannelName,
        commandEventListTV,
        commandCurrentNextInfoChannelName,
        commandDumpStatusInformation,
        commandAllEventsChannelName,
        commandSetHoursToCache,
        commandSetEventsAreOldInMinutes,
        commandDumpAllServices,
        commandEventListRadio,
        commandGetNextEPG,
        commandGetNextShort,
        commandPauseScanning,
        commandActualEPGchannelID,
        commandEventListTVids,
        commandEventListRadioIDs,
        commandCurrentNextInfoChannelID,
        commandEPGepgID,
        commandEPGepgIDshort,
        commandComponentTagsUniqueKey,
        commandAllEventsChannelID,
        commandTimesNVODservice,
        commandGetEPGPrevNext,
        commandGetIsTimeSet,
        commandserviceChanged,
        commandLinkageDescriptorsUniqueKey,
        commandPauseSorting
    };

static void *connectionThread(void *conn)
{

	struct connectionData *client = (struct connectionData *)conn;

	try
	{
		dprintf("Connection from UDS\n");

		if (fcntl(client->connectionSocket, F_SETFL, O_NONBLOCK))
		{
			perror ("[sectionsd] fcntl");
			return 0;
		}

		struct sectionsd::msgRequestHeader header;

		memset(&header, 0, sizeof(header));

		int readbytes = readNbytes(client->connectionSocket, (char *) & header, sizeof(header) , TIMEOUT_CONNECTIONS);

		if (readbytes > 0)
		{
			dprintf("version: %hhd, cmd: %hhd, numbytes: %d\n", header.version, header.command, readbytes);

			if (header.version == 2 && header.command < sectionsd::numberOfCommands)
			{
				dprintf("data length: %hd\n", header.dataLength);
				char *data = new char[header.dataLength + 1];

				if (!data)
					fprintf(stderr, "low on memory!\n");
				else
				{
					int rc = 1;

					if (header.dataLength)
						rc = readNbytes(client->connectionSocket, data, header.dataLength, TIMEOUT_CONNECTIONS);

					if (rc > 0)
					{
						dprintf("Starting command %hhd\n", header.command);
						connectionCommands[header.command](client, data, header.dataLength);
					}

					delete[] data;
				}
			}
			else if (header.version == 3 && header.command < sectionsd::numberOfCommands_v3)
			{
				int connfd = client->connectionSocket;

				if ( header.command == sectionsd::CMD_registerEvents )
				{

					CEventServer::commandRegisterEvent msg;

					//int readresult= readNbytes(client->connectionSocket, (char *)&msg, sizeof(msg) , TIMEOUT_CONNECTIONS);
					readNbytes(client->connectionSocket, (char *)&msg, sizeof(msg) , TIMEOUT_CONNECTIONS);

					//printf("[connectionThread]: read bytes  %d/%d\n", readresult,  sizeof(msg));
					//printf("[connectionThread]: register event (%d) to: %d - %s\n", msg.eventID, msg.clientID, msg.udsName);
					eventServer->registerEvent2(msg.eventID, msg.clientID, msg.udsName);

					if ( msg.eventID == CSectionsdClient::EVT_TIMESET )
						messaging_neutrino_sets_time = true;

					//eventServer->registerEvent( connfd );
				}
				else if ( header.command == sectionsd::CMD_unregisterEvents )
					eventServer->unRegisterEvent( connfd );

			}
			else
				dputs("Unknow format or version of request!");
		}

		close(client->connectionSocket);
		dprintf("Connection closed!\n");
		delete client;

	} // try
	catch (std::exception& e)
	{
		fprintf(stderr, "Caught std-exception in connection-thread %s!\n", e.what());
	}
	catch (...)
	{
		fprintf(stderr, "Caught exception in connection-thread!\n");
	}

	return 0;
}

//---------------------------------------------------------------------
//			sdt-thread
// reads sdt for service list
//---------------------------------------------------------------------
static void *sdtThread(void *)
{

	struct SI_section_header header;
	char *buf;
	const unsigned timeoutInMSeconds = 500;
	bool sendToSleepNow = false;

	dmxSDT.addfilter(0x42, 0xff );
	//    dmxSDT.addfilter(0x46, 0xff );

	try
	{
		dprintf("sdt-thread started.\n");

		int timeoutsDMX = 0;
		dmxSDT.start(); // -> unlock

		for (;;)
		{
			time_t zeit = time(NULL);

			if (timeset)
			{
				// Nur wenn ne richtige Uhrzeit da ist

				if ( (sendToSleepNow) )
				{
					sendToSleepNow = false;

					struct timespec abs_wait;

					struct timeval now;

					gettimeofday(&now, NULL);
					TIMEVAL_TO_TIMESPEC(&now, &abs_wait);
					abs_wait.tv_sec += (TIME_SDT_SCHEDULED_PAUSE);

					dmxSDT.real_pause();
					pthread_mutex_lock( &dmxSDT.start_stop_mutex );
					dprintf("dmxSDT: going to sleep...\n");

					int rs = pthread_cond_timedwait( &dmxSDT.change_cond, &dmxSDT.start_stop_mutex, &abs_wait );

					if (rs == ETIMEDOUT)
					{
						dprintf("dmxSDT: waking up again - looking for new events :)\n");
						pthread_mutex_unlock( &dmxSDT.start_stop_mutex );
						dmxSDT.change( 0 ); // -> restart
					}
					else if (rs == 0)
					{
						dprintf("dmxSDT: waking up again - requested from .change()\n");
						pthread_mutex_unlock( &dmxSDT.start_stop_mutex );
					}
					else
					{
						dprintf("dmxSDT:  waking up again - unknown reason?!\n");
						pthread_mutex_unlock( &dmxSDT.start_stop_mutex );
						dmxSDT.real_unpause();
					}
				}
				else if (zeit > dmxSDT.lastChanged + TIME_SDT_SKIPPING )
				{
					lockMessaging();

					if ( dmxSDT.filter_index + 1 < (signed) dmxSDT.filters.size() )
					{
						dmxSDT.change(dmxSDT.filter_index + 1);
						dprintf("[sdtThread] skipping to next filter (> TIME_SDT_SKIPPING)\n" );
					}
					else
						sendToSleepNow = true;

					unlockMessaging();
				};
			}

			if (timeoutsDMX >= RESTART_DMX_AFTER_TIMEOUTS)
			{
				timeoutsDMX = 0;
				dmxSDT.stop();
				dmxSDT.start(); // leaves unlocked
				dputs("dmxSDT restarted");
			}

			dmxSDT.lock();
			int rc = dmxSDT.read((char *) & header, sizeof(header), timeoutInMSeconds);

			if (!rc)
			{
				dmxSDT.unlock();
				dputs("dmxSDT.read timeout");
				timeoutsDMX++;
				continue; // timeout -> kein EPG
			}
			else if (rc < 0)
			{
				dmxSDT.unlock();
				// DMX neu starten
				dmxSDT.real_pause();
				dmxSDT.real_unpause(); // leaves unlocked
				continue;
			}

			timeoutsDMX = 0;
			buf = new char[sizeof(header) + header.section_length - 5];

			if (!buf)
			{
				dmxSDT.unlock();
				fprintf(stderr, "Not enough memory!\n");
				break;
			}

			// Den Header kopieren
			memcpy(buf, &header, sizeof(header));

			rc = dmxSDT.read(buf + sizeof(header), header.section_length - 5, timeoutInMSeconds);

			dmxSDT.unlock();

			if (!rc)
			{
				delete[] buf;
				dputs("dmxSDT.read timeout after header");
				// DMX neu starten, noetig, da bereits der Header gelesen wurde

				dmxSDT.real_pause();
				dmxSDT.real_unpause(); // leaves unlocked
				continue; // timeout -> kein EPG
			}
			else if (rc < 0)
			{
				delete[] buf;
				// DMX neu starten
				dmxSDT.real_pause(); // -> lock
				dmxSDT.real_unpause(); // -> unlock
				continue;
			}

			if ((header.current_next_indicator) && (!dmxSDT.pauseCounter))
			{
				// Wir wollen nur aktuelle sections
				SIsectionSDT sdt(SIsection(sizeof(header) + header.section_length - 5, buf));
				lockServices();

				for (SIservices::iterator s = sdt.services().begin(); s != sdt.services().end(); s++)
					addService(*s);

				unlockServices();


				lockMessaging();

				//SI_section_SDT_header* _header = (SI_section_SDT_header*) &header;
				int msg_index = ( header.table_id == 0x42) ? 0 : 1;

				if ( !messaging_sdt_sections_got_all[msg_index] )
				{
					long long _id = (((unsigned long long)header.table_id) << 40) +
					                (((unsigned long long)header.table_id_extension) << 32) +
					                (header.section_number << 16) +
					                (header.version_number << 8) +
					                header.current_next_indicator;

					if ( messaging_sdt_sections_max_ID[msg_index] == -1 )
					{
						messaging_sdt_sections_max_ID[msg_index] = _id;
					}
					else
					{
						if ( !messaging_sdt_sections_got_all[msg_index] )
						{
							for ( std::vector<long long>::iterator i = messaging_sdt_skipped_sections_ID[msg_index].begin();
							        i != messaging_sdt_skipped_sections_ID[msg_index].end(); ++i )
								if ( *i == _id)
								{
									messaging_sdt_skipped_sections_ID[msg_index].erase(i);
									break;
								}

							if ( ( messaging_sdt_sections_max_ID[msg_index] == _id ) &&
							        ( messaging_sdt_skipped_sections_ID[msg_index].size() == 0 ) )
							{
								// alle pakete für den ServiceKey da!
								dprintf("[sdtThread] got all packages for table_id 0x%x (%d)\n", header.table_id, msg_index);
								messaging_sdt_sections_got_all[msg_index] = true;
							}
						}
					}

					// überprüfen, ob nächster Filter gewünscht :)
					if ( messaging_sdt_sections_got_all[dmxSDT.filter_index] )
					{
						if ( dmxSDT.filter_index + 1 < (signed) dmxSDT.filters.size() )
						{
							dmxSDT.change(dmxSDT.filter_index + 1);
						}
						else
							sendToSleepNow = true;
					}
				}

				unlockMessaging();


			} // if
			else
			{
				lockMessaging();

				int msg_index = ( header.table_id == 0x42) ? 0 : 1;
				long long _id = (((unsigned long long)header.table_id) << 40) +
				                (((unsigned long long)header.table_id_extension) << 32) +
				                (header.section_number << 16) +
				                (header.version_number << 8) +
				                header.current_next_indicator;

				if ( messaging_sdt_sections_max_ID[msg_index] != -1 )
					messaging_sdt_skipped_sections_ID[msg_index].push_back(_id);

				unlockMessaging();

				dprintf("[sdtThread] skipped sections for table 0x%x\n", header.table_id);

				delete[] buf;
			}
		} // for

		dmxSDT.closefd();
	} // try
	catch (std::exception& e)
	{
		fprintf(stderr, "Caught std-exception in connection-thread %s!\n", e.what());
	}
	catch (...)
	{
		fprintf(stderr, "Caught exception in sdt-thread!\n");
	}

	dprintf("sdt-thread ended\n");
	return 0;
}

//---------------------------------------------------------------------
//			Time-thread
// updates system time according TOT every 30 minutes
//---------------------------------------------------------------------

struct SI_section_TOT_header
{

unsigned char table_id :
	8;
	// 1 byte

unsigned char section_syntax_indicator :
	1;

unsigned char reserved_future_use :
	1;

unsigned char reserved1 :
	2;

unsigned short section_length :
	12;
	// 3 bytes

unsigned long long UTC_time :
	40;
	// 8 bytes

unsigned char reserved2 :
	4;

unsigned short descriptors_loop_length :
	12;
}

__attribute__ ((packed)) ; // 10 bytes

struct SI_section_TDT_header
{

unsigned char table_id :
	8;
	// 1 byte

unsigned char section_syntax_indicator :
	1;

unsigned char reserved_future_use :
	1;

unsigned char reserved1 :
	2;

unsigned short section_length :
	12;
	// 3 bytes

unsigned long long UTC_time :
	40;
}

__attribute__ ((packed)) ; // 8 bytes

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
	unsigned timeoutInMSeconds = 31 * 1000;
	char *buf;
	time_t tim;

	dmxTOT.addfilter(0x73, 0xff);
	dmxTOT.addfilter(0x70, (0xff - 0x03));

	try
	{
		dprintf("time-thread started.\n");

		do
		{
			if (!dmxTOT.isOpen())
			{
				dmxTOT.start(); // -> unlock
			}

			if (dmxTOT.change( true )) // von TOT nach TDT wechseln

				;

			struct SI_section_TDT_header tdt_tot_header;

			dmxTOT.lock();

			int rc = dmxTOT.read((char *) & tdt_tot_header, sizeof(tdt_tot_header), timeoutInMSeconds);

			if (!rc)
			{
				dmxTOT.unlock();
				dputs("dmxTOT.read timeout");
				continue; // timeout -> keine Zeit
			}
			else if (rc < 0)
			{
				dmxTOT.unlock();
				dmxTOT.closefd();
				continue;
			}

			switch ( tdt_tot_header.table_id )
			{

			case 0x73:
				{
					// TOT - Rest einlesen!
					buf = new char[tdt_tot_header.section_length - 5];

					if (!buf)
					{
						dmxTOT.unlock();
						fprintf(stderr, "Not enough memory!\n");
						dmxTOT.closefd();
						continue;
					}

					rc = dmxTOT.read(buf, tdt_tot_header.section_length - 5, timeoutInMSeconds);
					delete[] buf;
					// und weiter unterhalb  ...
					dprintf("TDT/TOT: got local time via TOT :)");
				}

			case 0x70:
				{
					dmxTOT.unlock();
					tim = changeUTCtoCtime(((const unsigned char *) & tdt_tot_header) + 3);

					if (tim)
					{
						if ( !messaging_neutrino_sets_time )
							if (stime(&tim) < 0)
							{
								perror("[sectionsd] cannot set date");
								dmxTOT.closefd();
								continue;
							}

						timeset = 1;
					}

					break;
				}

			default:
				dmxTOT.unlock();
			}
		}
		while (timeset != 1);

		eventServer->sendEvent(CSectionsdClient::EVT_TIMESET, CEventServer::INITID_SECTIONSD, &tim, sizeof(tim) );

		dmxTOT.closefd();

		dprintf("dmxTOT: changing from TDT/TOT to TOT.\n");

		// Jetzt wird die Uhrzeit nur noch per TOT gesetzt (CRC)
		for (;;)
		{
			if (!dmxTOT.isOpen())
			{
				dmxTOT.start(); // -> unlock
			}

			struct SI_section_TOT_header header;

			dmxTOT.lock();

			int rc = dmxTOT.read((char *) & header, sizeof(header), timeoutInMSeconds);

			if (!rc)
			{
				dmxTOT.unlock();
				dputs("dmxTOT.read timeout");
				continue; // timeout -> keine Zeit
			}
			else if (rc < 0)
			{
				dmxTOT.unlock();
				dmxTOT.closefd();
				break;
			}

			buf = new char[header.section_length - 7];

			if (!buf)
			{
				dmxTOT.unlock();
				fprintf(stderr, "Not enough memory!\n");
				dmxTOT.closefd();
				break;
			}

			rc = dmxTOT.read(buf, header.section_length - 7, timeoutInMSeconds);

			dmxTOT.unlock();
			delete[] buf;

			if (!rc)
			{
				dputs("dmxTOT.read timeout after header");
				// DMX neu starten, noetig, da bereits der Header gelesen wurde
				dmxTOT.real_pause(); // -> lock
				dmxTOT.real_unpause(); // -> unlock
				continue; // timeout -> kein TDT
			}
			else if (rc < 0)
			{
				dmxTOT.closefd();
				break;
			}

			time_t tim = changeUTCtoCtime(((const unsigned char *) & header) + 3);

			if (tim)
			{
				if ( !messaging_neutrino_sets_time )
					if (stime(&tim) < 0)
					{
						perror("[sectionsd] cannot set date");
						dmxTOT.closefd();
						continue;
					}

				eventServer->sendEvent(CSectionsdClient::EVT_TIMESET, CEventServer::INITID_SECTIONSD, &tim, sizeof(tim) );
			}

			dmxTOT.closefd();

			if (timeset)
			{
				rc = 60 * 30;  // sleep 30 minutes
				dprintf("dmxTOT: going to sleep for 30mins...\n");
			}
			else
				//rc=60;  // sleep 1 minute
				rc = 1;

			while (rc)
				rc = sleep(rc);

		} // for
	} // try
	catch (std::exception& e)
	{
		fprintf(stderr, "Caught std-exception in connection-thread %s!\n", e.what());
	}
	catch (...)
	{
		fprintf(stderr, "Caught exception in time-thread!\n");
	}

	dprintf("time-thread ended\n");
	return 0;
}

//---------------------------------------------------------------------
//			EIT-thread
// reads EPG-datas
//---------------------------------------------------------------------
static void *eitThread(void *)
{

	struct SI_section_header header;
	char *buf;
	unsigned timeoutInMSeconds = 500;
	bool sendToSleepNow = false;
	//dmxEIT.addfilter( 0x4e, (0xff) );
	//dmxEIT.addfilter( 0x4f, (0xff) );
	dmxEIT.addfilter( 0x4e, (0xff - 0x01) );
	dmxEIT.addfilter( 0x50, (0xff) );
	dmxEIT.addfilter( 0x51, (0xff) );
	dmxEIT.addfilter( 0x52, (0xff - 0x01) );
	dmxEIT.addfilter( 0x54, (0xff - 0x03) );
	dmxEIT.addfilter( 0x58, (0xff - 0x03) );
	dmxEIT.addfilter( 0x5c, (0xff - 0x03) );
	dmxEIT.addfilter( 0x60, (0xff - 0x03) );
	dmxEIT.addfilter( 0x64, (0xff - 0x03) );
	dmxEIT.addfilter( 0x68, (0xff - 0x03) );
	dmxEIT.addfilter( 0x6c, (0xff - 0x03) );

	try
	{
		dputs("[eitThread] start");
		int timeoutsDMX = 0;
		time_t lastRestarted = time(NULL);
		dmxEIT.start(); // -> unlock

		for (;;)
		{

			time_t zeit = time(NULL);

			if (timeoutsDMX >= CHECK_RESTART_DMX_AFTER_TIMEOUTS - 1)
			{
				lockServices();
				lockMessaging();

				MySIservicesOrderUniqueKey::iterator si = mySIservicesOrderUniqueKey.end();
				//dprintf("timeoutsDMX %x\n",currentServiceKey);

				if ( messaging_current_servicekey )
					si = mySIservicesOrderUniqueKey.find( messaging_current_servicekey );

				if (si != mySIservicesOrderUniqueKey.end())
				{
					if ( ( ( dmxEIT.filter_index == 0 ) && ( !si->second->eitPresentFollowingFlag() ) ) ||
					        ( ( dmxEIT.filter_index == 1 ) && ( !si->second->eitScheduleFlag() ) ) )
					{
						timeoutsDMX = 0;
						dprintf("[eitThread] timeoutsDMX for 0x%x reset to 0 (not broadcast)\n", messaging_current_servicekey );

						dmxEIT.change( dmxEIT.filter_index + 1 );
					}
					else
						if ( dmxEIT.filter_index > 1 )
						{
							bool dont_change = false;

							for ( int i = (dmxEIT.filters[dmxEIT.filter_index].filter & dmxEIT.filters[dmxEIT.filter_index].mask); i <= dmxEIT.filters[dmxEIT.filter_index].filter; i++)
							{
								//dprintf("%x - %x |", i, messaging_sections_max_ID[i- 0x4e]);

								if ( messaging_sections_max_ID[i - 0x4e] != -1 )
								{
									dont_change = true;
									break;
								}
							}

							dprintf("dontchange %d\n", dont_change);

							if ( !dont_change )
							{
								if ( dmxEIT.filter_index + 1 < (signed) dmxEIT.filters.size() )
								{
									dmxEIT.change(dmxEIT.filter_index + 1);
									//dprintf("[eitThread] timeoutsDMX for 0x%x reset to 0 (skipping to next filter)\n" );

									timeoutsDMX = 0;
								}
								else
									sendToSleepNow = true;
							}
						}
				}

				unlockMessaging();
				unlockServices();
			}

			if (timeoutsDMX >= CHECK_RESTART_DMX_AFTER_TIMEOUTS)
			{
				if ( (zeit > lastRestarted + 3) || (dmxEIT.real_pauseCounter != 0) ) // letzter restart länger als 3secs her, daher cache NICHT verkleinern
				{
					dmxEIT.stop(); // -> lock
					dmxEIT.start(); // -> unlock
					dprintf("[eitThread] dmxEIT restarted, cache NOT decreased (dt=%ld)\n", (int)zeit - lastRestarted);
				}
				else
				{

					// sectionsd ist zu langsam, da zu viele events -> cache kleiner machen
					dmxEIT.stop(); // -> lock
					/*                    lockEvents();
					                    if(secondsToCache>24*60L*60L && mySIeventsOrderUniqueKey.size()>3000)
					                    {
					                        // kleiner als 1 Tag machen wir den Cache nicht,
					                        // da die timeouts ja auch von einem Sender ohne EPG kommen koennen
					                        // Die 3000 sind ne Annahme und beruhen auf (wenigen) Erfahrungswerten
					                        // Man koennte auch ab 3000 Events nur noch jedes 3 Event o.ae. einsortieren
					                        dmxSDT.real_pause();
					                        lockServices();
					                        unsigned anzEventsAlt=mySIeventsOrderUniqueKey.size();
					                        secondsToCache-=5*60L*60L; // 5h weniger
					                        dprintf("[eitThread] decreasing cache 5h (now %ldh)\n", secondsToCache/(60*60L));
					                        removeNewEvents();
					                        removeOldEvents(oldEventsAre);
					                        if(anzEventsAlt>mySIeventsOrderUniqueKey.size())
					                            dprintf("[eitThread] Removed %u Events (%u -> %u)\n", anzEventsAlt-mySIeventsOrderUniqueKey.size(), anzEventsAlt, mySIeventsOrderUniqueKey.size());
					                        unlockServices();
					                        dmxSDT.real_unpause();
					                    }
					                    unlockEvents();
					*/
					dmxEIT.start(); // -> unlock
					dputs("[eitThread] dmxEIT restarted");

				}

				lastRestarted = zeit;
				timeoutsDMX = 0;
			}

			if (timeset)
			{
				// Nur wenn ne richtige Uhrzeit da ist

				if ( (sendToSleepNow) )
				{
					sendToSleepNow = false;

					struct timespec abs_wait;

					struct timeval now;

					gettimeofday(&now, NULL);
					TIMEVAL_TO_TIMESPEC(&now, &abs_wait);
					abs_wait.tv_sec += (TIME_EIT_SCHEDULED_PAUSE);

					dmxEIT.real_pause();
					pthread_mutex_lock( &dmxEIT.start_stop_mutex );
					dprintf("dmxEIT: going to sleep...\n");

					int rs = pthread_cond_timedwait( &dmxEIT.change_cond, &dmxEIT.start_stop_mutex, &abs_wait );

					if (rs == ETIMEDOUT)
					{
						dprintf("dmxEIT: waking up again - looking for new events :)\n");
						pthread_mutex_unlock( &dmxEIT.start_stop_mutex );
						dmxEIT.change( 0 ); // -> restart
					}
					else if (rs == 0)
					{
						dprintf("dmxEIT: waking up again - requested from .change()\n");
						pthread_mutex_unlock( &dmxEIT.start_stop_mutex );
					}
					else
					{
						dprintf("dmxEIT:  waking up again - unknown reason?!\n");
						pthread_mutex_unlock( &dmxEIT.start_stop_mutex );
						dmxEIT.real_unpause();
					}
				}
				else if (zeit > dmxEIT.lastChanged + TIME_EIT_SKIPPING )
				{
					lockMessaging();

					if ( dmxEIT.filter_index + 1 < (signed) dmxEIT.filters.size() )
					{
						dmxEIT.change(dmxEIT.filter_index + 1);
						dprintf("[eitThread] skipping to next filter (> TIME_EIT_SKIPPING)\n" );
					}
					else
						sendToSleepNow = true;

					unlockMessaging();
				};
			}

			dmxEIT.lock();
			dputs("[eitThread] read");
			int rc = dmxEIT.read((char *) &header, 3, timeoutInMSeconds);

			if (!rc)
			{
				dmxEIT.unlock();
				dputs("[eitThread] dmxEIT.read timeout...");
				timeoutsDMX++;
				continue; // timeout -> kein EPG
			}
			else if (rc < 0)
			{
				dputs("[eitThread] dmxEIT.read rc<0");
				dmxEIT.unlock();
				// DMX neu starten
				dmxEIT.real_pause();
				dmxEIT.real_unpause();
				continue;
			}

			timeoutsDMX = 0;
			buf = new char[3 + header.section_length];
			
			if (!buf)
			{
				dmxEIT.closefd();
				dmxEIT.unlock();
				fprintf(stderr, "[eitThread] Not enough memory!\n");
				break;
			}
			    
			if (header.section_length > 0)
				rc = dmxEIT.read(buf + 3, header.section_length, timeoutInMSeconds);

			dmxEIT.unlock();

			if (rc <= 0)
			{
				delete[] buf;
				if (rc == 0)
				{
					dputs("[eitThread] dmxEIT.read timeout after header");
				}
				else
					dputs("[eitThread] dmxEIT.read rc < 0 after header");
				// DMX neu starten, noetig, da bereits der Header gelesen wurde
				dmxEIT.real_pause(); // -> lock
				dmxEIT.real_unpause(); // -> unlock
				continue;
			}

			if (header.section_length < 5)
			{
				delete[] buf;
				continue;
			}

			if (((header.table_id ^ dmxEIT.filters[dmxEIT.filter_index].filter) & dmxEIT.filters[dmxEIT.filter_index].mask) != 0)
			{
				dprintf("[eitThread] filter 0x%x mask 0x%x -> skip sections for table 0x%x\n", dmxEIT.filters[dmxEIT.filter_index].filter, dmxEIT.filters[dmxEIT.filter_index].mask, header.table_id);
				delete[] buf;
				continue;
			}

			// Den Header kopieren
			memcpy(buf, &header, 3);
			memcpy(((char *)&header) + 3, buf + 3, min((unsigned int)header.section_length, sizeof(header) - 3));

			if ((header.current_next_indicator) && (!dmxEIT.pauseCounter ))
			{
				// Wir wollen nur aktuelle sections

				/*                // Zum debuggen
				        //        if(dmxEIT.isScheduled)
				                {
				                    printf("%hhx\n", header.section_number);
				                    //if(header.section_number==0xb1)
				                    {
				                        printf("length: %hd\n", header.section_length);
				                        dmxEIT.pause();
				                        char    dn[100];
				                        sprintf(dn, "eit.debug.%d", header.section_number);
				                        FILE *file=fopen(dn, "wb");
				                        if(file)
				                        {
				                            fwrite(buf, sizeof(header)+header.section_length-5, 1, file);
				                            fclose(file);
				                        }
				                        dmxEIT.unpause();
				                    }
				                }
				*/
				dputs("[eitThread] Parsing");

				SIsectionEIT eit(SIsection(sizeof(header) + header.section_length - 5, buf));

				if (eit.header())
				{
					// == 0 -> kein event

					dputs("[eitThread] adding events (begin)");

					zeit = time(NULL);
					// Nicht alle Events speichern

					for (SIevents::iterator e = eit.events().begin(); e != eit.events().end(); e++)
					{
						if (e->times.size() > 0)
						{
							if ( ( e->times.begin()->startzeit < zeit + secondsToCache ) &&
							        ( ( e->times.begin()->startzeit + (long)e->times.begin()->dauer ) > zeit - oldEventsAre ) )
							{
								lockEvents();
								addEvent(*e);
								unlockEvents();
							}
						}
						else
						{
							// pruefen ob nvod event
							lockServices();
							MySIservicesNVODorderUniqueKey::iterator si = mySIservicesNVODorderUniqueKey.find(SIservice::makeUniqueKey(e->originalNetworkID, e->serviceID));

							if (si != mySIservicesNVODorderUniqueKey.end())
							{
								// Ist ein nvod-event
								lockEvents();

								for (SInvodReferences::iterator i = si->second->nvods.begin(); i != si->second->nvods.end(); i++)
									mySIeventUniqueKeysMetaOrderServiceUniqueKey.insert(std::make_pair(i->uniqueKey(), e->uniqueKey()));

								addNVODevent(*e);

								unlockEvents();
							}
							unlockServices();
						}


					} // for

					dputs("[eitThread] adding events (end)");

				} // if

				lockMessaging();

				if ( ( header.table_id != 0x4e ) ||
				        ( header.table_id_extension == ( messaging_current_servicekey & 0xFFFF ) ) )
				{
					if ( !messaging_sections_got_all[header.table_id - 0x4e] )
					{
						long long _id = (((unsigned long long)header.table_id) << 40) +
						                (((unsigned long long)header.table_id_extension) << 32) +
						                (header.section_number << 16) +
						                (header.version_number << 8) +
						                header.current_next_indicator;

						if ( messaging_sections_max_ID[header.table_id - 0x4e] == -1 )
						{
							messaging_sections_max_ID[header.table_id - 0x4e] = _id;
						}
						else
						{
							if ( !messaging_sections_got_all[header.table_id - 0x4e] )
							{
								for ( std::vector<long long>::iterator i = messaging_skipped_sections_ID[header.table_id - 0x4e].begin();
								        i != messaging_skipped_sections_ID[header.table_id - 0x4e].end(); ++i )
									if ( *i == _id)
									{
										messaging_skipped_sections_ID[header.table_id - 0x4e].erase(i);
										break;
									}

								if ( ( messaging_sections_max_ID[header.table_id - 0x4e] == _id ) &&
								        ( messaging_skipped_sections_ID[header.table_id - 0x4e].size() == 0 ) )
								{
									// alle pakete für den ServiceKey da!
									dprintf("[eitThread] got all packages for table_id 0x%x\n", header.table_id);
									messaging_sections_got_all[header.table_id - 0x4e] = true;
								}
							}
						}

						if ( messaging_wants_current_next_Event && messaging_sections_got_all[0] )
						{
							dprintf("[eitThread] got all current_next - sending event!\n");
							messaging_wants_current_next_Event = false;
							eventServer->sendEvent(CSectionsdClient::EVT_GOT_CN_EPG, CEventServer::INITID_SECTIONSD, &messaging_current_servicekey, sizeof(messaging_current_servicekey) );
						}

						// überprüfen, ob nächster Filter gewünscht :)
						int	change_filter = 0;

						for ( int i = (dmxEIT.filters[dmxEIT.filter_index].filter & dmxEIT.filters[dmxEIT.filter_index].mask); i <= dmxEIT.filters[dmxEIT.filter_index].filter; i++)
						{
							if ( messaging_sections_got_all[i - 0x4e] )
								change_filter++;
							else
								if ( messaging_sections_max_ID[i - 0x4e] != -1 )
								{
									change_filter = -1;
									break;
								}
						}

						if ( change_filter > 0 )
						{
							if ( dmxEIT.filter_index + 1 < (signed) dmxEIT.filters.size() )
								dmxEIT.change(dmxEIT.filter_index + 1);
							else
								sendToSleepNow = true;
						}
					}
				}

				unlockMessaging();

			} // if
			else
			{
				lockMessaging();
				//SI_section_EIT_header* _header = (SI_section_EIT_header*) & header;

				long long _id = (((unsigned long long)header.table_id) << 40) +
				                (((unsigned long long)header.table_id_extension) << 32) +
				                (header.section_number << 16) +
				                (header.version_number << 8) +
				                header.current_next_indicator;

				if ( messaging_sections_max_ID[header.table_id - 0x4e] != -1 )
					messaging_skipped_sections_ID[header.table_id - 0x4e].push_back(_id);

				unlockMessaging();

				delete[] buf;

				dprintf("[eitThread] skipped sections for table 0x%x\n", header.table_id);
			}
		} // for
	} // try
	catch (std::exception& e)
	{
		fprintf(stderr, "[eitThread] Caught std-exception %s!\n", e.what());
	}
	catch (...)
	{
		fprintf(stderr, "[eitThread] Caught exception!\n");
	}

	dputs("[eitThread] end");
	return 0;
}

//---------------------------------------------------------------------
//			housekeeping-thread
// does cleaning on fetched datas
//---------------------------------------------------------------------
static void *houseKeepingThread(void *)
{
	try
	{
		dprintf("housekeeping-thread started.\n");

		for (;;)
		{
			int rc = 5 * 60;  // sleep 5 minutes

			while (rc)
				rc = sleep(rc);

			while (!scanning)
				sleep(1);	// wait for streaming to end...

			dprintf("housekeeping.\n");

			dmxEIT.pause();

			dmxSDT.pause();

			struct mallinfo speicherinfo1;

			if (debug)
			{
				// Speicher-Info abfragen
				speicherinfo1 = mallinfo();
			}

			lockEvents();

			unsigned anzEventsAlt = mySIeventsOrderUniqueKey.size();
			removeOldEvents(oldEventsAre); // alte Events

			if (mySIeventsOrderUniqueKey.size() != anzEventsAlt)
			{
				dprintf("total size of memory occupied by chunks handed out by malloc: %d\n", speicherinfo1.uordblks);
				dprintf("total bytes memory allocated with `sbrk' by malloc, in bytes: %d (%dkb, %.2fMB)\n", speicherinfo1.arena, speicherinfo1.arena / 1024, (float)speicherinfo1.arena / (1024.*1024));
				dprintf("Removed %d old events.\n", anzEventsAlt - mySIeventsOrderUniqueKey.size());
			}

			dprintf("Number of sptr events (event-ID): %u\n", mySIeventsOrderUniqueKey.size());
			dprintf("Number of sptr events (service-id, start time, event-id): %u\n", mySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey.size());
			dprintf("Number of sptr events (end time, service-id, event-id): %u\n", mySIeventsOrderFirstEndTimeServiceIDEventUniqueKey.size());
			dprintf("Number of sptr nvod events (event-ID): %u\n", mySIeventsNVODorderUniqueKey.size());
			dprintf("Number of cached meta-services: %u\n", mySIeventUniqueKeysMetaOrderServiceUniqueKey.size());

			unlockEvents();

			if (debug)
			{
				lockServices();
				dprintf("Number of services: %u\n", mySIservicesOrderUniqueKey.size());
				dprintf("Number of services (name): %u\n", mySIservicesOrderServiceName.size());
				dprintf("Number of cached nvod-services: %u\n", mySIservicesNVODorderUniqueKey.size());
				unlockServices();
			}

			if (debug)
			{
				// Speicher-Info abfragen

				struct mallinfo speicherinfo = mallinfo();
				dprintf("total size of memory occupied by chunks handed out by malloc: %d\n", speicherinfo.uordblks);
				dprintf("total bytes memory allocated with `sbrk' by malloc, in bytes: %d (%dkb, %.2fMB)\n", speicherinfo.arena, speicherinfo.arena / 1024, (float)speicherinfo.arena / (1024.*1024));
			}

			dmxSDT.unpause();
			dmxEIT.unpause();

		} // for endlos
	} // try
	catch (std::exception& e)
	{
		fprintf(stderr, "Caught std-exception in housekeeping-thread %s!\n", e.what());
	}
	catch (...)
	{
		fprintf(stderr, "Caught Exception in housekeeping-thread!\n");
	}

	dprintf("housekeeping-thread ended.\n");

	return 0;
}

static void printHelp(void)
{
	printf("\nUsage: sectionsd [-d]\n\n");
}

static int listenSocket = 0;

// Just to get our listen socket closed cleanly
static void signalHandler(int signum)
{
	switch (signum)
	{

	case SIGHUP:
		break;

	default:

		if (listenSocket)
			close(listenSocket);

		listenSocket = 0;

		exit(0);
	}
}

int main(int argc, char **argv)
{
	pthread_t threadTOT, threadEIT, threadSDT, threadHouseKeeping;
	int rc;

	printf("$Id: sectionsd.cpp,v 1.135 2002/10/04 16:31:25 thegoodguy Exp $\n");

	try
	{

		if (argc != 1 && argc != 2)
		{
			printHelp();
			return 1;
		}

		if (argc == 2)
		{
			if (!strcmp(argv[1], "-d"))
				debug = 1;
			else
			{
				printHelp();
				return 1;
			}
		}

		printf("caching %ld hours\n", secondsToCache / (60*60L));
		printf("events are old %ldmin after their end time\n", oldEventsAre / 60);
		tzset(); // TZ auswerten

		switch (fork()) // switching to background
		{

		case - 1:
			perror("[sectionsd] fork");
			return -1;

		case 0:
			break;

		default:
			return 0;
		}

		if (setsid() == -1)
		{
			perror("[sectionsd] setsid");
			return -1;
		}

		// from here on forked

		//catch all signals... (busybox workaround)
		signal(SIGHUP, signalHandler);

		signal(SIGINT, signalHandler);

		signal(SIGQUIT, signalHandler);

		signal(SIGTERM, signalHandler);

		for (int x = 0;x < 32;x++)
			signal(x, signalHandler);

		struct sockaddr_un servaddr;

		int clilen;

		memset(&servaddr, 0, sizeof(struct sockaddr_un));

		servaddr.sun_family = AF_UNIX;

		strcpy(servaddr.sun_path, SECTIONSD_UDS_NAME);

		clilen = sizeof(servaddr.sun_family) + strlen(servaddr.sun_path);

		unlink(SECTIONSD_UDS_NAME);

		//network-setup
		if ((listenSocket = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
		{
			perror("socket");
		}

		if ( bind(listenSocket, (struct sockaddr*) &servaddr, clilen) < 0 )
		{
			perror("sectionsd] bind failed...\n");
			exit( -1);
		}


		if (listen(listenSocket, 5) != 0)
		{
			perror("[sectionsd] listen failed...\n");
			exit( -1 );
		}


		eventServer = new CEventServer;
		/*
				timerdClient = new CTimerdClient;
		 
				printf("[sectionsd ] checking timerd\n");
				timerd = timerdClient->isTimerdAvailable();
				if (timerd)
					printf("[sectionsd ] timerd available\n");
				else
					printf("[sectionsd ] timerd NOT available\n");
		*/ 
		// SDT-Thread starten
		rc = pthread_create(&threadSDT, 0, sdtThread, 0);

		if (rc)
		{
			fprintf(stderr, "[sectionsd] failed to create sdt-thread (rc=%d)\n", rc);
			return 1;
		}

		// EIT-Thread starten
		rc = pthread_create(&threadEIT, 0, eitThread, 0);

		if (rc)
		{
			fprintf(stderr, "[sectionsd] failed to create eit-thread (rc=%d)\n", rc);
			return 1;
		}

		// time-Thread starten
		rc = pthread_create(&threadTOT, 0, timeThread, 0);

		if (rc)
		{
			fprintf(stderr, "[sectionsd] failed to create time-thread (rc=%d)\n", rc);
			return 1;
		}

		// housekeeping-Thread starten
		rc = pthread_create(&threadHouseKeeping, 0, houseKeepingThread, 0);

		if (rc)
		{
			fprintf(stderr, "[sectionsd] failed to create houskeeping-thread (rc=%d)\n", rc);
			return 1;
		}

		pthread_attr_t conn_attrs;
		pthread_attr_init(&conn_attrs);
		pthread_attr_setdetachstate(&conn_attrs, PTHREAD_CREATE_DETACHED);

		// Unsere Endlosschliefe

		socklen_t clientInputLen = sizeof(servaddr);

		for (;listenSocket;)
		{
			// wir warten auf eine Verbindung

			struct connectionData *client = new connectionData; // Wird vom Thread freigegeben

			if (!client)
			{
				printf("[sectionsd::main] new connectionData failed.\n");
				throw std::bad_alloc();
			}

			do
			{

				client->connectionSocket = accept(listenSocket, (struct sockaddr *) & (client->clientAddr), &clientInputLen);
			}
			while (client->connectionSocket == -1);

			/*
						pthread_t threadConnection;
						rc = pthread_create(&threadConnection, &conn_attrs, connectionThread, client);
						if(rc)
						{
							fprintf(stderr, "[sectionsd] failed to create connection-thread (rc=%d)\n", rc);
							return 4;
						}
			*/ 
			// VERSUCH OHNE CONNECTION-THREAD!
			// spart die thread-creation-zeit, und die Locks lassen ohnehin nur ein cmd gleichzeitig zu
			connectionThread(client);
		}

	} // try
	catch (std::exception& e)
	{
		fprintf(stderr, "[sectionsd] Caught std-exception %s in main-thread!\n", e.what());
	}
	catch (...)
	{
		fprintf(stderr, "[sectionsd] Caught exception in main-thread!\n");
	}

	puts("[sectionsd] ended");
	return 0;
}
