/*
	webserver  -   DBoxII-Project

	Copyright (C) 2001/2002 Dirk Szymanski 'Dirch'

	$ID$

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



*/


#include "request.h"
#include "webdbox.h"
#include <arpa/inet.h> 



//-------------------------------------------------------------------------
CWebserverRequest::CWebserverRequest(TWebserver *server) 
{
	Parent = server;
	Method = M_UNKNOWN; 
	URL = "";
	Filename = "";
	FileExt = "";
	Path = "";
	Param_String="";
	ContentType = "";
//	Boundary = NULL;
	Upload = NULL;
	HttpStatus = 0;
}

//-------------------------------------------------------------------------
CWebserverRequest::~CWebserverRequest() 
{
	if(Upload) delete Upload;
//	if(HeaderList) delete HeaderList;
	if(rawbuffer) 
	{
		free(rawbuffer); 
		rawbuffer=NULL;
		rawbuffer_len = 0;
	}
}

//-------------------------------------------------------------------------
bool CWebserverRequest::GetRawRequest(int socket)
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
void CWebserverRequest::SplitParameter(string param_str)
{
	if(param_str.length() > 0)
	{
		int pos = param_str.find('=');
		if(pos != -1)
		{
			ParameterList[param_str.substr(0,pos)] = param_str.substr(pos+1,param_str.length() - (pos+1));
			if(Parent->DEBUG) printf("Parameter[%s] = '%s'\n",param_str.substr(0,pos).c_str(),param_str.substr(pos+1,param_str.length() - (pos+1)).c_str());
		}
		else
		{
			ParameterList["1"] = param_str;
			if(Parent->DEBUG) printf("Parameter[1] = '%s'\n",param_str.c_str());
		}
	}
}
//-------------------------------------------------------------------------

bool CWebserverRequest::ParseParams(string param_string)
{
string name,value,param;
string param_str;
int pos;
bool ende = false;

	if(Parent->DEBUG) printf("param_string: %s\n",param_string.c_str());
	if(param_string.length() <= 0)
		return false;

	param_str = param_string;
	while(!ende)
	{
		pos = param_str.find_first_of("&");
		if(pos > 0)
		{
			param = param_str.substr(0,pos);	
			param_str = param_str.substr(pos+1,param_str.length() - (pos+1));
		}
		else
		{
			param = param_str;
			ende = true;
		}
		if(Parent->DEBUG) printf("param: '%s' param_str: '%s'\n",param.c_str(),param_str.c_str());
		SplitParameter(param);
	}
	return true;
};
//-------------------------------------------------------------------------


bool CWebserverRequest::ParseFirstLine(string zeile)
{
int ende, anfang, t;

	anfang = zeile.find(' ');				// GET /images/elist.gif HTTP/1.1 
	ende = zeile.rfind(' ');				// nach leerzeichen splitten

	if (anfang > 0 && ende > 0 && anfang != ende)
	{
		string method,url,http;
		method= zeile.substr(0,anfang);
		url = zeile.substr(anfang+1,ende - (anfang+1));
		http = zeile.substr(ende+1,zeile.length() - ende+1);
//		if(Parent->DEBUG) printf("m: '%s' u: '%s' h:'%s'\n",method.c_str(),url.c_str(),http.c_str());

		if(method.compare("POST") == 0)
			Method = M_POST;
		else if(method.compare("GET") == 0)
			Method = M_GET;
		else if(method.compare("PUT") == 0)
			Method = M_PUT;
		else
		{
			Parent->Ausgabe("Ungültige Methode oder fehlerhaftes Packet");
			if(Parent->DEBUG) printf("Request: '%s'\n",rawbuffer);
			return false;
		}
		
		if((t = url.find('?')) > 0)			// eventuellen Parameter inner URL finden
		{
			URL = url.substr(0,t);
			Param_String = url.substr(t+1,url.length() - (t+1));

			return ParseParams(Param_String);
		}
		else
			URL = url;
	}
	return true;
}


//-------------------------------------------------------------------------
bool CWebserverRequest::ParseHeader(string header)
{
bool ende = false;
int pos;
string sheader;
	if(Parent->DEBUG) printf("Header:\n");
	while(!ende)
	{
		if((pos = header.find_first_of("\n")) > 0)
		{
			sheader = header.substr(0,pos-1);	
			header = header.substr(pos+1,header.length() - (pos+1));
		}
		else
		{
			sheader = header;
			ende = true;
		}
		
		if((pos = sheader.find_first_of(':')) > 0)
		{
			HeaderList[sheader.substr(0,pos)] = sheader.substr(pos+2,sheader.length() - pos - 2);
			if(Parent->DEBUG) printf("%s: %s\n",sheader.substr(0,pos).c_str(),HeaderList[sheader.substr(0,pos)].c_str());
		}
	}
	
}

//-------------------------------------------------------------------------
bool CWebserverRequest::ParseRequest()
{
int i = 0,zeile2_offset = 0;
char *zeile;
int ende;
string buffer;
	if(rawbuffer && (rawbuffer_len > 0) )
	{
		buffer= string(rawbuffer,rawbuffer_len);
		if((ende = buffer.find_first_of('\n')) == 0)
		{
			printf("Kein Zeilenende gefunden\n");
			Send500Error();
			return false;
		}
		string zeile1 = buffer.substr(0,ende-1);
		if(Parent->DEBUG) printf("REQUEST: '%s'\n",zeile1.c_str());

		if(ParseFirstLine(zeile1))
		{
			int headerende = buffer.length() - 1;
			if(headerende == 0)
			{
				printf("Keine Header gefunden\n");
				Send500Error();
				return false;
			}
			string header = buffer.substr(ende+1,headerende - ende - 4);
			ParseHeader(header);


/*			
			if(Method == M_POST) // TODO: Und testen ob content = formdata
			{
				if( (ende + 3) < rawbuffer + rawbuffer_len)
				{
//					Parent->Debug("Post Parameter vorhanden\n");
					anfang = ende + 3;
					Param_String = string(anfang,rawbuffer + rawbuffer_len - anfang);
					if(Parent->DEBUG) printf("Post Param_String: %s\n",Param_String.c_str());
					ParseParams(Param_String);
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
*/
		}
		return true;
	}
}
//-------------------------------------------------------------------------

void CWebserverRequest::PrintRequest()
{
	char method[6] = {0};
	if(Method == M_GET)
		sprintf(method,"GET");
	if(Method == M_POST)
		sprintf(method,"POST");


	printf("%s %3d %-6s %-35s %-20s %-25s %-10s %s\n",inet_ntoa(cliaddr.sin_addr),HttpStatus,method,Path.c_str(),Filename.c_str(),URL.c_str(),ContentType.c_str(),Param_String.c_str());
}

//-------------------------------------------------------------------------
void CWebserverRequest::SendHTMLHeader(char * Titel)
{
	SocketWrite("<html>\n<head><title>");
	SocketWrite(Titel);
	SocketWrite("DBOX2-Neutrino Kanalliste</title><link rel=\"stylesheet\" type=\"text/css\" href=\"../channellist.css\">");
	SocketWriteLn("<meta http-equiv=\"cache-control\" content=\"no-cache\">\n</head>\n<body>\n");
}

//-------------------------------------------------------------------------
void CWebserverRequest::SendHTMLFooter()
{
	SocketWriteLn("</body></html>");
}
//-------------------------------------------------------------------------
void CWebserverRequest::Send404Error()
{
	SocketWrite("HTTP/1.0 404 Not Found\n");		//404 - file not found
	SocketWrite("Content-Type: text/plain\n\n");
	SocketWrite("404 : File not found\n\n");
	HttpStatus = 404;
	if(Parent->DEBUG) printf("Sende 404 Error\n");
}
//-------------------------------------------------------------------------
void CWebserverRequest::Send500Error()
{
	SocketWrite("HTTP/1.0 500 InternalError\n");		//500 - internal error
	SocketWrite("Content-Type: text/plain\n\n");
	SocketWrite("500 : InternalError\n\n");
	HttpStatus = 500;
	if(Parent->DEBUG) printf("500 : InternalError\n");
}
//-------------------------------------------------------------------------

void CWebserverRequest::SendPlainHeader(char *contenttype = NULL)
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
void CWebserverRequest::RewriteURL()
{

	if(Parent->DEBUG) printf("Schreibe URL um\n");

	if(( URL.length() == 1) && (URL[URL.length()-1] == '/' ))		// Wenn letztes Zeichen ein / ist dann index.html anhängen
	{
		Path = URL;
		Filename = "index.html";
	}
	else
	{		// Sonst aufsplitten
		int split = URL.rfind('/') + 1;

		if(split > 0)
			Path = URL.substr(0,split);
		else
			Path = "/";


		if(split < URL.length())
			Filename= URL.substr(split,URL.length()- split);
		else
			printf("[nhttpd] Kein Dateiname !\n");	
	}
	
	if(Parent->DEBUG) printf("Path: '%s'\n",Path.c_str());
	if(Parent->DEBUG) printf("Filename: '%s'\n",Filename.c_str());

	if( (strncmp(Path.c_str(),"/fb",3) != 0) && (strncmp(Path.c_str(),"/control",8) != 0) )	// Nur umschreiben wenn nicht mit /fb/ anfängt
	{
//		sprintf(urlbuffer,"%s%s\0",Parent->PublicDocumentRoot->c_str(),Path.c_str());
		Path = Parent->PublicDocumentRoot + Path;
//		Path = urlbuffer;
		if(Parent->DEBUG) printf("Umgeschrieben: '%s'\n",Path.c_str());
	}
	else
		if(Parent->DEBUG) printf("FB oder control, URL nicht umgeschrieben\n",Path.c_str());
	
	if(Parent->DEBUG) printf("Auf Sonderzeichen prüfen\n");
/*
	if(strchr(Filename.c_str(),'%'))	// Wenn Sonderzeichen im Dateinamen sind
	{
		char filename[255]={0};
		char * str = (char *) Filename.c_str();
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
		Filename = filename;
	}
*/
	if(Parent->DEBUG) printf("Auf FileExtension prüfen: ");
	FileExt = "";
	if(Filename.length() > 0) 
	{
		int fileext = Filename.rfind('.');

		if(fileext == -1)		// Dateiendung
			if(Parent->DEBUG) printf("Keine Dateiendung gefunden !\n");
		else
		{
			FileExt = Filename.substr(fileext,Filename.length()-fileext);
			if(Parent->DEBUG) printf("FileExt = %s\n",FileExt.c_str());
		}
	}
}		

//-------------------------------------------------------------------------
bool CWebserverRequest::SendResponse()
{
int file;

	if(Parent->DEBUG) printf("SendeResponse()\n");

	RewriteURL();		// Erst mal die URL umschreiben

	if(strncmp(Path.c_str(),"/control",8) == 0)
	{
		if(Parent->DEBUG) printf("Web api\n");
		Parent->WebDbox->ExecuteCGI(this);
		return true;
	}
	if(strncmp(Path.c_str(),"/fb",3) == 0)
	{
		if(Parent->DEBUG) printf("Browser api\n");
		Parent->WebDbox->Execute(this);
		return true;
	}
	else
	{
	// Normale Datei
		if(Parent->DEBUG) printf("Normale Datei\n");
		if( (file = OpenFile(Path.c_str(),Filename.c_str()) ) != -1 )		// Testen ob Datei auf Platte geöffnet werden kann
		{											// Wenn Datei geöffnet werden konnte
			SocketWrite("HTTP/1.0 200 OK\n");		
			HttpStatus = 200;
			if( FileExt != "" )		// Anhand der Dateiendung den Content bestimmen
				ContentType = "text/html";
			else
			{
				
				if( (strcasecmp(FileExt.c_str(),"html") == 0) || (strcasecmp(FileExt.c_str(),"htm") == 0) )
				{
					ContentType = "text/html";
				}
				else if(strcasecmp(FileExt.c_str(),"gif") == 0)
				{
					ContentType = "image/gif";
				}
				else if(strcasecmp(FileExt.c_str(),"jpg") == 0)
				{
					ContentType = "image/jpeg";
				}
				else
					ContentType = "text/plain";

			}
			SocketWrite("Content-Type: ");SocketWrite((char *)ContentType.c_str());SocketWrite("\n\n");

			if(Parent->DEBUG) printf("content-type: %s - %s\n", ContentType.c_str(),Filename.c_str());
			SendOpenFile(file);
		}
		else
		{											// Wenn Datei nicht geöffnet werden konnte
			Send404Error();							// 404 Error senden
		}
		if(Parent->DEBUG) printf("Response gesendet\n");
		return true;
	}
}
//-------------------------------------------------------------------------
bool CWebserverRequest::EndRequest()
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
void CWebserverRequest::SocketWrite(char *text)
{
	write(Socket, text, strlen(text) );
}
//-------------------------------------------------------------------------
void CWebserverRequest::SocketWriteLn(char *text)
{
	write(Socket, text, strlen(text) );
	write(Socket,"\n",1);
}
//-------------------------------------------------------------------------
void CWebserverRequest::SocketWriteData( char* data, long length )
{
	write(Socket, data, length );
}
//-------------------------------------------------------------------------

bool CWebserverRequest::SendFile(string path,string filename)
{
int file;
	if( (file = OpenFile(path.c_str(),filename.c_str()) ) != -1 )		// Testen ob Datei auf Platte geöffnet werden kann
	{											// Wenn Datei geöffnet werden konnte
		SendOpenFile(file);
		return true;
	}
	else
		return false;
}
//-------------------------------------------------------------------------
void CWebserverRequest::SendOpenFile(int file)
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
int CWebserverRequest::OpenFile(const char *path, const char *filename)
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
		if (file<=0)
		{
			printf("cannot open file %s\n", fname);
			if(Parent->DEBUG) perror("");
		}	
		delete[] fname;
	}
	return file;
}


//-------------------------------------------------------------------------
