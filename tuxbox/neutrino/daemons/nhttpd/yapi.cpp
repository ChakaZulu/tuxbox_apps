/*
	nhttpd  -  DBoxII-Project

	Copyright (C) 2001/2002 Dirk Szymanski 'Dirch'

	$Id: yapi.cpp,v 1.2 2005/09/10 12:32:54 yjogol Exp $

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

/* ---------------------------------------------------------------------------------------
	Mini Web Parsing & Generation Engine
   ----------------------------------------------------------------------------------------
	The engine parses and replace ycgi-commands in the given html-file on the fly

	Commands are escaped like this: "{="<command>"=}" They can be nested like
	{=<command1>{=<command2=}=} for dynamic building commands :)
	If the output of a command has itselfs commands, they will be recursivly replaced.

	<command> can be:
		- <http - GET Variable>  
			get value of a http GET argument
			example: /y/cgi?a=hello -> replaces {=a=} with "hello"

		- "script:<shell-script-filename without .sh>
			get output of a shell-script
			example: replaces {=script:Y_Live url=} Output of shell-script-call
			with argument "Y_Live.sh url"

		-"ini-get:<ini/conf-filename>;<varname>"
			get value of a ini/conf file variable
			example: replaces {=ini-get:/var/tuxbox/config/nhttpd.conf;AuthPassword=}
				with the password for the WebInterface from the conf-file				 

		-"inculde:<filename>"
			insert file
			example: {=include:/var/tuxbox/config/nhttpd.conf=} wile be replaced
				with the given file (the output will be parsed}

		-"shell:<shell commands>"
			execute shell command and replace with output
			example: {=shell:echo "hello" >\tmp\a \ncat /tmp/a=} 

		-"func:<func name>[ <arguments>]"
			execute secial functions
			example: {=func:mount-get-list=} generates html-Radio-Buttons 
				for a mount list

	Special url calls
		- /y/cgi?[...&]tmpl=<html file> parses html file (e.g. given in a form)
		- /y/cgi?<html file> parses html file
		- /y/cgi?[...&]debug=1	returns debug: STEP by STEP replacement

   --------------------------------------------------------------------------------------- 
*/

#define NEUTRINO_CONFIGFILE CONFIGDIR "/neutrino.conf"
// c++
#include <cstdlib>
#include <cstring>

// system
#include <unistd.h>
#include <stdio.h>
#include <cctype>
#include <dirent.h>
#include <iostream>
#include <fstream>
#include <map>

// tuxbox
#include <global.h>
#include <neutrino.h>
//#include <system/settings.h>

#include <config.h>
#include <configfile.h>


//#include <dbox/fp.h>
//#include <fcntl.h>
//#include <unistd.h>

// nhttpd
#include "debug.h"
#include "yapi.h"

//-------------------------------------------------------------------------
//	Call Dispatcher
//-------------------------------------------------------------------------
bool CyAPI::Execute(CWebserverRequest* request)
{
	int operation = 0;

	const char *operations[] = {
		"cgi", NULL};

	dprintf("Executing %s\n",request->Filename.c_str());

	while (operations[operation]) {
		if (request->Filename.compare(operations[operation]) == 0) {
			break;
		}
		operation++;
	}

	if (operations[operation] == NULL) {
		request->Send404Error();
		return false;
	}

	if (request->Method == M_HEAD) {
		request->SendPlainHeader("text/html");
		return true;
	}
	switch(operation)
	{
		case 0:	return cgi(request);
			break;
		default:
			request->Send404Error();
			return false;
	}
}

//-------------------------------------------------------------------------
// constructor und destructor
//-------------------------------------------------------------------------

CyAPI::CyAPI(CWebDbox *webdbox)
{
	Parent=webdbox;
	if(Parent != NULL)
		if(Parent->Parent != NULL)
		{	// given in nhttpd.conf
			HTML_DIRS[0]=Parent->Parent->PublicDocumentRoot;
			HTML_DIRS[1]=Parent->Parent->PrivateDocumentRoot;
			HTML_DIRS[2]=yHTML_SEARCHFOLDER_MOUNT;
		}
}

//-------------------------------------------------------------------------
// Get Value from ini/conf-file (filename) vor var (varname)
//-------------------------------------------------------------------------
std::string  CyAPI::YWeb_cgi_get_ini(std::string filename, std::string varname)
{
	CConfigFile *Config = new CConfigFile(',');
	std::string result;

	Config->loadConfig(filename);
	result = Config->getString(varname, "");
	delete Config;
	return result;	
}

//-------------------------------------------------------------------------
void  CyAPI::YWeb_cgi_set_ini(std::string filename, std::string varname)
{
	CConfigFile *Config = new CConfigFile(',');
	std::string result;

	Config->loadConfig(filename);
	Config->setString(varname, "");
	Config->saveConfig(filename);
	delete Config;
}

//-------------------------------------------------------------------------
// ycgi : cmd executing 
//-------------------------------------------------------------------------

std::string  CyAPI::YWeb_cgi_cmd(CWebserverRequest* request, std::string ycmd)
{
	int pos;
	std::string ycmd_type, ycmd_name, yresult;

	if ((pos = ycmd.find_first_of(":")) > 0)
	{
		ycmd_type = ycmd.substr(0, pos); // cmd type
		ycmd_name = ycmd.substr(pos+1,ycmd.length() - (pos+1)); // cmd and args
		//aprintf("ycgi type:%s name:%s\n",ycmd_type.c_str(), ycmd_name.c_str());

		if(ycmd_type == "script")
			yresult = Parent->ControlAPI->YexecuteScript( request, ycmd_name);
		else if(ycmd_type == "include")
		{
			std::string ytmp;
			std::fstream fin(ycmd_name.c_str());
			while (fin >> ytmp) 
				yresult += ytmp;
			fin.close();
		}
		else if(ycmd_type == "func")
			yresult = YWeb_cgi_func(request, ycmd_name);
		else if(ycmd_type == "ini-get")
		{
			if ((pos = ycmd_name.find_first_of(";")) > 0)
			{
				std::string filename, varname;
				filename = ycmd_name.substr(0, pos); 
				varname  = ycmd_name.substr(pos+1,ycmd_name.length() - (pos+1));
				//aprintf("ycgi ini-get:[%s] [%s]\n",filename.c_str(), varname.c_str());
				yresult = YWeb_cgi_get_ini(filename, varname);
			}
			else
				yresult = "ycgi: ini-get: no ; found";
		}
		else
			yresult = "ycgi-type unknown";
	}
	if (request->ParameterList[ycmd] != "")
		yresult = request->ParameterList[ycmd];

	return yresult;
}

//-------------------------------------------------------------------------
// command parsing (nested and recursive)
//-------------------------------------------------------------------------
std::string  CyAPI::cgi_cmd_parsing(CWebserverRequest* request, std::string html_template, bool ydebug)
{
	// ycgi escape command parsing
	int start,end;
	int esc_len = strlen(YCGI_ESCAPE_START);
	bool is_cmd;
	std::string ycmd,yresult;
	
	do // replace all {=<cmd>=} nested and recursive
	{
		is_cmd=false;
		if((start = html_template.rfind(YCGI_ESCAPE_START)) > 0) 				// 1. find LAST y-begin
		{
			if(ydebug) request->printf("[ycgi debug]: START at:%d following:%s<br>\n", start, 
				(html_template.substr(start+esc_len, 10)).c_str() );
			if((end = html_template.find(YCGI_ESCAPE_END, start+esc_len)) > 0) 		// 2. find next y-end
			{
				if(ydebug) request->printf("[ycgi debug]: END at:%d following:%s<br>\n", end, 
					(html_template.substr(end+esc_len, 10)).c_str() );

				ycmd = html_template.substr(start+esc_len,end - (start+esc_len)); 	//3. get cmd
				if(ydebug) request->printf("[ycgi debug]: CMD%s<br>\n", ycmd.c_str());
				yresult = YWeb_cgi_cmd( request, ycmd ); 				// 4. execute cmd
				html_template.replace(start,end - start + esc_len, yresult); 		// 5. replace cmd with output
				is_cmd = true;	// one command found

				if(ydebug) request->printf("[ycgi debug]: STEP<br>\n%s<br>\n", html_template.c_str() );
			}
		}
	}
	while(is_cmd);	
	return html_template;
}

//-------------------------------------------------------------------------
// mini cgi Engine
//-------------------------------------------------------------------------

bool CyAPI::cgi(CWebserverRequest *request)
{
	bool found = false;
	bool ydebug = false;
	std::string htmlfilename, htmlfullfilename, yresult, html_template;

	// ToDo: There is no Contructor/init until now
	request->SendPlainHeader("text/html");          // Standard httpd header senden MIME html

	if (request->ParameterList.size() > 0)
	{
		if (request->ParameterList["tmpl"] != "") // for GET and POST
			htmlfilename = request->ParameterList["tmpl"];
		else
			htmlfilename = request->ParameterList["1"];
		if (request->ParameterList["debug"] != "") // switch debug on
			ydebug = true;

		if (request->ParameterList["execute"] != "") // execute done first!
		{
			html_template = request->ParameterList["execute"];
			html_template = YCGI_ESCAPE_START + html_template + YCGI_ESCAPE_END;
			html_template = " "+html_template;

			yresult = cgi_cmd_parsing(request, html_template, ydebug); // parsing engine
		}

		// parsing given file
		html_template.clear();
		for (unsigned int i=0;i<HTML_DIR_COUNT && !found;i++) 
		{
			htmlfullfilename = HTML_DIRS[i]+"/"+htmlfilename;
			std::fstream fin(htmlfullfilename.c_str(), std::fstream::in);
			if(fin.good())
			{
				found = true;
				// read whole file into html_template
				std::string ytmp;
				while (!fin.eof()) 
				{
					getline(fin, ytmp);
					html_template += ytmp;
				}
				yresult += cgi_cmd_parsing(request, html_template, ydebug); // parsing engine
				request->SocketWrite(yresult);	
				fin.close();
			}
		}
		if (!found)
		{
			printf("[CyAPI] Y-cgi:template %s not found in\n",htmlfilename.c_str());
			for (unsigned int i=0;i<HTML_DIR_COUNT;i++) {
				printf("%s\n",HTML_DIRS[i].c_str());
			}
		}
	}
	else
		printf("[CyAPI] Y-cgi:no template given\n");
	if (!found)
		request->Send404Error();

	return found;
}


//-------------------------------------------------------------------------
// ycgi : <func> dispatching and executing 
//-------------------------------------------------------------------------


std::string  CyAPI::YWeb_cgi_func(CWebserverRequest* request, std::string ycmd)
{
	int pos;
	std::string func, para, yresult;

	if ((pos = ycmd.find_first_of(" ")) > 0)
	{
		func = ycmd.substr(0, pos); // cmd type
		para = ycmd.substr(pos+1,ycmd.length() - (pos+1)); // cmd and args
	}
	else
		func = ycmd;

	// RAD: until now : if-else chain - better Name and function array
	if(func == "mount-get-list")
	{
		CConfigFile *Config = new CConfigFile(',');
		std::string ysel, ytype, yip, ydir,ynr;
		int yitype;
		char ytmp[100];

		Config->loadConfig(NEUTRINO_CONFIGFILE);
		for(unsigned int i=0; i <= 7; i++)
		{
			ynr=itoa(i);
			ysel = ((i==0) ? "selected" : "");
			yitype = Config->getInt32("network_nfs_type_"+ynr,0);
			ytype = ( (yitype==0) ? "NFS" :((yitype==1) ? "CIFS" : "FTPFS") ); 
			yip = Config->getString("network_nfs_ip_"+ynr,"");
			ydir = Config->getString("network_nfs_dir_"+ynr,"");
			
			if(ydir != "") 
				ydir="("+ydir+")";

			sprintf(ytmp,"<input type='radio' name='R1' value='%d' %s>%d %s - %s %s<br>",
				i,ysel.c_str(),i,ytype.c_str(),yip.c_str(),ydir.c_str());

			yresult += ytmp;
		}
		delete Config;
	}
	else if(func == "mount-set-values")
	{
		CConfigFile *Config = new CConfigFile(',');
		std::string ynr;

		Config->loadConfig(NEUTRINO_CONFIGFILE);
		ynr = request->ParameterList["nr"];
		Config->setString("network_nfs_type_"+ynr,request->ParameterList["type"]);
		Config->setString("network_nfs_ip_"+ynr,request->ParameterList["ip"]);
		Config->setString("network_nfs_dir_"+ynr,request->ParameterList["dir"]);
		Config->setString("network_nfs_local_dir_"+ynr,request->ParameterList["localdir"]);
		Config->setString("network_nfs_mac_"+ynr,request->ParameterList["mac"]);
		Config->setString("network_nfs_mount_options1_"+ynr,request->ParameterList["opt1"]);
		Config->setString("network_nfs_mount_options2_"+ynr,request->ParameterList["opt2"]);
		Config->setString("network_nfs_username_"+ynr,request->ParameterList["username"]);
		Config->setString("network_nfs_password_"+ynr,request->ParameterList["password"]);
		Config->saveConfig(NEUTRINO_CONFIGFILE);
	
		delete Config;
	}
	else
		yresult="func not found";

	return yresult;
}
