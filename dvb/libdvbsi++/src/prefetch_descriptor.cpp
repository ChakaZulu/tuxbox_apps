/*
 * $Id: prefetch_descriptor.cpp,v 1.4 2006/03/28 17:22:00 ghostrider Exp $
 *
 * Copyright (C) 2004-2005 Stéphane Esté-Gracias <sestegra@free.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
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
