/*
 * $Id: cam.cpp,v 1.28 2002/12/07 13:37:59 thegoodguy Exp $
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

/* zapit */
#include <zapit/cam.h>
#include <zapit/settings.h>           /* CAMD_UDS_NAME         */
#include <connection/messagetools.h>  /* get_length_field_size */


bool CCam::sendMessage(char* data, const size_t length)
{
	if (!open_connection(CAMD_UDS_NAME))
		return false;

	bool return_value = send_data(data, length);

	close_connection();

	return return_value;
}

bool CCam::setCaPmt(CCaPmt * caPmt)
{
	if (caPmt == NULL)
		return true;

	unsigned int size = caPmt->getLength();

	char buffer[3 + get_length_field_size(size) + size];

	size_t pos = caPmt->writeToBuffer((unsigned char*)buffer);

	return sendMessage(buffer, pos);
}
