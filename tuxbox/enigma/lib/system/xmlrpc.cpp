#include <time.h>
#include <qmap.h>
#include "xmltree.h"
#include "enigma.h"
#include "http_dyn.h"
#include "dvb.h"
#include "edvb.h"
#include "xmlrpc.h"

static QMap<QString, int (*)(const QVector<eXMLRPCVariant>&, QList<eXMLRPCVariant>&)> rpcproc;

void eXMLRPCVariant::zero()
{
	_struct=0;
	_array=0;
	_i4=0;
	_boolean=0;
	_string=0;
	_double=0;
	_datetime=0;
	_base64=0;
}

eXMLRPCVariant::eXMLRPCVariant(QMap<QString,eXMLRPCVariant*> *__struct)
{
	zero();
	_struct=__struct;
}

eXMLRPCVariant::eXMLRPCVariant(QVector<eXMLRPCVariant> *__array)
{
	zero();
	_array=__array;
	_array->setAutoDelete(true);
}

eXMLRPCVariant::eXMLRPCVariant(__s32 *__i4)
{
	zero();
	_i4=__i4;
}

eXMLRPCVariant::eXMLRPCVariant(bool *__boolean)
{
	zero();
	_boolean=__boolean;
}

eXMLRPCVariant::eXMLRPCVariant(QString *__string)
{
	zero();
	_string=__string;
}

eXMLRPCVariant::eXMLRPCVariant(double *__double)
{
	zero();
	_double=__double;
}

eXMLRPCVariant::eXMLRPCVariant(QDateTime *__datetime)
{
	zero();
	_datetime=__datetime;
}

eXMLRPCVariant::eXMLRPCVariant(QByteArray *__base64)
{
	zero();
	_base64=__base64;
}

eXMLRPCVariant::~eXMLRPCVariant()
{
	if (_struct)
	{
		for (QMap<QString,eXMLRPCVariant*>::Iterator i=_struct->begin(); i!=_struct->end(); ++i)
			delete i.data();
		delete _struct;
	}
	if (_array)
		delete _array;
	if (_i4)
		delete _i4;
	if (_boolean)
		delete _boolean;
	if (_string)
		delete _string;
	if (_double)
		delete _string;
	if (_datetime)
		delete _datetime;
	if (_base64)
		delete _base64;
}

QMap<QString,eXMLRPCVariant*> *eXMLRPCVariant::getStruct()
{
	return _struct;
}

QVector<eXMLRPCVariant> *eXMLRPCVariant::getArray()
{
	return _array;
}

__s32 *eXMLRPCVariant::getI4()
{
	return _i4;
}

bool *eXMLRPCVariant::getBoolean()
{
	return _boolean;
}

QString *eXMLRPCVariant::getString()
{
	return _string;
}

double *eXMLRPCVariant::getDouble()
{
	return _double;
}

QDateTime *eXMLRPCVariant::getDatetime()
{
	return _datetime;
}

QByteArray *eXMLRPCVariant::getBase64()
{
	return _base64;
}

void eXMLRPCVariant::toXML(QString &result)
{
	if (getArray())
	{
		static QString s1("<value><array><data>");
		result+=s1;
		for (unsigned int i=0; i<getArray()->count(); i++)
		{
			static QString s("  ");
			result+=s;
			(*getArray())[i]->toXML(result);
			static QString s1("\n");
			result+=s1;
		}
		static QString s2("</data></aray></value>\n");
		result+=s2;
	} else if (getStruct())
	{
		static QString s1("<value><struct>");
		result+=s1;
		for (QMap<QString,eXMLRPCVariant*>::Iterator i=_struct->begin(); i!=_struct->end(); ++i)
		{
			static QString s1("  <member><name>");
			result+=s1;
			result+=i.key();
			static QString s2("</name>");
			result+=s2;
			i.data()->toXML(result);
			static QString s3("</member>\n");
			result+=s3;
		}
		static QString s2("</struct></value>\n");
		result+=s2;
	} else if (getI4())
	{
		static QString s1("<value><i4>"); 
		result+=s1;
		result+=QString().setNum(*getI4());
		static QString s2("</i4></value>");
		result+=s2;
	} else if (getBoolean())
	{
		static QString s0("<value><boolean>0</boolean></value>");
		static QString s1("<value><boolean>1</boolean></value>");
		result+=(*getBoolean())?s1:s0;
	} else if (getString())
	{
		static QString s1("<value><string>");
		static QString s2("</string></value>");
		result+=s1;
		result+=*getString();
		result+=s2;
	} else if (getDouble())
	{
		result.append(QString().sprintf("<value><double>%lf</double></value>", *getDouble()));
	}	else
		qFatal("couldn't append");
}

static eXMLRPCVariant *fromXML(XMLTreeNode *n)
{
	if (strcmp(n->GetType(), "value"))
		return 0;
	n=n->GetChild();
	const char *data=n->GetData();
	if (!data)
		data="";
	if ((!strcmp(n->GetType(), "i4")) || (!strcmp(n->GetType(), "int")))
		return new eXMLRPCVariant(new int(atoi(data)));
	else if (!strcmp(n->GetType(), "boolean"))
		return new eXMLRPCVariant(new bool(atoi(data)));
	else if (!strcmp(n->GetType(), "string"))
		return new eXMLRPCVariant(new QString(data));
	else if (!strcmp(n->GetType(), "double"))
		return new eXMLRPCVariant(new double(atof(data)));
	else if (!strcmp(n->GetType(), "struct")) {
		QMap<QString,eXMLRPCVariant*> *s=new QMap<QString,eXMLRPCVariant*>;
		for (n=n->GetChild(); n; n=n->GetNext())
		{
			if (strcmp(data, "member"))
			{
				delete s;
				return 0;
			}
			QString name=0;
			eXMLRPCVariant *value;
			for (XMLTreeNode *v=n->GetChild(); v; v=v->GetNext())
			{
				if (!strcmp(v->GetType(), "name"))
					name=QString(v->GetData());
				else if (!strcmp(v->GetType(), "value"))
					value=fromXML(v);
			}
			if ((!value) || (!name))
			{
				delete s;
				return 0;
			}
			s->insert(name, value);
		}
		return new eXMLRPCVariant(s);
	} else if (!strcmp(n->GetType(), "array"))
	{
		QList<eXMLRPCVariant> l;
		n=n->GetChild();
		if (strcmp(data, "data"))
			return 0;
		for (n=n->GetChild(); n; n=n->GetNext())
			if (!strcmp(n->GetType(), "value"))
			{
				eXMLRPCVariant *value=fromXML(n);
				if (!value)
					return 0;
				l.append(value);
			}
		QVector<eXMLRPCVariant> *nv=new QVector<eXMLRPCVariant>;
		nv->resize(l.count());
		l.toVector(nv);
		return new eXMLRPCVariant(nv);
	}
	qDebug("couldn't convert %s", n->GetType());
	return 0;
}

static QString xmlrpc(QString request, QString path, QString opt, const eHTTPConnection *conn)
{
	if (conn->options["Content-Type"]!="text/xml")
	{
		qDebug("xmlrpc: wrong content-type");
		return 0;
	}
	XMLTreeParser parser("ISO-8859-1");
	if (!parser.Parse((const char*)conn->content, conn->content_length, 1))
	{
		qDebug("xml parse error");
		return 0;
	}
	
	QString result;

		// get method name
	QString methodName=0;
	
	XMLTreeNode *methodCall=parser.RootNode();
	if (!methodCall)
	{
		qDebug("empty xml");
		return 0;
	}
	if (strcmp(methodCall->GetType(), "methodCall"))
	{
		qDebug("no methodCall found");
		return 0;
	}

	QList<eXMLRPCVariant> params;
	params.setAutoDelete(true);
	
	for (XMLTreeNode *c=methodCall->GetChild(); c; c=c->GetNext())
	{
		if (!strcmp(c->GetType(), "methodName"))
			methodName=QString(c->GetData());
		else if (!strcmp(c->GetType(), "params"))
		{
			for (XMLTreeNode *p=c->GetChild(); p; p=p->GetNext())
				if (!strcmp(p->GetType(), "param"))
					params.append(fromXML(p->GetChild()));
		} else
		{
			qDebug("unknown stuff found");
			return 0;
		}
	}
	
	if (!methodName)
	{
		qDebug("no methodName found!");
		return 0;
	}
	
	qDebug("methodName: %s", (const char*)methodName);
	
	result="<?xml version=\"1.0\"?>\n"
		"<methodResponse>";
	
	QList<eXMLRPCVariant> ret;
	ret.setAutoDelete(true);

	QVector<eXMLRPCVariant> vparams;
	vparams.resize(params.count());
	params.toVector(&vparams);
	int (*proc)(const QVector<eXMLRPCVariant>&, QList<eXMLRPCVariant> &)=rpcproc[methodName];
	int fault;
	
	if (!proc)
	{
		fault=1;
		xmlrpc_fault(ret, -1, "called method not present");
	} else
		fault=proc(vparams, ret);

	qDebug("converting to text...");

	if (fault)
	{
		result+="<fault>\n";
		ret.current()->toXML(result);
		result+="</fault>\n";
	} else
	{
		result+="<params>\n";
		for (QListIterator<eXMLRPCVariant> i(ret); i.current(); ++i)
		{
			result.append("<param>");
			i.current()->toXML(result);
			result.append("</param>");
		}
		result+="</params>";
	}
	result+="</methodResponse>";
	qDebug("done");
	QString header="Content-Type: text/xml\r\nContent-Length: " + QString().setNum(result.length()) + "\r\n\r\n";
	result.prepend(header);
	return result;
}

void xmlrpc_initialize(eHTTPDynPathResolver *dyn_resolver)
{
	dyn_resolver->addDyn("POST", "/RPC2", xmlrpc);
}

void xmlrpc_addMethod(QString methodName, int (*proc)(const QVector<eXMLRPCVariant>&, QList<eXMLRPCVariant>&))
{
	rpcproc.insert(methodName, proc);
}

void xmlrpc_fault(QList<eXMLRPCVariant> &res, int faultCode, QString faultString)
{
	QMap<QString,eXMLRPCVariant*> *s=new QMap<QString,eXMLRPCVariant*>;
	s->insert("faultCode", new eXMLRPCVariant(new __s32(faultCode)));
	s->insert("faultString", new eXMLRPCVariant(new QString(faultString)));
	res.append(new eXMLRPCVariant(s));
}

int xmlrpc_checkArgs(QString args, const QVector<eXMLRPCVariant> &parm, QList<eXMLRPCVariant> &res)
{
	if (parm.count() != args.length())
	{
	 	xmlrpc_fault(res, -500, QString().sprintf("parameter count mismatch (found %d, expected %d)", parm.count(), args.length()));
		return 1;
	}
	
	for (unsigned int i=0; i<args.length(); i++)
	{
		switch (args[i].latin1())
		{
		case 'i':
			if (parm[i]->getI4())
				continue;
			break;
		case 'b':
			if (parm[i]->getBoolean())
				continue;
			break;
		case 's':
			if (parm[i]->getString())
				continue;
			break;
		case 'd':
			if (parm[i]->getDouble())
				continue;
			break;
		case 't':
			if (parm[i]->getDatetime())
				continue;
			break;
		case '6':
			if (parm[i]->getBase64())
				continue;
			break;
		case '$':
			if (parm[i]->getStruct())
				continue;
			break;
		case 'a':
			if (parm[i]->getArray())
				continue;
			break;
		}
		xmlrpc_fault(res, -501, QString().sprintf("parameter type mismatch, expected %c as #%d", args[i].latin1(), i));
		return 1;
	}
	return 0;
}
