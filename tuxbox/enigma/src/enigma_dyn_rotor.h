#ifdef ENABLE_DYN_ROTOR
#ifndef __enigma_dyn_rotor_h
#define __enigma_dyn_rotor_h

class eHTTPDynPathResolver;
void ezapRotorInitializeDyn(eHTTPDynPathResolver *dyn_resolver, bool lockWeb);
eString getRotorConfig(void);

#endif /* __enigma_dyn_rotor_h */
#endif

