//
// $Id: SIutils.cpp,v 1.3 2001/05/18 13:11:46 fnbrd Exp $
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
// $Log: SIutils.cpp,v $
// Revision 1.3  2001/05/18 13:11:46  fnbrd
// Fast komplett, fehlt nur noch die Auswertung der time-shifted events
// (Startzeit und Dauer der Cinedoms).
//
// Revision 1.2  2001/05/17 01:53:35  fnbrd
// Jetzt mit lokaler Zeit.
//
// Revision 1.1  2001/05/16 15:23:47  fnbrd
// Alles neu macht der Mai.
//
//

#include <stdio.h>

#include <time.h>
#include <string.h>

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

time_t changeUTCtoCtime(const unsigned char *buffer)
{
    int year, month, day, y_, m_, k,
        hour, minutes, seconds;
    unsigned long long utc;
    int mjd, time;

    utc = buffer[0];
    utc = (utc << 32) & 0xff00000000LL;
    utc = utc | (buffer[1] << 24) | (buffer[2] << 16)
              | (buffer[3] << 8) | buffer[4];
    if(utc==0xffffffffffffffffLL)
      // keine Uhrzeit
      return 0;
    mjd  = (utc >> 24) & 0xffff;
    time = utc & 0xffffff;

    y_   = (int) ((mjd - 15078.2) / 365.25);
    m_   = (int) ((mjd - 14956.1 - (int) (y_ * 365.25)) / 30.6001);
    day  = mjd - 14956 - (int) (y_ * 365.25) - (int) (m_ * 30.60001);
    if ((m_ == 14) || (m_ == 15))
      k = 1;
    else
      k = 0;
    year  = y_ + k + 1900;
    month = m_ - 1 - k*12;

    hour    = (time >> 16) & 0xff;
    minutes = (time >>  8) & 0xff;
    seconds = (time      ) & 0xff;
    struct tm zeit;
    memset(&zeit, 0, sizeof(zeit));
    zeit.tm_mday=day;
    zeit.tm_mon=month-1;
    zeit.tm_year=year-1900;
    zeit.tm_hour=(hour>>4)*10+(hour&0x0f);
    zeit.tm_min=(minutes>>4)*10+(minutes&0x0f);
    zeit.tm_sec=(seconds>>4)*10+(seconds&0x0f);
//    printf ("Startzeit: GMT: %.2d.%.2d.%.4d  %.2x:%.2x:%.2x\n",
//            day, month, year, hour, minutes, seconds);
//    printf ("Startzeit: GMT: %.2d.%.2d.%.4d  %.2d:%.2d:%.2d\n",
//      zeit.tm_mday, zeit.tm_mon+1, zeit.tm_year+1900,
//      zeit.tm_hour, zeit.tm_min, zeit.tm_sec);
    return mktime(&zeit)+timezone;
}
