//
// $Id: bat.cpp,v 1.7 2009/11/22 15:36:48 rhabarber1848 Exp $
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
  SIsectionsBAT batset;

  starttime=time(NULL);
  batset.readSections();
  endtime=time(NULL);
  printf("Sections read: %d\n", batset.size());
  printf("Time needed: %ds\n", (int)difftime(endtime, starttime));
//  for_each(batset.begin(), batset.end(), printSmallSectionHeader());
//  for_each(batset.begin(), batset.end(), printSIsection());
  for_each(batset.begin(), batset.end(), printSIsectionBAT());
  return 0;
}
