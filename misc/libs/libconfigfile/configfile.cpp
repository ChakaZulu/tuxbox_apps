/*
 * $Id: configfile.cpp,v 1.1 2002/04/14 23:19:49 obi Exp $
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

CConfigFile::CConfigFile(const char p_delimiter)
{
	modifiedFlag = false;
	delimiter = p_delimiter;
}

const bool CConfigFile::loadConfig(std::string p_filename)
{
	FILE* fd = fopen(p_filename.c_str(), "r");

	if (fd == NULL)
	{
		perror(p_filename.c_str());
		return false;
	}

	configData.clear();
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

std::string CConfigFile::getString (std::string p_keyName)
{
	return configData[p_keyName];
}

void CConfigFile::setString (std::string p_keyName, std::string p_keyValue)
{
	configData[p_keyName] = p_keyValue;
}

int CConfigFile::getInt (std::string p_keyName)
{
	return atoi(configData[p_keyName].c_str());
}

void CConfigFile::setInt (std::string p_keyName, int p_keyValue)
{
	char *configDataChar = (char *) malloc(sizeof(p_keyValue));
	sprintf(configDataChar, "%d", p_keyValue);
	configData[p_keyName] = std::string(configDataChar);
	free(configDataChar);
}

bool CConfigFile::getBool (std::string p_keyName)
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

void CConfigFile::setBool (std::string p_keyName, bool p_keyValue)
{
	if (p_keyValue)
	{
		configData[p_keyName] = std::string("true");
	}
	else
	{
		configData[p_keyName] = std::string("false");
	}
}

const bool CConfigFile::saveConfig (std::string p_filename)
{
	std::ofstream configFile (p_filename.c_str());

	if (configFile != NULL)
	{
		std::map <std::string, std::string>::iterator it;
		
		for (it = configData.begin(); it != configData.end(); it++)
		{
			configFile << it->first << "=" << it->second << std::endl;
		}

		configFile.close();
		return true;
	}
	else
	{
		std::cerr << "unable to open file " << p_filename << "for writing." << std::endl;
		return false;
	}
}

std::vector <std::string> CConfigFile::getStringVector (std::string p_keyName)
{
	std::string keyValue = configData[p_keyName];
	std::vector <std::string> vec;
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

	vec.push_back(keyValue.substr(pos, length));
	return vec;
}

void CConfigFile::setStringVector (std::string p_keyName, std::vector <std::string> p_vec)
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

std::vector <int> CConfigFile::getIntVector (std::string p_keyName)
{
	std::string keyValue = configData[p_keyName];
	std::vector <int> vec;
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

	vec.push_back(atoi(keyValue.substr(pos, length).c_str()));
	return vec;
}

void CConfigFile::setIntVector (std::string p_keyName, std::vector <int> p_vec)
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
		configData[p_keyName] += std::string(tmp);
		free(tmp);
	}
}

