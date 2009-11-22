//
// $Id: SIutils.cpp,v 1.16 2009/11/22 15:36:48 rhabarber1848 Exp $
//
// utility functions for the SI-classes (dbox-II-project)
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
#include <string.h>

// Houdini: not used in the code at the moment, found in SIsections.cpp
#if 0
static const char descr_tbl[][50] = {
// defined by ISO/IEC 13818-1 P64
	"Reserved",
	"Reserved",
	"Video Stream",
	"Audio Stream",
	"Hierarchy",
	"Registration",
	"Data Stream Alignment",
	"Target Background Grid",
	"Video Window",
	"CA",
	"ISO 639 Language",
	"System Clock",
	"Multiplex Buffer Utilization",
	"Copyright",
	"Maximum Bitrate",
	"Private Data Indicator",
	"Smoothing Buffer",
	"STD",
	"IBP",
	"ITU-T Rec. H.222.0 | ISO/IEC 13818-1 Reserved",
// defined by ETSI
	"Network Name",
	"Service List",
	"Stuffing",
	"Satellite Delivery System",
	"Cable Delivery System",
	"Reserved for future use",
	"Reserved for future use",
	"Bouquet Name",
	"Service",
	"Country Availability",
	"Linkage",
	"NVOD Reference",
	"Time Shifted Service",
	"Short Event",
	"Extended Event",
	"Time Shifted Event",
	"Component",
	"Mosaic",
	"Stream Identifier",
	"CA Identifier",
	"Content",
	"Parental Rating",
	"Teletext",
	"Telephone",
	"Local Time Offset",
	"Subtitling",
	"Terrestrial Delivery System",
	"Multilingual Network Name",
	"Multilingual Bouquet Name",
	"Multilingual Service Name",
	"Multilingual Component",
	"Private Data Specifier",
	"Service Move",
	"Short Smoothing Buffer",
	"Reserved for future use",
	"User defined",
	"FORBIDDEN"
};

// Thanks to tmbinc
const char *decode_descr (unsigned char _index) {
	int index = _index;

	if (_index>=0x13 && _index<=0x3F)
		index = 0x13;

	if (_index>=0x40)
		index -= (0x3F - 0x13);

	if (_index>=0x62 && _index<=0x7F)
		index = 0x62 - (_index - index);

	if (_index>=0x80)
		index -= (0x7F - 0x62);

	if (_index>=0x80 && _index<=0xFE)
		index = 0x80 - (_index - index);

	if (_index == 0xFF)
		index = 0xFF - (_index - index) - (0xFE - 0x80);

	return descr_tbl[index];
}
#endif

// Thanks to kwon
time_t changeUTCtoCtime(const unsigned char *buffer, int local_time)
{
	int year, month, day, y_, m_, k, hour, minutes, seconds, mjd;

	if (!memcmp(buffer, "\xff\xff\xff\xff\xff", 5))
		return 0; // keine Uhrzeit

	mjd = (buffer[0] << 8) | buffer[1];
	hour = buffer[2];
	minutes = buffer[3];
	seconds = buffer[4];

	y_   = (int) ((mjd - 15078.2) / 365.25);
	m_   = (int) ((mjd - 14956.1 - (int) (y_ * 365.25)) / 30.6001);
	day  = mjd - 14956 - (int) (y_ * 365.25) - (int) (m_ * 30.60001);

	k = !!((m_ == 14) || (m_ == 15));

	year  = y_ + k + 1900;
	month = m_ - 1 - k * 12;

	struct tm time;
	memset(&time, 0, sizeof(struct tm));

	time.tm_mday = day;
	time.tm_mon = month - 1;
	time.tm_year = year - 1900;
	time.tm_hour = (hour >> 4) * 10 + (hour & 0x0f);
	time.tm_min = (minutes >> 4) * 10 + (minutes & 0x0f);
	time.tm_sec = (seconds >> 4) * 10 + (seconds & 0x0f);

#if 0
	printf ("Startzeit: GMT: %.2d.%.2d.%.4d  %.2x:%.2x:%.2x\n",
	      day, month, year, hour, minutes, seconds);
	printf ("Startzeit: GMT: %.2d.%.2d.%.4d  %.2d:%.2d:%.2d\n",
	      time.tm_mday, time.tm_mon + 1, time.tm_year + 1900,
	      time.tm_hour, time.tm_min, time.tm_sec);
#endif

	return mktime(&time) + (local_time ? -timezone : 0);
}

// Thanks to tmbinc
int saveStringToXMLfile(FILE *out, const char *c, int withControlCodes)
{
  if(!c)
    return 1;
  // Die Umlaute sind ISO-8859-9 [5]
/*
  char buf[6000];
  int inlen=strlen(c);
  int outlen=sizeof(buf);
//  UTF8Toisolat1((unsigned char *)buf, &outlen, (const unsigned char *)c, &inlen);
  isolat1ToUTF8((unsigned char *)buf, &outlen, (const unsigned char *)c, &inlen);
  buf[outlen]=0;
  c=buf;
*/
  for(;*c; c++) {
    switch ((unsigned char)*c) {
      case '&':
        fprintf(out, "&amp;");
        break;
      case '<':
        fprintf(out, "&lt;");
        break;
      case '\"':
        fprintf(out, "&quot;");
	break;
      case 0x81:
      case 0x82:
        break;
      case 0x86:
        if(withControlCodes)
          fprintf(out, "<b>");
        break;
      case 0x87:
        if(withControlCodes)
          fprintf(out, "</b>");
        break;
      case 0x8a:
        if(withControlCodes)
          fprintf(out, "<br/>");
        break;
      default:
        if (*c<32)
          break;
        if ((*c>=32) && (((unsigned char)*c)<128))
          fprintf(out, "%c", *c);
        else
          fprintf(out, "&#%d;", *c);
     } // case

  } // for
  return 0;
}

// Entfernt die ControlCodes aus dem String (-> String wird evtl. kuerzer)
void removeControlCodes(char *string)
{
  if(!string)
    return;
  for(;*string; )
    if (!((*string>=32) && (((unsigned char)*string)<128)))
      memmove(string, string+1, strlen(string+1)+1);
    else
      string++;
  return ;
}

