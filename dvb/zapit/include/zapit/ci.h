/*
 * $Id: ci.h,v 1.2 2002/07/17 02:16:50 obi Exp $
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

#ifndef __ci_h__
#define __ci_h__

#include <vector>

class CCaDescriptor
{
	public:
		unsigned char	descriptor_tag		: 8;
		unsigned char	descriptor_length	: 8;
		unsigned short	CA_system_ID		: 16;
		unsigned char	reserved1		: 3;
		unsigned short	CA_PID			: 13;

		/* for (i = 0; i < n; i++) */
		std::vector <unsigned char> private_data_byte;
};

/*
 * children of this class need to delete all
 * CCaDescriptors in their destructors
 */
class CCaTable
{
	public:
		/* if (info_length != 0) */
		unsigned char   ca_pmt_cmd_id		: 8;
		std::vector <CCaDescriptor *> ca_descriptor;

		void addCaDescriptor (unsigned char * buffer);
};

class CEsInfo : public CCaTable
{
	public:
		CEsInfo ();
		~CEsInfo ();

		unsigned char	stream_type		: 8;
		unsigned char	reserved1		: 3;
		unsigned short	elementary_PID		: 13;
		unsigned char	reserved2		: 4;
		unsigned short	ES_info_length		: 12;
};

class CCaPmt : public CCaTable
{
	public:
		CCaPmt ();
		~CCaPmt ();
		unsigned int getLength ();

		unsigned int	ca_pmt_tag		: 24;
		std::vector <unsigned char> length_field;
		unsigned char	ca_pmt_list_management	: 8;
		unsigned short	program_number		: 16;
		unsigned char	reserved1		: 2;
		unsigned char	version_number		: 5;
		unsigned char	current_next_indicator	: 1;
		unsigned char	reserved2		: 4;
		unsigned short	program_info_length	: 12;

		/* for (i = 0; i < n; i++) */
		std::vector <CEsInfo *> es_info;
};

#endif /* __ci_h__ */
