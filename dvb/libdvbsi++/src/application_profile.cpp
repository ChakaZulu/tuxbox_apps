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

#include <dvbsi++/application_profile.h>
#include <dvbsi++/byte_stream.h>
 
ApplicationProfile::ApplicationProfile(const uint8_t * const buffer)
{
	applicationProfile = r16(&buffer[0]);
	versionMajor = buffer[2];
	versionMinor = buffer[3];
	versionMicro = buffer[4];
}

uint16_t ApplicationProfile::getApplicationProfile(void) const
{
	return applicationProfile;
}

uint8_t ApplicationProfile::getVersionMajor(void) const
{
	return versionMajor;
}

uint8_t ApplicationProfile::getVersionMinor(void) const
{
	return versionMinor;
}

uint8_t ApplicationProfile::getVersionMicro(void) const
{
	return versionMicro;
}
