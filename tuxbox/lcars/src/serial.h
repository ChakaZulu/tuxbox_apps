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

#ifndef SERIAL_H
#define SERIAL_H

#include <termios.h>
#include <pthread.h>

#include "container.h"

class serial
{
	struct termios oldtio;
	pthread_t thread;

public:
	container cont;
	serial(container &container);
	int fd;
	void init();
	static void *startlistening(void *object);
	void startThread();
	bool char_available();
	void end();
};

#endif
