/*
 * $Id: multilingual_component_descriptor.cpp,v 1.1 2004/02/13 15:27:47 obi Exp $
 *
 * Copyright (C) 2002-2004 Andreas Oberritter <obi@saftware.de>
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

#include <dvbsi++/multilingual_component_descriptor.h>

MultilingualComponent::MultilingualComponent(const uint8_t * const buffer)
{
	iso639LanguageCode.assign((char *)&buffer[0], 3);
	textDescriptionLength = buffer[3];
	text.assign((char *)&buffer[4], textDescriptionLength);
}

const std::string &MultilingualComponent::getIso639LanguageCode(void) const
{
	return iso639LanguageCode;
}

const std::string &MultilingualComponent::getText(void) const
{
	return text;
}

MultilingualComponentDescriptor::MultilingualComponentDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	componentTag = buffer[2];

	for (size_t i = 0; i < descriptorLength - 1; i += buffer[i + 4] + 2)
		multilingualComponents.push_back(new MultilingualComponent(&buffer[i + 3]));
}

MultilingualComponentDescriptor::~MultilingualComponentDescriptor(void)
{
	for (MultilingualComponentIterator i = multilingualComponents.begin(); i != multilingualComponents.end(); ++i)
		delete *i;
}

uint8_t MultilingualComponentDescriptor::getComponentTag(void) const
{
	return componentTag;
}

const MultilingualComponentVector *MultilingualComponentDescriptor::getMultilingualComponents(void) const
{
	return &multilingualComponents;
}

