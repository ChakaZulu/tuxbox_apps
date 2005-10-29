/*
 * $Id: multilingual_component_descriptor.cpp,v 1.4 2005/10/29 00:10:17 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
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

	for (size_t i = 0; i < descriptorLength - 1; i += buffer[i + 6] + 4)
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

const MultilingualComponentList *MultilingualComponentDescriptor::getMultilingualComponents(void) const
{
	return &multilingualComponents;
}

