/*
 * $Id: configfile.cpp,v 1.5 2002/05/07 22:53:00 McClean Exp $
 *
 * configuration object for the d-box 2 linux project
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

#include <stdint.h>
#include "configfile.h"

using namespace std;

CConfigFile::CConfigFile(const char p_delimiter)
{
	modifiedFlag = false;
	unknownKeyQueryedFlag = false;
	delimiter = p_delimiter;
}

void CConfigFile::clear()
{
	configData.clear();
}

const bool CConfigFile::loadConfig(string p_filename)
{
	FILE* fd = fopen(p_filename.c_str(), "r");

	if (fd == NULL)
	{
		perror(p_filename.c_str());
		return false;
	}

	clear();
	modifiedFlag = false;

	char buf[1000];
	char keystr[1000];
	char valstr[1000];

	while (!feof(fd))
	{
		if (fgets(buf, sizeof(buf), fd) != NULL)
		{
			char* tmpptr = buf;
			char* key = (char*) &keystr;
			char* val = (char*) &valstr;
			bool keyfound = false;

			for (; (*tmpptr != 10) && (*tmpptr != 13) && (*tmpptr != '#'); tmpptr++)
			{
				if ((*tmpptr == '=') && (keyfound == false))
				{
					keyfound = true;
				}
				else
				{
					if (keyfound == false)
					{
						*key = *tmpptr;
						key++;
					}
					else
					{
						*val = *tmpptr;
						val++;
					}
				}
			}

			*val = 0;
			*key = 0;
			configData[keystr] = valstr;
		}
	}

	fclose(fd);
	return true;
}

string CConfigFile::getString (string p_keyName, const string defaultValue = "")
{
	if ( configData.find( p_keyName) == configData.end())
	{
		unknownKeyQueryedFlag = true;
		return defaultValue;
	}
	else
	{
		return configData[p_keyName];
	}
}

void CConfigFile::setString (string p_keyName, string p_keyValue)
{
	bool tmpUnknownKeyQueryedFlag = unknownKeyQueryedFlag;
	unknownKeyQueryedFlag = false;
	string oldValue = getString(p_keyName);
	if((oldValue!=p_keyValue) || (unknownKeyQueryedFlag))
	{
		modifiedFlag = true;
		configData[p_keyName] = p_keyValue;
	}
	unknownKeyQueryedFlag = tmpUnknownKeyQueryedFlag;
}

int CConfigFile::getInt (string p_keyName, const int defaultValue = 0)
{
	if ( configData.find( p_keyName) == configData.end())
	{
		unknownKeyQueryedFlag = true;
		return defaultValue;
	}
	else
	{
		return atoi(configData[p_keyName].c_str());
	}
}

void CConfigFile::setInt (string p_keyName, int p_keyValue)
{
	bool tmpUnknownKeyQueryedFlag = unknownKeyQueryedFlag;
	unknownKeyQueryedFlag = false;
	int oldValue = getInt(p_keyName);
	if((oldValue!=p_keyValue) || (unknownKeyQueryedFlag))
	{
		modifiedFlag = true;
		char *configDataChar = (char *) malloc(sizeof(p_keyValue));
		sprintf(configDataChar, "%d", p_keyValue);
		configData[p_keyName] = string(configDataChar);
		free(configDataChar);
	}
	unknownKeyQueryedFlag = tmpUnknownKeyQueryedFlag;
}

bool CConfigFile::getBool (string p_keyName, const bool defaultValue = false)
{
	if ( configData.find( p_keyName) == configData.end())
	{
		unknownKeyQueryedFlag = true;
		return defaultValue;
	}
	else
	{
		if (configData[p_keyName] == "true")
		{
			return true;
		}
		else
		{
			return false;
		}
	}
}

void CConfigFile::setBool (string p_keyName, bool p_keyValue)
{
	bool tmpUnknownKeyQueryedFlag = unknownKeyQueryedFlag;
	unknownKeyQueryedFlag = false;
	bool oldValue = getBool(p_keyName);
	if((oldValue!=p_keyValue) || (unknownKeyQueryedFlag))
	{
		modifiedFlag = true;
		if (p_keyValue)
		{
			configData[p_keyName] = string("true");
		}
		else
		{
			configData[p_keyName] = string("false");
		}
	}
	unknownKeyQueryedFlag = tmpUnknownKeyQueryedFlag;
}

const bool CConfigFile::saveConfig (string p_filename)
{
	ofstream configFile (p_filename.c_str());

	if (configFile != NULL)
	{
		ConfigDataMap::iterator it;

		for (it = configData.begin(); it != configData.end(); it++)
		{
			configFile << it->first << "=" << it->second << endl;
		}

		configFile.close();
		return true;
	}
	else
	{
		cerr << "unable to open file " << p_filename << "for writing." << endl;
		return false;
	}
}

vector <string> CConfigFile::getStringVector (string p_keyName)
{
	string keyValue = configData[p_keyName];
	vector <string> vec;
	uint16_t length = 0;
	uint16_t pos = 0;
	uint16_t i;

	for (i = 0; i < keyValue.length(); i++)
	{
		if (keyValue[i] == delimiter)
		{
			vec.push_back(keyValue.substr(pos, length));
			pos = i + 1;
			length = 0;
		}
		else
		{
			length++;
		}
	}
	if(length==0)
	{
		unknownKeyQueryedFlag = true;
	}

	vec.push_back(keyValue.substr(pos, length));
	return vec;
}

void CConfigFile::setStringVector (string p_keyName, vector <string> p_vec)
{
	uint16_t i;

	for (i = 0; i < p_vec.size(); i++)
	{
		if (i > 0)
		{
			configData[p_keyName] += delimiter;
		}

		configData[p_keyName] += p_vec[i];
	}
}

vector <int> CConfigFile::getIntVector (string p_keyName)
{
	string keyValue = configData[p_keyName];
	vector <int> vec;
	uint16_t length = 0;
	uint16_t pos = 0;
	uint16_t i;

	for (i = 0; i < keyValue.length(); i++)
	{
		if (keyValue[i] == delimiter)
		{
			vec.push_back(atoi(keyValue.substr(pos, length).c_str()));
			pos = i + 1;
			length = 0;
		}
		else
		{
			length++;
		}
	}

	if(length==0)
	{
		unknownKeyQueryedFlag = true;
	}

	vec.push_back(atoi(keyValue.substr(pos, length).c_str()));
	return vec;
}

void CConfigFile::setIntVector (string p_keyName, vector <int> p_vec)
{
	uint16_t i;

	for (i = 0; i < p_vec.size(); i++)
	{
		if (i > 0)
		{
			configData[p_keyName] += delimiter;
		}

		char *tmp = (char *) malloc (sizeof(p_vec[i]));
		sprintf(tmp, "%d", p_vec[i]);
		configData[p_keyName] += string(tmp);
		free(tmp);
	}
}

