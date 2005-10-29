/*
 * $Id: subtitling_descriptor.h,v 1.3 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __subtitling_descriptor_h__
#define __subtitling_descriptor_h__

#include "descriptor.h"

class Subtitling
{
	protected:
		std::string iso639LanguageCode;
		unsigned subtitlingType				: 8;
		unsigned compositionPageId			: 16;
		unsigned ancillaryPageId			: 16;

	public:
		Subtitling(const uint8_t * const buffer);

		const std::string &getIso639LanguageCode(void) const;
		uint8_t getSubtitlingType(void) const;
		uint16_t getCompositionPageId(void) const;
		uint16_t getAncillaryPageId(void) const;
};

typedef std::list<Subtitling *> SubtitlingList;
typedef SubtitlingList::iterator SubtitlingIterator;
typedef SubtitlingList::const_iterator SubtitlingConstIterator;

class SubtitlingDescriptor : public Descriptor
{
	protected:
		SubtitlingList subtitlings;

	public:
		SubtitlingDescriptor(const uint8_t * const buffer);
		~SubtitlingDescriptor(void);

		const SubtitlingList *getSubtitlings(void) const;
};

#endif /* __subtitling_descriptor_h__ */
