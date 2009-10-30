/*

        $Id: settings.cpp,v 1.52 2009/10/30 22:06:05 seife Exp $

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


#include <system/settings.h>

#include <zapit/settings.h>

#include <cstring>

const int default_timing[TIMING_SETTING_COUNT] =
{
	60,
	60,
	240,
	6,
	10,
	60,
	3,
	6
};

const neutrino_locale_t timing_setting_name[TIMING_SETTING_COUNT] =
{
	LOCALE_TIMING_MENU,
	LOCALE_TIMING_CHANLIST,
	LOCALE_TIMING_EPG,
	LOCALE_TIMING_INFOBAR,
	LOCALE_TIMING_INFOBAR_RADIO,
	LOCALE_TIMING_FILEBROWSER,
	LOCALE_TIMING_NUMERICZAP,
	LOCALE_TIMING_INFOBAR_MOVIEPLAYER
};


CScanSettings::CScanSettings(void)
	: configfile('\t')
{
	delivery_system = DVB_S;
	satNameNoDiseqc[0] = 0;
	for( int i = 0; i < MAX_SATELLITES; i++)
	{
		satName[i][0] = 0;
		satDiseqc[i] = -1;
		satMotorPos[i] = 0;
	}
}

int * CScanSettings::diseqscOfSat(char* satname)
{
	for( int i=0; i<MAX_SATELLITES; i++)
	{
		if ( !strcmp(satName[i], ""))
		{
			strncpy(satName[i], satname, 30);
			return &satDiseqc[i];
		}
		else if (!strcmp(satName[i], satname))
		{
			return &satDiseqc[i];
		}
	}
	return(NULL);
}

int * CScanSettings::motorPosOfSat(char* satname)
{
	for( int i = 0; i < MAX_SATELLITES; i++)
	{
		if ( !strcmp(satName[i], ""))
		{
			strncpy(satName[i], satname, 30);
			return &satMotorPos[i];
		}
		else if (!strcmp(satName[i], satname))
		{
			return &satMotorPos[i];
		}
	}
	return(NULL);
}
#include <iostream>
char * CScanSettings::satOfDiseqc(int diseqc) const
{
	if (diseqcMode == NO_DISEQC || diseqc == 0 )
	{
		//get satname from scan.conf if diseqc is disabled or diseqc for current  satellite is not used
		return (char *)&satNameNoDiseqc;
	}
	else if (diseqc >= 0 && diseqc < MAX_SATELLITES)
	{
		//get satname from satlist if diseqc is enabled and diseqc is in use with current satellite
		for (int i = 0; i < MAX_SATELLITES; i++)
		{
			if(diseqc == satDiseqc[i])
			{
				return (char *)&satName[i];
			}
		}
	}

	//can't find a current satellite in all modes
	return "Unknown";
}

char * CScanSettings::satOfMotorPos(int32_t motorPos) const
{
	for (int i = 0; i < MAX_SATELLITES; i++) 
	{
		if (motorPos == satMotorPos[i]) 
			return (char *)&satName[i];
	}	
	return "Unknown";
}

void CScanSettings::toSatList( CZapitClient::ScanSatelliteList& satList) const
{
	satList.clear();
	CZapitClient::commandSetScanSatelliteList sat;
	if  (TP_scan)
	{
		for (int i = 0; i < MAX_SATELLITES; i++)
		{
			if (satDiseqc[i] != -1) {
				sat.diseqc = satDiseqc[i];
				strncpy(sat.satName, satName[i], 30);
				satList.push_back(sat);
			}
		}
		if (satList.size() == 0) {
			strncpy(sat.satName, satNameNoDiseqc, 30);
			sat.diseqc = 0;
			satList.push_back(sat);
		}
	}
	else if  (diseqcMode == NO_DISEQC)
	{
		strncpy(sat.satName, satNameNoDiseqc, 30);
		sat.diseqc = 0;
		satList.push_back(sat);
	}
	else if  (diseqcMode == DISEQC_1_2)
	{
		strncpy(sat.satName, satNameNoDiseqc, 30);
		sat.diseqc = -1;
		for (int i = 0; i < MAX_SATELLITES; i++)
		{
			if (!strcmp(satName[i], satNameNoDiseqc))
			{
				if (satDiseqc[i] != -1)
					sat.diseqc = satDiseqc[i];
				break;
			}
		}
		satList.push_back(sat);
	}
	else
	{
		for( int i = 0; i < MAX_SATELLITES; i++)
		{
			if (satDiseqc[i] != -1)
			{
				strncpy(sat.satName, satName[i], 30);
				sat.diseqc = satDiseqc[i];
				satList.push_back(sat);
			}
		}
	}
}

void CScanSettings::toMotorPosList(CZapitClient::ScanMotorPosList& motorPosList) const
{
	motorPosList.clear();
	CZapitClient::commandSetScanMotorPosList sat;
	
	for (int i = 0; i < MAX_SATELLITES; i++)
	{
		if (strlen(satName[i]) != 0)
		{
			sat.satPosition = satPosition[i];
			sat.motorPos = satMotorPos[i];
			motorPosList.push_back(sat);
		}
	}
}

void CScanSettings::useDefaults(const delivery_system_t _delivery_system)
{
	delivery_system	= _delivery_system;
	bouquetMode	= CZapitClient::BM_UPDATEBOUQUETS;
	scanType	= CZapitClient::ST_ALL;
	diseqcMode	= NO_DISEQC;
	diseqcRepeat	= 0;
#ifdef HAVE_TRIPLEDRAGON
	TP_mod		= 0;
#elif HAVE_DVB_API_VERSION >= 3
	TP_mod		= QAM_AUTO;
#else
	// i do not know how to do it correctly for old API -- seife
	TP_mod		= QAM_256;
#endif

	switch (delivery_system)
	{
		case DVB_C:
			strcpy(satNameNoDiseqc, "Kabel Deutschland");
			break;
		case DVB_S:
			strcpy(satNameNoDiseqc, "Astra 19.2E");
			break;
		case DVB_T:
			strcpy(satNameNoDiseqc, "");
			break;
	}
}

bool CScanSettings::loadSettings(const char * const fileName, const delivery_system_t _delivery_system)
{
	useDefaults(_delivery_system);

	if(!configfile.loadConfig(fileName))
		return false;

	if (configfile.getInt32("delivery_system", -1) != delivery_system)
	{
		// configfile is not for this delivery system
		configfile.clear();
		return false;
	}

	diseqcMode = (diseqc_t) configfile.getInt32("diseqcMode"  , diseqcMode);
	diseqcRepeat = configfile.getInt32("diseqcRepeat", diseqcRepeat);
	bouquetMode = (CZapitClient::bouquetMode) configfile.getInt32("bouquetMode" , bouquetMode);
	scanType=(CZapitClient::scanType) configfile.getInt32("scanType", scanType);
	strcpy(satNameNoDiseqc, configfile.getString("satNameNoDiseqc", satNameNoDiseqc).c_str());

	if (1/*diseqcMode != NO_DISEQC*/)
	{
		char tmp[20];
		int i;
		int s_Count = configfile.getInt32("satCount", 0);
		for (i = 0; i < s_Count; i++)
		{
			sprintf((char*)&tmp, "SatName%d", i);
			strcpy( satName[i], configfile.getString(tmp, "").c_str());
			sprintf((char*)&tmp, "satDiseqc%d", i);
			satDiseqc[i] = configfile.getInt32(tmp, -1);
			
			if (1/*diseqcMode == DISEQC_1_2*/)
			{
				sprintf((char*)&tmp, "satMotorPos%d", i);
				satMotorPos[i] = configfile.getInt32(tmp, -1);
			}
		}
	}
	scan_mode = configfile.getInt32("scan_mode", 0);
	TP_scan = configfile.getInt32("TP_scan", 0);
	TP_fec = configfile.getInt32("TP_fec", 1);
	TP_pol = configfile.getInt32("TP_pol", 0);
#ifdef HAVE_TRIPLEDRAGON
	TP_mod = configfile.getInt32("TP_mod", 0); // dummy
#elif HAVE_DVB_API_VERSION >= 3
	TP_mod = configfile.getInt32("TP_mod", QAM_AUTO); // default qam auto
#else
	// i do not know how to do it correctly for old API -- seife
	TP_mod = configfile.getInt32("TP_mod", QAM_256); // default qam 256
#endif
	strcpy(TP_freq, configfile.getString("TP_freq", "10100000").c_str());
	strcpy(TP_rate, configfile.getString("TP_rate", "27500000").c_str());
	strncpy(TP_satname, configfile.getString("TP_satname", "Astra 19.2E").c_str(), 30);
	TP_diseqc = *diseqscOfSat(TP_satname);
#if HAVE_DVB_API_VERSION >= 3
	if(TP_fec == 4) TP_fec = 5;
#endif
	scanSectionsd = configfile.getInt32("scanSectionsd", 0);

	return true;
}

bool CScanSettings::saveSettings(const char * const fileName)
{
	int s_Count = 0;

	configfile.setInt32( "delivery_system", delivery_system);
	configfile.setInt32( "diseqcMode", diseqcMode );
	configfile.setInt32( "diseqcRepeat", diseqcRepeat );
	configfile.setInt32( "bouquetMode", bouquetMode );
	configfile.setInt32( "scanType", scanType );
	configfile.setString( "satNameNoDiseqc", satNameNoDiseqc );
	
	if (1/*diseqcMode != NO_DISEQC*/)	
	{
		char tmp[20];
		int i;

		for (i = 0; i < MAX_SATELLITES; i++)
			if (satName[i][0] != 0)
				s_Count++;

		configfile.setInt32("satCount", s_Count);
		
		for (i = 0; i < s_Count; i++)
		{
			sprintf((char*)&tmp, "SatName%d", i);
			configfile.setString(tmp, satName[i]);
			sprintf((char*)&tmp, "satDiseqc%d", i);
			configfile.setInt32(tmp, satDiseqc[i]);

			if (1/*diseqcMode == DISEQC_1_2*/)
			{
				sprintf((char*)&tmp, "satMotorPos%d", i);
				configfile.setInt32(tmp, satMotorPos[i]);
			}
		}
	}
	configfile.setInt32("scan_mode",scan_mode );
	configfile.setInt32("TP_scan", TP_scan);
	configfile.setInt32("TP_fec", TP_fec);
	configfile.setInt32("TP_pol", TP_pol);
	configfile.setInt32("TP_mod", TP_mod);
	configfile.setString("TP_freq", TP_freq);
	configfile.setString("TP_rate", TP_rate);
	configfile.setString("TP_satname", TP_satname);

	configfile.setInt32("scanSectionsd",scanSectionsd );

	// check if sat is used
	if(TP_SatSelectMenu) {
		std::vector<std::string> tmpsatNameList;
		tmpsatNameList.clear();
		bool satfound = false;
		// scan the new available sat configurations and change update TP_SatSelectMenu
		for (int i = 0; i < s_Count; i++)
		{
			if (((0 <= satDiseqc[i])   && (diseqcMode != DISEQC_1_2)) ||
			    ((0 != satMotorPos[i]) && (diseqcMode == DISEQC_1_2)))
			{
				if (strcmp(satName[i], TP_satname) == 0) satfound = true;
				tmpsatNameList.push_back(satName[i]);
//				printf("scan settings: (%d) Name = %s, DiseqC = %d, MotorPos = %d\n", i, satName[i], satDiseqc[i], satMotorPos[i]);
			}
		}
	
		if (!satfound && !tmpsatNameList.empty())
			strcpy(TP_satname, tmpsatNameList[0].c_str());

		TP_SatSelectMenu->removeOptions();
		for (uint i=0; i < tmpsatNameList.size(); i++)
		{
			TP_SatSelectMenu->addOption(tmpsatNameList[i].c_str());
		}
	}
	if(configfile.getModifiedFlag())
		configfile.saveConfig(fileName);
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
