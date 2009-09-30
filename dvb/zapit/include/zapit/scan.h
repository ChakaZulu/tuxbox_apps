/*
 *  $Id: scan.h,v 1.31 2009/09/30 17:12:39 seife Exp $
 */

#ifndef __scan_h__
#define __scan_h__

#ifdef HAVE_TRIPLEDRAGON
#include "td-frontend-compat.h"
#elif HAVE_DVB_API_VERSION < 3
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
