#ifndef __http_dyn_h_
#define __http_dyn_h_

#include "httpd.h"

class eHTTPDyn: public eHTTPDataSource
{
	QString result;
	int wptr, size;
public:
	eHTTPDyn(eHTTPConnection *c, QString result);
	~eHTTPDyn();
	int doWrite(int);
};

class eHTTPDynPathResolver: public eHTTPPathResolver
{
	struct eHTTPDynEntry
	{
		QString request, path;
		QString (*function)(QString request, QString path, QString opt, eHTTPConnection *content);
		
		eHTTPDynEntry(QString request, QString path, QString (*function)(QString, QString, QString, eHTTPConnection *)): request(request), path(path), function(function)
		{
		}
	};
	QList<eHTTPDynEntry> dyn;
public:
	void addDyn(QString request, QString path, QString (*function)(QString, QString, QString, eHTTPConnection *conn));
	eHTTPDynPathResolver();
	eHTTPDataSource *getDataSource(QString request, QString path, eHTTPConnection *conn);
};

#endif
