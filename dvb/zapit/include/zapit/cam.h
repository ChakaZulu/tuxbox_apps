/*
 * $Id: cam.h,v 1.10 2002/04/28 05:38:51 obi Exp $
 *
 * (C) 2002 by Andreas Oberritter <obi@tuxbox.org>
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

#ifndef __cam_h__
#define __cam_h__

#include <ost/ca.h>
#include <stdint.h>

#include "getservices.h"

class CCam
{
	private:
		bool initialized;
		uint16_t caSystemId;

#ifdef USE_EXTERNAL_CAMD
		uint8_t camdBuffer[2 + 255];
		int camdSocket;

		bool camdConnect ();
		void camdDisconnect ();
#endif

		uint16_t readCaSystemId ();
		ca_msg_t CCam::getMessage (uint16_t length);
		int sendMessage (uint8_t *data, uint16_t length);

	public:
		CCam();
		~CCam();

		uint16_t getCaSystemId() { return caSystemId; }
		bool isInitialized() { return initialized; }

		int reset ();
		int setEcm (CZapitChannel *channel);
		int setEmm (CZapitChannel *channel);
};

#endif /* __cam_h__ */
