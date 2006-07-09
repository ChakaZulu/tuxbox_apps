/*
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


#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include <string>
#include <configfile.h>

class CRadioboxSettings
{
private:
	
	CConfigFile	configfile;
	void Defaults();

public:
	
	void Load(  );
	void Save(  );

	CRadioboxSettings( );
	~CRadioboxSettings( );

public:

	std::string	playlist_root;
	std::string library_root;

	enum PLAYORDER 
	{
		PO_Random = 0,
		PO_Normal,
		PO_RepeatAll,
		PO_RepeatFile
	} playorder;

};





#endif /* __SETTINGS_H__ */
