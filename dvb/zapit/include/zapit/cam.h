/*
 * $Id: cam.h,v 1.5 2002/04/14 06:06:31 obi Exp $
 */

#ifndef __cam_h__
#define __cam_h__

#ifndef DVBS
#include <ost/ca.h>
#include <ost/dmx.h>
#include <stdint.h>

#include "getservices.h"

#define CAM_DEV "/dev/dbox/cam0"
#define CA_DEV  "/dev/ost/ca0"

int get_caver();
int get_caid();
int _writecamnu (uint8_t cmd, uint8_t *data, uint8_t len);
int writecam (uint8_t *data, uint8_t len);
int descramble (uint32_t tsid_onid, uint16_t unknown, uint16_t ca_system_id, pids *decode_pids);
int cam_reset ();
int setemm (uint16_t unknown, uint16_t ca_system_id, dvb_pid_t emm_pid);

#endif /* DVBS */

#endif /* __cam_h__ */
