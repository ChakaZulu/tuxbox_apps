/*
 * $Id: pat.h,v 1.6 2002/04/19 14:53:29 obi Exp $
 */

#ifndef __pat_h__
#define __pat_h__

#include "getservices.h"

int parse_pat (uint16_t original_network_id, std::map <uint, CZapitChannel> *cmap);

#endif /* __pat_h__ */
