#include <unistd.h>
#include <dirent.h>
#include <signal.h>
#include <sys/wait.h>
#include <enigma_streamer.h>

long *getPID(const char *procname)
{
	struct dirent *entry;
	long *pidList = (long *)malloc(sizeof(long));

	char *name;
	int pid, i = 0;
	char buf[1024], cmdline[40];
	FILE *fp;

	DIR *dir = opendir("/proc");
        if (dir)
        {
        	while (1)
        	{
			if ((entry = readdir(dir)) == NULL)
			{
				closedir(dir);
				break;
			}

			name = entry->d_name;

			if (!(*name >= '0' && *name <= '9'))
				continue;

			pid = atoi(name);
			sprintf(cmdline, "/proc/%d/cmdline", pid);
			if ((fp = fopen(cmdline, "r")) == NULL)
				continue;

			if ((fread(buf, 1, sizeof(buf) - 1, fp)) > 0)
			{
				if (strcmp(buf, procname) == 0)
				{
					pidList = (long *)realloc( pidList, sizeof(long) * (i + 2));
					pidList[i++] = pid;
				}
			}
			fclose(fp);
		}
	}

	pidList[i] = (i == 0) ? -1 : 0;

	return pidList;
}

void killPID(long *pid)
{
	if (*pid != -1 && *pid != 0)
	{
		if (kill(*pid, SIGTERM)!= 0)
			kill(*pid, SIGKILL);
		waitpid(*pid, 0, 0);
		*pid = -1;
        }
}

void killProcess(const char *procname)
{
	if(strlen(procname) != 0)
	{
		long *pidList;
		pidList = getPID(procname);

		if (*pidList > 0)
		{
			long *pid;
			for (pid = pidList; *pid != 0; pid++)
				killPID(pid);
		}
		free(pidList);
	}
}

eStreamer *eStreamer::instance = 0;

eStreamer::eStreamer()
{
	if (!instance)
		instance = this;
}

eStreamer::~eStreamer()
{
}

void eStreamer::setServiceReference(eServiceReference psref)
{
	sref = psref;
}

bool eStreamer::getServiceReference(eServiceReference& psref)
{
	long *pidList = NULL;
	psref = sref;
	bool streaming;
	pidList = getPID("streamts");
	streaming = !(*pidList == -1);
	free(pidList);
	return streaming;
}
