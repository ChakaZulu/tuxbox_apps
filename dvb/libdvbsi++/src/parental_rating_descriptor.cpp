/*
 * $Id: parental_rating_descriptor.cpp,v 1.4 2006/03/28 17:22:00 ghostrider Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/parental_rating_descriptor.h>

ParentalRating::ParentalRating(const uint8_t * const buffer)
{
	countryCode.assign((char *)&buffer[0], 3);
	rating = buffer[3];
}

const std::string &ParentalRating::getCountryCode(void) const
{
	return countryCode;
}

uint8_t ParentalRating::getRating(void) const
{
	return rating;
}

ParentalRatingDescriptor::ParentalRatingDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	for (size_t i = 0; i < descriptorLength; i += 4) {
		ASSERT_MIN_DLEN(i + 4);
		parentalRatings.push_back(new ParentalRating(&buffer[i + 2]));
	}
}

ParentalRatingDescriptor::~ParentalRatingDescriptor(void)
{
	for (ParentalRatingIterator i = parentalRatings.begin(); i != parentalRatings.end(); ++i)
		delete *i;
}

const ParentalRatingList *ParentalRatingDescriptor::getParentalRatings(void) const
{
	return &parentalRatings;
}

