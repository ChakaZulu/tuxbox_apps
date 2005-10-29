/*
 * $Id: audio_stream_descriptor.h,v 1.2 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __audio_stream_descriptor_h__
#define __audio_stream_descriptor_h__

#include "descriptor.h"

class AudioStreamDescriptor : public Descriptor
{
	protected:
		unsigned freeFormatFlag				: 1;
		unsigned id					: 1;
		unsigned layer					: 2;
		unsigned variableRateAudioIndicator		: 1;

	public:
		AudioStreamDescriptor(const uint8_t * const buffer);

		uint8_t getFreeFormatFlag(void) const;
		uint8_t getId(void) const;
		uint8_t getLayer(void) const;
		uint8_t getVariableRateAudioIndicator(void) const;
};

#endif /* __audio_stream_descriptor_h__ */
