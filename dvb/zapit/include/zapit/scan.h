/*
 *  $Id: scan.h,v 1.29 2005/01/24 19:19:20 thegoodguy Exp $
 */

#ifndef __scan_h__
#define __scan_h__

#include <linux/dvb/frontend.h>

#include <inttypes.h>

#include <map>
#include <string>

#include "bouquets.h"

#include "getservices.h"

typedef transponder_list_t::iterator stiterator;

extern CBouquetManager* scanBouquetManager;

const char * getFrontendName(void);

#endif /* __scan_h__ */
