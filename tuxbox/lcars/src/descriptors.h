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
$Log: descriptors.h,v $
Revision 1.2  2001/11/15 00:43:45  TheDOC
 added

*/
struct short_event_descriptor_header
{
	unsigned char descriptor_tag:8;
	unsigned char descriptor_length:8;
	unsigned char language_code[3];
	unsigned char event_name_length:8;	
}__attribute__ ((packed));

struct extended_event_descriptor_header
{
	unsigned char descriptor_tag:8;
	unsigned char descriptor_length:8;
	unsigned char descriptor_number:4;
	unsigned char last_descriptor_number:4;
	unsigned char language_code[3];
	unsigned char length_of_items:8;	
}__attribute__ ((packed));
