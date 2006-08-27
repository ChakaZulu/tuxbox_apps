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

#ifndef __MOUNTER_H__
#define __MOUNTER_H__


#include <string>


class CShareExplorer
{
protected:
	CShareExplorer() {};
	

};

class CMount
{
protected:
	unsigned int idx;
public:
	CMount( unsigned int _idx ) { this->idx = _idx; }
	//remote ip
	std::string getIP();
	void setIP( std::string _IP );
	//remote dir
	std::string getDir();
	void setDir( std::string _dir );
	//local dir
	std::string getLocalDir();
	void setLocalDir( std::string _dir );
	//type
	int getType();
	void setType( int _type );
	//username
	std::string getUsername();
	void setUsername( std::string _username );
	//password
	std::string getPassword();
	void setPassword( std::string _password );
	//options1
	std::string getOptions1();
	void setOptions1( std::string _opts );
	//options2
	std::string getOptions2();
	void setOptions2( std::string _opts );

};

class CMounter : public CShareExplorer
{
protected:
	unsigned int count;

public:
	CMounter();
	
	unsigned int getCount();

	CMount getMount( unsigned int _idx ) { return CMount( _idx ); }

	void Commit();
	void Rollback();
};







#endif /* __MOUNTER_H__ */


