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
 
#ifndef __application_descriptor_h__
#define __application_descriptor_h__

#include "application_profile.h"
#include "descriptor.h"

typedef std::vector<uint8_t> TransportProtocolLabelVector;
typedef TransportProtocolLabelVector::iterator TransportProtocolLabelIterator;
typedef TransportProtocolLabelVector::const_iterator TransportProtocolLabelConstIterator;

class ApplicationDescriptor : public Descriptor
{
	protected:
		unsigned applicationProfilesLength		:8;
		ApplicationProfileVector applicationProfiles;
		unsigned serviceBoundFlag			:1;
		unsigned visibility				:2;
		unsigned applicationPriority			:8;
		TransportProtocolLabelVector transportProtocolLabels;
	
	public:
		ApplicationDescriptor(const uint8_t * const buffer);
		~ApplicationDescriptor(void);

		const ApplicationProfileVector *getApplicationProfiles(void) const;
		uint8_t getServiceBoundFlag(void) const;
		uint8_t getVisibility(void) const;
		uint8_t getApplicationPriority(void) const;
		const TransportProtocolLabelVector *getTransportProtocolLabels(void) const;
};

#endif /* __application_descriptor_h__ */
