/***************************************************************************
    copyright            : (C) 2001 by TheDOC
    email                : thedoc@chatville.de
	homepage			 : www.chatville.de
	modified by			 : -
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/*
$Log: network.h,v $
Revision 1.3  2001/12/11 13:38:44  TheDOC
new cdk-path-variables, about 10 new features and stuff

Revision 1.2  2001/11/15 00:43:45  TheDOC
 added

*/
#ifndef NETWORK_H
#define NETWORK_H

#include <termios.h>
#include <pthread.h>

#include "container.h"
#include "xmlrpc.h"

#define PORT 80

class network
{
	pthread_t thread;
	
public:
	xmlrpc xmlrpc_obj;
	container cont;
	void writetext(std::string text);
	network(container &container);
	int fd;
	int inbound_connection;
	static void *startlistening(void *object);
	void startThread();
};

#endif
