/*
 * $Id: pat.cpp,v 1.2 2003/08/20 22:47:35 obi Exp $
 *
 * Copyright (C) 2002, 2003 Andreas Oberritter <obi@saftware.de>
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

#include <dvb/byte_stream.h>
#include <dvb/table/pat.h>

NetworkAssociation::NetworkAssociation(const uint8_t * const buffer)
{
	programNumber = UINT16(&buffer[0]);
	networkPid = DVB_PID(&buffer[2]);
}

uint16_t NetworkAssociation::getNetworkPid(void) const
{
	return networkPid;
}

ProgramAssociation::ProgramAssociation(const uint8_t * const buffer)
{
	programNumber = UINT16(&buffer[0]);
	programMapPid = DVB_PID(&buffer[2]);
}

uint16_t ProgramAssociation::getProgramNumber(void) const
{
	return programNumber;
}

uint16_t ProgramAssociation::getProgramMapPid(void) const
{
	return programMapPid;
}

ProgramAssociationTable::ProgramAssociationTable(const uint8_t * const buffer) : LongCrcTable(buffer)
{
	for (uint16_t i = 8; i < sectionLength - 1; i += 4) {
		if (UINT16(&buffer[i]) == 0)
			networks.push_back(new NetworkAssociation(&buffer[i]));
		else
			programs.push_back(new ProgramAssociation(&buffer[i]));
	}
}

ProgramAssociationTable::~ProgramAssociationTable(void)
{
	for (NetworkAssociationIterator i = networks.begin(); i != networks.end(); ++i)
		delete *i;

	for (ProgramAssociationIterator i = programs.begin(); i != programs.end(); ++i)
		delete *i;
}

const NetworkAssociationVector *ProgramAssociationTable::getNetworks(void) const
{
	return &networks;
}

const ProgramAssociationVector *ProgramAssociationTable::getPrograms(void) const
{
	return &programs;
}

