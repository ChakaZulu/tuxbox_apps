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
$Log: pmt.h,v $
Revision 1.2  2001/11/15 00:43:45  TheDOC
 added

*/
#ifndef PMT_H
#define PMT_H

#include <map>

struct pmt_data
{
	int sid;
	int pid_counter;
	int type[10];
	int PID[10];
	int subtype[10]; // 0 - nichts, 1 - vtxt, 2 - ac3
	int component[10];
	int ecm_counter;
	int CAID[10];
	int ECM[10];
	int PCR;
};

class pmt
{

public:	
	pmt_data readPMT(int pmt_pid);
};

#endif
