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
$Log: checker.h,v $
Revision 1.2  2001/11/15 00:43:45  TheDOC
 added

*/
#ifndef CHECKER_H
#define CHECKER_H

#include "zap.h"



class checker
{
	pthread_t timeThread;
 
    static void* start_16_9_checker( void * );
	
public:	
	int start_16_9_thread();
	void run_checker();
	void set_16_9_mode(int mode);
	int get_16_9_mode();
};

#endif
