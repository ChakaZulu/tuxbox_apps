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


#include "request.h"
#include "webdbox.h"

//-------------------------------------------------------------------------
TWebserverRequest::TWebserverRequest(TWebserver *server) 
{
	Parent = server;
	Method = M_UNKNOWN; 
	URL = NULL;
	Filename = NULL;
	FileExt = NULL;
	Path = NULL;
	Param_String=NULL;
	ContentType = NULL;
	Boundary = NULL;
	Upload = NULL;
	HttpStatus = 0;

	ParameterList= new TParameterList; 
	HeaderList = new TParameterList;
}

//-------------------------------------------------------------------------
TWebserverRequest::~TWebserverRequest() 
{

	if(Upload) delete Upload;
	if(URL != NULL) delete URL;
	if(Path) delete Path;
	if(Filename) delete Filename;
	if(Param_String) delete Param_String;
	if(ParameterList) delete ParameterList;
	if(FileExt) delete FileExt;
	if(ContentType) delete ContentType;
	if(Boundary) delete Boundary;
	if(HeaderList) delete HeaderList;
	if(rawbuffer) 
	{
		free(rawbuffer); 
		rawbuffer=NULL;
		rawbuffer_len = 0;
	}

	if(Parent->DEBUG) printf("Request destruktor ok\n");	
}

//-------------------------------------------------------------------------
bool TWebserverRequest::GetRawRequest(int socket)
{
	char buffer[1024];
	int anzahl_bytes = 0;

	Socket = socket;

	if((anzahl_bytes = read(Socket,&buffer,sizeof(buffer))) != -1)
	{
		buffer[anzahl_bytes] = 0;		// Ende markieren
		rawbuffer_len = anzahl_bytes;
		if( (rawbuffer = (char *) malloc(rawbuffer_len)) != NULL )
		{
			memcpy(rawbuffer,buffer,rawbuffer_len);
			if(Parent->DEBUG) printf("GetRequest ok\n");
			return true;
		}
	}
	return false;
}
//-------------------------------------------------------------------------
bool TWebserverRequest::ParseParams(char *param_string)
{
char * anfang;
char * ende;
	if(Parent->DEBUG) printf("param_string: %s\n",param_string);
	if(strlen(param_string) < 1)
		return false;

	if(!ParameterList)
		ParameterList = new TParameterList;
	anfang = param_string;
	do{
		ende = strchr(anfang,'&');

		if(!ende)
			ParameterList->Add(anfang, param_string + strlen(param_string) - anfang);
		else
			ParameterList->Add(anfang, ende - anfang -1);
		anfang = ende + 1;
	}while(ende);
	return true;
};
//-------------------------------------------------------------------------

void TWebserverRequest::ParseHeader(char * t, int len)
{
	if(!HeaderList)
		HeaderList = new TParameterList;
	
	HeaderList->Add(t,len,':');
}


bool TWebserverRequest::ParseFirstLine(char * zeile, int len)
{
char *ende, *anfang, *t;
	
	if(strncmp("POST",zeile,4) == 0)
		Method = M_POST;
	if(strncmp("GET",zeile,3) == 0)
		Method = M_GET;
	if(strncmp("PUT",zeile,3) == 0)
		Method = M_PUT;
//			printf("Method=%ld\n",Method);
	if(Method == M_UNKNOWN)
	{
		Parent->Ausgabe("Ungültige Methode oder fehlerhaftes Packet");
		if(Parent->DEBUG) printf("Request: '%s'\n",rawbuffer);
		return false;
	}

	anfang = strchr(zeile,' ')+1;
	ende = strchr(anfang,' ');
	if( ((t = strchr(anfang,'?')) != 0) && (t < ende) )
	{
		Param_String= new TString(t+1,ende - t);
		URL = new TString(anfang,t - anfang);
		// URL in Path und Filename aufsplitten

		if(Parent->DEBUG) printf("URL: %s\nParams: %s\n",URL->c_str(),Param_String->c_str());
		if(ParseParams(Param_String->c_str()))
			return true;

	}
	else
	{
		URL = new TString(anfang,ende - anfang);
		if(Parent->DEBUG) printf("URL: %s\n",URL->c_str());
		return true;
	}
	return true;
}

//-------------------------------------------------------------------------
bool TWebserverRequest::ParseRequest()
{
int i = 0,zeile2_offset = 0;
char *zeile;
char *ende,*anfang;

	if(rawbuffer && (rawbuffer_len > 0) )
	{
		if(Parent->DEBUG) printf("Parse jetzt Request . . .\n");
		anfang=rawbuffer;
		if(!(ende = strchr(rawbuffer,'\n')))
		{
			printf("Kein Zeilenende gefunden\n");
			Send500Error();
			return false;
		}
		
		int len  = ((int) ende - (int) anfang) -1;
		zeile = new char[len];
		strncpy(zeile,anfang,len);
		zeile[len] = 0;
		if(ParseFirstLine(zeile,len))
		{
			
			anfang = ende + 1;
			if(anfang >= (rawbuffer + rawbuffer_len))
			{
				printf("Keine Header gefunden\n");
				Send500Error();
				return false;
			}

			if(Parent->DEBUG) printf("Und jetzt die Header :\n");
			do{
				ende = strchr(anfang,'\n');
				if(ende <(rawbuffer + rawbuffer_len))
					ParseHeader(anfang, ende - anfang);
				anfang = ende + 1;
			}while( (ende[2] != '\n') && (ende < (rawbuffer + rawbuffer_len)) );
			
			if(Method == M_POST) // TODO: Und testen ob content = formdata
			{
				if( (ende + 3) < rawbuffer + rawbuffer_len)
				{
//					Parent->Debug("Post Parameter vorhanden\n");
					anfang = ende + 3;
					Param_String = new TString(anfang,rawbuffer + rawbuffer_len - anfang);
					if(Parent->DEBUG) printf("Post Param_String: %s\n",Param_String->c_str());
					ParseParams(Param_String->c_str());
				}
				if(HeaderList->GetIndex("Content-Type") != -1)
				{
					if(Parent->DEBUG) printf("Content-Type: %s\n",HeaderList->GetValue(HeaderList->GetIndex("Content-Type")));
					if(strcasecmp("application/x-www-form-urlencoded",HeaderList->GetValue(HeaderList->GetIndex("Content-Type"))) == 0)
						if(Parent->DEBUG) printf("Form Daten in Parameter String\n");
					if(strstr(HeaderList->GetValue(HeaderList->GetIndex("Content-Type")),"multipart/form-data") != 0)
					{
						char * boundary;
						boundary = strstr(HeaderList->GetValue(HeaderList->GetIndex("Content-Type")),"boundary=");
						if(boundary)
						{
							boundary += strlen("boundary=");

							if(Parent->DEBUG) printf("boundary : %s\n",boundary);
							Upload = new TUpload(this);
							Upload->Boundary = new TString(boundary);
							Boundary = new TString(boundary);
							if(Parent->DEBUG) printf("Form Daten in Parameter String und Datei upload\nBoundary: %ld\n",Boundary);
						}
					}					
				}
			}
		}
		return true;
	}
	return false;
}
//-------------------------------------------------------------------------

void TWebserverRequest::PrintRequest()
{

//	printf("------ Request Data: ------\n");

	char method[6] = {0};
	if(Method == M_GET)
		sprintf(method,"GET");
	if(Method == M_POST)
		sprintf(method,"POST");


	printf("%3d %-6s %-30s %-20s %-25s %-25s %-12s\n",HttpStatus,method,Path?Path->c_str():"",Filename?Filename->c_str():"",URL?URL->c_str():"",ContentType?ContentType->c_str():"",Param_String?Param_String->c_str():"");
	if(Parent->DEBUG) printf("Requestlänge: %ld\n",rawbuffer_len);
	if(Parent->DEBUG) printf("Buffer:\n%s\n",rawbuffer);

//	printf("----------------------------\n");
}

//-------------------------------------------------------------------------
void TWebserverRequest::Send404Error()
{
	SocketWrite("HTTP/1.0 404 Not Found\n");		//404 - file not found
	SocketWrite("Content-Type: text/plain\n\n");
	SocketWrite("404 : File not found\n\n");
	HttpStatus = 404;
	if(Parent->DEBUG) printf("Sende 404 Error\n");
}
//-------------------------------------------------------------------------
void TWebserverRequest::Send500Error()
{
	SocketWrite("HTTP/1.0 500 InternalError\n");		//500 - internal error
	SocketWrite("Content-Type: text/plain\n\n");
	SocketWrite("500 : InternalError\n\n");
	HttpStatus = 500;
	if(Parent->DEBUG) printf("500 : InternalError\n");
}
//-------------------------------------------------------------------------

void TWebserverRequest::SendPlainHeader(char *contenttype = NULL)
{
	SocketWrite("HTTP/1.0 200 OK\nContent-Type: ");
	if(contenttype)
		SocketWrite(contenttype);
	else
		SocketWrite("text/plain");
	SocketWrite("\n\n");
	HttpStatus = 200;
}

//-------------------------------------------------------------------------
void TWebserverRequest::RewriteURL()
{

	if(Parent->DEBUG) printf("Schreibe URL um\n");

	char* a= URL->c_str();

	if(Parent->DEBUG) printf("Vor split URL:'%s'\n",a);
	
	if(a[strlen(a)-1] == '/' )		// Wenn letztes Zeichen ein / ist dann index.html anhängen
	{
		Path = new TString(a);
		Filename = new TString("index.html");
	}
	else
	{		// Sonst aufsplitten
//		if(strchr(a,'/') != strrchr(a,'/'))
		{
			char* split = strrchr(a,'/');

			if(split > a)
				Path = new TString(a,split - a);
			else
				Path = new TString("/");


			if(split < (a + strlen(a)))
			{
				Filename = new TString(split+1,strlen(a) - (split - a) -1);
			}
			else
				printf("[nhttpd] Kein Dateiname !\n");
		}
		
	}
	
	if(Parent->DEBUG) printf("Path: '%s'\n",Path->c_str());
	if(Parent->DEBUG) printf("Filename: '%s'\n",Filename->c_str());

	if( (strncmp(Path->c_str(),"/fb",3) != 0) && (strncmp(Path->c_str(),"/control",8) != 0) )	// Nur umschreiben wenn nicht mit /fb/ anfängt
	{
		char urlbuffer[255]={0};
		if(Parent->DEBUG) printf("Umschreiben\n");
		sprintf(urlbuffer,"%s%s\0",Parent->PublicDocumentRoot->c_str(),Path->c_str());
		delete Path;
		Path = new TString(urlbuffer);
	}
	else
		if(Parent->DEBUG) printf("FB oder control, URL nicht umgeschrieben\n",Path->c_str());

	if(strchr(Filename->c_str(),'%'))	// Wenn Sonderzeichen im Dateinamen sind
	{
		char filename[255]={0};
		char * str = Filename->c_str();
		if(Parent->DEBUG) printf("Mit Sonderzeichen: '%s'\n",str);
		for (int i = 0,n = 0; i < strlen(str) ;i++ )
		{
			if(str[i] == '%')
			{
				switch (*((short *)(&str[i+1])))
				{
					case 0x3230 : filename[n++] = ' '; break;
					default: filename[n++] = ' '; break;				
				
				}
				i += 2;
			}
			else
				filename[n++] = str[i];
		}
		if(Parent->DEBUG) printf("Ohne Sonderzeichen: '%s'\n",filename);
		delete Filename;
		Filename = new TString(filename);


	}

	char * fileext = NULL;
	
	if((fileext = strrchr(Filename->c_str(),'.')) == NULL)		// Dateiendung
	{
		if(Parent->DEBUG) printf("Keine Dateiendung gefunden !\n");
//			return false;
	}
	else
	{
		if(Parent->DEBUG) printf("FileExt = %s\n",fileext);
//			fileext++;
		FileExt = new TString(++fileext);
	}

}		

//-------------------------------------------------------------------------
bool TWebserverRequest::SendResponse()
{
int file;

	if(Parent->DEBUG) printf("SendeResponse()\n");
	{
		RewriteURL();		

		if(strncmp(Path->c_str(),"/control",8) == 0)
		{
			if(Parent->DEBUG) printf("Web api\n");
			Parent->WebDbox->ExecuteCGI(this);
			return true;
		}
		if(strcmp(Path->c_str(),"/fb") == 0)
		{
			if(Parent->DEBUG) printf("Webbrowser api\n");
			Parent->WebDbox->Execute(this);
			return true;
		}
		else
		{
		// Normale Datei
			if(Parent->DEBUG) printf("Normale Datei\n");
/*
			char dateiname[255]={0};
			if(Path)
				strcpy(dateiname,Path->c_str());
			if(Filename)
			{
				if(strlen(Path->c_str()) > 1)
					strcat(dateiname,"/");
				strcat(dateiname,);
			}
			if(Parent->DEBUG) printf("Oeffne Datei '%s'\n",dateiname);		
*/
			if( (file = OpenFile(Path->c_str(),Filename->c_str()) ) != -1 )		// Testen ob Datei auf Platte geöffnet werden kann
			{											// Wenn Datei geöffnet werden konnte
				SocketWrite("HTTP/1.0 200 OK\n");		
				HttpStatus = 200;
				if( (!FileExt) )		// Anhand der Dateiendung den Content bestimmen
					ContentType = new TString("text/html");
				else
				{
					
					if( (strcasecmp(FileExt->c_str(),"html") == 0) || (strcasecmp(FileExt->c_str(),"htm") == 0) )
					{
						ContentType = new TString("text/html");
					}
					else if(strcasecmp(FileExt->c_str(),"gif") == 0)
					{
						ContentType = new TString("image/gif");
					}
					else if(strcasecmp(FileExt->c_str(),"jpg") == 0)
					{
						ContentType = new TString("image/jpeg");
					}
					else
						ContentType = new TString("text/plain");

				}
				SocketWrite("Content-Type: ");SocketWrite(ContentType->c_str());SocketWrite("\n\n");

				if(Parent->DEBUG) printf("content-type: %s - %s\n", ContentType->c_str(),Filename->c_str());
				SendOpenFile(file);
			}
			else
			{											// Wenn Datei nicht geöffnet werden konnte
				Send404Error();							// 404 Error senden
			}
		}
		if(Parent->DEBUG) printf("Response gesendet\n");
		return true;
	}
}
//-------------------------------------------------------------------------
bool TWebserverRequest::EndRequest()
{
	if(Socket)
	{
		close(Socket);
		Socket = 0;
	}
	if(Parent->DEBUG) printf("Request beendet\n");
	return true;
}
//-------------------------------------------------------------------------
void TWebserverRequest::SocketWrite(char *text)
{
	write(Socket, text, strlen(text) );
}
//-------------------------------------------------------------------------
void TWebserverRequest::SocketWriteLn(char *text)
{
	write(Socket, text, strlen(text) );
	write(Socket,"\n",1);
}
//-------------------------------------------------------------------------
void TWebserverRequest::SocketWriteData( char* data, long length )
{
	write(Socket, data, length );
}
//-------------------------------------------------------------------------

bool TWebserverRequest::SendFile(char *path,char *filename)
{
int file;
	if( (file = OpenFile(path,filename) ) != -1 )		// Testen ob Datei auf Platte geöffnet werden kann
	{											// Wenn Datei geöffnet werden konnte
		SendOpenFile(file);
		return true;
	}
	else
	{
		return false;
	}
}
//-------------------------------------------------------------------------
void TWebserverRequest::SendOpenFile(int file)
{
	long filesize = lseek( file, 0, SEEK_END);
	lseek( file, 0, SEEK_SET);

	char buf[1024];
	long fsize = filesize;
	while(fsize>0)
	{
		long block = fsize;
		if(block>(long)sizeof(buf))
		{
			block = sizeof(buf);
		}
		read( file, &buf, block);
		SocketWriteData( (char*) &buf, block);
		fsize -= block;
	}
	close(file);
	if(Parent->DEBUG) printf("Datei gesendet\n");
}
//-------------------------------------------------------------------------
int TWebserverRequest::OpenFile(char *path, char *filename)
{
int file = 0;
	char *fname = new char[strlen(path) + strlen(filename)+1];
	if(fname)
	{
		char format[8]={0};
		strcpy(format,((path[strlen(path)-1] != '/') && (filename[0] != '/'))?"%s/%s\0":"%s%s\0");
		memset(fname,0,strlen(path) + strlen(filename)+1);
		sprintf(fname,format,path,filename);
		file = open( fname, O_RDONLY );
		delete[] fname;
		if (file<=0)
		{
			printf("cannot open file %s\n", fname);
			if(Parent->DEBUG) perror("");
		}	
	}
	return file;
}


//-------------------------------------------------------------------------
