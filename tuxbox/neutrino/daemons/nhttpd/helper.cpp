/*
	webserver  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/



	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
 
	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

	// Revision 1.1  11.02.2002 20:20  dirch

*/

#include "helper.h"


void TString::Set(char * t, int len)
{
	laenge=len;
	if(laenge > 0)
		if((text = (char *) malloc(laenge+1)) != NULL){
			strncpy(text,t,laenge);
			text[laenge] = 0;
		}else perror("Kein Speicher in TString\n");	
}

//-------------------------------------------------------------------------
/*
TParameter::TParameter(char * parameter,int len)
{
	TParameter(parameter,len,'=');
}
//-------------------------------------------------------------------------
*/

TParameter::TParameter(char *name,char *value)
{
	Name = new TString(name);
	Value = new TString(value);	
}
//-------------------------------------------------------------------------
TParameter::TParameter(char *parameter,int len,char seperator='=')
{
bool DEBUG = false;

	if( (strlen(parameter) > 0) )
	{
	int i;
	char *t;

		if(len == 0)								// Wenn keine Länge angegeben dann ganzer String
			len = strlen(parameter);

		if(parameter[len] == '\n')				// Wenn letztes Zeichen \n dann abschneiden
			len--;

		t = strchr(parameter,seperator);						//Nach Trennzeichen suchen
		if( t && (t < parameter + len) )
		{												// Wenn Trennzeichen gefunden			
			Name = new TString(parameter,t - parameter);
			char *start = t+1;
			int laenge = parameter + len - t;
			for(;*start == ' ';start++,laenge--);
			for(;(start[laenge] == ' ') || (start[laenge] == '\n');laenge--);
			Value = new TString(start,laenge);
			if(Name && DEBUG)
				printf("Name: '%s' Value '%s'\n",Name->c_str(),Value->c_str());
		}
		else 
		{												// Wenn kein Trennzeichen dann nur Namen mit " " speichern
			if(DEBUG) printf("[nhttpd] Kein Trennzeichen '%c' gefunden\n",seperator);
			int laenge = len+1;
			char *start = parameter;
			if(DEBUG) printf("start[laenge] '%c' start[laenge-1]: '%c'\n",start[laenge],start[laenge-1]);
			while(*start == ' ')
			{
				start++;
				laenge--;
			};
			while( (start[laenge-1] == ' ') || (start[laenge-1] == '\n') || (start[laenge-1] == 0) )
			{
				laenge--;
			};
			Name = new TString(start,laenge);
			Value = new TString(" ");
			if(DEBUG) printf("Name: '%s' Value '%s'\n",Name->c_str(),Value->c_str());

		}
//		printf("TParameter: %x\n",this);
	}
}
//-------------------------------------------------------------------------

TParameter::~TParameter()
{
	if(Name)
	{
		free(Name);
		Name = NULL;
	}
	if(Value)
	{
		free(Value);
		Value = NULL;
	}
}
//-------------------------------------------------------------------------

void TParameterList::Add(char * name,char *value)  
{
TParameter *tmp;
	tmp = new TParameter(name,value);
	tmp->Next = Head;
	Head = tmp;
	Count++;

}
//-------------------------------------------------------------------------

void TParameterList::Add(char * text,int len,char seperator='=')  
{
TParameter *tmp;
	tmp = new TParameter(text,len,seperator);
	tmp->Next = Head;
	Head = tmp;
	Count++;
}

//-------------------------------------------------------------------------

int TParameterList::GetIndex(char * Name)
{
TParameter *tmp;
int i;
bool gefunden = false;

	tmp = Head;
	for(i = 0; (i < Count) && tmp ;i++)
	{
		if(strcmp(Name,tmp->Name->c_str()) == 0)
		{
//			printf("Parameter gefunden\n");
			gefunden = true;
			break;
		}
		else
			tmp = tmp->Next;

	}
	if(gefunden)
		return i;
	else
		return -1;
}

//-------------------------------------------------------------------------
char * TParameterList::GetValue(int index)
{
int i;
TParameter *tmp;
	tmp = Head;
	for(int i = 0; i < index && tmp; i++)
		tmp = tmp->Next;
	if(tmp)
		return tmp->Value->c_str();
	else
		return NULL;
}

//-------------------------------------------------------------------------
void TParameterList::PrintParameterList()
{
	TParameter *t = Head;
	for(int i = 0; i < Count;i++)
	{
		if(t->Name)
			printf("Parameter: %s\n",t->Name->c_str());
		if(t->Value)
			printf("Value: %s\n",t->Value->c_str());
		t = t->Next;		
	}
}

//-------------------------------------------------------------------------
TParameterList::~TParameterList()
{
TParameter *tmp;
	while(Head)
	{
		tmp = Head->Next;
		delete Head;
		Head = tmp;
	}
}
//-------------------------------------------------------------------------
