#ifdef ENABLE_DYN_FLASH
#ifndef __enigma_dyn_flash_h
#define __enigma_dyn_flash_h

class eHTTPDynPathResolver;
void ezapFlashInitializeDyn(eHTTPDynPathResolver *dyn_resolver, bool lockWeb);
eString getConfigFlashMgr(void);

#endif /* __enigma_dyn_flash_h */
#endif

