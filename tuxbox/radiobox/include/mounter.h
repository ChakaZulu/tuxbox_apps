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


#include <global.h>


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
	std::string getIP() { return g_settings.network_nfs_ip[ idx ]; }
	void setIP( std::string _IP ) { g_settings.network_nfs_ip[ idx ] = _IP; }
	//remote dir
	std::string getDir() { return g_settings.network_nfs_dir[ idx ]; }
	void setDir( std::string _dir ) { strncpy( g_settings.network_nfs_dir[ idx ], _dir.c_str(), 99 ); }
	//local dir
	std::string getLocalDir() { return g_settings.network_nfs_local_dir[ idx ]; }
	void setLocalDir( std::string _dir ) { strncpy( g_settings.network_nfs_local_dir[ idx ], _dir.c_str(), 99 ); }
	//type
	int getType() { return g_settings.network_nfs_type[ idx ]; }
	void setType( int _type ) { g_settings.network_nfs_type[ idx ] = _type; }
	//username
	std::string getUsername() { return g_settings.network_nfs_username[ idx ]; }
	void setUsername( std::string _username ) { strncpy( g_settings.network_nfs_username[ idx ], _username.c_str(), 30 ); }
	//password
	std::string getPassword() { return g_settings.network_nfs_password[ idx ]; }
	void setPassword( std::string _password ) { strncpy( g_settings.network_nfs_password[ idx ], _password.c_str(), 30 ); }
	//options1
	std::string getOptions1() { return g_settings.network_nfs_mount_options1[ idx ]; }
	void setOptions1( std::string _opts ) { strncpy( g_settings.network_nfs_mount_options1[ idx ], _opts.c_str(), 30 ); }
	//options2
	std::string getOptions2() { return g_settings.network_nfs_mount_options2[ idx ]; }
	void setOptions2( std::string _opts ) { strncpy( g_settings.network_nfs_mount_options2[ idx ], _opts.c_str(), 30 ); }

};

class CMounter : public CShareExplorer
{
protected:
	unsigned int count;

public:
	CMounter() { g_settings.Load(); };
	
	unsigned int getCount() { return g_settings.network_nfs_count; }

	CMount getMount( unsigned int _idx ) { return CMount( _idx ); }

	void Commit() { g_settings.Save(); };
	void Rollback() { g_settings.Load(); };;
};







#endif /* __MOUNTER_H__ */


