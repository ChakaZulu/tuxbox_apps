/*
 * $Header: /cvs/tuxbox/apps/dvb/zapit/include/zapit/types.h,v 1.7 2005/01/21 21:50:29 thegoodguy Exp $
 *
 * zapit's types - d-box2 linux project
 * these types are used by the clientlib and zapit itself
 *
 * (C) 2002 by thegoodguy <thegoodguy@berlios.de>
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

#ifndef __zapit__types_h__
#define __zapit__types_h__


#include "client/zapittypes.h"

typedef uint64_t transponder_id_t;
#define PRINTF_TRANSPONDER_ID_TYPE "%12llx"
#define TRANSPONDER_ID_NOT_TUNED 0
#define CREATE_TRANSPONDER_ID_FROM_SATELLITEPOSITION_ORIGINALNETWORK_TRANSPORTSTREAM_ID(satellite_position,original_network_id,transport_stream_id) (((transponder_id_t)satellite_position << 32) | ((transponder_id_t)transport_stream_id << 16) | (transponder_id_t)original_network_id)
#define GET_ORIGINAL_NETWORK_ID_FROM_TRANSPONDER_ID(transponder_id) ((t_original_network_id)(transponder_id      ))
#define GET_TRANSPORT_STREAM_ID_FROM_TRANSPONDER_ID(transponder_id) ((t_transport_stream_id)(transponder_id >> 16))
#define GET_SATELLITEPOSITION_FROM_TRANSPONDER_ID(transponder_id)   ((t_satellite_position )(transponder_id >> 32))

#endif /* __zapit__types_h__ */
