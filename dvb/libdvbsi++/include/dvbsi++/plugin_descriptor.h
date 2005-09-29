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
 
#ifndef __plugin_descriptor_h__
#define __plugin_descriptor_h__

#include "application_profile.h"
#include "descriptor.h"

class PluginDescriptor : public Descriptor
{
	protected:
		unsigned applicationType			: 16;
		ApplicationProfileList applicationProfiles;

	public:
		PluginDescriptor(const uint8_t * const buffer);
		~PluginDescriptor(void);

		uint16_t getApplicationType(void) const;
		const ApplicationProfileList *getApplicationProfiles(void) const;
};

#endif /* __plugin_descriptor_h__ */
