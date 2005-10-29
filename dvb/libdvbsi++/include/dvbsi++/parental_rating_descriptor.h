/*
 * $Id: parental_rating_descriptor.h,v 1.3 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __parental_rating_descriptor_h__
#define __parental_rating_descriptor_h__

#include "descriptor.h"

class ParentalRating
{
	protected:
		std::string countryCode;
		unsigned rating					: 8;

	public:
		ParentalRating(const uint8_t * const buffer);

		const std::string &getCountryCode(void) const;
		uint8_t getRating(void) const;
};

typedef std::list<ParentalRating *> ParentalRatingList;
typedef ParentalRatingList::iterator ParentalRatingIterator;
typedef ParentalRatingList::const_iterator ParentalRatingConstIterator;

class ParentalRatingDescriptor : public Descriptor
{
	protected:
		ParentalRatingList parentalRatings;

	public:
		ParentalRatingDescriptor(const uint8_t * const buffer);
		~ParentalRatingDescriptor(void);

		const ParentalRatingList *getParentalRatings(void) const;
};

#endif /* __parental_rating_descriptor_h__ */
