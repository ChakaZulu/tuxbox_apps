/*
 *  $Id: scan.h,v 1.30 2007/06/03 14:27:45 dbluelle Exp $
 */

#ifndef __scan_h__
#define __scan_h__

#if HAVE_DVB_API_VERSION < 3
#include <ost/frontend.h>
#else
#include <linux/dvb/frontend.h>
#endif

#include <inttypes.h>

#include <map>
#include <string>

#include "bouquets.h"

#include "getservices.h"

typedef transponder_list_t::iterator stiterator;

extern CBouquetManager* scanBouquetManager;

const char * getFrontendName(void);

#endif /* __scan_h__ */
