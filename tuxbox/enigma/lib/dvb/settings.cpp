#include <lib/dvb/settings.h>
#include <lib/dvb/edvb.h>

typedef std::list<eServiceReferenceDVB>::iterator ServiceReferenceDVBIterator;

eDVBSettings::eDVBSettings(eDVB &dvb): dvb(dvb)
{
	transponderlist=new eTransponderList;
	loadServices();
	loadBouquets();
	bouquets.setAutoDelete(true);
}

void eDVBSettings::removeDVBBouquets()
{
	for (ePtrList<eBouquet>::iterator i(bouquets); i != bouquets.end();)
	{
		if ( i->bouquet_id >= 0)
		{
			eDebug("removing bouquet '%s'", i->bouquet_name.c_str());
			i = bouquets.erase(i);
		}
		else
		{
			eDebug("leaving bouquet '%s'", i->bouquet_name.c_str());
			i++;
		}
	}
}

void eDVBSettings::addDVBBouquet(const BAT *bat)
{
	eDebug("wir haben da eine bat, id %x", bat->bouquet_id);
	eString bouquet_name="Weiteres Bouquet";
	for (ePtrList<Descriptor>::const_iterator i(bat->bouquet_descriptors); i != bat->bouquet_descriptors.end(); ++i)
	{
		if (i->Tag()==DESCR_BOUQUET_NAME)
			bouquet_name=((BouquetNameDescriptor*)*i)->name;
	}
	eBouquet *bouquet=createBouquet(bat->bouquet_id, bouquet_name);
	
	for (ePtrList<BATEntry>::const_iterator be(bat->entries); be != bat->entries.end(); ++be)
		for (ePtrList<Descriptor>::const_iterator i(be->transport_descriptors); i != be->transport_descriptors.end(); ++i)
			if (i->Tag()==DESCR_SERVICE_LIST)
			{
				const ServiceListDescriptor *s=(ServiceListDescriptor*)*i;
				for (ePtrList<ServiceListDescriptorEntry>::const_iterator a(s->entries); a != s->entries.end(); ++a)
					bouquet->add(
						eServiceReferenceDVB(
							eTransportStreamID(be->transport_stream_id), 
							eOriginalNetworkID(be->original_network_id), 
							eServiceID(a->service_id), -1));
			}
}

eBouquet *eDVBSettings::getBouquet(int bouquet_id)
{
	for (ePtrList<eBouquet>::iterator i(bouquets); i != bouquets.end(); i++)
		if (i->bouquet_id==bouquet_id)
			return *i;
	return 0;
}

static eString beautifyBouquetName(eString bouquet_name)
{
	if ( (bouquet_name.find("ARD") != eString::npos)
		  || (bouquet_name.find("ZDF") != eString::npos)
			|| (bouquet_name.find("RTL") != eString::npos)
			|| (bouquet_name.find("n-tv") != eString::npos)
			|| (bouquet_name.find("ProSieben") != eString::npos)
			|| (bouquet_name.find("VIVA") != eString::npos) )
		bouquet_name="German Free";		
	else if (bouquet_name.find("POLSAT") != eString::npos)
		bouquet_name="POLSAT";
	else if (bouquet_name.find("HRT") != eString::npos)
		bouquet_name="HRT Zagreb";
	else if (bouquet_name.find("TVP") != eString::npos)
		bouquet_name="TVP";
	else if (bouquet_name.find("RVTS") != eString::npos)
		bouquet_name="RVTS";
	else if (bouquet_name=="ABsat")
		bouquet_name="AB sat";
	else if (bouquet_name=="Astra-Net")
		bouquet_name="ASTRA";
	else if (bouquet_name=="CSAT")
		bouquet_name="CANALSATELLITE";
	else if (bouquet_name.find("SES")!=eString::npos)
		bouquet_name="SES Multimedia";
	else if (!bouquet_name)
		bouquet_name="no name";
	return bouquet_name;
}

eBouquet *eDVBSettings::getBouquet(eString bouquet_name)
{
	for (ePtrList<eBouquet>::iterator i(bouquets); i != bouquets.end(); i++)
		if (!i->bouquet_name.icompare(bouquet_name))
			return *i;
	return 0;
}

eBouquet* eDVBSettings::createBouquet(int bouquet_id, eString bouquet_name)
{
	eBouquet *n=getBouquet(bouquet_id);
	if (!n)
		bouquets.push_back(n=new eBouquet(bouquet_id, bouquet_name));
	return n;
}

eBouquet *eDVBSettings::createBouquet(eString bouquet_name)
{
	eBouquet *n=getBouquet(bouquet_name);
	if (!n)
	{
		int bouquet_id=getUnusedBouquetID(0);
		bouquets.push_back(n=new eBouquet(bouquet_id, bouquet_name));
	}
	return n;
}

int eDVBSettings::getUnusedBouquetID(int range)
{
	if (range)
		range=-1;
	else
		range=1;

	int bouquet_id=0;

	while(true)  // Evtl hier nochmal nachschauen....
	{
		if (!getBouquet(bouquet_id))
			return bouquet_id;

		bouquet_id+=range;
	}
}

void eDVBSettings::revalidateBouquets()
{
#if 0
	eDebug("revalidating bouquets");
	if (transponderlist)
		for (ePtrList<eBouquet>::iterator i(bouquets); i != bouquets.end(); i++)
			for (ServiceReferenceDVBIterator service = i->list.begin(); service != i->list.end(); ++service)
				service->service=transponderlist->searchService(service);
#endif
	/*emit*/ dvb.bouquetListChanged();
}

eTransponderList *eDVBSettings::getTransponders()
{
	return transponderlist;
}

ePtrList<eBouquet>* eDVBSettings::getBouquets()
{
	return &bouquets;
}

void eDVBSettings::setTransponders(eTransponderList *tlist)
{
	if (transponderlist)
		delete transponderlist;
	transponderlist=tlist;
	/*emit*/ dvb.serviceListChanged();
}

struct sortinChannel: public std::unary_function<const eServiceDVB&, void>
{
	eDVBSettings &edvb;
	sortinChannel(eDVBSettings &edvb): edvb(edvb)
	{
	}
	void operator()(eServiceDVB &service)
	{
		eBouquet *b = edvb.createBouquet(beautifyBouquetName(service.service_provider) );
		b->add(eServiceReferenceDVB(service.transport_stream_id, service.original_network_id, service.service_id, service.service_type));
	}
};

void eDVBSettings::sortInChannels()
{
	eDebug("sorting in channels");
	removeDVBBouquets();
	getTransponders()->forEachService(sortinChannel(*this));
	revalidateBouquets();
	saveBouquets();
}

struct saveService: public std::unary_function<const eServiceDVB&, void>
{
	FILE *f;
	saveService(FILE *out): f(out)
	{
	 	fprintf(f, "services\n");
	}
	void operator()(eServiceDVB& s)
	{
		fprintf(f, "%04x:%04x:%04x:%d:%d\n", s.service_id.get(), s.transport_stream_id.get(), s.original_network_id.get(), s.service_type, s.service_number);
		fprintf(f, "%s\n", s.service_name.c_str());
		if (s.dxflags)
			fprintf(f, "f:%x,", s.dxflags);
		if (s.dxflags & eServiceDVB::dxNoDVB)
			for (int i=0; i<eServiceDVB::cacheMax; ++i)
			{
				if (s.cache[i] != -1)
					fprintf(f, "c:%02d%04x,", i, s.cache[i]);
			}
		fprintf(f, "p:%s\n", s.service_provider.c_str());
	}
	~saveService()
	{
		fprintf(f, "end\n");
	}
};

struct saveTransponder: public std::unary_function<const eTransponder&, void>
{
	FILE *f;
	saveTransponder(FILE *out): f(out)
	{
		fprintf(f, "transponders\n");
	}
	void operator()(eTransponder &t)
	{
		if (t.state!=eTransponder::stateOK)
			return;
		fprintf(f, "%04x:%04x %d\n", t.transport_stream_id.get(), t.original_network_id.get(), t.state);
		if (t.cable.valid)
			fprintf(f, "\tc %d:%d:%d:%d\n", t.cable.frequency, t.cable.symbol_rate, t.cable.inversion, t.cable.modulation);
		if (t.satellite.valid)
			fprintf(f, "\ts %d:%d:%d:%d:%d:%d\n", t.satellite.frequency, t.satellite.symbol_rate, t.satellite.polarisation, t.satellite.fec, t.satellite.orbital_position, t.satellite.inversion);
		fprintf(f, "/\n");
	}
	~saveTransponder()
	{
		fprintf(f, "end\n");
	}
};

void eDVBSettings::saveServices()
{
	FILE *f=fopen(CONFIGDIR "/enigma/services", "wt");
	if (!f)
		eFatal("couldn't open servicefile - create " CONFIGDIR "/enigma!");
	fprintf(f, "eDVB services /1/\n");

	getTransponders()->forEachTransponder(saveTransponder(f));
	getTransponders()->forEachService(saveService(f));
	fprintf(f, "Have a lot of fun!\n");
	fclose(f);
}

void eDVBSettings::loadServices()
{
	FILE *f=fopen(CONFIGDIR "/enigma/services", "rt");
	if (!f)
		return;
	char line[256];
	if ((!fgets(line, 256, f)) || strncmp(line, "eDVB services", 13))
	{
		eDebug("not a servicefile");
		return;
	}
	eDebug("reading services");
	if ((!fgets(line, 256, f)) || strcmp(line, "transponders\n"))
	{
		eDebug("services invalid, no transponders");
		return;
	}
/*	if (transponderlist)
		delete transponderlist;
	transponderlist=new eTransponderList;*/
	if (transponderlist)
		transponderlist->clearAllTransponders();

	while (!feof(f))
	{
		if (!fgets(line, 256, f))
			break;
		if (!strcmp(line, "end\n"))
			break;
		int transport_stream_id=-1, original_network_id=-1, state=-1;
		sscanf(line, "%04x:%04x %d", &transport_stream_id, &original_network_id, &state);
		eTransponder &t=transponderlist->createTransponder(eTransportStreamID(transport_stream_id), eOriginalNetworkID(original_network_id));
		t.state=state;
		while (!feof(f))
		{
			fgets(line, 256, f);
			if (!strcmp(line, "/\n"))
				break;
			if (line[1]=='s')
			{
				int frequency, symbol_rate, polarisation, fec, sat, inversion=0;
				sscanf(line+2, "%d:%d:%d:%d:%d:%d", &frequency, &symbol_rate, &polarisation, &fec, &sat, &inversion);
				t.setSatellite(frequency, symbol_rate, polarisation, fec, sat, inversion);
			}
			if (line[1]=='c')
			{
				int frequency, symbol_rate, inversion=0, modulation=3;
				sscanf(line+2, "%d:%d:%d:%d", &frequency, &symbol_rate, &inversion, &modulation);
				t.setCable(frequency, symbol_rate, inversion, modulation);
			}
		}
	}

	if ((!fgets(line, 256, f)) || strcmp(line, "services\n"))
	{
		eDebug("services invalid, no services");
		return;
	}

	if (transponderlist)
		transponderlist->clearAllServices();
	
	int count=0;

	while (!feof(f))
	{
		if (!fgets(line, 256, f))
			break;
		if (!strcmp(line, "end\n"))
			break;

		int service_id=-1, transport_stream_id=-1, original_network_id=-1, service_type=-1, service_number=-1;
		sscanf(line, "%04x:%04x:%04x:%d:%d", &service_id, &transport_stream_id, &original_network_id, &service_type, &service_number);
		eServiceDVB &s=transponderlist->createService(
				eServiceReferenceDVB(
						eTransportStreamID(transport_stream_id), 
						eOriginalNetworkID(original_network_id), 
						eServiceID(service_id),
						service_type), service_number);
		count++;
		s.service_type=service_type;
		fgets(line, 256, f);
		if (strlen(line))
			line[strlen(line)-1]=0;
		s.service_name=line;
		fgets(line, 256, f);
		if (strlen(line))
			line[strlen(line)-1]=0;

		eString str=line;
		
		if (str[1]!=':')	// old ... (only service_provider)
		{
			s.service_provider=line;
		} else
			while ((!str.empty()) && str[1]==':') // new: p:, f:, c:%02d...
			{
				int c=str.find(',');
				char p=str[0];
				eString v;
				if (c == eString::npos)
				{
					v=str.mid(2);
					str="";
				} else
				{
					v=str.mid(2, c-2);
					str=str.mid(c+1);
				}
				eDebug("%c ... %s", p, v.c_str());
				if (p == 'p')
					s.service_provider=v;
				else if (p == 'f')
				{
					sscanf(v.c_str(), "%x", &s.dxflags);
					eDebug("dxflags: %d", s.dxflags);
				} else if (p == 'c')
				{
					int cid, val;
					sscanf(v.c_str(), "%02d%04x", &cid, &val);
					if (cid < eServiceDVB::cacheMax)
						s.cache[cid]=val;
				}
			}
	}
	
	eDebug("loaded %d services", count);
	
	fclose(f);
}

void eDVBSettings::saveBouquets()
{
	eDebug("saving bouquets...");
	
	FILE *f=fopen(CONFIGDIR "/enigma/bouquets", "wt");
	if (!f)
		eFatal("couldn't open bouquetfile - create " CONFIGDIR "/enigma!");
	fprintf(f, "eDVB bouquets /1/");
	fprintf(f, "bouquets\n");
	for (ePtrList<eBouquet>::iterator i(*getBouquets()); i != getBouquets()->end(); ++i)
	{
		eBouquet *b=*i;
		fprintf(f, "%0d\n", b->bouquet_id);
		fprintf(f, "%s\n", b->bouquet_name.c_str());
		for (ServiceReferenceDVBIterator s = b->list.begin(); s != b->list.end(); s++)
			fprintf(f, "%04x:%04x:%04x:%d\n", s->getServiceID().get(), s->getTransportStreamID().get(), s->getOriginalNetworkID().get(), s->getServiceType());
		fprintf(f, "/\n");
	}
	fprintf(f, "end\n");
	fprintf(f, "Have a lot of fun!\n");
	fclose(f);
	eDebug("done");
}

void eDVBSettings::loadBouquets()
{
	FILE *f=fopen(CONFIGDIR "/enigma/bouquets", "rt");
	if (!f)
		return;
	char line[256];
	if ((!fgets(line, 256, f)) || strncmp(line, "eDVB bouquets", 13))
	{
		eDebug("not a bouquetfile");
		return;
	}
	eDebug("reading bouquets");
	if ((!fgets(line, 256, f)) || strcmp(line, "bouquets\n"))
	{
		eDebug("settings invalid, no transponders");
		return;
	}

	bouquets.clear();

	while (!feof(f))
	{
		if (!fgets(line, 256, f))
			break;
		if (!strcmp(line, "end\n"))
			break;
		int bouquet_id=-1;
		sscanf(line, "%d", &bouquet_id);
		if (!fgets(line, 256, f))
			break;
		line[strlen(line)-1]=0;
		eBouquet *bouquet=createBouquet(bouquet_id, line);
		while (!feof(f))
		{
			fgets(line, 256, f);
			if (!strcmp(line, "/\n"))
				break;
			int service_id=-1, transport_stream_id=-1, original_network_id=-1, service_type=-1;
			sscanf(line, "%04x:%04x:%04x:%d", &service_id, &transport_stream_id, &original_network_id, &service_type);
			bouquet->add(
				eServiceReferenceDVB(
					eTransportStreamID(transport_stream_id), 
					eOriginalNetworkID(original_network_id), 
					eServiceID(service_id), 
					service_type));
		}
	}

	eDebug("loaded %d bouquets", getBouquets()->size());
	
	fclose(f);
	
	revalidateBouquets();
	eDebug("ok");
}

void eDVBSettings::clearList()
{
	if (transponderlist)
	{
		transponderlist->clearAllTransponders();
		transponderlist->clearAllServices();
		removeDVBBouquets(); // user Bouquets do not delete...
	}

	/*emit*/ dvb.bouquetListChanged();
}

void eDVBSettings::removeOrbitalPosition(int orbital_position)
{
	if (transponderlist)
	{
		transponderlist->removeOrbitalPosition(orbital_position);
		removeDVBBouquets();
		sortInChannels();
	}

	/*emit*/ dvb.bouquetListChanged();
}

eDVBSettings::~eDVBSettings()
{
	if (transponderlist)
		delete transponderlist;
}
