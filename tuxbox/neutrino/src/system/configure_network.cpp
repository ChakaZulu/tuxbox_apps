/*
 * $Header: /cvs/tuxbox/apps/tuxbox/neutrino/src/system/configure_network.cpp,v 1.1 2003/03/05 02:20:48 thegoodguy Exp $
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

#include "configure_network.h"
#include "libnet.h"             /* netGetNameserver, netSetNameserver   */
#include "network_interfaces.h" /* getInetAttributes, setInetAttributes */
#include <stdlib.h>             /* system                               */

CNetworkConfig::CNetworkConfig(void)
{
	char our_nameserver[16];
	netGetNameserver(our_nameserver);
	nameserver = our_nameserver;
	inet_static = getInetAttributes("eth0", address, netmask, broadcast, gateway);
	copy_to_orig();
}

void CNetworkConfig::copy_to_orig(void)
{
	orig_address     = address;
	orig_netmask     = netmask;
	orig_broadcast   = broadcast;
	orig_gateway     = gateway;
	orig_inet_static = inet_static;
}

bool CNetworkConfig::modified_from_orig(void)
{
	return ((orig_address     != address    ) ||
		(orig_netmask     != netmask    ) ||
		(orig_broadcast   != broadcast  ) ||
		(orig_gateway     != gateway    ) ||
		(orig_inet_static != inet_static));
}

void CNetworkConfig::commitConfig(void)
{
	if (inet_static)
	{
		if (modified_from_orig())
		{
			copy_to_orig();
			netSetNameserver(nameserver.c_str());
			setInetAttributes("eth0", address, netmask, broadcast, gateway);
			system("ifdown eth0");
			system("ifup eth0");
		}
	}
}
