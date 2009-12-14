/*
 * $Id: start.c,v 1.2 2009/12/14 21:57:37 rhabarber1848 Exp $
 *
 * shellexec - d-box2 linux project
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <plugin.h>
#define SCRIPT "/bin/shellexec"
#define SONAME "shellexec"

void plugin_exec()
{
	int ret, pid, status;
	pid=fork();
	if (pid == -1) {
		fprintf(stderr, "[%s.so] fork\n", SONAME);
		return;
	} else
	if (pid == 0) {
		fprintf(stderr, "[%s.so] forked, executing %s\n", SONAME, SCRIPT);
		for (ret=3 ; ret < 255; ret++)
			close (ret);
			ret = system(SCRIPT);
			if (ret)
				fprintf(stderr, "[%s.so] script return code: %d (%m)\n", SONAME, ret);
			else
				fprintf(stderr, "[%s.so] script return code: %d\n", SONAME, ret);
		_exit(ret);
	}
	fprintf(stderr, "[%s.so] parent, waiting for child with pid %d...\n", SONAME, pid);
	waitpid(pid, &status, 0);
	fprintf(stderr, "[%s.so] parent, waitpid() returned..\n", SONAME);
	if (WIFEXITED(status))
		fprintf(stderr, "[%s.so] child returned with status %d\n", SONAME, WEXITSTATUS(status));
}
