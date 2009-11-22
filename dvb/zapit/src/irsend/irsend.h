/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2002 Sven Traenkle 'Zwen'
	License: GPL

*/
#ifndef __irsend__
#define __irsend__

#include <string>

class CIRSend
{
   public:
		CIRSend(const char * const configfile);
		bool Send();
   private:
		std::string m_configFile;
};
#endif
