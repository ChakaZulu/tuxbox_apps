/*

        $Id: settings.cpp,v 1.13 2003/02/19 11:51:44 thegoodguy Exp $

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


#include "settings.h"

CScanSettings::CScanSettings(void)
	: configfile('\t')
{
	delivery_system = DVB_S;
	satNameNoDiseqc[0] = 0;
	for( int i=0; i<MAX_SATELLITES; i++)
	{
		satName[i][0] = 0;
		satDiseqc[i]  = -1;
	}
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

char* CScanSettings::satOfDiseqc(int diseqc) const{
	if(diseqcMode == NO_DISEQC) return (char*)&satNameNoDiseqc;
	if(diseqc >= 0 && diseqc < MAX_SATELLITES) {
		for(int i=0; i<MAX_SATELLITES;i++) {
			if(diseqc == satDiseqc[i]) return (char*)&satName[i];
		}
	}
	return "Unknown Satellite";
}

void CScanSettings::toSatList( CZapitClient::ScanSatelliteList& satList) const
{
	satList.clear();
	CZapitClient::commandSetScanSatelliteList sat;
	if ( diseqcMode == NO_DISEQC)
	{
		strncpy( sat.satName, satNameNoDiseqc, 30);
		sat.diseqc = 0;
		satList.insert( satList.end(), sat);
	}
	else
	{
		for( int i=0; i<MAX_SATELLITES; i++)
		{
			if ( satDiseqc[i] != -1)
			{
				strncpy( sat.satName, satName[i], 30);
				sat.diseqc = satDiseqc[i];
				satList.insert( satList.end(), sat);
			}
		}
	}
}

void CScanSettings::useDefaults(const delivery_system_t _delivery_system)
{
	delivery_system = _delivery_system;
	bouquetMode     = CZapitClient::BM_DONTTOUCHBOUQUETS;
	diseqcMode      = NO_DISEQC;
	diseqcRepeat    = 0;

	switch (delivery_system)
	{
	case DVB_C:
		strcpy(satNameNoDiseqc, "Telekom/Ish");
		break;
	case DVB_S:
		strcpy(satNameNoDiseqc, "Astra 19.2E");
		break;
	case DVB_T:
		strcpy(satNameNoDiseqc, "");
		break;
	}
}

bool CScanSettings::loadSettings(const std::string fileName, const delivery_system_t _delivery_system)
{
	useDefaults(_delivery_system);

	if(!configfile.loadConfig(fileName))
	{
		return false;
	}

	if (configfile.getInt32("delivery_system",-1) != delivery_system)
	{
		// configfile is not for this delivery system
		configfile.clear();
		return false;
	}

	diseqcMode   = (diseqc_t)                  configfile.getInt32("diseqcMode"  , diseqcMode);
	diseqcRepeat =                             configfile.getInt32("diseqcRepeat", diseqcRepeat);
	bouquetMode  = (CZapitClient::bouquetMode) configfile.getInt32("bouquetMode" , bouquetMode);
	strcpy(satNameNoDiseqc, configfile.getString("satNameNoDiseqc", satNameNoDiseqc).c_str());

	if (diseqcMode != NO_DISEQC)
	{
		int satCount = configfile.getInt32( "satCount", 0 );
		for (int i=0; i<satCount; i++)
		{
			char tmp[10];
			sprintf((char*)&tmp, "SatName%d", i);
			strcpy( satName[i], configfile.getString( tmp, "" ).c_str() );
			sprintf((char*)&tmp, "satDiseqc%d", i);
			satDiseqc[i] = configfile.getInt32( tmp, -1 );
		}
	}
	return true;
}

bool CScanSettings::saveSettings(const std::string fileName)
{
	configfile.setInt32("delivery_system", delivery_system);
	configfile.setInt32( "diseqcMode", diseqcMode );
	configfile.setInt32( "diseqcRepeat", diseqcRepeat );
	configfile.setInt32( "bouquetMode", bouquetMode );
	configfile.setString( "satNameNoDiseqc", satNameNoDiseqc );
	if (diseqcMode != NO_DISEQC)
	{
		int satCount = 0;
		for (int i=0; i<MAX_SATELLITES; i++)
		{
			if (satDiseqc[i] != -1)
			{
				satCount++;
			}
		}
		configfile.setInt32( "satCount", satCount );
		satCount = 0;
		for (int i=0; i<MAX_SATELLITES; i++)
		{
			if (satDiseqc[i] != -1)
			{
				char tmp[10];
				sprintf((char*)&tmp, "SatName%d", satCount);
				configfile.setString( tmp, satName[i] );
				sprintf((char*)&tmp, "satDiseqc%d", satCount);
				configfile.setInt32( tmp, satDiseqc[i] );
				satCount++;
			}
		}
	}

	if(configfile.getModifiedFlag())
	{
		printf("saveing neutrino scan-config\n");
		configfile.saveConfig(fileName);
	}

	return true;
}

/*
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
	try
	{
		is >> (int)settings.bouquetMode;
		is >> (int)settings.diseqcMode;
		is >> settings.diseqcRepeat;
		if (settings.diseqcMode == NO_DISEQC)
		{
			std::string token, satname = "";
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
				std::string token, satname = "";
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
		if (is.bad() || is.fail())
		{
			cout << "Error while loading scansettings! Using default." << endl;
			settings.useDefaults();
		}
		else
		{
			cout << "Loaded scansettings:" << endl << settings << endl;
		}
	}
	catch (...)
	{
		cout << "Exception while loading scansettings! Using default." << endl;
		settings.useDefaults();
	}
	return is;
}
*/
