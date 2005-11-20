#ifndef SIBOUQUETS_HPP
#define SIBOUQUETS_HPP
//
// $Id: SIbouquets.hpp,v 1.1 2005/11/20 15:11:40 mogway Exp $
//
// classes SIservices and SIservices (dbox-II-project)
//
//    Homepage: http://dbox2.elxsi.de
//
//    Copyright (C) 2001 fnbrd (fnbrd@gmx.de),
//                  2002 thegoodguy (thegoodguy@berlios.de)
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//


#include <algorithm>
#include <endian.h>

#include <sectionsdclient/sectionsdMsg.h>

// forward references
class SIservice;
class SIevent;
class SIbouquet;

struct loop_len {
#if __BYTE_ORDER == __BIG_ENDIAN
	unsigned reserved_future_use		: 4;
	unsigned descriptors_loop_length_hi	: 4;
#else
	unsigned descriptors_loop_length_hi	: 4;
	unsigned reserved_future_use		: 4;
#endif
	unsigned descriptors_loop_length_lo	: 8;
} __attribute__ ((packed)) ; // 2 Bytes

struct bat_service {

	unsigned transport_stream_id_hi		: 8;
	unsigned transport_stream_id_lo		: 8;
	unsigned original_network_id_hi		: 8;
	unsigned original_network_id_lo		: 8;
#if __BYTE_ORDER == __BIG_ENDIAN
	unsigned reserved_future_use		: 4;
	unsigned descriptors_loop_length_hi	: 4;
#else
	unsigned descriptors_loop_length_hi	: 4;
	unsigned reserved_future_use		: 4;
#endif
	unsigned descriptors_loop_length_lo	: 8;
	
} __attribute__ ((packed)) ; // 6 Bytes



class SIbouquet {
public:
	SIbouquet(const t_bouquet_id _bouquet_id) {
	
		bouquet_id		= _bouquet_id;
		transport_stream_id	= 0;
		original_network_id	= 0;
		service_id		= 0;
		serviceTyp		= 0;
		bouquetName		= "";
	}
	
	// Um einen service zum Suchen zu erstellen
	SIbouquet(const t_bouquet_id _bouquet_id, const t_service_id _service_id, const t_original_network_id _original_network_id, const t_transport_stream_id _transport_stream_id)
	{
		service_id		= _service_id;
		original_network_id	= _original_network_id;
		transport_stream_id	= _transport_stream_id;
		serviceTyp		= 0;
	}
	// Std-Copy
	SIbouquet(const SIbouquet &s) {
	
		service_id		= s.service_id;
		original_network_id	= s.original_network_id;
		transport_stream_id 	= s.transport_stream_id;
		bouquet_id 		= s.bouquet_id;
		bouquetName		= s.bouquetName;
		serviceTyp		= s.serviceTyp;
	}
	
	t_bouquet_id bouquet_id; // This is because of wrong design of sectionsd. Normally we would parse only tables instead of sections...
	std::string bouquetName; // This is because of wrong design of sectionsd. Normally we would parse only tables instead of sections...
	t_original_network_id original_network_id;
	t_transport_stream_id transport_stream_id;
	t_service_id          service_id;
	unsigned char serviceTyp;
	bool operator < (const SIbouquet& s) const {
		return uniqueKey() < s.uniqueKey();
	}

	t_channel_id uniqueKey(void) const {
		return CREATE_CHANNEL_ID;
	}

	void dump(void) const {
	
		printf("Bouquet-ID: %hu\n", bouquet_id);
		printf("Original-Network-ID: %hu\n", original_network_id);
		printf("Transport-Stream-ID: %hu\n", transport_stream_id);
		printf("Service-ID: %hu\n", service_id);
		printf("Service-Typ: %hhu\n", serviceTyp);
		if(bouquetName.length())
			printf("Bouquet-Name: %s\n", bouquetName.c_str());
	}
};

// Fuer for_each
struct printSIbouquet : public std::unary_function<SIbouquet, void>
{
	void operator() (const SIbouquet &s) { s.dump();}
};

// Als Klasse, da ich nicht weiss, wie man eine Forward-Referenz auf ein typedef macht
class SIbouquets : public std::set <SIbouquet, std::less<SIbouquet> >
{
};

#endif // SIBOUQUETS_HPP
