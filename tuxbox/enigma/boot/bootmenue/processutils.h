#ifndef __processutils__
#define __processutils__


class ProcUtils
{
public:
	static long *getPID(const char *);
	static void killProcess(const char *);
};
#endif

