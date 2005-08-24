#ifndef __enigma_dyn_h
#define __enigma_dyn_h

#if ENABLE_DYN_MOUNT && ENABLE_DYN_CONF && ENABLE_DYN_FLASH && ENABLE_DYN_ROTOR && ENABLE_DYN_STREAM
#define WEBIFVERSION "5.0.0-Expert"
#else
#define WEBIFVERSION "5.0.0"
#endif

#define ZAPMODETV 0
#define ZAPMODERADIO 1
#define ZAPMODEDATA 2
#define ZAPMODERECORDINGS 3
#define ZAPMODEROOT 4
#define ZAPMODESTREAMING 5

#define ZAPSUBMODENAME 0
#define ZAPSUBMODECATEGORY 1
#define ZAPSUBMODESATELLITES 2
#define ZAPSUBMODEPROVIDERS 3
#define ZAPSUBMODEBOUQUETS 4
#define ZAPSUBMODEALLSERVICES 5

class eHTTPDynPathResolver;

void ezapInitializeDyn(eHTTPDynPathResolver *dyn_resolver);

#endif /* __enigma_dyn_h */
