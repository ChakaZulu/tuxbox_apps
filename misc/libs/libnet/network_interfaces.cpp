/*
 * $Header: /cvs/tuxbox/apps/misc/libs/libnet/network_interfaces.cpp,v 1.2 2003/03/05 14:39:42 thegoodguy Exp $
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

#include <fstream>
#include <list>
#include <map>
#include <string>
#include <sstream>

/*
 * Known bugs:
 * -----------
 *
 * - extension of lines across multiple lines using \ is not supported
 * 
 */

bool read_file(const std::string filename, std::list<std::string> &line)
{
	std::string   s;
	std::ifstream in(filename.c_str());

	line.clear();

	if (!in.is_open())
		return false;

	while (getline(in, s))
		line.push_back(s);

	return true;
}

bool write_file(const std::string filename, const std::list<std::string> line)
{
	std::ofstream out(filename.c_str());

	if (!out.is_open())
		return false;

	for (std::list<std::string>::const_iterator it = line.begin(); it != line.end(); it++)
		out << (*it) << std::endl;

	return true;
}

std::list<std::string>::iterator add_attributes(const std::map<std::string, std::string> attribute, std::list<std::string> &line, std::list<std::string>::iterator here)
{
	for (std::map<std::string, std::string>::const_iterator it = attribute.begin(); it != attribute.end(); it++)
	{
		std::ostringstream out;
		out << '\t' << (it -> first) << ' ' << (it -> second);
		line.insert(here, out.str());
	}
	return here;
}

bool write_interface(const std::string filename, const std::string name, const std::string family, const std::string method, const std::map<std::string, std::string> attribute)
{
	std::string            s;
	std::list<std::string> line;
	bool                   found = false;

	read_file(filename, line); /* ignore return value */

	for (std::list<std::string>::iterator it = line.begin(); it != line.end(); it++)
	{
		{
			std::istringstream in((*it).c_str());
			
			if (!(in >> std::ws >> s))
				continue;
			
			if (s != std::string("iface"))
				continue;
			
			if (!(in >> std::ws >> s))
				continue;
			
			if (s != std::string(name))
				continue;
		}
			
		found = true;

		/* replace line */
		std::ostringstream out;
		out << "iface " << name << ' ' << family << ' ' << method;
		(*it) = out.str();

		/* add attributes */
		it++;
		it = add_attributes(attribute, line, it);

		/* remove current attributes */
		while (it != line.end())
		{
			std::istringstream in((*it).c_str());

			if (!(in >> std::ws >> s))  /* retain empty lines */	
			{
				it++;
				continue;
			}
		
			if (s[0] == '#')            /* retain comments */
			{
				it++;
				continue;
			}
			
			if (s == std::string("iface"))
				break;
			
			if (s == std::string("auto"))
				break;
			
			if (s == std::string("mapping"))
				break;
			
			it = line.erase(it);
		}

		break;
	}

	if (!found)
	{
		std::ostringstream out;
		out << "iface " << name << ' ' << family << ' ' << method;
		line.push_back(out.str());
		add_attributes(attribute, line, line.end());
	}

	return write_file(filename, line);
}

bool read_interface(const std::string filename, const std::string name, std::string &family, std::string &method, std::map<std::string, std::string> &attribute)
{
	std::string   s;
	std::string   t;
	std::ifstream in(filename.c_str());

	attribute.clear();

	if (!in.is_open())
		return false;

	while (getline(in, s))
	{
		{
			std::istringstream in(s.c_str());
			
			if (!(in >> std::ws >> s))
				continue;
			
			if (s != std::string("iface"))
				continue;

			if (!(in >> std::ws >> s))
				continue;
			
			if (s != std::string(name))
				continue;
			
			if (!(in >> std::ws >> s))
				continue;
			
			if (!(in >> std::ws >> t))
				continue;
			
			family = s;
			method = t;
		}
	
		while (getline(in, s))
		{
			std::istringstream in(s.c_str());

			if (!(in >> std::ws >> s))  /* ignore empty lines */	
				continue;
		
			if (s[0] == '#')            /* ignore comments */
				continue;
			
			if (s == std::string("iface"))
				break;
			
			if (s == std::string("auto"))
				break;
			
			if (s == std::string("mapping"))
				break;
			
			if (!(in >> std::ws >> t))
				continue;
			
			attribute[s] = t;
		}
	}

	return true;
}

bool getInetAttributes(const std::string name, std::string &address, std::string &netmask, std::string &broadcast, std::string &gateway)
{
	std::string family;
	std::string method;
	std::map<std::string, std::string> attribute;

	if (!read_interface("/etc/network/interfaces", name, family, method, attribute))
		return false;

	if (family != "inet")
		return false;

	if (method != "static")
		return false;

	address   = "";
	netmask   = "";
	broadcast = "";
	gateway   = "";

	for (std::map<std::string, std::string>::const_iterator it = attribute.begin(); it != attribute.end(); it++)
	{
		if ((*it).first == "address")
			address = (*it).second;
		if ((*it).first == "netmask")
			netmask = (*it).second;
		if ((*it).first == "broadcast")
			broadcast = (*it).second;
		if ((*it).first == "gateway")
			gateway = (*it).second;
	}
	return true;
}

bool setInetAttributes(const std::string name, const std::string address, const std::string netmask, const std::string broadcast, const std::string gateway)
{
	std::map<std::string, std::string> attribute;

	attribute["address"] = address;
	attribute["netmask"] = netmask;

	if (!broadcast.empty())
		attribute["broadcast"] = broadcast;

	if (!gateway.empty())
		attribute["gateway"] = gateway;

	return write_interface("/etc/network/interfaces", name, "inet", "static", attribute);
}
