/*
 * $Id: pmt.h,v 1.3 2002/04/24 18:51:18 field Exp $
 */

#ifndef __pmt_h__
#define __pmt_h__

#include <ost/dmx.h>

#include "getservices.h"

uint16_t parse_ES_info(uint8_t *buffer, pids *ret_pids, uint16_t ca_system_id);
int init_parse_pmt(dvb_pid_t pmt_pid, uint16_t program_number);
pids parse_pmt (dvb_pid_t pmt_pid, uint16_t ca_system_id, int fd);

#endif /* __pmt_h__ */
