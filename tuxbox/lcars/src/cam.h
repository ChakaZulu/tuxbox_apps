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
$Log: cam.h,v $
Revision 1.3  2002/03/03 22:57:59  TheDOC
lcars 0.20

Revision 1.2  2001/12/16 22:36:05  tux
IP Eingaben erweitert

Revision 1.2  2001/11/15 00:43:45  TheDOC
 added

*/
#ifndef CAM_H
#define CAM_H

class cam
{
	void sendCAM(void *command, unsigned int len);
	
	unsigned short ONID;
	unsigned short TS;
	unsigned short SID;
	unsigned short pid_count;
	unsigned short PIDs[10];
	unsigned short CAID;
	unsigned short ECM;
	unsigned short EMM;
public:
	cam();

	void initialize();

	// These functions set the different PIDs
	void setONID(unsigned short PID) { ONID = PID; }
	void setTS(unsigned short PID) { TS = PID; }
	void setSID(unsigned short PID) { SID = PID; }
	void addPID(unsigned short PID) { PIDs[pid_count++] = PID; }
	void setCAID(unsigned short PID) { CAID = PID; }
	void setECM(unsigned short PID) { ECM = PID; }
	void setEMM(unsigned short PID) { EMM = PID; }

	void cam::cam_answer();

	bool isfree();

	// Returns the CAID - First the CAID has to be read by cam::readCAID()
	unsigned short getCAID() { return CAID; }

	// *** Change the methods below this point for your needs ***

	// Reads the CAID from the card
	void readCAID();

	// Inits the CAM
	void init();

	// Inits the CAM
	void init2();

	// Resets the CAM
	void reset();

	// Starts the descambling
	void start();

	// Starts the EMM-parsing with the previously set EMM-PID
	void startEMM();

	// Descrambles a channel with the previously set PIDs
	void descramble();

};

#endif
