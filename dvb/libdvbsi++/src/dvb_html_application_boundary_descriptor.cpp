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

#include <dvbsi++/dvb_html_application_boundary_descriptor.h>

DvbHtmlApplicationBoundaryDescriptor::DvbHtmlApplicationBoundaryDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	labelLength = buffer[2];
	label.assign((char *)&buffer[3], labelLength);
	regularExpression.assign((char *)&buffer[labelLength + 3], descriptorLength - labelLength - 1);
}

const std::string &DvbHtmlApplicationBoundaryDescriptor::getLabel(void) const
{
	return label;
}

const std::string &DvbHtmlApplicationBoundaryDescriptor::getRegularExpression(void) const
{
	return regularExpression;
}
