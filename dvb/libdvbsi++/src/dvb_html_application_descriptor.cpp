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

#include <dvbsi++/dvb_html_application_descriptor.h>
#include <dvbsi++/byte_stream.h>

DvbHtmlApplicationDescriptor::DvbHtmlApplicationDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	appidSetLength = buffer[2];
	for (size_t i = 0; i < appidSetLength; i += 2)
		applicationIds.push_back(r16(&buffer[i + 3]));
	parameter.assign((char *)&buffer[appidSetLength + 3], descriptorLength - appidSetLength - 1);
}

const ApplicationIdVector *DvbHtmlApplicationDescriptor::getApplicationIds(void) const
{
	return &applicationIds;
}

const std::string &DvbHtmlApplicationDescriptor::getParameter(void) const
{
	return parameter;
}
