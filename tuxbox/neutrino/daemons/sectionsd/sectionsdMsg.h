#ifndef SECTIONSDMSG_H
#define SECTIONSDMSG_H
//
//  $Id: sectionsdMsg.h,v 1.15 2001/09/18 18:15:28 fnbrd Exp $
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
//  Revision 1.15  2001/09/18 18:15:28  fnbrd
//  2 new commands.
//
//  Revision 1.14  2001/08/16 13:13:12  fnbrd
//  New commands.
//
//  Revision 1.13  2001/08/09 23:35:54  fnbrd
//  Moved the stuff into a struct.
//
//  Revision 1.12  2001/07/25 20:46:21  fnbrd
//  Neue Kommandos, kleine interne Verbesserungen.
//
//  Revision 1.11  2001/07/25 16:46:46  fnbrd
//  Added unique-keys to all commands.
//
//  Revision 1.10  2001/07/25 11:39:42  fnbrd
//  Added Port number
//
//  Revision 1.9  2001/07/20 00:02:47  fnbrd
//  Kleiner Hack fuer besseres Zusammenspiel mit Neutrino.
//
//  Revision 1.8  2001/07/19 22:19:41  fnbrd
//  Noch ne Beschreibung.
//
//  Revision 1.6  2001/07/19 22:02:13  fnbrd
//  Mehr Befehle.
//
//  Revision 1.5  2001/07/17 12:39:18  fnbrd
//  Neue Kommandos
//
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

struct sectionsd {
  static const unsigned short portNumber=1600;

  struct msgRequestHeader {
    char version;
    char command;
    unsigned short dataLength;
  } __attribute__ ((packed)) ;

  struct msgResponseHeader {
    unsigned short dataLength;
  } __attribute__ ((packed)) ;

  static const int numberOfCommands=18;
  enum commands {
    actualEPGchannelName=0,
    actualEventListTVshort,
    currentNextInformation,
    dumpStatusinformation,
    allEventsChannelName,
    setHoursToCache,
    setEventsAreOldInMinutes,
    dumpAllServices,
    actualEventListRadioshort,
    getNextEPG,
    getNextShort,
    pauseScanning, // for the grabbers ;)
    actualEPGchannelID,
    actualEventListTVshortIDs,
    actualEventListRadioShortIDs,
    currentNextInformationID,
    epgEPGid,
    epgEPGidShort
  };
};

//
// Description of Commands:
//
// If a command is recognize then sectionsd will always send a response.
// When requested datas are not found the data length of the response is 0.
//
// actualEPGchannelName:
//   data of request:
//     is channel name with trailing 0 (c-string)
//   data of response:
//     is a string (c-string) describing the EPG (for
//     compatibility with old epgd)
//
// actualEventListTVshort:
//   data of request:
//     -
//   data of response:
//     is a string (c-string) with all cached actual events,
//     3 lines per service, first line unique-event-key, second line service name, third line event name
//
// currentNextInformation:
//   data of request:
//     is channel name with trailing 0 (c-string)
//   data of response:
//     is a string (c-string) describing the current/next EPGs
//
// dumpStatusinformation:
//   data of request:
//     -
//   data of response:
//     is a string (c-string) describing current status of sectionsd
//
// allEventsChannelName:
//   data of request:
//     is channel name with trailing 0 (c-string)
//   data of response:
//     is a string (c-string) describing the cached events for the requestet channel
//     1 line per event format: uniqueEventKey DD.MM HH:MM durationInMinutes Event name
//
// setHoursToCache
//   data of request:
//     unsigned short (hours to cache)
//   data of response:
//     -
//
// setEventsAreOldInMinutes
//   data of request:
//     unsigned short (minutes after events are old (after their end time))
//   data of response:
//     -
//
// dumpAllServicesinformation:
//   data of request:
//     -
//   data of response:
//     is a string (c-string) with all cached services
//     3 lines per service
//     1. line: unique-service-key, service-ID, service-type, eitScheduleFlag (bool),
//              eitPresentFollowingFlag (bool), runningStatus (bool),
//              freeCAmode (bool), number of nvod services
//     2. line: service name
//     3. line: provider name
//
// actualEventListRadioshort:
//   data of request:
//     -
//   data of response:
//     is a string (c-string) with all cached actual events,
//     3 lines per service, first line unique-event-key, second line service name, third line event name
//
// getNextEPG:
//   data of request:
//     8 bytes (long long in 32 bit world) with unique key (binary) of the event for wich the next should be delivered
//     4 bytes with start time (ctime) of the above event
//   data of response:
//     is a string (c-string) describing the EPG:
//     unique key (long long, hex) 0xff name  0xff text  0xff extended text  0xff start time GMT (ctime, hex ) 0xff duration (seconds, hex)
//
// getNextShort:
//   data of request:
//     8 bytes (long long in 32 bit world) with unique key (binary) of the event for wich the next should be delivered
//     4 bytes with start time (ctime) of the above event
//   data of response:
//     is a string (c-string) describing the Event in short terms:
//     1. line unique key (long long, hex), 2. line name, 3. line start time GMT (ctime, hex ), 4 line  duration (seconds, hex)
//
// pauseScanning:
//   data of request:
//     int (1 = pause, 0 = continue)
//   data of response:
//     -
//
// actualEPGchannelID:
//   data of request:
//     is channel ID (4 byte onid<<16+sid)
//   data of response:
//     is a string (c-string) describing the EPG:
//     unique key (long long, hex) 0xff name  0xff text  0xff extended text  0xff start time GMT (ctime, hex ) 0xff duration (seconds, hex) 0xff
//
// actualEventListTVshortIDs:
//   data of request:
//     -
//   data of response:
//     is a string (c-string) with all cached actual events,
//     3 lines per service, first line unique-event-key, second line unique-service-key, third line event name
//
// actualEventListRadioShortIDs:
//   data of request:
//     -
//   data of response:
//     is a string (c-string) with all cached actual events,
//     3 lines per service, first line unique-event-key, second line unique-service-key, third line event name
//
// currentNextInformationID:
//   data of request:
//     4 byte channel ID (4 byte onid<<16+sid)
//     1 byte number of Events (noch nicht implementiert)
//   data of response:
//     is a string (c-string) describing the current/next EPGs
//     every event: 1. line unique key (long long, hex), 2. line name, 3. line start time GMT (ctime, hex ), 4 line  duration (seconds, hex)
//
// epgEPGid:
//   data of request:
//     unique epg ID (8 byte)
//     time_t starttime GMT (4 bytes)
//   data of response:
//     is a string (c-string) describing the EPG:
//     name 0xff text 0xff extended text 0xff start time GMT (ctime, hex ) 0xff duration (seconds, hex) 0xff
//
// epgEPGidShort:
//   data of request:
//     unique epg ID (8 byte)
//   data of response:
//     is a string (c-string) describing the EPG:
//     name  0xff text  0xff extended text 0xff
//
#endif // SECTIONSDMSG_H
