/*
 * $Header: /cvs/tuxbox/apps/dvb/zapit/include/zapit/dvbstring.h,v 1.1 2002/10/06 22:18:25 thegoodguy Exp $
 *
 * Strings conforming to the DVB Standard - d-box2 linux project
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

#ifndef __dvbstring_h__
#define __dvbstring_h__

#include <string>

class CDVBString
{
	private:
		std::string content;

	public:
		CDVBString(const char * the_content, const int size)
			{
				int i;

				for (i = 0; i < size; i++)                            // skip initial encoding information
					if (((unsigned char)the_content[i]) >= 0x20)
						break;

				if (size - i == 0)
					content = "";
				else
					content = std::string(&(the_content[i]), size - i);
			};

		bool operator== (const CDVBString s)
			{
				return (this->content == s.content);
			};


		bool operator!= (const CDVBString s)
			{
				return !(operator==(s));
			};

		std::string           getContent()           { return content; };
// TODO:
// getEncoding()
};

#endif /* __dvbstring_h__ */
