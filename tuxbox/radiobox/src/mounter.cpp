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

// we implement here CMountSetup from statehandler.h
// and CMounter from mounter.h

#include <statehandler.h>
#include <mounter.h>
#include <system/fsmounter.h>
#include <global.h>


#include <sstream>

CMountSetup::CMountSetup()
{
	int i;
	title = "Mounts";
	for( i = 0; i < NETWORK_NFS_NR_OF_ENTRIES; i++ )
	{
		std::stringstream ss;
		ss << i+1 << ": " << CMounter::getMount( i ).getLocalDir();
		
		entries.push_back( ss.str() );
	}
}

void CMountSetup::DoAction( std::string _action )
{


}