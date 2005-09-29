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

#include <dvbsi++/dvb_j_application_descriptor.h>

DvbJApplication::DvbJApplication(const uint8_t * const buffer)
{
	parameterLength = buffer[0];
	parameter.assign((char *)&buffer[1], parameterLength);
}

uint8_t DvbJApplication::getParameterLength(void) const
{
	return parameterLength;
}

const std::string &DvbJApplication::getParameter(void) const
{
	return parameter;
}

DvbJApplicationDescriptor::DvbJApplicationDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	for (size_t i = 0; i < descriptorLength; ) {
		DvbJApplication *dvbJApplication = new DvbJApplication(&buffer[i + 2]);
		dvbJApplications.push_back(dvbJApplication);
		i += dvbJApplication->getParameterLength() + 1;
	}
}

DvbJApplicationDescriptor::~DvbJApplicationDescriptor	(void)
{
	for (DvbJApplicationIterator i = dvbJApplications.begin(); i != dvbJApplications.end(); ++i)
		delete *i;
}

const DvbJApplicationList *DvbJApplicationDescriptor::getDvbJApplications(void) const
{
	return &dvbJApplications;
}
