/*
 * $Id: fp_reg.c,v 1.2 2006/03/03 23:09:02 zwen Exp $
 *
 * (C) 2003 Andreas Oberritter <obi@saftware.de>
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

#include <dbox/fp.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define FP_DEV "/dev/dbox/fp0"

int main(int argc, char **argv)
{
	int fd;
	unsigned long foo;
	unsigned long reg;
	unsigned long val;

	if ((argc < 2) || (argc > 3)) {
		printf("usage: %s <register> [<value>]\n", argv[0]);
		return EXIT_FAILURE;
	}

	fd = open(FP_DEV, O_RDWR);

	if (fd == -1) {
		perror(FP_DEV);
		return EXIT_FAILURE;
	}
	
	reg = strtoul(argv[1], NULL, 0) & 0xff;
	
	if (argc == 2) {
		foo = reg; /* und eigentlich | ((len - 1) << 8), wobei len <= 4 */
		
		if (ioctl(fd, FP_IOCTL_GET_REGISTER, &foo) == -1) {
			perror("FP_IOCTL_GET_REGISTER");
		}
		else {
			val = foo;
			printf("reg %02lx: %02x\n", reg, ((unsigned char*)(&foo))[0]);
		}
	}

	else {
		val = strtoul(argv[2], NULL, 0) & 0xff;

		foo = (val << 8) | reg;

		if (ioctl(fd, FP_IOCTL_SET_REGISTER, &foo) == -1)
			perror("FP_IOCTL_SET_REGISTER");
	}
	
	close(fd);
	
	return EXIT_SUCCESS;
}

