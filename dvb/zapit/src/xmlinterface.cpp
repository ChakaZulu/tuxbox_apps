/*
 * $Header: /cvs/tuxbox/apps/dvb/zapit/src/Attic/xmlinterface.cpp,v 1.1 2002/09/18 13:26:18 thegoodguy Exp $
 */
#include "xmlinterface.h"

#ifdef MASK_SPECIAL_CHARACTERS
#include <stdio.h>
#endif

#include <stdio.h>

std::string convertForXML(const std::string s)
{
	std::string r;
	unsigned int i;
	for (i=0; i<s.length(); i++)
	{
		switch (s[i])          // cf. xml/xmltimpl.c: PREFIX(predefinedEntityName)
		{
		  case '<':           
			r += "&lt;";
			break;
		  case '>':
			r += "&gt;";
			break;
		  case '&':
			r += "&amp;";
			break;
		  case '\"':
			r += "&quot;";
			break;
		  case '\'':
			r += "&apos;";
			break;
		  default:
#ifdef MASK_SPECIAL_CHARACTERS
			if ((((unsigned char)s[i])>=32) && (((unsigned char)s[i])<128))
#endif
				r += s[i];
#ifdef MASK_SPECIAL_CHARACTERS
			else if (((unsigned char)s[i]) >= 128)
			{
				char val[5];
				sprintf(val, "%d", (unsigned char)s[i]);
				r = r + "&#" + val + ";";
			}
#endif
		}
	}
	return r;
}

std::string Utf8_to_Latin1(const std::string s)  // only works correct if we had latin1 in the parsed xml-file
{
	std::string r;
	unsigned int i;
	for (i=0; i<s.length(); i++)
	{
	    if ((i < s.length() - 1) && ((s[i] & 0xc0) == 0xc0))
	    {
		r += ((s[i] & 3) << 6) | (s[i + 1] & 0x3f);
		i++;
	    }
	    else r += s[i];
	}
	return r;
}
