/*      
        nhttpd  -  DBoxII-Project

        Copyright (C) 2001/2002 Dirk Szymanski 'Dirch'

        $Id: webserver.h,v 1.24 2005/09/10 12:30:33 yjogol Exp $

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
	// Revision 1.2  22.03.2002 20:20  dirch
	// Revision 2.0b 20.09.2002 16:20  dirch
	// Revision 2.0c 14.03.2003 05:00  obi
	// Revision 2.1a 28.08.2005        yjogol

*/

#ifndef __nhttpd_webserver_h__
#define __nhttpd_webserver_h__

// c++
#include <string>

// system
#include <netinet/in.h>

#define SA	struct sockaddr
#define SAI	struct sockaddr_in

#define NHTTPD_VERSION "2.1a"

#define PRIVATEDOCUMENTROOT	"/share/tuxbox/neutrino/httpd-y"
#define PUBLICDOCUMENTROOT	"/var/httpd"

class CWebDbox;
class TWebserverRequest;

struct Tmconnect
{
	int sock_fd;
	SAI servaddr;
};

class CWebserver
{
protected:
	int			Port;
	int			ListenSocket;
	bool			THREADS;

public:
	// config vars / switches
	bool			STOP;
	bool			VERBOSE;
	bool			MustAuthenticate;
	bool			NewGui;

	std::string		PrivateDocumentRoot;
	std::string		PublicDocumentRoot;
	std::string		Zapit_XML_Path;

	std::string		AuthUser;
	std::string		AuthPassword;

	CWebDbox		*WebDbox;

	CWebserver(bool debug);
	~CWebserver(void);

	bool Init(bool debug);
	bool Start(void);
	void DoLoop(void);
	void Stop(void);

	int SocketConnect(Tmconnect *con, int Port);
	void SetSockOpts(void);
	void ReadConfig(void);

	friend class CWebserverRequest;
	friend class TWebDbox;
};

#endif /* __nhttpd_webserver_h__ */
