/*
 * $Header: /cvs/tuxbox/apps/dvb/zapit/src/Attic/xmlinterface.cpp,v 1.2 2002/09/19 10:24:50 thegoodguy Exp $
 */

#include "xmlinterface.h"

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


XMLTreeParser* parseXmlFile(const std::string filename)
{
	char buffer[2048];
	XMLTreeParser* tree_parser;
	size_t done;
	size_t length;
	FILE* xml_file;

	xml_file = fopen(filename.c_str(), "r");

	if (xml_file == NULL)
	{
		perror(filename.c_str());
		return NULL;
	}

	tree_parser = new XMLTreeParser("ISO-8859-1");

	do
	{
		length = fread(buffer, 1, sizeof(buffer), xml_file);
		done = length < sizeof(buffer);

		if (!tree_parser->Parse(buffer, length, done))
		{
			printf("[xmlinterface.cpp] Error parsing \"%s\": %s at line %d\n",
			       filename.c_str(),
			       tree_parser->ErrorString(tree_parser->GetErrorCode()),
			       tree_parser->GetCurrentLineNumber());

			fclose(xml_file);
			delete tree_parser;
			return NULL;
		}
	}
	while (!done);

	fclose(xml_file);

	if (!tree_parser->RootNode())
	{
		delete tree_parser;
		return NULL;
	}
	return tree_parser;
}
