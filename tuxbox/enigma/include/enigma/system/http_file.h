#ifndef __http_file_h
#define __http_file_h

#include "httpd.h"

class eHTTPFile: public eHTTPDataSource
{
	int errcode;
	QFile file;
	int err;
	int wroteheader;
	QString mime;
public:
	eHTTPFile(QString file);
	~eHTTPFile();
	int getCode();
	int writeData(eHTTPConnection *conn);
	int haveData(eHTTPConnection *conn);
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
	eHTTPDataSource *getDataSource(QString request, QString path, const eHTTPConnection *conn);
	void addTranslation(QString path, QString root);
};

#endif
