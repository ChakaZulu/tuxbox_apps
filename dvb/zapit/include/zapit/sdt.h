/*
 * $Id: sdt.h,v 1.4 2002/04/14 06:06:31 obi Exp $
 */

#ifndef __sdt_h__
#define __sdt_h__

#include <stdint.h>
#include <map>
#include "getservices.h"

enum service_type_e {
	RESERVED,
	DIGITAL_TELEVISION_SERVICE,
	DIGITAL_RADIO_SOUND_SERVICE,
	TELETEXT_SERVICE,
	NVOD_REFERENCE_SERVICE,
	NVOD_TIME_SHIFTED_SERVICE,
	MOSAIC_SERVICE,
	PAL_CODED_SIGNAL,
	SECAM_CODED_SIGNAL,
	D_D2_MAC,
	FM_RADIO,
	NTSC_CODED_SIGNAL,
	DATA_BROADCAST_SERVICE,
	COMMON_INTERFACE_RESERVED,
	RCS_MAP,
	RCS_FLS,
	DVB_MHP_SERVICE
};

int parse_sdt (uint16_t osid, bool scan_mode);

#endif /* __sdt_h__ */
