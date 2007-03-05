/*
 *
 * xmlinterface for neutrino - d-box2 linux project
 *
 * (C) 2002 by thegoodguy <thegoodguy@berlios.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <cstdio>

#include <xmlinterface.h>

#ifdef USE_LIBXML
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/parserInternals.h>
#else  /* USE_LIBXML */
#include <xmltok.h>
#endif /* USE_LIBXML */

#ifdef USE_LIBXML
xmlDocPtr parseXml(const char * data)
{
	xmlDocPtr doc;
	xmlNodePtr cur;

	doc = xmlParseMemory(data, strlen(data));

	if (doc == NULL)
	{
		WARN("Error parsing VLC Answer");
		return NULL;
	}
	else
	{
		cur = xmlDocGetRootElement(doc);
		if (cur == NULL)
		{
			WARN("Empty document\n");
			xmlFreeDoc(doc);
			return NULL;
		}
		else
			return doc;
	}
}
#else /* USE_LIBXML */
xmlDocPtr parseXml(const char * data)
{
	XMLTreeParser* tree_parser;

	tree_parser = new XMLTreeParser(NULL);

	if (!tree_parser->Parse(data, strlen(data), true))
	{
			printf("Error parsing \"VLC Answer\": %s at line %d\n",
			       tree_parser->ErrorString(tree_parser->GetErrorCode()),
			       tree_parser->GetCurrentLineNumber());

			delete tree_parser;
			return NULL;
		}

	if (!tree_parser->RootNode())
	{
        printf("Error: No Root Node\n");
		delete tree_parser;
		return NULL;
	}
	return tree_parser;
}
#endif /* USE_LIBXML */

