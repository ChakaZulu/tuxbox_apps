//
// Beispiel zur Benutzung der Klasse Section
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

int main(int argc, char **argv)
{
  int fd;
  int i, sidx;
  struct dmxSctFilterParams flt;
  int first_number=-1;
  int first_id=-1;
  time_t starttime, endtime;
  Sections epgset;
  struct section_header header;


  // So soll es mal sein:
  epgset.readSections(0x12, 0x4e, 0xff);

  if ((fd = open("/dev/ost/demux0", O_RDWR)) == -1) {
    perror ("/dev/ost/demux0");
    return 2;
  }

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
  flt.flags            = DMX_IMMEDIATE_START;
//  flt.flags            = DMX_IMMEDIATE_START | DMX_CHECK_CRC;

// Mit CHECK_CRC muss man den Algorithmus anders machen, da evtl. Packete mit falscher
// CRC neu eingelesen werden muessen

  if (ioctl (fd, DMX_SET_FILTER, &flt) == -1) {
    perror ("DMX_SET_FILTER");
    return 3;
  }
  i=0;
  starttime=time(NULL);
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
  epgset.insert(Section(buf, sizeof(header)+header.section_length-11));
  delete[] buf;
  first_id=header.service_id;
  first_number=header.section_number;
  i++;
  // Die restlichen Segmente lesen
  for(;;i++) {
//    printf("Reading section %04d\n", i);
    if(readNbytes(fd, (char *)&header, sizeof(header))<0) {
      perror ("read header");
      return 4;
    }
    if(header.service_id==first_id && header.section_number==first_number)
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
    epgset.insert(Section(buf, sizeof(header)+header.section_length-11));
    delete[] buf;
  }
  close(fd);
  endtime=time(NULL);
  printf("Sections read: %d\n", i);
  printf("Elements in Set: %d\n\n", epgset.size());
  for_each(epgset.begin(), epgset.end(), printSectionHeader());
  printf("\nSections read: %d\n", i);
  printf("Time needed: %ds\n", (int)difftime(endtime, starttime));
  // Speicher freigeben :)
  return 0;
}
