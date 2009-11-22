//
// $Id: epg.cpp,v 1.19 2009/11/22 15:36:48 rhabarber1848 Exp $
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

//#define READ_PRESENT_INFOS

#include <stdio.h>
#include <time.h>

#include <set>
#include <algorithm>
#include <string>

#include "SIutils.hpp"
#include "SIservices.hpp"
#include "SIevents.hpp"
#include "SIbouquets.hpp"
#include "SIsections.hpp"

int main(int /*argc*/, char** /*argv*/)
{
  time_t starttime, endtime;
  SIsectionsSDT sdtset;
#ifdef READ_PRESENT_INFOS
  SIsectionsEIT epgset;
#else
  SIsectionsEITschedule epgset;
#endif

  tzset(); // TZ auswerten

  starttime=time(NULL);
  epgset.readSections();
  sdtset.readSections();
  endtime=time(NULL);
  printf("EIT Sections read: %d\n", epgset.size());
  printf("SDT Sections read: %d\n", sdtset.size());
  printf("Time needed: %ds\n", (int)difftime(endtime, starttime));
//  for_each(epgset.begin(), epgset.end(), printSmallSectionHeader());
//  for_each(epgset.begin(), epgset.end(), printSIsection());
  SIevents events;
  for(SIsectionsEIT::iterator k=epgset.begin(); k!=epgset.end(); k++)
    events.insert((k->events()).begin(), (k->events()).end());

  SIservices services;
  for(SIsectionsSDT::iterator k=sdtset.begin(); k!=sdtset.end(); k++)
    services.insert((k->services()).begin(), (k->services()).end());

  // Damit wir die Zeiten der nvods richtig einsortiert bekommen
  // Ist bei epgLarge eigentlich nicht noetig, da die NVODs anscheinend nur im present/actual (epgSmall) vorkommen
  events.mergeAndRemoveTimeShiftedEvents(services);

  for_each(events.begin(), events.end(), printSIeventWithService(services));
//  for_each(events.begin(), events.end(), printSIevent());
//  for_each(epgset.begin(), epgset.end(), printSIsectionEIT());

//  int i=0;
//  for(SIsectionsEIT::iterator s=epgset.begin(); s!=epgset.end(); s++, i++) {
//    char fname[20];
//    sprintf(fname, "seit%d", i+1);
//    s->saveBufferToFile(fname);
//  }
  return 0;
}
