/*
 * $Header: /cvs/tuxbox/apps/dvb/zapit/include/zapit/dvbstring.h,v 1.3 2002/10/08 10:01:35 thegoodguy Exp $
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

#include "xmlinterface.h"

class CDVBString
{
 private:
	enum t_encoding {
		ISO_8859_5     = 0x01,
		ISO_8859_6     = 0x02,
		ISO_8859_7     = 0x03,
		ISO_8859_8     = 0x04,
		ISO_8859_9     = 0x05,
		NONE_SPECIFIED = 0x20
	};

	t_encoding encoding;

	std::string content;

	void add_character(unsigned char character)
		{
			int character_unicode_value = character;
			switch (encoding)
			{
			case ISO_8859_5:
				if ((character >= 0xA1) && (character != 0xAD))
				{
					switch (character)
					{
					case 0xF0:
						character_unicode_value = 0x2116;
						break;
					case 0xFD:
						character_unicode_value = 0x00A7;
						break;
					default:
						character_unicode_value += (0x0401 - 0xA1);
						break;
					}
				}
				break;

			case ISO_8859_9:
				switch (character)
				{
				case 0xD0:
					character_unicode_value = 0x011E;
					break;
				case 0xDD:
					character_unicode_value = 0x0130;
					break;
				case 0xDE:
					character_unicode_value = 0x015E;
					break;
				case 0xF0:
					character_unicode_value = 0x011F;
					break;
				case 0xFD:
					character_unicode_value = 0x0131;
					break;
				case 0xFE:
					character_unicode_value = 0x015F;
					break;
				}
				break;
			default:
				break;
			}
			content += Unicode_Character_to_UTF8(character_unicode_value);
		}

 public:
	CDVBString(const char * the_content, const int size)
		{
			int i;

			if ((size > 0) && (((unsigned char)the_content[0]) >= 0x01) && (((unsigned char)the_content[0]) <= 0x05))
				encoding = (t_encoding)((unsigned char)the_content[0]);
			else
				encoding = NONE_SPECIFIED;

			for (i = 0; i < size; i++)                            // skip initial encoding information
				if (((unsigned char)the_content[i]) >= 0x20)
					break;

			if (size - i == 0)
				content = "";
			else
			{
				std::string s;
				while(i < size)
				{
					i++;
					// skip characters 0x00 - 0x1F & 0x80 - 0x9F
					if ((((unsigned char)the_content[i]) & 0x60) != 0)
						add_character((unsigned char)the_content[i]);
				}
			}
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
};

#endif /* __dvbstring_h__ */
