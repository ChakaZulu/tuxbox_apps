#ifndef __webserver_helper__
#define __webserver_helper__


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//----------------------------------------------------------------------

class TString
{
	int laenge;
	char * text;
	void Set(char * t, int len);

public:

	~TString()		{if((laenge > 0) && (text != NULL)) free(text); text = NULL;}
	char *c_str()	{if(text) return text; else return "";}
	TString()		{ laenge=0;	text = NULL;}
	TString(char *t){Set(t,strlen(t));}
	TString(char *t,int len){Set(t,len);}

};

#endif
