/*
 * $Id: configfile.h,v 1.3 2002/04/20 21:46:17 Simplex Exp $
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

class CConfigFile
{
	private:

		typedef std::map <std::string, std::string> ConfigDataMap;

		ConfigDataMap configData;
		char delimiter;
		bool modifiedFlag;

	public:
		CConfigFile (const char p_delimiter);

		const bool loadConfig (const std::string p_filename);
		const bool saveConfig (const std::string p_filename);

		std::string getString (const std::string p_keyName, const std::string defaultValue = "");
		void setString (const std::string p_keyName, const std::string p_keyValue);
		int getInt (const std::string p_keyName, const int defaultValue = 0);
		void setInt (const std::string p_keyName, const int p_keyValue);
		bool getBool (const std::string p_keyName, const bool defaultValue = false);
		void setBool (const std::string p_keyName, const bool p_keyValue);

		const bool getModifiedFlag () { return modifiedFlag; }
		void setModifiedFlag (const bool p_value) { modifiedFlag = p_value; }

		std::vector <std::string> getStringVector (const std::string p_keyName);
		void setStringVector (const std::string p_keyName, const std::vector <std::string> p_vec);

		std::vector <int> getIntVector (const std::string p_keyName);
		void setIntVector (const std::string p_keyName, const std::vector <int> p_vec);
};

#endif /* __configfile_h__ */
