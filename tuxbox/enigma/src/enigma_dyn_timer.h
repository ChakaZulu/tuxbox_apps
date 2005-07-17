#ifndef __enigma_dyn_timer_h
#define __enigma_dyn_timer_h

class eHTTPDynPathResolver;
void ezapTimerInitializeDyn(eHTTPDynPathResolver *dyn_resolver, bool lockWeb);
eString getTimerList(eString format);

#endif /* __enigma_dyn_timer_h */

