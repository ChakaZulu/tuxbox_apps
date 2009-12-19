/*
 * $Id: text.h,v 1.1 2009/12/19 19:42:49 rhabarber1848 Exp $
 *
 * tuxwetter - d-box2 linux project
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
*/

#ifndef __TEXT_H__

#define __TEXT_H__

#include "tuxwetter.h"

extern int FSIZE_BIG;
extern int FSIZE_MED;
extern int FSIZE_SMALL;

FT_Error MyFaceRequester(FTC_FaceID face_id, FT_Library library, FT_Pointer request_data, FT_Face *aface);
void RenderString(char *string, int sx, int sy, int maxwidth, int layout, int size, int color);
void ShowMessage(char *message, int wait);
int GetStringLen(unsigned char *string);
void TranslateString(char *src);

#endif
