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
		QString path;
		QString root;
		eHTTPFilePath(QString path, QString root): path(path), root(root)
		{
		}
	};
	QList<eHTTPFilePath> translate;
public:
	eHTTPFilePathResolver();
	eHTTPDataSource *getDataSource(QString request, QString path, eHTTPConnection *conn);
	void addTranslation(QString path, QString root);
};

#endif
