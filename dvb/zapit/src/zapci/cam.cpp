/*
 * $Id: cam.cpp,v 1.23 2002/09/25 14:53:58 thegoodguy Exp $
 *
 * (C) 2002 by Andreas Oberritter <obi@tuxbox.org>,
 *             thegoodguy         <thegoodguy@berlios.de>
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

#include "cam.h"

/* zapit */
#include <settings.h>   // CAMD_UDS_NAME

bool CCam::sendMessage(char* data, const unsigned short length)
{
	if (!open_connection(CAMD_UDS_NAME))
		return false;

	if (send_data(data, length))
		return true;
	else
		return false;
}

bool CCam::setCaPmt(CCaPmt * caPmt)
{
	if (caPmt == NULL)
		return true;

	char buffer[caPmt->getLength()];

	unsigned int pos = caPmt->writeToBuffer((unsigned char*)buffer);

	return sendMessage(buffer, pos);
}
