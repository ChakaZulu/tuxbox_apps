/*
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

#include <dvbsi++/application_identifier.h>
#include <dvbsi++/byte_stream.h>
 
ApplicationIdentifier::ApplicationIdentifier(const uint8_t * const buffer)
{
	organisationId = r32(&buffer[0]);
	applicationId = r16(&buffer[4]);
}

uint32_t ApplicationIdentifier::getOrganisationId(void) const
{
	return organisationId;
}

uint16_t ApplicationIdentifier::getApplicationId(void) const
{
	return applicationId;
}
