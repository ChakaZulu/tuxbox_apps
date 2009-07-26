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


class CFlashTool
{
	private:
	
		int* statusViewer;
		std::string mtdDevice;
		std::string ErrorMessage;

		bool erase(int globalProgressEnd=-1);

	public:
		CFlashTool();
		~CFlashTool();

		const std::string & getErrorMessage(void) const;

		void setMTDDevice( const std::string & mtddevice );
		void setStatusViewer( int* statusview );

		bool program( const std::string & filename, int globalProgressEndErase=-1, int globalProgressEndFlash=-1 );
		bool readFromMTD( const std::string & filename, int globalProgressEnd=-1 );

		void reboot();
};


class CFlashVersionInfo
{
 private:
	
	char date[11];
	char time[6];
	char releaseCycle[5];
	char snapshot;
	
 public:
	
	CFlashVersionInfo(const std::string & versionString);
	
	const char * const getDate(void) const;
	const char * const getTime(void) const;
	const char * const getReleaseCycle(void) const;
	const char * const getType(void) const;
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
		std::string getMTDName(const int pos );
		std::string getMTDFileName(const int pos );
		int getMTDSize(const int pos );
		int getMTDEraseSize(const int pos );

		//mtdinfos abfragen (nach mtd-filename)
		std::string getMTDName(const std::string & filename);
		int getMTDSize( const std::string & filename );
		int getMTDEraseSize( const std::string & filename );

		int findMTDNumber(const std::string & filename);
		int findMTDNumberByName(const std::string & mtdname);

};


#endif
