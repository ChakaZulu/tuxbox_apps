//
//  $Id: sectionsd.cpp,v 1.5 2001/07/14 10:19:26 fnbrd Exp $
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
#include <algorithm>
#include <string>

#include <ost/dmx.h>

#include "epgTypes.h"

#include "SIutils.hpp"
#include "SIservices.hpp"
#include "SIevents.hpp"
#include "SIsections.hpp"

#define PORT_NUMBER 1600

static SIevents events; // die Menge mit den epg's
static SIservices services; // die Menge mit den services (aus der sdt)

static pthread_mutex_t eventsLock=PTHREAD_MUTEX_INITIALIZER; // Unsere (fast-)mutex, damit nicht gleichzeitig in die Menge events geschrieben und gelesen wird
static pthread_mutex_t servicesLock=PTHREAD_MUTEX_INITIALIZER; // Unsere (fast-)mutex, damit nicht gleichzeitig in die Menge services geschrieben und gelesen wird


static const SIevent &findSIeventForServiceName(const char *serviceName)
{
static SIevent nullEvt; // Null-Event, falls keins gefunden

  // Die for-Schleifen sind laestig,
  // Evtl. sollte man aus den sets maps machen, damit man den key einfacher aendern
  // kann und somit find() funktioniert
  for(SIservices::iterator s=services.begin(); s!=services.end(); s++) {
    // Erst mal die Controlcodes entfernen
//      printf("Servicename: '%s'\n", ks->serviceName.c_str());
    char servicename[50];
    strncpy(servicename, s->serviceName.c_str(), sizeof(servicename)-1);
    servicename[sizeof(servicename)-1]=0;
    removeControlCodes(servicename);
      // Jetz pruefen ob der Servicename der gewuenschte ist
//      printf("Servicename: '%s'\n", servicename);
    printf("testing '%s'\n", servicename);
    if(!strcasecmp(servicename, serviceName)) {
      // Event (serviceid) suchen
      time_t zeit=time(NULL);
      for(SIevents::iterator e=events.begin(); e!=events.end(); e++)
        if(e->serviceID==s->serviceID)
          for(SItimes::iterator t=e->times.begin(); t!=e->times.end(); t++)
            if(t->startzeit<=zeit && zeit<=(long)(t->startzeit+t->dauer))
              return *e;
      break;
    }
  }
  return nullEvt;
}

// Liest n Bytes aus einem Socket per read
// Liefert 0 bei timeout
// und -1 bei Fehler
// ansonsten die Anzahl gelesener Bytes
inline int readNbytes(int fd, char *buf, int n, unsigned timeoutInSeconds)
{
int j;

  for(j=0; j<n;) {
    struct pollfd ufds;
    ufds.fd=fd;
    ufds.events=POLLIN;
    ufds.revents=0;
    int rc=poll(&ufds, 1, timeoutInSeconds*1000);
    if(!rc)
      return 0; // timeout
    else if(rc<0 && errno==EINTR)
      continue; // interuppted
    else if(rc<0) {
      perror ("poll");
      printf("errno: %d\n", errno);
      return -1;
    }
    int r=read (fd, buf, n-j);
    if(r>0) {
      j+=r;
      buf+=r;
    }
    else if(r<=0 && errno!=EINTR) {
      perror ("read");
      return -1;
    }
  }
  return j;
}

//*********************************************************************
//			connection-thread
// handles incoming requests
//*********************************************************************
struct connectionData {
  int connectionSocket;
  struct sockaddr_in clientAddr;
};

// Mostly copied from epgd (bugfixed ;) )
static void oldDaemonCommands(struct connectionData *client)
{
  struct msgEPGRequest request;
  memset(&request, 0, sizeof(request) );
  if(readNbytes(client->connectionSocket, (char *)&request, sizeof(request) , 2)>0) {
    // do
    int nResultDataSize=0;
    char* pResultData=0;

    printf("Request of actual EPG for '%s'\n", request.Name);

    const SIevent &evt=findSIeventForServiceName(request.Name);

//  readSection(request.Name, &pResultData, &nResultDataSize);

    if(evt.serviceID!=0) {//Found
      printf("EPG found.\n");
      nResultDataSize=strlen(evt.name.c_str())+2+		//Name + del
        strlen(evt.text.c_str())+2+		//Text + del
        strlen(evt.extendedText.c_str())+2+	//ext + del
        3+3+4+1+					//dd.mm.yyyy + del
        3+2+1+					//std:min + del
        3+2+1+1;				//std:min+ del + 0
      pResultData = new char[nResultDataSize];
      SItime siStart = *(evt.times.begin());
      struct tm *pStartZeit = localtime(&siStart.startzeit);
      int nSDay(pStartZeit->tm_mday), nSMon(pStartZeit->tm_mon), nSYear(pStartZeit->tm_year+1900),
       nSH(pStartZeit->tm_hour), nSM(pStartZeit->tm_min);

      long int uiEndTime(siStart.startzeit+siStart.dauer);
      struct tm *pEndeZeit = localtime((time_t*)&uiEndTime);
      int nFH(pEndeZeit->tm_hour), nFM(pEndeZeit->tm_min);

      sprintf(pResultData, "%s\xFF%s\xFF%s\xFF%02d.%02d.%04d\xFF%02d:%02d\xFF%02d:%02d\xFF",
        evt.name.c_str(),
        evt.text.c_str(),
        evt.extendedText.c_str(), nSDay, nSMon, nSYear, nSH, nSM, nFH, nFM );
    }
    else
      printf("actual EPG not found!\n");

    // response
    char* pResponse=new char[sizeof(msgEPGResponse)+nResultDataSize];;
    struct msgEPGResponse* pmResponse=(struct msgEPGResponse*)pResponse;
    pmResponse->version=2;
    snprintf( pmResponse->sizeOfBuffer, sizeof(pmResponse->sizeOfBuffer), "%d", nResultDataSize);

    if( nResultDataSize > 0 ) {
      memcpy(pmResponse->pEventBuffer, pResultData, nResultDataSize+1);
      delete[] pResultData;
    }
//    printf("%s\n", pResultData);

    write(client->connectionSocket, pResponse, sizeof(*pmResponse)+nResultDataSize );
    delete[] pResponse;
  }
}

static void *connectionThread(void *conn)
{
struct connectionData *client=(struct connectionData *)conn;

  printf("Connection from %s\n", inet_ntoa(client->clientAddr.sin_addr));
//  int connectionSocket=(int)socketNr;
  oldDaemonCommands(client);
  close(client->connectionSocket);
  printf("Connection from %s closed!\n", inet_ntoa(client->clientAddr.sin_addr));
  delete client;
  return 0;
}

//*********************************************************************
//			sdt-thread
// reads sdt for service list
//*********************************************************************
static void *sdtThread(void *)
{
int fd;
struct SI_section_header header;
struct dmxSctFilterParams flt;
char *buf;
const unsigned timeoutInSeconds=2;

  printf("sdt-thread started.\n");
  memset (&flt.filter, 0, sizeof (struct dmxFilter));
  flt.pid              = 0x11;
  flt.filter.filter[0] = 0x42;
  flt.filter.mask[0]   = 0xff;
//  flt.filter.mask[0]   = 0xff;
  flt.timeout          = 0;
  flt.flags            = DMX_IMMEDIATE_START | DMX_CHECK_CRC;

  if ((fd = open("/dev/ost/demux0", O_RDWR)) == -1) {
    perror ("/dev/ost/demux0");
    return 0;
//    return 1;
  }
  if (ioctl (fd, DMX_SET_FILTER, &flt) == -1) {
    close(fd);
    perror ("DMX_SET_FILTER");
    return 0;
//    return 2;
  }
  for(;;) {
    int rc=readNbytes(fd, (char *)&header, sizeof(header), timeoutInSeconds);
    if(!rc) {
//      return 0; // timeout -> kein EPG
      continue; // timeout -> kein EPG
    }
    else if(rc<0) {
      close(fd);
//      perror ("read header");
      break;
//      return 3;
    }
    buf=new char[sizeof(header)+header.section_length-5];
    if(!buf) {
      fprintf(stderr, "Not enough memory!\n");
      break;
    }
    // Den Header kopieren
    memcpy(buf, &header, sizeof(header));
    rc=readNbytes(fd, buf+sizeof(header), header.section_length-5, timeoutInSeconds);
    if(!rc) {
      delete[] buf;
      continue; // timeout -> kein EPG
//      return 0; // timeout -> kein EPG
    }
    else if(rc<0) {
//      perror ("read section");
      delete[] buf;
      break;
//      return 5;
    }
    if(header.current_next_indicator) {
      // Wir wollen nur aktuelle sections
      SIsectionSDT sdt(SIsection(sizeof(header)+header.section_length-5, buf));
//      SIsection section(sizeof(header)+header.section_length-5, buf);
//      SIsectionSDT sdt(section);
//      printf("Section read\n");
      pthread_mutex_lock(&servicesLock);
//      printf("Services: %d\n", sdt.services().size());
      services.insert(sdt.services().begin(), sdt.services().end());
//      printf("Services over all: %d\n", services.size());
      pthread_mutex_unlock(&servicesLock);
//      printf("Services inserted\n");
    } // if
    else
      delete[] buf;
  } // for
  close(fd);
  printf("sdt-thread ended\n");
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
      unsigned char utc_date_hi : 8;
      unsigned char utc_date_lo : 8;
      unsigned char utc_hour : 4;
      unsigned char utc_hour_ten : 4;
      unsigned char utc_min : 4;
      unsigned char utc_min_ten : 4;
      unsigned char utc_sec : 4;
      unsigned char utc_sec_ten : 4;
//      unsigned long long UTC_time : 40;
      // 8 bytes
      unsigned char reserved2 : 4;
      unsigned short descriptors_loop_length : 12;
} __attribute__ ((packed)) ; // 10 bytes

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

static void *timeThread(void *)
{
int fd;
struct SI_section_TOT_header header;
struct dmxSctFilterParams flt;
const unsigned timeoutInSeconds=31;
char *buf;

  printf("time-thread started.\n");
  memset (&flt.filter, 0, sizeof (struct dmxFilter));
  flt.pid              = 0x14;
  flt.filter.filter[0] = 0x73; // TOT
  flt.filter.mask[0]   = 0xff;
  flt.timeout          = 0;
  flt.flags            = DMX_IMMEDIATE_START | DMX_CHECK_CRC;

  fd=0;  for(;;) {
    if(!fd) {
      if ((fd = open("/dev/ost/demux0", O_RDWR)) == -1) {
        perror ("/dev/ost/demux0");
        return 0;
      }
      if (ioctl (fd, DMX_SET_FILTER, &flt) == -1) {
        close(fd);
        perror ("DMX_SET_FILTER");
        return 0;
      }
    }
    int rc=readNbytes(fd, (char *)&header, sizeof(header), timeoutInSeconds);
    if(!rc) {
      continue; // timeout -> kein EPG
    }
    else if(rc<0) {
      close(fd);
      break;
    }
    buf=new char[sizeof(header)+header.section_length-5];
    if(!buf) {
      fprintf(stderr, "Not enough memory!\n");
      close(fd);
      break;
    }
    // Den Header kopieren
    memcpy(buf, &header, sizeof(header));
    rc=readNbytes(fd, buf+sizeof(header), header.section_length-5, timeoutInSeconds);
    if(!rc) {
      delete[] buf;
      continue; // timeout -> kein TDT
    }
    else if(rc<0) {
      delete[] buf;
      break;
    }
    time_t tim=changeUTCtoCtime(((const unsigned char *)&header)+3);
    printf("time: %ld\n", tim);
    if(tim) {
      timeOffsetFound=0;
      parseDescriptors(buf+sizeof(struct SI_section_TOT_header), ((struct SI_section_TOT_header *)buf)->descriptors_loop_length, "DEU");
/*
      struct tm tm_time;
      memset(&tm_time, 0, sizeof(tm_time));
      tm_time.tm_sec=(seconds&0x0f)+((seconds&0xf0)>>4)*10;
      tm_time.tm_min=(minutes&0x0f)+((minutes&0xf0)>>4)*10;
      tm_time.tm_hour=(hour&0x0f)+((hour&0xf0)>>4)*10;
      tm_time.tm_mday=day;
      tm_time.tm_mon=month-1;
      tm_time.tm_year=year-1900;
      tm_time.tm_isdst=0;
*/
//      time_t t=time(NULL);
//      printf("local time: %s", ctime(&t));
//      printf("%d:%d:%d\n", tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
//      printf("%d.%d.%d\n", tm_time.tm_mday, tm_time.tm_mon, tm_time.tm_year);
//      tim=mktime(&tm_time);
//      printf("local time: %s", ctime(&tim));
      if(timeOffsetFound)
        tim+=timeOffsetMinutes*60L;
      printf("local time: %s", ctime(&tim));
      if(stime(&tim)< 0) {
        perror("cannot set date");
	break;
      }
      time_t t=time(NULL);
      printf("local time: %s", ctime(&t));
    }
    delete[] buf;
    close(fd);
    fd=0;
    rc=60*30;
    while(rc)
      rc=sleep(rc); // sleep 30 minutes
  } // for
  printf("time-thread ended\n");
  return 0;
}

//*********************************************************************
//			EIT-thread
// reads EPG-datas
//*********************************************************************
static void *eitThread(void *)
{
int fd;
struct SI_section_header header;
struct dmxSctFilterParams flt;
char *buf;
const unsigned timeoutInSeconds=2;

  printf("eit-thread started.\n");
  memset (&flt.filter, 0, sizeof (struct dmxFilter));
  flt.pid              = 0x12;
  flt.filter.filter[0] = 0x4e; // present/following
  flt.filter.mask[0]   = 0xfe; // -> 4e und 4f
  flt.timeout          = 0;
  flt.flags            = DMX_IMMEDIATE_START | DMX_CHECK_CRC;

  if ((fd = open("/dev/ost/demux0", O_RDWR)) == -1) {
    perror ("/dev/ost/demux0");
    return 0;
//    return 1;
  }
  if (ioctl (fd, DMX_SET_FILTER, &flt) == -1) {
    close(fd);
    perror ("DMX_SET_FILTER");
    return 0;
//    return 2;
  }
  for(;;) {
    int rc=readNbytes(fd, (char *)&header, sizeof(header), timeoutInSeconds);
    if(!rc) {
//      close(fd);
//      return 0; // timeout -> kein EPG
      continue; // timeout -> kein EPG
    }
    else if(rc<0) {
      close(fd);
//      perror ("read header");
      break;
//      return 3;
    }
    buf=new char[sizeof(header)+header.section_length-5];
    if(!buf) {
      close(fd);
      fprintf(stderr, "Not enough memory!\n");
      break;
//      return 4;
    }
    // Den Header kopieren
    memcpy(buf, &header, sizeof(header));
    rc=readNbytes(fd, buf+sizeof(header), header.section_length-5, timeoutInSeconds);
    if(!rc) {
//      close(fd);
      delete[] buf;
      continue; // timeout -> kein EPG
//      return 0; // timeout -> kein EPG
    }
    else if(rc<0) {
      close(fd);
//      perror ("read section");
      delete[] buf;
      break;
//      return 5;
    }
    if(header.current_next_indicator) {
      // Wir wollen nur aktuelle sections
      SIsectionEIT eit(SIsection(sizeof(header)+header.section_length-5, buf));
//      SIsection section(sizeof(header)+header.section_length-5, buf);
//      SIsectionEIT eit(section);
      pthread_mutex_lock(&eventsLock);
      events.insert(eit.events().begin(), eit.events().end());
      pthread_mutex_unlock(&eventsLock);
    } // if
    else
      delete[] buf;
  } // for

  printf("eit-thread ended\n");
  return 0;
}

//*********************************************************************
//			housekeeping-thread
// does cleaning on fetched datas
//*********************************************************************
static void *houseKeepingThread(void *)
{
  printf("housekeeping-thread started.\n");
  for(;;) {
    int rc=60;
    while(rc)
      rc=sleep(rc); // sleep 60 seconds
    printf("housekeeping.\n");
    pthread_mutex_lock(&eventsLock);
    printf("Number of events: %u\n", events.size());
    pthread_mutex_unlock(&eventsLock);
    pthread_mutex_lock(&servicesLock);
    printf("Number of services: %u\n", services.size());
    pthread_mutex_unlock(&servicesLock);
    // Speicher-Info abfragen
    struct mallinfo speicherinfo=mallinfo();
    printf("total size of memory occupied by chunks handed out by malloc: %d\n", speicherinfo.uordblks);
    printf("total bytes memory allocated with `sbrk' by malloc, in bytes: %d (%dkb, %fMB)\n",speicherinfo.arena, speicherinfo.arena/1024, (float)speicherinfo.arena/(1024.*1024));
  }

}

int main(void)
{
pthread_t threadTOT, threadEIT, threadSDT, threadHouseKeeping;
int rc;
int listenSocket;
struct sockaddr_in serverAddr;

  printf("$Id: sectionsd.cpp,v 1.5 2001/07/14 10:19:26 fnbrd Exp $\n");

  tzset(); // TZ auswerten

//  if( fork()!= 0 ) // switching to background
//    return 0;

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
    printf("failed to create sdt-thread (rc=%d)\n", rc);
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


  // Unsere Endlosschliefe
  socklen_t clientInputLen = sizeof(serverAddr);
  for(;;) {
    // wir warten auf eine Verbindung
    struct connectionData *client=new connectionData; // Wird vom Thread freigegeben
    do {
      client->connectionSocket = accept(listenSocket, (struct sockaddr *)&(client->clientAddr), &clientInputLen);
    } while(client->connectionSocket == -1);
    pthread_t threadConnection;
    rc=pthread_create(&threadConnection, 0, connectionThread, client);
    if(rc) {
      fprintf(stderr, "failed to create connection-thread (rc=%d)\n", rc);
      return 4;
    }
  }
  printf("sectionsd ended\n");
  return 0;
}

