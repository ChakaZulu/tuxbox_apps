#ifndef __xmlrpc_h_
#define __xmlrpc_h_

#include <asm/types.h>
#include <qmap.h>
#include <qstring.h>
#include <qvector.h>
#include <qdatetime.h>
#include <qlist.h>

#include "xmltree.h"
#include "httpd.h"

class eXMLRPCVariant
{
	QMap<QString,eXMLRPCVariant*> *_struct;
	QVector<eXMLRPCVariant> *_array;
	__s32 *_i4;
	bool *_boolean;
	QString *_string;
	double *_double;
	QDateTime *_datetime;
	QByteArray *_base64;
	void zero();
public:
	eXMLRPCVariant(QMap<QString,eXMLRPCVariant*> *_struct);
	eXMLRPCVariant(QVector<eXMLRPCVariant> *_array);
	eXMLRPCVariant(__s32 *_i4);
	eXMLRPCVariant(bool *_boolean);
	eXMLRPCVariant(QString *_string);
	eXMLRPCVariant(double *_double);
	eXMLRPCVariant(QDateTime *_datetime);
	eXMLRPCVariant(QByteArray *_base64);
	~eXMLRPCVariant();
	
	QMap<QString,eXMLRPCVariant*> *getStruct();
	QVector<eXMLRPCVariant> *getArray();
	__s32 *getI4();
	bool *getBoolean();
	QString *getString();
	double *getDouble();
	QDateTime *getDatetime();
	QByteArray *getBase64();
	
	void toXML(QString &);
};

class eXMLRPCResponse: public eHTTPDataSource
{
	XMLTreeParser parser;
	QString result;
	int size;
	int wptr;
	int doCall();
public:
	eXMLRPCResponse(eHTTPConnection *c);
	~eXMLRPCResponse();
	
	int doWrite(int);
	void haveData(void *data, int len);
};

void xmlrpc_initialize(eHTTPD *httpd);
void xmlrpc_addMethod(QString methodName, int (*)(const QVector<eXMLRPCVariant>&, QList<eXMLRPCVariant>&));
void xmlrpc_fault(QList<eXMLRPCVariant> &res, int faultCode, QString faultString);
int xmlrpc_checkArgs(QString args, const QVector<eXMLRPCVariant>&, QList<eXMLRPCVariant> &res);

class eHTTPXMLRPCResolver: public eHTTPPathResolver
{
public:
	eHTTPXMLRPCResolver();
	eHTTPDataSource *getDataSource(QString request, QString path, eHTTPConnection *conn);
};

#endif
