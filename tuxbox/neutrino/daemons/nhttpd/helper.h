#ifndef __webserver_helper__
#define __webserver_helper__

/*
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <config.h>
*/
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
/*
//-------------------------------------------------------------------------
class TParameter
{
public:
	TParameter	* Next;
	TString		* Name;
	TString		* Value;
//	TParameter(){Name=NULL;Value=NULL;};
	TParameter(char *parameter,char * value);
	TParameter(char *parameter,int len,char seperator='=');
	~TParameter();
};

//-------------------------------------------------------------------------
class TParameterList
{
public:
	int Count;
	TParameter * Head;

	TParameterList(){Count=0;Head = NULL;};
	void Add(char * text,int len,char seperator='=');
	void Add(char * text,char *value);
	int GetIndex(char * Name);
	char * GetValue(int index);
	void PrintParameterList();
	~TParameterList();
};
//-------------------------------------------------------------------------
*/
#endif
