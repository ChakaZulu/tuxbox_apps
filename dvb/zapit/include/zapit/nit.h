/*
 * $Id: nit.h,v 1.2 2002/04/06 11:26:11 obi Exp $
 */

#ifndef __nit_h__
#define __nit_h__

#include <fcntl.h>
#include <ost/dmx.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "descriptors.h"
#include "getservices.h"

#define DEMUX_DEV "/dev/ost/demux0"

int nit (uint8_t DiSEqC);

#endif /* __nit_h__ */
