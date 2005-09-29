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

#include <dvbsi++/prefetch_descriptor.h>

PrefetchLabel::PrefetchLabel(const uint8_t * const buffer)
{
	labelLength = buffer[0];
	label.assign((char *)&buffer[1], labelLength);
	prefetchPriority = buffer[labelLength + 1];
}

uint8_t PrefetchLabel::getLabelLength(void) const
{
	return labelLength;
}

const std::string &PrefetchLabel::getLabel(void) const
{
	return label;
}

uint8_t PrefetchLabel::getPrefetchPriority(void) const
{
	return prefetchPriority;
}


PrefetchDescriptor::PrefetchDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	transportProtocolLabel = buffer[2];
	for (size_t i = 0; i < descriptorLength - 1; ) {
		PrefetchLabel *prefetchLabel = new PrefetchLabel(&buffer[i + 3]);
		prefetchLabels.push_back(prefetchLabel);
		i += prefetchLabel->getLabelLength() + 1;
	}
}

PrefetchDescriptor::~PrefetchDescriptor(void)
{
	for (PrefetchLabelIterator i = prefetchLabels.begin(); i != prefetchLabels.end(); ++i)
		delete *i;	
}

uint8_t PrefetchDescriptor::getTransportProtocolLabel(void) const
{
	return transportProtocolLabel;
}

const PrefetchLabelList *PrefetchDescriptor::getPrefetchLabels(void) const
{
	return &prefetchLabels;
}
