/*
 *  $Id: image_icon_descriptor.h,v 1.1 2009/06/30 12:03:02 mws Exp $
 *
 *  Copyright (C) 2009 Marcel Siegert <mws@twisted-brains.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */
#ifndef __image_icon_descriptor_h__
#define __image_icon_descriptor_h__

#include "descriptor.h"

typedef std::vector<uint8_t> SelectorByteVector;
typedef SelectorByteVector::iterator SelectorByteByteIterator;
typedef SelectorByteVector::const_iterator SelectorByteConstIterator;

class ImageIconDescriptor : public Descriptor
{
	protected:
		unsigned extensionTag		: 8;
		unsigned descriptorNumber	: 4;
		unsigned lastDescriptorNumber	: 4;
		unsigned iconId			: 3;
		unsigned iconTransportMode	: 2;
		unsigned positionFlag		: 1;
		unsigned coordinateSystem	: 3;
		unsigned iconHorizontalOrigin	:12;
		unsigned iconVerticalOrigin	:12;
		unsigned iconTypeLength		: 8;
		unsigned iconDataLength		: 8;


		SelectorByteVector iconTypeChars;

		SelectorByteVector iconDataBytes;

	public:
		ImageIconDescriptor(const uint8_t* const buffer);
		virtual ~ImageIconDescriptor();

		uint8_t getExtensionTag() const;
		uint8_t getDescriptorNumber() const;
		uint8_t getLastDescriptorNumber() const;
		uint8_t getIconId() const;
		uint8_t getIconTransportMode() const;
		uint8_t getPositionFlag() const;
		uint8_t getCoordinateSystem() const;
		uint16_t getIconHorizontalOrigin() const;
		uint16_t getIconVerticalOrigin() const;

		const SelectorByteVector* getIconTypeChars() const;
		const SelectorByteVector* getIconDataBytes() const;
		const SelectorByteVector* getUrlsChars() const;

};

#endif /* __image_icon_descriptor_h__*/
