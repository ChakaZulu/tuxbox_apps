/*
 * $Id: ci.cpp,v 1.6 2002/08/27 20:01:03 obi Exp $
 *
 * (C) 2002 by Andreas Oberritter <obi@tuxbox.org>
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

#include "ci.h"

/*
 * conditional access descriptors
 */
CCaDescriptor::CCaDescriptor (unsigned char * buffer)
{
	descriptor_tag = buffer[0];
	descriptor_length = buffer[1];
	CA_system_ID = *(unsigned short*)(&(buffer[2]));
	reserved1 = buffer[4] >> 5;
	CA_PID = ((buffer[4] & 0x1F) << 8) | buffer[5];

	private_data_byte = std::vector<unsigned char>(&(buffer[6]), &(buffer[descriptor_length + 2]));
}

unsigned int CCaDescriptor::writeToBuffer (unsigned char * buffer) // returns number of bytes written
{
	buffer[0] = descriptor_tag;
	buffer[1] = descriptor_length;
	*(unsigned short*)(&(buffer[2])) = CA_system_ID;
	buffer[4] = (reserved1 << 5) | (CA_PID >> 8);
	buffer[5] = CA_PID;

	std::copy(private_data_byte.begin(), private_data_byte.end(), &(buffer[6]));

	return descriptor_length + 2;
}


/*
 * generic table containing conditional access descriptors
 */

void CCaTable::addCaDescriptor (unsigned char * buffer)
{
	ca_descriptor.push_back(new CCaDescriptor(buffer));
}

/*
 * elementary stream information
 */

CEsInfo::CEsInfo ()
{
	ES_info_length = 0;
}

CEsInfo::~CEsInfo ()
{
	unsigned char i;

	for (i = 0; i < ca_descriptor.size(); i++)
	{
		delete ca_descriptor[i];
	}
}

/*
 * contitional access program map table
 */

CCaPmt::CCaPmt ()
{
	ca_pmt_tag = 0x9F8032;
	program_info_length = 0;
}

CCaPmt::~CCaPmt ()
{
	unsigned char i;

	for (i = 0; i < ca_descriptor.size(); i++)
	{
		delete ca_descriptor[i];
	}

	for (i = 0; i < es_info.size(); i++)
	{
		delete es_info[i];
	}
}

unsigned int CCaPmt::getLength ()
{
	unsigned char size_indicator = (length_field[0] >> 7) & 0x01;
	unsigned int length_value = 0;
	unsigned char length_field_size = 0;

	if (size_indicator == 0)
	{
		length_field_size = 1;
		length_value = length_field[0] & 0x7F;
	}

	else if (size_indicator == 1)
	{
		unsigned int i;

		length_field_size = length_field[0] & 0x7F;

		for (i = 0; i < length_field_size; i++)
		{
			length_value = (length_value << 8) | length_field[i + 1];
		}
	}

	return 3 + length_field_size + length_value;
}

