/*
 * $Id: cam.h,v 1.27 2009/03/21 13:01:37 rhabarber1848 Exp $
 *
 * (C) 2002-2003 Andreas Oberritter <obi@tuxbox.org>,
 *               thegoodguy         <thegoodguy@berlios.de>
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

#ifndef __zapit_cam_h__
#define __zapit_cam_h__

#include "ci.h"
#include <basicclient.h>

class CCam : public CBasicClient
{
	private:
		virtual unsigned char getVersion(void) const;
		virtual const char *getSocketName(void) const;
		bool sendMessage(const char * const data, const size_t length);

	public:
		bool setCaPmt(CCaPmt * const caPmt);
		virtual ~CCam(void);
};

#endif /* __cam_h__ */
