/*
 * $Id: cam.h,v 1.6 2002/04/17 08:03:07 obi Exp $
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

#include <fcntl.h>
#include <ost/ca.h>
#include <ost/dmx.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>


#include "getservices.h"

#define CAM_DEV "/dev/dbox/cam0"
#define CA_DEV  "/dev/ost/ca0"

class CCam
{
	private:
		bool initialized;
		uint16_t caSystemId;
		int ca_fd;

		uint16_t readCaSystemId ();
		ca_msg_t CCam::getMessage (uint16_t length);
		int sendMessage (uint8_t *data, uint16_t length);

	public:
		CCam();
		~CCam();

		uint16_t getCaSystemId() { return caSystemId; }
		bool isInitialized() { return initialized; }

		int reset ();
		int setEcm (uint32_t tsidOnid, pids *decodePids);
		int setEmm (dvb_pid_t emmPid);
};

#endif /* __cam_h__ */
