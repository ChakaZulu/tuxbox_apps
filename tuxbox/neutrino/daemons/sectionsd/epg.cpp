//
// $Id: epg.cpp,v 1.6 2001/05/13 12:37:11 fnbrd Exp $
//
// Beispiel zur Benutzung der SI class lib (dbox-II-project)
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
// $Log: epg.cpp,v $
// Revision 1.6  2001/05/13 12:37:11  fnbrd
// Noch etwas verbessert.
//
// Revision 1.5  2001/05/13 00:39:30  fnbrd
// Etwas aufgeraeumt.
//
// Revision 1.4  2001/05/13 00:08:54  fnbrd
// Kleine Debugausgabe dazu.
//
// Revision 1.2  2001/05/12 23:55:04  fnbrd
// Ueberarbeitet, geht aber noch nicht ganz.
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
  time_t starttime, endtime;
#ifdef READ_PRESENT_INFOS
  SIsectionsEIT epgset;
#else
  SIsectionsEITschedule epgset;
#endif

  starttime=time(NULL);
  epgset.readSections();
  endtime=time(NULL);
  printf("\nSections read: %d\n", epgset.size());
  printf("Time needed: %ds\n", (int)difftime(endtime, starttime));
//  for_each(epgset.begin(), epgset.end(), printSmallSectionHeader());
//  for_each(epgset.begin(), epgset.end(), printSectionHeader());
  return 0;
}
