#ifndef __enigma_dyn_h
#define __enigma_dyn_h

#define ZAPMODETV 0
#define ZAPMODERADIO 1
#define ZAPMODEDATA 2
#define ZAPMODERECORDINGS 3
#define ZAPMODEROOT 4

#define ZAPSUBMODENAME 0
#define ZAPSUBMODECATEGORY 1
#define ZAPSUBMODESATELLITES 2
#define ZAPSUBMODEPROVIDERS 3
#define ZAPSUBMODEBOUQUETS 4

class eHTTPDynPathResolver;

void ezapInitializeDyn(eHTTPDynPathResolver *dyn_resolver);

#endif /* __enigma_dyn_h */
