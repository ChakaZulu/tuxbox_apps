//
// Beispiel zur Benutzung der Klasse Section
//
//
// $Id: epg.cpp,v 1.2 2001/05/12 23:55:04 fnbrd Exp $
//
// $Log: epg.cpp,v $
// Revision 1.2  2001/05/12 23:55:04  fnbrd
// Ueberarbeitet, geht aber noch nicht ganz.
//
//

//#define READ_PRESENT_INFOS

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <sys/poll.h>
#include <string.h>

#include <set>
#include <algorithm>
#include <list>

#include <ost/dmx.h>

#include "Section.hpp"

inline int readNbytes(int fd, char *buf, int n)
{
int j;
  for(j=0; j<n;) {
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

/*
struct sID {
  unsigned tableID;
  unsigned serviceID;
  unsigned sectionNr;
};
*/

int main(int argc, char **argv)
{
  int fd;
  int i, sidx;
  struct dmxSctFilterParams flt;
  int first_number=-1;
  int first_id=-1;
  int first_table=-1;
  time_t starttime, endtime;
  Sections epgset, missingSections;
  struct section_header header;

  starttime=time(NULL);

  // So soll es mal sein:
//  epgset.readSections(0x12, 0x4e, 0xff);

  memset (&flt.filter, 0, sizeof (struct dmxFilter));

  flt.pid              = 0x12;
#ifdef READ_PRESENT_INFOS
  flt.filter.filter[0] = 0x4e; // actual TS present/following information
  flt.filter.mask[0]   = 0xFF;
#else
  flt.filter.filter[0] = 0x50; // actual TS event schedule information
  flt.filter.mask[0]   = 0xF0;
#endif
  flt.timeout          = 0;
//  flt.flags            = DMX_IMMEDIATE_START;
  flt.flags            = DMX_IMMEDIATE_START | DMX_CHECK_CRC;

// Mit CHECK_CRC muss man den Algorithmus anders machen, da evtl. Packete mit falscher
// CRC neu eingelesen werden muessen

  if ((fd = open("/dev/ost/demux0", O_RDWR)) == -1) {
    perror ("/dev/ost/demux0");
    return 2;
  }
  if (ioctl (fd, DMX_SET_FILTER, &flt) == -1) {
    perror ("DMX_SET_FILTER");
    return 3;
  }
  i=0;
  // Erstes Segment lesen
  if(readNbytes(fd, (char *)&header, sizeof(header))<0) {
    perror ("read header");
    return 4;
  }
  char *buf=new char[sizeof(header)+header.section_length-11];
  if(!buf) {
    printf("Not enough memory!\n");
    return 1;
  }
  // Den Header kopieren
  memcpy(buf, &header, sizeof(header));
  if(readNbytes(fd, buf+sizeof(header), header.section_length-11)<0) {
    perror ("read section");
    return 6;
  }
  epgset.insert(Section(sizeof(header)+header.section_length-11, buf));
//  epgset.insert(Section(buf, sizeof(header)+header.section_length-11));
//  delete[] buf;
  first_id=header.service_id;
  first_number=header.section_number;
  first_table=header.table_id;
  i++;
  // Die restlichen Segmente lesen
  for(;;i++) {
//    printf("Reading section %04d\n", i);
    if(readNbytes(fd, (char *)&header, sizeof(header))<0) {
      perror ("read header");
      return 4;
    }
    if(header.service_id==first_id && header.section_number==first_number && header.table_id==first_table)
      break;
    buf=new char[sizeof(header)+header.section_length-11];
    if(!buf) {
      printf("Not enough memory!\n");
      return 1;
    }
    // Den Header kopieren (evtl. malloc und realloc nehmen)
    memcpy(buf, &header, sizeof(header));
    // den Rest der Section einlesen
    if(readNbytes(fd, buf+sizeof(header), header.section_length-11)<0) {
      perror ("read section");
      return 6;
    }
    epgset.insert(Section(sizeof(header)+header.section_length-11, buf));
//    epgset.insert(Section(buf, sizeof(header)+header.section_length-11));
//    delete[] buf;
  }
  close(fd);
/*
  endtime=time(NULL);
  printf("Sections read: %d\n", i);
  printf("Elements in Set: %d\n\n", epgset.size());
*/
  // Jetzt erstellen wir eine Liste der fehlenden Sections
  Sections::iterator k;
  unsigned actualServiceID=(unsigned)-1;
  unsigned actualTableID=(unsigned)-1;
  unsigned maxNr=0;
  unsigned lastNr=0;
  for(k=epgset.begin(); k!=epgset.end(); k++) {
    if(k->serviceID()!=actualServiceID || k->tableID()!=actualTableID) {
      // Neue Service-ID
      maxNr=k->lastSectionNumber();
      lastNr=k->sectionNumber();
      actualServiceID=k->serviceID();
      actualTableID=k->tableID();
    }
    else if(k->sectionNumber()!=lastNr+1) {
      // Es fehlen Sections
      for(unsigned l=lastNr+1; l<k->sectionNumber(); l++) {
	printf("Hallo t: 0x%02x s: %u nr: %u last: %u max: %u l: %u\n", actualTableID, actualServiceID, k->sectionNumber(), lastNr, maxNr, l);
        missingSections.insert(Section(actualTableID, actualServiceID, l));
      }
      lastNr=k->sectionNumber();
    }
    else
      lastNr=k->sectionNumber();
  }

//  printf("Sections read: %d\n", i);
  printf("Sections read: %d\n\n", epgset.size());
  printf("Sections misssing:\n");

  // Die fehlenden Sections ausgeben
  for_each(missingSections.begin(), missingSections.end(), printSmallSectionHeader());

  printf("Sections read: %d\n\n", epgset.size());
  printf("Sections misssing: %d\n", missingSections.size());

  printf("Searching missing sections\n");
  if ((fd = open("/dev/ost/demux0", O_RDWR)) == -1) {
    perror ("/dev/ost/demux0");
    return 2;
  }
  if (ioctl (fd, DMX_SET_FILTER, &flt) == -1) {
    perror ("DMX_SET_FILTER");
    return 3;
  }
  // Jetzt lesen wir die fehlenden Sections ein
  for(;;i++) {
//    printf("Reading section %04d\n", i);
    if(readNbytes(fd, (char *)&header, sizeof(header))<0) {
      perror ("read header");
      return 4;
    }
    buf=new char[sizeof(header)+header.section_length-11];
    if(!buf) {
      printf("Not enough memory!\n");
      return 1;
    }
    // Den Header kopieren (evtl. malloc und realloc nehmen)
    memcpy(buf, &header, sizeof(header));
    // den Rest der Section einlesen
    if(readNbytes(fd, buf+sizeof(header), header.section_length-11)<0) {
      perror ("read section");
      return 6;
    }
    if(missingSections.find(Section(header.table_id, header.service_id, header.section_number))!=missingSections.end()) {
      printf("Find missing section\n");
//    if(missingSections.find(Section(header.table_id, header.service_id, header.section_number).key())!=missingSections.end()) {
      // War bisher vermisst
      // In die Menge einfuegen
      epgset.insert(Section(sizeof(header)+header.section_length-11, buf));
      // Und aus der vermissten Menge entfernen
//      printf("Sections misssing: %d\n", missingSections.size());
      missingSections.erase(Section(header.table_id, header.service_id, header.section_number));
      printf("Sections misssing: %d\n", missingSections.size());
//      missingSections.erase(Section(header.table_id, header.service_id, header.section_number).key());
    }
    else
      // Puffer wieder loeschen
      delete[] buf;
  }
  close(fd);
  endtime=time(NULL);

  printf("\nSections read: %d\n", i);
//  printf("Sections misssing: %d\n", missingSections.size());
  printf("Time needed: %ds\n", (int)difftime(endtime, starttime));
  // Speicher freigeben :)
  return 0;
}
