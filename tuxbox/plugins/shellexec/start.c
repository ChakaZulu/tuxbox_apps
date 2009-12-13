/*
 * $Id: start.c,v 1.1 2009/12/13 17:59:23 rhabarber1848 Exp $
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

void plugin_exec()
{
	int ret, pid, status;
	pid=fork();
	if (pid == -1) {
		perror("[shellplugin.so] fork");
		return;
	} else
	if (pid == 0) {
		fprintf(stderr, "[shellplugin.so] forked, executing " SCRIPT "\n");
		for (ret=3 ; ret < 255; ret++)
			close (ret);
			ret = system(SCRIPT);
			if (ret)
				fprintf(stderr, "[shellplugin.so] script return code: %d (%m)\n", ret);
			else
				fprintf(stderr, "[shellplugin.so] script return code: %d\n", ret);
		_exit(ret);
	}
	fprintf(stderr, "[shellplugin.so] parent, waiting for child with pid %d...\n", pid);
	waitpid(pid, &status, 0);
	fprintf(stderr, "[shellplugin.so] parent, waitpid() returned..\n");
	if (WIFEXITED(status))
		fprintf(stderr, "[shellplugin.so] child returned with status %d\n", WEXITSTATUS(status));
}
