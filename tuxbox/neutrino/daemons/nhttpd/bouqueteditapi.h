#ifndef __bouqueteditapi__
#define __bouqueteditapi__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/timeb.h>

#include "request.h"
#include "webdbox.h"


using namespace std;

//-------------------------------------------------------------------------
class CBouqueteditAPI
{
	private:
		CWebDbox * Parent;

	bool showBouquets(CWebserverRequest* request);
	bool addBouquet(CWebserverRequest* request);
	bool moveBouquet(CWebserverRequest* request);
	bool deleteBouquet(CWebserverRequest* request);
	bool saveBouquet(CWebserverRequest* request);
	bool renameBouquet(CWebserverRequest* request);
	bool editBouquet(CWebserverRequest* request);
	bool changeBouquet(CWebserverRequest* request);
	bool setBouquet(CWebserverRequest* request);


	public:
		CBouqueteditAPI(CWebDbox *parent){Parent = parent;};
		~CBouqueteditAPI(){};
		bool Execute(CWebserverRequest* request);
};

#endif
