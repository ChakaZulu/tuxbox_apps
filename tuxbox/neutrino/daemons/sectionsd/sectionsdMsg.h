#ifndef SECTIONSDMSG_H
#define SECTIONSDMSG_H
//
//  $Id: sectionsdMsg.h,v 1.4 2001/07/16 12:52:30 fnbrd Exp $
//
//	sectionsdMsg.h (header file with msg-definitions for sectionsd)
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
//  $Log: sectionsdMsg.h,v $
//  Revision 1.4  2001/07/16 12:52:30  fnbrd
//  Fehler behoben.
//
//  Revision 1.3  2001/07/16 11:49:31  fnbrd
//  Neuer Befehl, Zeichen fuer codetable aus den Texten entfernt
//
//  Revision 1.2  2001/07/15 11:58:20  fnbrd
//  Vergangene Zeit in Prozent beim EPG
//
//  Revision 1.1  2001/07/15 04:33:10  fnbrd
//  first release
//
//

struct msgSectionsdRequestHeader {
  char version;
  char command;
  unsigned short dataLength;
};

struct msgSectionsdResponseHeader {
  unsigned short dataLength;
};

#define NUMBER_OF_SECTIONSD_COMMANDS 3

enum sectionsdCommands {
  actualEPGchannelName=0,
  actualEventListTVshort,
  currentNextInformation
};

//
// Description of Commands:
//
// actualEPGchannelName:
//   data of request:
//     is channel name with trailing 0 (c-string)
//   data of response:
//     is a string (c-string) describing the EPG (for
//     compatibility with old epgd)

#endif // SECTIONSDMSG_H
