/*
 * $Id: video_stream_descriptor.h,v 1.2 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __video_stream_descriptor_h__
#define __video_stream_descriptor_h__

#include "descriptor.h"

class VideoStreamDescriptor : public Descriptor
{
	protected:
		unsigned multipleFrameRateFlag			: 1;
		unsigned frameRateCode				: 4;
		unsigned mpeg1OnlyFlag				: 1;
		unsigned constrainedParameterFlag		: 1;
		unsigned stillPictureFlag			: 1;
		unsigned profileAndLevelIndication		: 8;
		unsigned chromaFormat				: 2;
		unsigned frameRateExtensionFlag			: 1;

	public:
		VideoStreamDescriptor(const uint8_t * const buffer);

		uint8_t getMultipleFrameRateFlag(void) const;
		uint8_t getFrameRateCode(void) const;
		uint8_t getMpeg1OnlyFlag(void) const;
		uint8_t getConstrainedParameterFlag(void) const;
		uint8_t getStillPictureFlag(void) const;
		uint8_t getProfileAndLevelIndication(void) const;
		uint8_t getChromaFormat(void) const;
		uint8_t getFrameRateExtensionFlag(void) const;
};

#endif /* __video_stream_descriptor_h__ */
