/*
 * $Id: pat.h,v 1.5 2002/04/10 18:36:21 obi Exp $
 */

#ifndef __pat_h__
#define __pat_h__

#include "getservices.h"
#include "scan.h"

int parse_pat (uint16_t original_network_id, std::map <uint, CZapitChannel> *cmap);

#endif /* __pat_h__ */
