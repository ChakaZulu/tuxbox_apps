/*
 * $Id: configfile.h,v 1.2 2002/04/20 21:20:56 Simplex Exp $
 *
 * Copyright (C) 2001, 2002 Andreas Oberritter <obi@tuxbox.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#ifndef __configfile_h__
#define __configfile_h__

#include <cstdio>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace std;

class CConfigFile
{
	private:

		typedef map <string, string> ConfigDataMap;

		ConfigDataMap configData;
		char delimiter;
		bool modifiedFlag;

	public:
		CConfigFile (const char p_delimiter);

		const bool loadConfig (const string p_filename);
		const bool saveConfig (const string p_filename);

		string getString (const string p_keyName, const string defaultValue = "");
		void setString (const string p_keyName, const string p_keyValue);
		int getInt (const string p_keyName, const int defaultValue = 0);
		void setInt (const string p_keyName, const int p_keyValue);
		bool getBool (const string p_keyName, const bool defaultValue = false);
		void setBool (const string p_keyName, const bool p_keyValue);

		const bool getModifiedFlag () { return modifiedFlag; }
		void setModifiedFlag (const bool p_value) { modifiedFlag = p_value; }

		vector <string> getStringVector (const string p_keyName);
		void setStringVector (const string p_keyName, const vector <string> p_vec);

		vector <int> getIntVector (const string p_keyName);
		void setIntVector (const string p_keyName, const vector <int> p_vec);
};

#endif /* __configfile_h__ */
