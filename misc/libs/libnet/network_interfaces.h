#ifndef __network_interfaces_h__
#define __network_interfaces_h__

/*
 * $Header: /cvs/tuxbox/apps/misc/libs/libnet/network_interfaces.h,v 1.2 2003/03/05 17:13:11 thegoodguy Exp $
 *
 * (C) 2003 by thegoodguy <thegoodguy@berlios.de>
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

#include <string>

bool getInetAttributes(const std::string name, bool &automatic_start, std::string &address, std::string &netmask, std::string &broadcast, std::string &gateway);
bool setInetAttributes(const std::string name, const bool automatic_start, const std::string address, const std::string netmask, const std::string broadcast, const std::string gateway);

#endif /* __network_interfaces_h__ */
