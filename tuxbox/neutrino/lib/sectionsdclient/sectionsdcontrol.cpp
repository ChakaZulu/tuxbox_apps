/*
 * $Header: /cvs/tuxbox/apps/tuxbox/neutrino/lib/sectionsdclient/sectionsdcontrol.cpp,v 1.1 2003/02/28 16:04:55 thegoodguy Exp $
 *
 * Sectionsd command line interface - The Tuxbox Project
 *
 * (C) 2003 by thegoodguy <thegoodguy@berlios.de>
 *
 * License: GPL
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

#include <sectionsdclient/sectionsdclient.h>

CSectionsdClient client;

int main(int argc, char** argv)
{
	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "--pause"))
			client.setPauseScanning(true);
		else
		if (strcmp(argv[i], "--nopause"))
			client.setPauseScanning(false);
	}

	return 0;
}
