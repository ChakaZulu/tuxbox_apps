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

#ifndef __dvb_j_application_location_descriptor_h__
#define __dvb_j_application_location_descriptor_h__

#include "descriptor.h"

class DvbJApplicationLocationDescriptor : public Descriptor
{
	protected:
		uint8_t baseDirectoryLength			: 8;
		std::string baseDirectory;
		uint8_t classpathExtensionLength		: 8;
		std::string classpathExtension;
		std::string initialClass;

	public:
		DvbJApplicationLocationDescriptor(const uint8_t * const buffer);

		const std::string &getBaseDirectory(void) const;
		const std::string &getClasspathExtension(void) const;
		const std::string &getInitialClass(void) const;
};

#endif /* __dvb_j_application_location_descriptor_h__ */

