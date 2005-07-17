#ifndef __enigma_dyn_pda_h
#define __enigma_dyn_pda_h

class eHTTPDynPathResolver;
void ezapPDAInitializeDyn(eHTTPDynPathResolver *dyn_resolver, bool lockWeb);
eString getPDAContent(eString opts);
eString getPDAZapContent(eString path);
eString getSatellites(void);
eString getTransponders(int orbital_position);

#endif /* __enigma_dyn_pda_h */

