/*
 * $Id: cam.cpp,v 1.36 2009/11/10 11:36:53 rhabarber1848 Exp $
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
#include <zapit/settings.h> /* CAMD_UDS_NAME         */
#include <messagetools.h>   /* get_length_field_size */
#include <stdio.h>
#include <string.h>

CCam::~CCam(void)
{
	close_connection();
}

unsigned char CCam::getVersion(void) const
{
	return 0x9F;
}

const char *CCam::getSocketName(void) const
{
	return CAMD_UDS_NAME;
}

bool CCam::sendMessage(const char * const data, const size_t length)
{
	close_connection();

	if (!open_connection())
		return false;

	return send_data(data, length);
}

bool CCam::setCaPmt(CCaPmt * const caPmt)
{
	if (!caPmt)
		return true;

	unsigned int size = caPmt->getLength();
	unsigned char buffer[3 + get_length_field_size(size) + size];
	size_t pos = caPmt->writeToBuffer(buffer);

	return sendMessage((char *)buffer, pos);
}

