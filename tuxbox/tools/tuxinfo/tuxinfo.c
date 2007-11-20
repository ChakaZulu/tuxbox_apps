/*
 * tuxinfo.c - TuxBox hardware info
 *
 * Copyright (C) 2003 Florian Schirmer <jolt@tuxbox.org>
 *                    Bastian Blank <waldi@tuxbox.org>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Id: tuxinfo.c,v 1.5 2007/11/20 17:00:52 seife Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <tuxbox.h>

int main(int argc, char **argv)
{

	unsigned char show_capabilities = 0;
	unsigned char show_model = 0;
	unsigned char show_model_str = 0;
	unsigned char show_model_complete_str = 0;
	unsigned char show_submodel = 0;
	unsigned char show_submodel_str = 0;
	unsigned char show_vendor = 0;
	unsigned char show_vendor_str = 0;
	unsigned char show_eval = 0;
	char *digit_format_str = "%d\n";

	while (1) {
	
		int c;
		
		if ((c = getopt(argc, argv, "cmMnNsSvVe")) < 0)
			break;
			
		switch (c) {
		
			case 'c':
				show_capabilities++;
				break;
			case 'e':
				show_eval++;
				show_model++;
				show_model_str++;
				show_submodel++;
				show_submodel_str++;
				show_vendor++;
				show_vendor_str++;
				break;
			case 'm':
				show_model++;
				break;
			case 'M':
				show_model_str++;
				break;
			case 'N':
				show_model_complete_str++;
				break;
			case 's':
				show_submodel++;
				break;
			case 'S':
				show_submodel_str++;
				break;
			case 'v':
				show_vendor++;
				break;
			case 'V':
				show_vendor_str++;
				break;
		}
		
	}

	if (show_capabilities)
		printf (digit_format_str, tuxbox_get_capabilities ());

	if (show_vendor) {
		if (show_eval)
			printf("VENDOR_ID=");
		printf (digit_format_str, tuxbox_get_vendor ());
	}

	if (show_vendor_str) {
		if (show_eval)
			printf("VENDOR=");
		printf ("%s\n", tuxbox_get_vendor_str ());
	}

	if (show_model_complete_str)
		printf ("%s %s\n", tuxbox_get_model_str (), tuxbox_get_submodel_str ());

	if (show_model) {
		if (show_eval)
			printf("MODEL_ID=");
		printf (digit_format_str, tuxbox_get_model ());
	}

	if (show_model_str) {
		if (show_eval)
			printf("MODEL=");
		printf ("%s\n", tuxbox_get_model_str ());
	}

	if (show_submodel) {
		if (show_eval)
			printf("SUBMODEL_ID=");
		printf (digit_format_str, tuxbox_get_submodel ());
	}

	if (show_submodel_str) {
		if (show_eval)
			printf("SUBMODEL=");
		printf ("%s\n", tuxbox_get_submodel_str ());
	}

	return 0;
}
