#ifdef ENABLE_DYN_ROTOR
#ifndef __enigma_dyn_rotor_h
#define __enigma_dyn_rotor_h

class eHTTPDynPathResolver;
void ezapRotorInitializeDyn(eHTTPDynPathResolver *dyn_resolver, bool lockWeb);
eString getConfigRotor(void);

#endif /* __enigma_dyn_rotor_h */
#endif

