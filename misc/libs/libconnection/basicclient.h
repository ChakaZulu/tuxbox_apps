/*
 * $Header: /cvs/tuxbox/apps/misc/libs/libconnection/basicclient.h,v 1.3 2002/12/07 23:07:21 thegoodguy Exp $
 *
 * Basic Client Class (Neutrino) - DBoxII-Project
 *
 * (C) 2002 by thegoodguy <thegoodguy@berlios.de>
 *
 * License: GPL
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef __basicclient__
#define __basicclient__

class CBasicClient
{
 private:
	int sock_fd;

 protected:
	bool open_connection(const char* socketname);
	bool send_data(const char* data, const size_t size);
	bool receive_data(char* data, const size_t size);
	bool send(const char* socketname, const unsigned char version, const unsigned char command, const char* data = NULL, const unsigned int size = 0);
	void close_connection();
	
	CBasicClient();
};

#endif
