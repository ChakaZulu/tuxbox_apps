#ifdef ENABLE_DYN_MOUNT

#ifndef __enigma_dyn_mount_h
#define __enigma_dyn_mount_h

class eHTTPDynPathResolver;
void ezapMountInitializeDyn(eHTTPDynPathResolver *dyn_resolver, bool lockWeb);

eString getConfigMountMgr(void);

#endif /* __enigma_dyn_mount_h */

#endif
