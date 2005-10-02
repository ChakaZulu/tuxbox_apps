#ifndef __enigma_dyn_boot_h
#define __enigma_dyn_boot_h

class eHTTPDynPathResolver;
void ezapBootManagerInitializeDyn(eHTTPDynPathResolver *dyn_resolver, bool lockWeb);
eString getConfigBoot();
#endif
