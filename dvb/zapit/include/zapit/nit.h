/*
 * $Id: nit.h,v 1.4 2002/04/19 14:53:29 obi Exp $
 */

#ifndef __nit_h__
#define __nit_h__

#include <fcntl.h>
#include <ost/dmx.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "descriptors.h"
#include "getservices.h"

#define DEMUX_DEV "/dev/ost/demux0"

#include <stdint.h>

int parse_nit (uint8_t DiSEqC);

#endif /* __nit_h__ */
