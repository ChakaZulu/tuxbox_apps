/*

        $Id: settings.cpp,v 1.1 2002/04/14 08:31:58 Simplex Exp $

	Neutrino-GUI  -   DBoxII-Project

	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA
*/

//#include <iostream>
#include <string>
#include "settings.h"

using namespace std;

void CScanSettings::useDefaults( bool cable = false)
{
	bouquetMode = donttouchBouquets;
	diseqcMode = noDiSEqC;
	satellites.clear();

	SSatellite defaultSat;
	if (cable)
		strcpy( defaultSat.name, "Telekom");
	else
		strcpy( defaultSat.name, "Astra 19.2E");
	defaultSat.diseqc = 0;
	satellites.insert( satellites.end(), defaultSat);
}
/*
ostream &operator<<(ostream& os, const CScanSettings& settings)
{
	os << settings.bouquetMode << endl;
	os << settings.useDiseqc << endl;
	for (uint i=0; i<settings.satellites.size(); i++)
	{
		os << '"' << settings.satellites[i].name << '"' << endl << settings.satellites[i].diseqc << endl;
	}
	return os;
}

istream &operator>>(istream& is, CScanSettings& settings)
{
	string token;
	settings.satellites.clear();
	is >> (int)settings.bouquetMode;
	is >> settings.useDiseqc;
	while (!is.eof())
	{
		is >> token;
		string satname = token;
		int diseqc;
		while ( satname[ satname.length()-1] != '"')
		{
			is >> token;
			satname += " " + token;
		}
		CScanSettings::SSatellite sat;
		is >> sat.diseqc;
		strncpy( sat.name, satname.substr(1, satname.length()-2).c_str(), 30);
		settings.satellites.insert( settings.satellites.end(), sat);

		cout << "[sat]:" << sat.name << "|" << sat.diseqc << endl;
	}
	// for an unknown reason the last entry is waste
	settings.satellites.erase( settings.satellites.end()--);
	return is;
}*/
