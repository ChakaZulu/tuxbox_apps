#ifndef __enigma_dyn_epg_h
#define __enigma_dyn_epg_h

class eHTTPDynPathResolver;
void ezapEPGInitializeDyn(eHTTPDynPathResolver *dyn_resolver, bool lockWeb);
eString getServiceEPG(eString format, eString opts);
eString getEITC(eString result);

#endif /* __enigma_dyn_epg_h */

