//
// $Id: sdt.cpp,v 1.1 2001/05/14 13:45:32 fnbrd Exp $
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
// $Log: sdt.cpp,v $
// Revision 1.1  2001/05/14 13:45:32  fnbrd
// Erweitert.
//
//

#include <stdio.h>
#include <time.h>

#include "Section.hpp"

int main(int argc, char **argv)
{
  time_t starttime, endtime;
  SIsectionsSDT sdtset;

  starttime=time(NULL);
  sdtset.readSections();
  endtime=time(NULL);
  printf("Sections read: %d\n", sdtset.size());
  printf("Time needed: %ds\n", (int)difftime(endtime, starttime));
//  for_each(sdtset.begin(), sdtset.end(), printSmallSectionHeader());
//  for_each(sdtset.begin(), sdtset.end(), printSIsection());
  for_each(sdtset.begin(), sdtset.end(), printSIsectionSDT());
  return 0;
}
