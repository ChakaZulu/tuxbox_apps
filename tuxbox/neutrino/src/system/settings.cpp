/*

        $Id: settings.cpp,v 1.2 2002/04/14 19:57:48 Simplex Exp $

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

#include <iostream>
#include <string>
#include "settings.h"

using namespace std;

CScanSettings::CScanSettings()
{
	satNameNoDiseqc[0] = 0;
	for( int i=0; i<MAX_SATELLITES; i++)
	{
		satName[i][0] = 0;
		satDiseqc[i]  = -1;
	}
}

void CScanSettings::useDefaults( bool cable = false)
{
	bouquetMode = donttouchBouquets;
	diseqcMode = NO_DISEQC;
	diseqcRepeat = 0;

	if (cable)
		strcpy( satNameNoDiseqc, "Telekom");
	else
		strcpy( satNameNoDiseqc, "Astra 19.2E");

}

int* CScanSettings::diseqscOfSat( char* satname)
{
	for( int i=0; i<MAX_SATELLITES; i++)
	{
		if ( !strcmp(satName[i], ""))
		{
			strncpy( satName[i], satname, 30);
			return &satDiseqc[i];
		}
		else if ( !strcmp(satName[i], satname))
		{
			return &satDiseqc[i];
		}
	}
	return(NULL);
}

ostream &operator<<(ostream& os, const CScanSettings& settings)
{
	os << settings.bouquetMode << endl;
	os << settings.diseqcMode << endl;
	os << settings.diseqcRepeat << endl;
	if (settings.diseqcMode == NO_DISEQC)
	{
		os << '"' << settings.satNameNoDiseqc << '"';
	}
	else
	{
		int satCount = 0;
		for (int i=0; i<MAX_SATELLITES; i++)
		{
			if (settings.satDiseqc[i] != -1)
				satCount++;
		}
		os << satCount;
		for (int i=0; i<MAX_SATELLITES; i++)
		{
			if (settings.satDiseqc[i] != -1)
			{
				os << endl << '"' << settings.satName[i] << '"' << endl << settings.satDiseqc[i];
			}
		}
	}
	return os;
}

istream &operator>>(istream& is, CScanSettings& settings)
{
	string token;
	is >> (int)settings.bouquetMode;
	is >> (int)settings.diseqcMode;
	is >> settings.diseqcRepeat;
	if (settings.diseqcMode == NO_DISEQC)
	{
		string token, satname = "";
		do
		{
			is >> token;
			satname += token + " ";
		}
		while (token[ token.length()-1] != '"');
		strncpy( settings.satNameNoDiseqc, satname.substr( 1, satname.length()-3).c_str(), 30);
	}
	else
	{
		int satCount;
		is >> satCount;
		cout << "have to read " << satCount << " sats" <<endl;
		for (int i=0; i<satCount; i++)
		{
			string token, satname = "";
			do
			{
				is >> token;
				satname += token + " ";
			}
			while (token[ token.length()-1] != '"');
			strncpy( settings.satName[i], satname.substr( 1, satname.length()-3).c_str(), 30);
			if (i==0)
			{
				strncpy( settings.satNameNoDiseqc, settings.satName[i], 30);
			}
			is >> settings.satDiseqc[i];
			cout << "read " << settings.satName[i] << " "<<settings.satDiseqc[i] <<endl;
		}
		for (int i=satCount; i<MAX_SATELLITES; i++)
		{
			settings.satName[i][0] = 0;
			settings.satDiseqc[i] = -1;
		}
	}
	cout << "Loaded scansettings:" << endl << settings << endl;
	return is;
}
