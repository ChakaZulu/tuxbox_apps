#ifndef __http_dyn_h_
#define __http_dyn_h_

#include "httpd.h"

class eHTTPDyn: public eHTTPDataSource
{
	int errcode;
	QString result;
public:
	eHTTPDyn(QString result);
	~eHTTPDyn();
	int getCode();
	int writeData(eHTTPConnection *conn);
	int haveData(eHTTPConnection *conn);
};


class eHTTPDynPathResolver: public eHTTPPathResolver
{
	struct eHTTPDynEntry
	{
		QString request, path;
		QString (*function)(QString request, QString path, QString opt, const eHTTPConnection *content);
		
		eHTTPDynEntry(QString request, QString path, QString (*function)(QString, QString, QString, const eHTTPConnection *)): request(request), path(path), function(function)
		{
		}
	};
	QList<eHTTPDynEntry> dyn;
public:
	void addDyn(QString request, QString path, QString (*function)(QString, QString, QString, const eHTTPConnection *conn));
	eHTTPDynPathResolver();
	eHTTPDataSource *getDataSource(QString request, QString path, const eHTTPConnection *conn);
};

#endif
