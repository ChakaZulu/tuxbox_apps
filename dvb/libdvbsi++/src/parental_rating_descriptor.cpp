/*
 * $Id: parental_rating_descriptor.cpp,v 1.2 2005/09/29 23:49:44 ghostrider Exp $
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
	for (size_t i = 0; i < descriptorLength; i += 4)
		parentalRatings.push_back(new ParentalRating(&buffer[i + 2]));
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

