#include "xmlrpc.h"
#include "dvb.h"
#include "edvb.h"
#include <errno.h>

static eBouquet *getBouquetByID(const char *id)
{
	int bouquet_id;
	if (sscanf(id, "B:%x", &bouquet_id)!=1)
		return 0;
	if (eDVB::getInstance()->getBouquets())
		for (QListIterator<eBouquet> i(*eDVB::getInstance()->getBouquets()); i.current(); ++i)
			if (i.current()->bouquet_id==bouquet_id)
				return i.current();
	return 0;
}

static eService *getServiceByID(const char *id)
{
	eTransponderList *tl=eDVB::getInstance()->getTransponders();
	if (!tl)
		return 0;
	int original_network_id, transport_stream_id, service_id;
	if (sscanf(id, "S:%x:%x:%x", &original_network_id, &transport_stream_id, &service_id)!=3)
		if (sscanf(id, "E:%x:%x:%x", &original_network_id, &transport_stream_id, &service_id)!=3)
			return 0;

	return tl->searchService(original_network_id, service_id);
}

static int testrpc(const QVector<eXMLRPCVariant> &params, QList<eXMLRPCVariant> &result)
{
	if (xmlrpc_checkArgs("ii", params, result))
		return 1;
	
	QMap<QString, eXMLRPCVariant*> *s=new QMap<QString, eXMLRPCVariant*>;
	s->insert("sum", new eXMLRPCVariant(new __s32(*params[0]->getI4()+*params[1]->getI4())));
	s->insert("difference", new eXMLRPCVariant(new __s32(*params[0]->getI4()-*params[1]->getI4())));
	
	result.append(new eXMLRPCVariant(s));
	
	return 0;
}

			// SID2

static int getList(const QVector<eXMLRPCVariant> &params, QList<eXMLRPCVariant> &result)
{
	if (xmlrpc_checkArgs("s", params, result))
		return 1;
	      
	QString &param=*params[0]->getString();
	
	qDebug("getList(%s);", (const char*)param);
	
	if (!param.length())		// root
	{
		QList<eXMLRPCVariant> l;
		if (eDVB::getInstance()->getBouquets())
		{
			for (QListIterator<eBouquet> i(*eDVB::getInstance()->getBouquets()); i.current(); ++i)
			{
				eBouquet *b=i.current();
				QMap<QString, eXMLRPCVariant*> *s=new QMap<QString, eXMLRPCVariant*>;
				static QString s0("caption");
				static QString s1("type");
				static QString s2("handle");
				static QString s3("zappable");
				
				s->insert(s0, new eXMLRPCVariant(new QString(b->bouquet_name)));
				static QString g("Group");
				s->insert(s1, new eXMLRPCVariant(new QString(g)));
				static QString bs("B:");
				QString handle=bs;
				handle+=QString().setNum(b->bouquet_id, 16);
				s->insert(s2, new eXMLRPCVariant(new QString(handle)));
				s->insert(s3, new eXMLRPCVariant(new bool(0)));
				l.append(new eXMLRPCVariant(s));
			}
		}
		QVector<eXMLRPCVariant> *nv=new QVector<eXMLRPCVariant>;
		nv->setAutoDelete(true);
		nv->resize(l.count());
		l.toVector(nv);
		result.append(new eXMLRPCVariant(nv));
	} else if (param[0]=='B')
	{
		eBouquet *b=getBouquetByID(param);
		if (!b)
			xmlrpc_fault(result, 3, "invalid handle");
		else
		{
			QList<eXMLRPCVariant> l;

			for (QListIterator<eServiceReference> s(b->list); s.current(); ++s)
			{
				eService *service=s.current()->service;
				if (!service)
					continue;
				QMap<QString, eXMLRPCVariant*> *s=new QMap<QString, eXMLRPCVariant*>;
				static QString s0("caption");
				static QString s1("type");
				static QString s2("handle");
				static QString s3("zappable");

				s->insert(s0, new eXMLRPCVariant(new QString(service->service_name)));
				static QString g("Service");
				s->insert(s1, new eXMLRPCVariant(new QString(g)));
				static QString bs("S:");
				QString handle=bs;
				handle+=QString().setNum(service->original_network_id, 16);
				handle+=':';
				handle+=QString().setNum(service->transport_stream_id, 16);
				handle+=':';
				handle+=QString().setNum(service->service_id, 16);
				s->insert(s2, new eXMLRPCVariant(new QString(handle)));
				s->insert(s3, new eXMLRPCVariant(new bool(1)));
				l.append(new eXMLRPCVariant(s));
			}
			QVector<eXMLRPCVariant> *nv=new QVector<eXMLRPCVariant>;
			nv->setAutoDelete(true);
			nv->resize(l.count());
			l.toVector(nv);
			result.append(new eXMLRPCVariant(nv));
		}
	} else if (param[0]=='S')
	{
		eService *service=getServiceByID(param);
		if (!service)
		{
			xmlrpc_fault(result, 3, "invalid handle");
			return 0;
		}
		if (eDVB::getInstance()->service != service)
		{
			xmlrpc_fault(result, 4, "service currently not tuned in");
			return 0;
		}
		if (eDVB::getInstance()->service_state==ENOENT)
		{
			xmlrpc_fault(result, 4, "service couldn't be tuned in");
			return 0;
		}
		EIT *eit=eDVB::getInstance()->getEIT();
		if (!eit)
		{
			xmlrpc_fault(result, 1, "no EIT yet");
			return 0;
		}

		QList<eXMLRPCVariant> l;

		for (QListIterator<EITEvent> i(eit->events); i.current(); ++i)
		{
			EITEvent *event=i.current();

			QString event_name=0;

			for (QListIterator<Descriptor> d(event->descriptor); d.current(); ++d)
			{
				Descriptor *descriptor=d.current();
				if (descriptor->Tag()==DESCR_SHORT_EVENT)
				{
					ShortEventDescriptor *ss=(ShortEventDescriptor*)descriptor;
						event_name=ss->event_name;
				}
			}
			
			if (!event_name)
				continue;

			QMap<QString, eXMLRPCVariant*> *s=new QMap<QString, eXMLRPCVariant*>;
			static QString s0("caption");
			static QString s1("type");
			static QString s2("handle");
			static QString s3("zappable");

			s->insert(s0, new eXMLRPCVariant(new QString(event_name)));
			static QString g("Event");
			s->insert(s1, new eXMLRPCVariant(new QString(g)));
			static QString bs("E:");
			QString handle=bs;
			handle+=QString().setNum(service->original_network_id, 16);
			handle+=':';
			handle+=QString().setNum(service->transport_stream_id, 16);
			handle+=':';
			handle+=QString().setNum(service->service_id, 16);
			handle+=':';
			handle+=QString().setNum(event->event_id, 16);

			s->insert(s2, new eXMLRPCVariant(new QString(handle)));
			s->insert(s3, new eXMLRPCVariant(new bool(1)));
			l.append(new eXMLRPCVariant(s));
		}
		eit->unlock();

		QVector<eXMLRPCVariant> *nv=new QVector<eXMLRPCVariant>;
		nv->setAutoDelete(true);
		nv->resize(l.count());
		l.toVector(nv);
		result.append(new eXMLRPCVariant(nv));
	} else
		xmlrpc_fault(result, 3, "couldn't get of this");
	return 0;
}

static int zapTo(const QVector<eXMLRPCVariant> &params, QList<eXMLRPCVariant> &result)
{
	if (xmlrpc_checkArgs("s", params, result))
		return 1;

	QString &param=*params[0]->getString();

	qDebug("zapTo(%s);", (const char*)param);
	
	eService *s=getServiceByID(param);
	if (!s)
		xmlrpc_fault(result, 3, "invalid handle");
	else
		eDVB::getInstance()->switchService(s);
	return 0;
}

static int getInfo(const QVector<eXMLRPCVariant> &params, QList<eXMLRPCVariant> &result)
{
/*	if (xmlrpc_checkArgs("s", params, result))
		return 1;

	QString &param=*params[0]->getString();

	qDebug("getInfo(%s);", (const char*)param);
	
	EITEntry *eitentry=0;
	eService *service=0;
	
	if (!param.length())    // currently running
	{
			// mal gucken
  } else if (param[0]=='S')
  {
  	
  }	*/
  
  xmlrpc_fault(result, -1, "not yet implemented");
}

void ezapInitializeXMLRPC(eHTTPDynPathResolver *dyn_resolver)
{
	xmlrpc_initialize(dyn_resolver);
	xmlrpc_addMethod("test.test", testrpc);
	xmlrpc_addMethod("getList", getList);
	xmlrpc_addMethod("zapTo", zapTo);
	xmlrpc_addMethod("getInfo", getInfo);
}
