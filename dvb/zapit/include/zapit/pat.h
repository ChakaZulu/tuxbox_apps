#ifndef __pat_h__
#define __pat_h__

#include "getservices.h"
#include "scan.h"

int pat(uint oonid,std::map<uint, channel> *cmap);
int fake_pat(std::map<int,transpondermap> *tmap, int freq, int sr);

#endif /* __pat_h__ */
