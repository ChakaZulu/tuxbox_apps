/*
 * $Id: parser.h,v 1.1 2009/12/19 19:42:49 rhabarber1848 Exp $
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


#include <string.h>
#include <stdlib.h>
#include <stdio.h>


#ifndef __wxparser__
#define __wxparser__

#define TRANSLATION 0x8000
#define PRE_STEP	32
#define NIGHT_STEP	13

// atual Values
#define ACT_CITY	16
#define ACT_TIME	17
#define ACT_LAT		18
#define ACT_LON		19
#define ACT_SUNR	20
#define ACT_SUNS	21
#define ACT_UPTIME	25
#define ACT_OBST	26
#define ACT_TEMP	27
#define ACT_FTEMP	28
#define ACT_COND	29 | TRANSLATION
#define ACT_ICON	30
#define ACT_PRESS	32
#define ACT_PRTEND	33 | TRANSLATION
#define ACT_WSPEED	36
#define ACT_WINDD	39 | TRANSLATION
#define ACT_HMID	41
#define ACT_VIS		42 | TRANSLATION
#define ACT_UVIND	44
#define ACT_UVTEXT	45 | TRANSLATION
#define ACT_DEWP	47
#define ACT_MOON	50 | TRANSLATION

// Preview Values
#define PRE_DAY		55
#define PRE_TEMPH	56
#define PRE_TEMPL	57
#define PRE_SUNR	58
#define PRE_SUNS	59
#define PRE_ICON	61
#define PRE_COND	62 | TRANSLATION
#define PRE_WSPEED	64
#define PRE_WINDD	67 | TRANSLATION
#define PRE_BT		69
#define PRE_PPCP	70
#define PRE_HMID	71

int  parser		(char *,char *, int, int, int);
int  prs_get_prev_count 	(void);
/*void prs_get_act_int (int what, char *out);
void prs_get_act_loc (int what, char *out);
void prs_get_act_dbl (int what, char *out);
void prs_get_act_time(int what, char *out);
void prs_get_act_dtime(int what, char *out);
*/
int  prs_get_day 	(int, char *, int);
int  prs_get_val (int i, int what, int nacht, char *out);
int  prs_get_dbl (int i, int what, int nacht, char *out);
int  prs_get_time(int i, int what, char *out, int metric);
int  prs_get_dtime(int i, int what, char *out, int metric);
char *prs_translate(char *trans, char *tfile);

#endif // __wxparser__

