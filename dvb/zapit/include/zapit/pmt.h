/*
 * $Id: pmt.h,v 1.2 2002/04/19 14:53:29 obi Exp $
 */

#ifndef __pmt_h__
#define __pmt_h__

#include <ost/dmx.h>

#include "getservices.h"

uint16_t parse_ES_info(uint8_t *buffer, pids *ret_pids, uint16_t ca_system_id);
pids parse_pmt (dvb_pid_t pmt_pid, uint16_t ca_system_id, uint16_t program_number);

#endif /* __pmt_h__ */
