/*
 * $Id: pmt.h,v 1.1 2002/04/04 19:36:49 obi Exp $
 */

#ifndef __pmt_h__
#define __pmt_h__

#include <fcntl.h>
#include <ost/dmx.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "descriptors.h"
#include "getservices.h"

#define DEMUX_DEV	"/dev/ost/demux0"
#define PMT_SIZE	1024

uint16_t parse_ES_info(uint8_t *buffer, pids *ret_pids, uint16_t ca_system_id);
pids parse_pmt (dvb_pid_t pmt_pid, uint16_t ca_system_id, uint16_t program_number);

#endif /* __pmt_h__ */
