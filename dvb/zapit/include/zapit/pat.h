/*
 * $Id: pat.h,v 1.3 2002/04/04 19:36:49 obi Exp $
 */

#ifndef __pat_h__
#define __pat_h__

#include "getservices.h"
#include "scan.h"

int pat(uint oonid,std::map<uint, channel> *cmap);
int fake_pat(std::map<int,transpondermap> *tmap, FrontendParameters feparams);

#endif /* __pat_h__ */
