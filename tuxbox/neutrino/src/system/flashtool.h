/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	Kommentar:

	Diese GUI wurde von Grund auf neu programmiert und sollte nun vom
	Aufbau und auch den Ausbaumoeglichkeiten gut aussehen. Neutrino basiert
	auf der Client-Server Idee, diese GUI ist also von der direkten DBox-
	Steuerung getrennt. Diese wird dann von Daemons uebernommen.


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
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/


#ifndef __flashtool__
#define __flashtool__

#include <string>
#include <vector>


#include <gui/widget/progressstatus.h>


class CFlashTool
{
	private:
	
		CProgress_StatusViewer* statusViewer;
		std::string mtdDevice;
		std::string ErrorMessage;

		bool erase(int globalProgressEnd=-1);

	public:
		CFlashTool();
		~CFlashTool();

		std::string getErrorMessage();

		void setMTDDevice( std::string mtddevice );
		void setStatusViewer( CProgress_StatusViewer* statusview );

		bool program( std::string filename, int globalProgressEndErase=-1, int globalProgressEndFlash=-1 );
		bool readFromMTD( std::string filename, int globalProgressEnd=-1 );

		bool check_cramfs( std::string filename );

		void reboot();
};


class CFlashVersionInfo
{
 private:
	
	char date[11];
	char time[6];
	char baseImageVersion[5];
	char snapshot;
	
 public:
	
	CFlashVersionInfo(const std::string versionString);
	
	const char * const getDate() const;
	const char * const getTime() const;
	const char * const getBaseImageVersion() const;
	const char * const getType() const;
};


class CMTDInfo
{
	private:

		struct SMTDPartition
		{
			int size;
			int erasesize;
			std::string name;
			std::string filename;
		};

		std::vector<SMTDPartition*> mtdData;
		
		void getPartitionInfo();

		CMTDInfo();
		~CMTDInfo();

	public: 
		static CMTDInfo* getInstance();
	
		int getMTDCount();

		//mtdinfos abfragen (nach mtdnummer)
		std::string getMTDName( int pos );
		std::string getMTDFileName( int pos );
		int getMTDSize( int pos );
		int getMTDEraseSize( int pos );

		//mtdinfos abfragen (nach mtd-filename)
		std::string getMTDName( std::string filename );
		std::string getMTDFileName( std::string filename );
		int getMTDSize( std::string filename );
		int getMTDEraseSize( std::string filename );

		int findMTDNumber( std::string filename );

};


#endif
