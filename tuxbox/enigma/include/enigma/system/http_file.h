#ifndef __http_file_h
#define __http_file_h

#include "httpd.h"

class eHTTPFile: public eHTTPDataSource
{
	int fd, size;
	const char *mime;
public:
	eHTTPFile(eHTTPConnection *c, int fd, const char *mime);
	~eHTTPFile();
	int doWrite(int);
};

class eHTTPFilePathResolver: public eHTTPPathResolver
{
	struct eHTTPFilePath
	{
		eString path;
		eString root;
		eHTTPFilePath(eString path, eString root): path(path), root(root)
		{
		}
	};
	ePtrList<eHTTPFilePath> translate;
public:
	eHTTPFilePathResolver();
	eHTTPDataSource *getDataSource(eString request, eString path, eHTTPConnection *conn);
	void addTranslation(eString path, eString root);
};

#endif
