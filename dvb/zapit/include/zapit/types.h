/*
 * $Header: /cvs/tuxbox/apps/dvb/zapit/include/zapit/types.h,v 1.1 2002/09/20 16:55:22 thegoodguy Exp $
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


#include <stdint.h>


/* unique channel identification */
typedef uint32_t t_channel_id;
#define CREATE_CHANNEL_ID ((original_network_id << 16) | service_id)


/* diseqc types */
enum diseqc_t
{
	NO_DISEQC,
	MINI_DISEQC,
	DISEQC_1_0,
	DISEQC_1_1,
	SMATV_REMOTE_TUNING
};


#endif /* __zapit__types_h__ */
