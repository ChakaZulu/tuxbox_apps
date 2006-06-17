/*
	nhttpd  -  DBoxII-Project

	$Id: yapi.cpp,v 1.12 2006/06/17 17:24:10 yjogol Exp $

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

		-"var-get:<varname>"
			get value of a ycgi variable (scope: one parsing session)
		
		-"var-set:<varname>=<value>"
			set value of a ycgi variable (scope: one parsing session)
		
		-"global-var-get:<varname>"
			get value of a ycgi variable (scope: nhttpd session)
		
		-"global-var-set:<varname>=<value>"
			set value of a ycgi variable (scope: nhttpd session)
		
		- "script:<shell-script-filename without .sh>
			get output of a shell-script
			example: replaces {=script:Y_Live url=} Output of shell-script-call
			with argument "Y_Live.sh url"

		-"ini-get:<ini/conf-filename>;<varname>[;<default>]"
			get value of a ini/conf file variable
			example: replaces {=ini-get:/var/tuxbox/config/nhttpd.conf;AuthPassword=}
				with the password for the WebInterface from the conf-file				 

		-"ini-set:<ini/conf-filename>;<varname>;<value>"
			set value for a ini/conf file variable with value
		
		-"inculde:<filename>"
			insert file
			example: {=include:/var/tuxbox/config/nhttpd.conf=} wile be replaced
				with the given file (the output will be parsed}

		-"shell:<shell commands>" // To Be Implemented
			execute shell command and replace with output
			example: {=shell:echo "hello" >\tmp\a \ncat /tmp/a=} 

		-"func:<func name>[ <arguments>]"
			execute secial functions
			example: {=func:mount-get-list=} generates html-Radio-Buttons 
				for a mount list
				
		-"if-empty:<value>~<then>~<else>"
		
		-"if-equal:<left_value>~<right_value>~<then>~<else>"
			 (left_value == right_value?)
		
		-"if-not-equal:<left_value>~<right_value>~<then>~<else>"
			 (left_value != right_value?)
		
		- "if-file-exists:<filename>~<them>~<else>"
		- "file-action:<filename>;<action>[;<content>]
			<action>::=none | write | append | delete
			<content> Content to write or append

		-"include-block:<filename>;<block-name>[;<default-text>]"
			insert text from file <filename> beeginning after "start-block+<blockname>"
			and ends before "end-block+<blockname>"

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
#include <zapit/client/zapittypes.h>

#include <zapit/bouquets.h>

//#include <system/settings.h>

#include <config.h>
#include <configfile.h>


//#include <dbox/fp.h>
//#include <fcntl.h>


// nhttpd
#include "helper.h"
#include "debug.h"
#include "yapi.h"

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
	yCached_blocks_attrib.st_mtime = 0;
}
//-------------------------------------------------------------------------
CyAPI::~CyAPI(void)
{

}
//-------------------------------------------------------------------------
//	Call Dispatcher (/y/<execute command>)
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
// mini cgi Engine (Entry for ycgi) 
//-------------------------------------------------------------------------

bool CyAPI::cgi(CWebserverRequest *request)
{
	bool found = false;
	bool ydebug = false, yexecute = false;
	std::string htmlfilename, yresult, ycmd;

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
			ycmd = request->ParameterList["execute"];
			ycmd = YCGI_ESCAPE_START + ycmd + YCGI_ESCAPE_END;
			ycmd = " "+ycmd;

			yresult = cgi_cmd_parsing(request, ycmd, ydebug); // parsing engine
			yexecute = true;
		}

		// parsing given file
		if(htmlfilename != "")
			yresult = cgi_file_parsing(request, htmlfilename, ydebug);
		else
			if(yexecute) //execute only
				found = true;
	}
	else
		printf("[CyAPI] Y-cgi:no parameter given\n");
	if (yresult.length()<=0)
		request->Send404Error();
	else
		request->SocketWrite(yresult);	

	return found;
}

//-------------------------------------------------------------------------
// Parsing and sending .yhtm Files (Entry) 
//-------------------------------------------------------------------------

bool CyAPI::ParseAndSendFile(CWebserverRequest *request)
{
	bool found = false;
	bool ydebug = false;
	std::string yresult, ycmd;

	request->SendPlainHeader("text/html");          // Standard httpd header senden MIME html
	if (request->Method == M_HEAD) {
			return true;
	}
	if (request->ParameterList["debug"] != "") // switch debug on
		ydebug = true;

	if (request->ParameterList["execute"] != "") // execute done first!
	{
		ycmd = request->ParameterList["execute"];
		ycmd = YCGI_ESCAPE_START + ycmd + YCGI_ESCAPE_END;
		ycmd = " "+ycmd;

		yresult = cgi_cmd_parsing(request, ycmd, ydebug); // parsing engine
	}
	// parsing given file
	yresult += cgi_file_parsing(request, request->Filename, ydebug);
	
	if (yresult.length()<=0)
		request->Send404Error();
	else
		request->SocketWrite(yresult);	

	return found;
}
// =============================================================================================
// parsing helpers
// =============================================================================================
//-------------------------------------------------------------------------
// mini cgi Engine (file parsing)
//-------------------------------------------------------------------------

std::string CyAPI::cgi_file_parsing(CWebserverRequest *request, std::string htmlfilename, bool ydebug)
{
	bool found = false;
	std::string htmlfullfilename, yresult, html_template;


	for (unsigned int i=0;i<HTML_DIR_COUNT && !found;i++) 
	{
		htmlfullfilename = HTML_DIRS[i]+"/"+htmlfilename;
		std::fstream fin(htmlfullfilename.c_str(), std::fstream::in);
		if(fin.good())
		{
			found = true;
			chdir(HTML_DIRS[i].c_str()); // set working dir
			
			// read whole file into html_template
			std::string ytmp;
			while (!fin.eof()) 
			{
				getline(fin, ytmp);
				html_template = html_template + ytmp + "\r\n";
			}
			yresult += cgi_cmd_parsing(request, html_template, ydebug); // parsing engine
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
	return yresult;
}

//-------------------------------------------------------------------------
// main parsing (nested and recursive)
//-------------------------------------------------------------------------
std::string  CyAPI::cgi_cmd_parsing(CWebserverRequest* request, std::string html_template, bool ydebug)
{
	unsigned int start, end, esc_len = strlen(YCGI_ESCAPE_START);
	bool is_cmd;
	std::string ycmd,yresult;
	
	do // replace all {=<cmd>=} nested and recursive
	{
		is_cmd=false;
		if((end = html_template.find(YCGI_ESCAPE_END)) != std::string::npos) 				// 1. find first y-end
		{
			if(ydebug) request->printf("[ycgi debug]: END at:%d following:%s<br/>\n", end, (html_template.substr(end, 10)).c_str() );
			if((start = html_template.rfind(YCGI_ESCAPE_START, end)) != std::string::npos) 		// 2. find next y-start befor
			{
				if(ydebug) request->printf("[ycgi debug]: START at:%d following:%s<br/>\n", start, (html_template.substr(start+esc_len, 10)).c_str() );

				ycmd = html_template.substr(start+esc_len,end - (start+esc_len)); 	//3. get cmd
				if(ydebug) request->printf("[ycgi debug]: CMD:[%s]<br/>\n", ycmd.c_str());
				yresult = YWeb_cgi_cmd( request, ycmd ); 				// 4. execute cmd
				if(ydebug) request->printf("[ycgi debug]: RESULT:[%s]<br/>\n", yresult.c_str());
				html_template.replace(start,end - start + esc_len, yresult); 		// 5. replace cmd with output
				is_cmd = true;	// one command found
				
				if(ydebug) request->printf("[ycgi debug]: STEP<br/>\n%s<br/>\n", html_template.c_str() );
			}
		}
	}
	while(is_cmd);	
	return html_template;
}
// =============================================================================================
// ycmd - Dispatching
// =============================================================================================

//-------------------------------------------------------------------------
// ycgi : cmd executing 
//	script:<scriptname without .sh>
//	include:<filename>
//	func:<funcname> (funcname to be implemented in CyAPI::YWeb_cgi_func)
//	ini-get:<filename>;<varname>[;<default>][~open|cache]
//	ini-set:<filename>;<varname>;<value>[~open|save|cache]
//	if-empty:<value>~<then>~<else>
//	if-equal:<left_value>~<right_value>~<then>~<else> (left_value == right_value?)
//	if-not-equal:<left_value>~<right_value>~<then>~<else> (left_val!e == right_value?)
//	if-file-exists:<filename>~<then>~<else>
//	include-block:<filename>;<block-name>[;<default-text>]
//	var-get:<varname>
//	var-set:<varname>=<varvalue>
//	global-var-get:<varname>
//	global-var-set:<varname>=<varvalue>
//-------------------------------------------------------------------------

std::string  CyAPI::YWeb_cgi_cmd(CWebserverRequest* request, std::string ycmd)
{
	std::string ycmd_type, ycmd_name, yresult;

	if (ySplitString(ycmd,":",ycmd_type,ycmd_name))
	{
		if(ycmd_type == "script")
			yresult = Parent->ControlAPI->YexecuteScript( request, ycmd_name);
		else if(ycmd_type == "if-empty")
		{
			std::string if_value, if_then, if_else;
			if(ySplitString(ycmd_name,"~",if_value,if_then))
			{
				ySplitString(if_then,"~",if_then,if_else);
				yresult = (if_value == "") ? if_then : if_else;
			}
		}
		else if(ycmd_type == "if-equal")
		{
			std::string if_left_value, if_right_value, if_then, if_else;
			if(ySplitString(ycmd_name,"~",if_left_value,if_right_value))
			{
				if(ySplitString(if_right_value,"~",if_right_value,if_then))
				{
					ySplitString(if_then,"~",if_then,if_else);
					yresult = (if_left_value == if_right_value) ? if_then : if_else;
				}
			}
		}
		else if(ycmd_type == "if-not-equal")
		{
			std::string if_left_value, if_right_value, if_then, if_else;
			if(ySplitString(ycmd_name,"~",if_left_value,if_right_value))
			{
				if(ySplitString(if_right_value,"~",if_right_value,if_then))
				{
					ySplitString(if_then,"~",if_then,if_else);
					yresult = (if_left_value != if_right_value) ? if_then : if_else;
				}
			}
		}
		else if(ycmd_type == "if-file-exists")
		{
			std::string if_value, if_then, if_else;
			if(ySplitString(ycmd_name,"~",if_value,if_then))
			{
				ySplitString(if_then,"~",if_then,if_else);
				yresult = (access(if_value.c_str(), 4) == 0) ? if_then : if_else;
			}
		}
		else if(ycmd_type == "include")
		{
			std::string ytmp;
			std::fstream fin(ycmd_name.c_str(), std::fstream::in);
			if(fin.good())
			{
				while (!fin.eof())
				{
					getline(fin, ytmp);
					yresult += ytmp +"\n";
				}
				fin.close();
			}
		}
		else if(ycmd_type == "include-block")
		{
			std::string filename, blockname, tmp, ydefault;
			if (ySplitString(ycmd_name,";",filename,tmp))
			{
				ySplitString(tmp,";",blockname, ydefault);
				yresult = YWeb_cgi_include_block(filename, blockname, ydefault);
			}
		}
		else if(ycmd_type == "func")
			yresult = YWeb_cgi_func(request, ycmd_name);
		else if(ycmd_type == "ini-get")
		{
			std::string filename, varname, tmp, ydefault, yaccess;
			ySplitString(ycmd_name,"~",filename,yaccess);
			if (ySplitString(filename,";",filename,tmp))
			{
				ySplitString(tmp,";",varname, ydefault);
				yresult = YWeb_cgi_get_ini(request, filename, varname, yaccess);
				if(yresult == "" && ydefault != "")
					yresult = ydefault;
			}
			else
				yresult = "ycgi: ini-get: no ; found";
		}
		else if(ycmd_type == "ini-set")
		{
			std::string filename, varname, varvalue, tmp, yaccess;
			ySplitString(ycmd_name,"~",filename,yaccess);
			if (ySplitString(filename,";",filename,tmp))
			{
				if(ySplitString(tmp,";",varname, varvalue))
					YWeb_cgi_set_ini(request, filename, varname, varvalue, yaccess);
			}
			else
				yresult = "ycgi: ini-get: no ; found";
		}
		else if(ycmd_type == "var-get")
		{
			yresult = request->ycgi_vars[ycmd_name];
		}
		else if(ycmd_type == "var-set")
		{
			std::string varname, varvalue;
			if (ySplitString(ycmd_name,"=",varname,varvalue))
				request->ycgi_vars[varname] = varvalue;
		}
		else if(ycmd_type == "global-var-get")
		{
			yresult = ycgi_global_vars[ycmd_name];
		}
		else if(ycmd_type == "global-var-set")
		{
			std::string varname, varvalue;
			if (ySplitString(ycmd_name,"=",varname,varvalue))
				ycgi_global_vars[varname] = varvalue;
		}
		else if(ycmd_type == "file-action")
		{
			std::string filename, actionname, content, tmp, ydefault;
			if (ySplitString(ycmd_name,";",filename,tmp))
			{
				ySplitString(tmp,";",actionname, content);
				if(actionname == "add")
				{
					std::fstream fout(filename.c_str(), std::fstream::out );
					fout << content;
					fout.close();
				}
				else
				if(actionname == "append")
				{
					std::fstream fout(filename.c_str(), std::fstream::app );
					fout << content;
					fout.close();
				}
				else
				if(actionname == "delete")
				{
					remove(filename.c_str());
				}
			}
		}
		else
			yresult = "ycgi-type unknown";
	}
	else if (request->ParameterList[ycmd] != "")
		yresult = request->ParameterList[ycmd];

	return yresult;
}

//-------------------------------------------------------------------------
// Get Value from ini/conf-file (filename) for var (varname)
// yaccess = open | cache 
//-------------------------------------------------------------------------
std::string  CyAPI::YWeb_cgi_get_ini(CWebserverRequest *request, std::string filename, std::string varname, std::string yaccess)
{
//	aprintf("ini-get: var:%s yaccess:(%s)\n", varname.c_str(), yaccess.c_str());
	std::string result;
	if((yaccess == "open") || (yaccess == ""))
	{
		request->yConfig->clear();
		request->yConfig->loadConfig(filename);
	}
	result = request->yConfig->getString(varname, "");
	return result;
}

//-------------------------------------------------------------------------
// set Value from ini/conf-file (filename) for var (varname)
// yaccess = open | cache | save
//-------------------------------------------------------------------------
void  CyAPI::YWeb_cgi_set_ini(CWebserverRequest *request, std::string filename, std::string varname, std::string varvalue, std::string yaccess)
{
//	aprintf("ini-set: var:%s yaccess:(%s)\n", varname.c_str(), yaccess.c_str());
	std::string result;
	if((yaccess == "open") || (yaccess == ""))
	{
		request->yConfig->clear();
		request->yConfig->loadConfig(filename);
	}
	request->yConfig->setString(varname, varvalue);
	if((yaccess == "save") || (yaccess == ""))
		request->yConfig->saveConfig(filename);
}

//-------------------------------------------------------------------------
// Get text block named <blockname> from file <filename>
// The textblock starts with "start-block~<blockname>" and ends with
// "end-block~<blockname>"
//-------------------------------------------------------------------------
std::string  CyAPI::YWeb_cgi_include_block(std::string filename, std::string blockname, std::string ydefault)
{
	std::string ytmp, yfile, yresult;
	struct stat attrib;
	yresult = ydefault;

	stat(filename.c_str(), &attrib);
	if( (attrib.st_mtime == yCached_blocks_attrib.st_mtime) && (filename == yCached_blocks_filename) )
		yfile = yCached_blocks_content;
	else
	{
		std::fstream fin(filename.c_str(), std::fstream::in);
		if(fin.good())
		{
			while (!fin.eof()) // read whole file into yfile 
			{
				getline(fin, ytmp);
				yfile += ytmp+"\n";
			}
			fin.close();
		}
		yCached_blocks_content = yfile;
		yCached_blocks_attrib = attrib;
		yCached_blocks_filename = filename;
	}
	if(yfile.length() != 0)
	{
		std::string t = "start-block~"+blockname;
		unsigned int start, end;
		if((start = yfile.find(t)) != std::string::npos)
		{
			if((end = yfile.find("end-block~"+blockname, start+t.length())) != std::string::npos)
			{
				yresult = yfile.substr(start+t.length(),end - (start+t.length()) );
			}
		}
	}
	else
		aprintf("include-blocks: file not found:%s\n",filename.c_str() );

	return yresult;
}

// =============================================================================================
// y-func : Dispatching
// =============================================================================================

//-------------------------------------------------------------------------
// y-func : dispatching and executing 
//-------------------------------------------------------------------------

std::string  CyAPI::YWeb_cgi_func(CWebserverRequest* request, std::string ycmd)
{
	std::string func, para, yresult;

	int operation = 0;

	const char *operations[] = {
		"mount-get-list", "mount-set-values", 
		"get_bouquets_as_dropdown", "get_actual_bouquet_number", "get_channels_as_dropdown", "get_actual_channel_id",
		"get_mode", "get_video_pids", "get_audio_pid", "get_audio_pids_as_dropdown", "get_request_data",
		"umount_get_list", "do_reload_nhttpd_config", "get_partition_list", "get_boxtype", "nhttpd_change",
		 NULL};

	ySplitString(ycmd," ",func, para);
	aprintf("ycgi [rnr:%ld] func dispatcher func:[%s] para:[%s]",request->RequestNumber, func.c_str(),para.c_str());

	while (operations[operation]) {
		if (func.compare(operations[operation]) == 0) {
			break;
		}
		operation++;
	}

	if (operations[operation] == NULL) {
		yresult = "ycgi func not found";
	} else
	switch(operation)
	{
		case 0:	yresult = func_mount_get_list();
			break;
			
		case 1:	yresult = func_mount_set_values(request);
			break;
			
		case 2:	yresult = func_get_bouquets_as_dropdown(para);
			break;
			
		case 3:	yresult = func_get_actual_bouquet_number();
			break;
			
		case 4:	yresult = func_get_channels_as_dropdown(para);
			break;
			
		case 5:	yresult = func_get_actual_channel_id();
			break;
			
		case 6:	yresult = func_get_mode();
			break;
			
		case 7:	yresult = func_get_video_pids(para);
			break;
			
		case 8:	yresult = func_get_radio_pid();
			break;
			
		case 9:	yresult = func_get_audio_pids_as_dropdown(para);
			break;
			
		case 10:yresult = func_get_request_data(request, para);
			break;
		
		case 11:yresult = func_unmount_get_list();
			break;
		
		case 12:yresult = func_do_reload_nhttpd_config(request);
			break;
		
		case 13:yresult = func_get_partition_list();
			break;
		
		case 14:yresult = func_get_boxtype();
			break;

		case 15:yresult = func_change_nhttpd(request,para);
			break;

		default:
			yresult = "ycgi func not found";
	}
	aprintf(" [rnr:%ld] result:[%s]\n",request->RequestNumber,yresult.c_str());
	return yresult;
}

//-------------------------------------------------------------------------
// y-func : mount_get_list
//-------------------------------------------------------------------------
std::string  CyAPI::func_mount_get_list()
{
	CConfigFile *Config = new CConfigFile(',');
	std::string ysel, ytype, yip, ylocal_dir, ydir, ynr, yresult;
	int yitype;
	char ytmp[100];

	Config->loadConfig(NEUTRINO_CONFIGFILE);
	for(unsigned int i=0; i <= 7; i++)
	{
		ynr=itoa(i);
		ysel = ((i==0) ? "checked=\"checked\"" : "");
		yitype = Config->getInt32("network_nfs_type_"+ynr,0);
		ytype = ( (yitype==0) ? "NFS" :((yitype==1) ? "CIFS" : "FTPFS") ); 
		yip = Config->getString("network_nfs_ip_"+ynr,"");
		ydir = Config->getString("network_nfs_dir_"+ynr,"");
		ylocal_dir = Config->getString("network_nfs_local_dir_"+ynr,"");
		if(ydir != "") 
			ydir="("+ydir+")";

		sprintf(ytmp,"<input type='radio' name='R1' value='%d' %s />%d %s - %s %s %s<br/>",
			i,ysel.c_str(),i,ytype.c_str(),yip.c_str(),ylocal_dir.c_str(), ydir.c_str());

		yresult += ytmp;
	}
	delete Config;
	return yresult;
}

//-------------------------------------------------------------------------
// y-func : mount_set_values
//-------------------------------------------------------------------------
std::string  CyAPI::func_mount_set_values(CWebserverRequest* request)
{
	CConfigFile *Config = new CConfigFile(',');
	std::string ynr, yresult;

	Config->loadConfig(NEUTRINO_CONFIGFILE);
	ynr = request->ParameterList["nr"];
	Config->setString("network_nfs_type_"+ynr,request->ParameterList["type"]);
	Config->setString("network_nfs_ip_"+ynr,request->ParameterList["ip"]);
	Config->setString("network_nfs_dir_"+ynr,request->ParameterList["dir"]);
	Config->setString("network_nfs_local_dir_"+ynr,request->ParameterList["localdir"]);
	Config->setString("network_nfs_mac_"+ynr,request->ParameterList["mac"]);
	Config->setString("network_nfs_mount_options1_"+ynr,request->ParameterList["opt1"]);
	Config->setString("network_nfs_mount_options2_"+ynr,request->ParameterList["opt2"]);
	Config->setString("network_nfs_automount_"+ynr,request->ParameterList["automount"]);
	Config->setString("network_nfs_username_"+ynr,request->ParameterList["username"]);
	Config->setString("network_nfs_password_"+ynr,request->ParameterList["password"]);
	Config->saveConfig(NEUTRINO_CONFIGFILE);

	delete Config;
	return yresult;
}
//-------------------------------------------------------------------------
// y-func : get_bouquets_as_dropdown [<bouquet>]
//-------------------------------------------------------------------------
std::string  CyAPI::func_get_bouquets_as_dropdown(std::string para)
{
	std::string ynr, yresult, sel, nr_str, do_show_hidden;
	char buf[60];
	unsigned int nr=1;
	
	ySplitString(para," ",nr_str, do_show_hidden);
	if(nr_str != "")
		nr = atoi(nr_str.c_str());
	
	for (unsigned int i = 0; i < Parent->BouquetList.size();i++)
	{	
		sel=(nr==(i+1)) ? "selected=\"selected\"" : "";
		if(!Parent->BouquetList[i].hidden || do_show_hidden == "true")
		{
			sprintf(buf,"<option value=%u %s>%s</option>\n", (Parent->BouquetList[i].bouquet_nr) + 1, sel.c_str(), Parent->BouquetList[i].name);
			yresult += buf;
		}
	}
/* NOT activated in R1.3.4 - still work on getPIDS for not current channel	
	// Transponder View
	sel=(nr==0) ? "selected" : "";
	sprintf(buf,"<option value=0 %s>Transponder</option>\n", sel.c_str());
	yresult += buf;
*/	
	return yresult;
}

//-------------------------------------------------------------------------
// y-func : get_actual_bouquet_number
//-------------------------------------------------------------------------
std::string  CyAPI::func_get_actual_bouquet_number()
{
	CZapitClient::BouquetChannelList *bouquet;
	int actual=0;
	int mode = CZapitClient::MODE_CURRENT;

	for (unsigned int i = 0; i < Parent->BouquetList.size() && actual == 0;i++)
	{
		bouquet = Parent->GetBouquet((Parent->BouquetList[i].bouquet_nr) + 1, mode);
		CZapitClient::BouquetChannelList::iterator channel = bouquet->begin();
		for (unsigned int j = 0; channel != bouquet->end() && actual == 0; channel++,j++)
		{
			if(channel->channel_id == Parent->Zapit->getCurrentServiceID())
				actual=i+1;
		}
	}
	return std::string(itoa(actual));
}

//-------------------------------------------------------------------------
// y-func : get_channel_dropdown [<bouquet_nr> [<channel_id>]]
//-------------------------------------------------------------------------
std::string  CyAPI::func_get_channels_as_dropdown(std::string para)
{
	CZapitClient::BouquetChannelList *bouquet;
	CZapitClient::BouquetList blist;
	std::string abouquet, achannel_id, yresult, sel;
	char buf[100],id[20];
	
	int bnumber = 1;
	int mode = CZapitClient::MODE_CURRENT;
	
	ySplitString(para," ",abouquet, achannel_id);
	if(abouquet != "")
		bnumber = atoi(abouquet.c_str());
	
	if(bnumber != 0) //Bouquet View
	{
		bouquet = Parent->GetBouquet(bnumber, mode);
		CZapitClient::BouquetChannelList::iterator channel = bouquet->begin();
		CEPGData epg;
		std::string sid = id;
		
		for (unsigned int i = 0; channel != bouquet->end(); channel++,i++)
		{	
			sprintf(id,PRINTF_CHANNEL_ID_TYPE_NO_LEADING_ZEROS,channel->channel_id);
			sid = std::string(id);

			sel = (sid == achannel_id) ? "selected=\"selected\"" : "";
			Parent->Sectionsd->getActualEPGServiceKey(channel->channel_id, &epg);
			sprintf(buf,"<option value="PRINTF_CHANNEL_ID_TYPE_NO_LEADING_ZEROS" %s>%.20s - %.30s</option>\n", channel->channel_id, sel.c_str(), channel->name,epg.title.c_str());
			yresult += buf;
		}
	}
	else // Transponder View		
	{
		bouquet = Parent->GetChannelList(mode);
		CZapitClient::BouquetChannelList::iterator channel = bouquet->begin();
		CEPGData epg;
		std::string sid = id;
		t_channel_id actual_channel=Parent->Zapit->getCurrentServiceID();
		for (unsigned int i = 0; channel != bouquet->end(); channel++,i++)
		{	
			if((channel->channel_id >> 64)==(actual_channel >> 64))
			{
				sprintf(id,PRINTF_CHANNEL_ID_TYPE_NO_LEADING_ZEROS,channel->channel_id);
				sid = std::string(id);
				sel = (sid == achannel_id) ? "selected=\"selected\"" : "";
			
				Parent->Sectionsd->getActualEPGServiceKey(channel->channel_id, &epg);
				sprintf(buf,"<option value="PRINTF_CHANNEL_ID_TYPE_NO_LEADING_ZEROS" %s>%.20s - %.30s</option>\n", channel->channel_id, sel.c_str(), channel->name,epg.title.c_str());
				yresult += buf;
			}
		}
	}		
	return yresult;
}
//-------------------------------------------------------------------------
// y-func : get_actual_channel_id
//-------------------------------------------------------------------------
std::string  CyAPI::func_get_actual_channel_id()
{
	char buf[100];
	sprintf(buf,PRINTF_CHANNEL_ID_TYPE_NO_LEADING_ZEROS,Parent->Zapit->getCurrentServiceID());	
	return std::string(buf);
}
//-------------------------------------------------------------------------
// y-func : get_mode (returns tv|radio|unknown)
//-------------------------------------------------------------------------
std::string  CyAPI::func_get_mode()
{
	std::string yresult;
	int mode = Parent->Zapit->getMode();
	
	if ( mode == CZapitClient::MODE_TV)
		yresult = "tv";
	else if ( mode == CZapitClient::MODE_RADIO)
		yresult = "radio";
	else
		yresult = "unknown";
	return yresult;
}

//-------------------------------------------------------------------------
// y-func : get_video_pids (para=audio channel, returns: 0x0000,0x0000,0x0000)
//-------------------------------------------------------------------------
std::string  CyAPI::func_get_video_pids(std::string para)
{
	char buf[30];
	CZapitClient::responseGetPIDs pids;
	int apid=0,apid_no=0,apid_idx=0;
	pids.PIDs.vpid=0;
	
	if(para != "")
		apid_no = atoi(para.c_str());
	Parent->Zapit->getPIDS(pids);

	if( apid_no < (int)pids.APIDs.size())
		apid_idx=apid_no;
	if(!pids.APIDs.empty())
		apid = pids.APIDs[apid_idx].pid;
	sprintf(buf,"0x%04x,0x%04x,0x%04x",pids.PIDs.pmtpid,pids.PIDs.vpid,apid);
	return std::string(buf);
}
//-------------------------------------------------------------------------
// y-func : get_radio_pids (returns: 0x0000)
//-------------------------------------------------------------------------
std::string  CyAPI::func_get_radio_pid()
{
	char buf[20];
	CZapitClient::responseGetPIDs pids;
	int apid=0;
	
	Parent->Zapit->getPIDS(pids);
	if(!pids.APIDs.empty())
		apid = pids.APIDs[0].pid;
	
	sprintf(buf,"0x%04x",apid);
	return std::string(buf);
}

//-------------------------------------------------------------------------
// y-func : get_audio_pids_as_dropdown (from controlapi)
// prara: [apid] option value = apid-Value. Default apid-Index
//-------------------------------------------------------------------------
std::string  CyAPI::func_get_audio_pids_as_dropdown(std::string para)
{
	std::string yresult;
	char buf[60];
	static bool init_iso=true;
	bool idx_as_id=true;
	
	if(para == "apid") 
		idx_as_id=false;
	if(init_iso)
	{
		if(initialize_iso639_map())
			init_iso=false;
	}
	bool eit_not_ok=true;
	CZapitClient::responseGetPIDs pids;

	CSectionsdClient::ComponentTagList tags;
	pids.PIDs.vpid=0;
	Parent->Zapit->getPIDS(pids);


	t_channel_id current_channel = Parent->Zapit->getCurrentServiceID();
	CSectionsdClient::responseGetCurrentNextInfoChannelID currentNextInfo;
	Parent->Sectionsd->getCurrentNextServiceKey(current_channel, currentNextInfo);
	if (Parent->Sectionsd->getComponentTagsUniqueKey(currentNextInfo.current_uniqueKey,tags))
	{
		for (unsigned int i=0; i< tags.size(); i++)
		{
			for (unsigned short j=0; j< pids.APIDs.size(); j++)
			{
				if ( pids.APIDs[j].component_tag == tags[i].componentTag )
				{
 					if(!tags[i].component.empty())
					{
						if(!(isalnum(tags[i].component[0])))
							tags[i].component=tags[i].component.substr(1,tags[i].component.length()-1);
						sprintf(buf,"<option value=%05u>%s</option>\r\n",idx_as_id ? j : pids.APIDs[j].pid,tags[i].component.c_str());
					}
					else
					{
						if(!(init_iso))
						{
							strcpy( pids.APIDs[j].desc, getISO639Description( pids.APIDs[j].desc ) );
						}
			 			sprintf(buf,"<option value=%05u>%s %s</option>\r\n",idx_as_id ? j : pids.APIDs[j].pid,pids.APIDs[j].desc,pids.APIDs[j].is_ac3 ? " (AC3)": " ");
					}
					yresult += buf;
					eit_not_ok=false;
					break;
				}
			}
		}
	}
	if(eit_not_ok)
	{
		unsigned short i = 0;
		for (CZapitClient::APIDList::iterator it = pids.APIDs.begin(); it!=pids.APIDs.end(); it++)
		{
			if(!(init_iso))
			{
				strcpy( pids.APIDs[i].desc, getISO639Description( pids.APIDs[i].desc ) );
			}
 			sprintf(buf,"<option value=%05u>%s %s</option>\r\n",idx_as_id ? i : it->pid,pids.APIDs[i].desc,pids.APIDs[i].is_ac3 ? " (AC3)": " ");
			yresult += buf;
			i++;
		}
	}

	if(pids.APIDs.empty())
		yresult = "00000"; // shouldnt happen, but print at least one apid
	return yresult;
}

//-------------------------------------------------------------------------
// y-func : get_actual_request_data
//-------------------------------------------------------------------------
std::string  CyAPI::func_get_request_data(CWebserverRequest* request, std::string para)
{
	std::string yresult;
	if(para == "host")
		yresult = request->Host;
	else if (para == "url")
		yresult = request->URL;
	else if (para == "path")
		yresult = request->Path;
	else if (para == "filename")
		yresult = request->Filename;
	else if (para == "fileext")
		yresult = request->FileExt;
	else if (para == "client_addr")
		yresult = request->Client_Addr;
	else if (para == "url")
		yresult = request->URL;
	return yresult;
}

//-------------------------------------------------------------------------
// y-func : build umount list
//-------------------------------------------------------------------------
std::string  CyAPI::func_unmount_get_list()
{
	std::string ysel, ymount, ylocal_dir, yfstype, ynr, yresult, mounts;
	char ytmp[200];
	
	std::ifstream in;
	in.open("/proc/mounts", std::ifstream::in);
	int j=0;
	while(in.good() && (j<8))
	{
		yfstype="";
		in >> ymount >> ylocal_dir >> yfstype;
		in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		yfstype = trim(yfstype);
		if( (yfstype == "nfs") << (yfstype == "ftp") || (yfstype == "lufsd") )
		{
			mounts=ylocal_dir +" on "+ ymount + " ("+yfstype+")";
			ysel = ((j==0) ? "checked=\"checked\"" : "");
			sprintf(ytmp,"<input type='radio' name='R1' value='%s' %s />%d %.120s<br/>",
				ylocal_dir.c_str(),ysel.c_str(),j,mounts.c_str());

			yresult += ytmp;
			j++;
		}
	}
	return yresult;
}
//-------------------------------------------------------------------------
// y-func : Reload the nhttpd.conf
//-------------------------------------------------------------------------
std::string  CyAPI::func_do_reload_nhttpd_config(CWebserverRequest* request)
{
	Parent->Parent->ReadConfig();
	return "";
}

//-------------------------------------------------------------------------
// y-func : build partition list
//-------------------------------------------------------------------------
std::string  CyAPI::func_get_partition_list()
{
	std::string ysel, ymtd, yname, dummy, yresult;
	char ytmp[200];
	
	std::ifstream in;
	in.open("/proc/mtd", std::ifstream::in);
	int j=0;
	while(in.good() && (j<8))
	{
		ymtd="";
		in >> ymtd >> dummy >> dummy; //format:  mtd# start end "name  "
		in.getline(ytmp, 200); // Rest of line is the mtd description
		yname = ytmp;
		if((j>0) && (ymtd != ""))// iggnore first line
		{
			ysel = ((j==1) ? "checked=\"checked\"" : "");
			sprintf(ytmp,"<input type='radio' name='R1' value='%d' %s title='%s' />%d %s<br/>",
				j-1,ysel.c_str(),yname.c_str(),j-1,yname.c_str());
			yresult += ytmp;
		}
		j++;
	}
	return yresult;
}
//-------------------------------------------------------------------------
// y-func : get boxtypetext (Nokia, Philips, Sagem)
//-------------------------------------------------------------------------
std::string  CyAPI::func_get_boxtype()
{
	return Parent->Dbox_Hersteller[Parent->Controld->getBoxType()];
}
//-------------------------------------------------------------------------
// y-func : Change nhttpd (process image) on the fly
//-------------------------------------------------------------------------
std::string  CyAPI::func_change_nhttpd(CWebserverRequest* request, std::string para)
{
	if(para != "" && access(para.c_str(), 4) == 0)
	{
		request->EndRequest();
		int err = execvp(para.c_str(), NULL); // no return if successful
		return "ERROR [change_httpd]: execvp returns error code: "+err;
	}
	
	else
	return "ERROR [change_httpd]: para has not path to a file";
}

