/*
 * $Header: /cvs/tuxbox/apps/dvb/zapit/include/zapit/Attic/xmlinterface.h,v 1.4 2002/09/19 10:44:57 thegoodguy Exp $
 *
 * xmlinterface for zapit - d-box2 linux project
 *
 * (C) 2002 by thegoodguy <thegoodguy@berlios.de>
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

#include "xml/xmltree.h"


std::string convertForXML(const std::string s);
std::string Utf8_to_Latin1(const std::string s);

XMLTreeParser* parseXmlFile(const std::string filename);
