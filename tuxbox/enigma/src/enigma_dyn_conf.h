#ifdef ENABLE_DYN_CONF
#ifndef __enigma_dyn_conf_h
#define __enigma_dyn_conf_h

class eHTTPDynPathResolver;
#ifndef DISABLE_FILE
void ezapConfInitializeDyn(eHTTPDynPathResolver *dyn_resolver, bool lockWeb);
#endif

#endif /* __enigma_dyn_conf_h */
#endif

