#include "dvbscan.h"

eDVBScanController::eDVBScanController(eDVB &dvb): eDVBController(dvb)
{
	CONNECT(dvb.tPAT.tableReady, eDVBScanController::PATready);
	CONNECT(dvb.tSDT.tableReady, eDVBScanController::SDTready);
	CONNECT(dvb.tNIT.tableReady, eDVBScanController::NITready);
	CONNECT(dvb.tONIT.tableReady, eDVBScanController::ONITready);
	CONNECT(dvb.tBAT.tableReady, eDVBScanController::BATready);

	flags=flagNetworkSearch|flagClearList;
}

eDVBScanController::~eDVBScanController()
{
	dvb.setState(eDVBState(eDVBState::stateIdle));
}

void eDVBScanController::handleEvent(const eDVBEvent &event)
{
	switch (event.type)
	{
	case eDVBEvent::eventTunedIn:
		eDebug("[SCAN] eventTunedIn");
		if (transponder==event.transponder)
			dvb.event(eDVBScanEvent(event.err?eDVBScanEvent::eventScanTuneError:eDVBScanEvent::eventScanTuneOK));
		break;
	case eDVBScanEvent::eventScanBegin:
		eDebug("[SCAN] eventScanBegin");
		
		if (flags & flagClearList)
			dvb.settings->clearList();

		if (flags & flagUseBAT)
			dvb.settings->removeDVBBouquets();

		/*emit*/ dvb.serviceListChanged();
		
				// ------------

		currentONID=-1;
		knownNetworks.clear();
		dvb.event(eDVBScanEvent(eDVBScanEvent::eventScanNext));
		break;
	case eDVBScanEvent::eventScanNext:
	{
		eDebug("[SCAN] eventScanNext");
		
		eTransponder* next = 0;

		for (std::list<eTransponder>::iterator i(knownTransponder.begin()); !next && i != knownTransponder.end(); ++i)
			if (i->state == eTransponder::stateToScan)
				next = &(*i);

		if (!next)
			dvb.event(eDVBScanEvent(eDVBScanEvent::eventScanCompleted));
		else
		{
			transponder=next;
			if (next->tune())
			{
				eDebug("[SCAN] tune failed because of missing infos");
				dvb.event(eDVBScanEvent(eDVBScanEvent::eventScanError));
			} else
				dvb.setState(eDVBScanState(eDVBScanState::stateScanTune));
		}
		break;
	}
	case eDVBScanEvent::eventScanTuneError:
		eDebug("[SCAN] tuned failed");
		dvb.event(eDVBScanEvent(eDVBScanEvent::eventScanError));
		break;
	case eDVBScanEvent::eventScanTuneOK:
		eDebug("[SCAN] tuned in");
			// found valid transponder
		dvb.setState(eDVBScanState(eDVBScanState::stateScanGetPAT));
		dvb.tPAT.start(new PAT());
		break;
	case eDVBScanEvent::eventScanGotPAT:
	{
		eDebug("[SCAN] eventScanGotPAT");
		if (dvb.getState()!=eDVBScanState::stateScanGetPAT)
			eFatal("[SCAN] unexpected gotPAT");

		if (!dvb.tPAT.ready())
			eFatal("[SCAN] tmb suckt -> no pat");

		PAT *pat=dvb.tPAT.getCurrent();
		int nitpid;
		PATEntry *pe=pat->searchService(0);
		if (!pe)
		{
			eDebug("[SCAN] no NIT-PMTentry, assuming 0x10");
			nitpid=0x10;
		}	else
			nitpid=pe->program_map_PID;
		pat->unlock();
		scanOK=0;
		dvb.tNIT.start(new NIT(nitpid));
		if (flags & flagUseONIT)
			dvb.tONIT.start(new NIT(nitpid, NIT::typeOther));
		else
			scanOK|=8;
		dvb.tSDT.start(new SDT());
		if (flags & flagUseBAT)
			dvb.tBAT.start(new BAT());
		else
			scanOK|=4;
		dvb.setState(eDVBScanState(eDVBScanState::stateScanWait));
		break;
	}
	case eDVBScanEvent::eventScanGotSDT:
	{
		eDebug("[SCAN] eventScanGotSDT");
		SDT *sdt=dvb.tSDT.ready()?dvb.tSDT.getCurrent():0;
		if (sdt)
		{
			if (handleSDT(transponder, sdt))
			{
				if (flags&flagSkipKnownNIT)
				{
					dvb.tNIT.abort();
					dvb.tONIT.abort();
				}
			}
			sdt->unlock();
		}

		scanOK|=1;
		eDebug("scanOK %d", scanOK);
		if (scanOK==15)
			dvb.event(eDVBScanEvent(eDVBScanEvent::eventScanComplete));

		break;
	}
	case eDVBScanEvent::eventScanGotNIT:
	case eDVBScanEvent::eventScanGotONIT:
	{
		eDebug("[SCAN] eventScanGotNIT/ONIT");
		NIT *nit=(event.type==eDVBScanEvent::eventScanGotNIT)?(dvb.tNIT.ready()?dvb.tNIT.getCurrent():0):(dvb.tONIT.ready()?dvb.tONIT.getCurrent():0);
		if (nit)
		{
			if (event.type==eDVBScanEvent::eventScanGotNIT)
				if (currentONID!=-1)
					knownNetworks.insert(currentONID);

			if (flags & flagNetworkSearch)
			{
#if 0
				for (ePtrList<Descriptor> i(nit->network_descriptor; i != nit->network_descriptor.end(); ++i)
				{
					if (i->Tag()==DESCR_LINKAGE)
					{
						LinkageDescriptor *l=(LinkageDescriptor*)*i;
						if ((l->linkage_type==0x01) && 		// information service
								(original_network_id==transponder->original_network_id) &&
								(transport_stream_id==transponder->transport_stream_id))
						{
							dvb.tSDT.abort();
						}
					}
				}
#endif
				for (ePtrList<NITEntry>::iterator i(nit->entries); i != nit->entries.end(); ++i)
				{
					// eTransponder &transponder=dvb.settings->transponderlist->createTransponder(eTransportStreamID(i->transport_stream_id), eOriginalNetworkID(i->original_network_id));

					// schon bekannte transponder nicht nochmal scannen
					if (dvb.settings->transponderlist->searchTS(eTransportStreamID(i->transport_stream_id), eOriginalNetworkID(i->original_network_id)))
						continue;

					eTransponder tp(*dvb.settings->transponderlist, eTransportStreamID(i->transport_stream_id), eOriginalNetworkID(i->original_network_id));
//					eDebug("tsid: %d, onid: %d", i->transport_stream_id, i->original_network_id);
					
					for (ePtrList<Descriptor>::iterator d(i->transport_descriptor); d != i->transport_descriptor.end(); ++d)
					{
						switch (d->Tag())
						{
						case DESCR_SAT_DEL_SYS:
							tp.setSatellite((SatelliteDeliverySystemDescriptor*)*d);
							break;
						case DESCR_CABLE_DEL_SYS:
							tp.setCable((CableDeliverySystemDescriptor*)*d);
							break;
						}
					}
  					
					if ( addTransponder(tp) )
						dvb.event(eDVBScanEvent(eDVBScanEvent::eventScanTPadded));
				}
			}
			nit->unlock();
		}

		scanOK|=(event.type==eDVBScanEvent::eventScanGotNIT)?2:8;
		eDebug("scanOK %d", scanOK);
		if (scanOK==15)
			dvb.event(eDVBScanEvent(eDVBScanEvent::eventScanComplete));
		break;
	}
	case eDVBScanEvent::eventScanGotBAT:
	{
		eDebug("[SCAN] eventScanGotBAT");
		BAT *bat=dvb.tBAT.ready()?dvb.tBAT.getCurrent():0;
		if (bat)
		{
			dvb.settings->addDVBBouquet(bat);
			bat->unlock();
		}
		scanOK|=4;
		eDebug("scanOK %d", scanOK);
		if (scanOK==15)
			dvb.event(eDVBScanEvent(eDVBScanEvent::eventScanComplete));
		break;
	}
	case eDVBScanEvent::eventScanError:
		eDebug("with error");		// fall through
	case eDVBScanEvent::eventScanComplete:
	{
		eDebug("completed");

		int count=0;

//		transponder->state = eTransponder::stateError;

//		eDebug("STATE ERROR -> freq = %i, srate = %i, pol = %i, fec = %i, svalid = %i, ycvalid = %i, onid = %i, tsid = %i, inv = %i, op = %i",transponder->satellite.frequency, transponder->satellite.symbol_rate, transponder->satellite.polarisation, transponder->satellite.fec,  transponder->satellite.valid, transponder->cable.valid, transponder->original_network_id.get(), transponder->transport_stream_id.get(), transponder->satellite.inversion, transponder->satellite.orbital_position);

		if (transponder)
			for (std::list<eTransponder>::iterator it(knownTransponder.begin()); it != knownTransponder.end(); it++)
			{
//				eDebug("COMPARE WITH -> freq = %i, srate = %i, pol = %i, fec = %i, svalid = %i, ycvalid = %i, onid = %i, tsid = %i, inv = %i, op = %i",it->satellite.frequency, it->satellite.symbol_rate, it->satellite.polarisation, it->satellite.fec,  it->satellite.valid, it->cable.valid, it->original_network_id.get(), it->transport_stream_id.get(), it->satellite.inversion, it->satellite.orbital_position);
				if (it->state != eTransponder::stateError && *it == *transponder)
				{
//					eDebug("EQUAL !!!!!!!!!!!!!!!!!!!!!!!!");
					it->state = eTransponder::stateError;
					count++;
				}
			}

		if (count > 1)
			eFatal("dupe transponder found");

  	eDebug("set %i Transponder to stateError", count);
	
		dvb.event(eDVBScanEvent(eDVBScanEvent::eventScanNext));
	}
		break;
	case eDVBScanEvent::eventScanCompleted:
		eDebug("scan has finally completed.");

		dvb.settings->saveServices();
		dvb.settings->sortInChannels();
		/*emit*/ dvb.serviceListChanged();

		dvb.setState(eDVBState(eDVBState::stateIdle));
		break;
	}
}

void eDVBScanController::PATready(int error)
{
	eDebug("[SCAN] PATready %d", error);
	dvb.event(eDVBScanEvent(error?eDVBScanEvent::eventScanError:eDVBScanEvent::eventScanGotPAT));
}

void eDVBScanController::SDTready(int error)
{
	eDebug("[SCAN] SDTready %d", error);
	dvb.event(eDVBScanEvent(eDVBScanEvent::eventScanGotSDT));
}

void eDVBScanController::NITready(int error)
{
	eDebug("[SCAN] NITready %d", error);
	dvb.event(eDVBScanEvent(eDVBScanEvent::eventScanGotNIT));
}

void eDVBScanController::ONITready(int error)
{
	eDebug("[SCAN] ONITready %d", error);
	dvb.event(eDVBScanEvent(eDVBScanEvent::eventScanGotONIT));
}

void eDVBScanController::BATready(int error)
{
	eDebug("[SCAN] BATready %d", error);
	dvb.event(eDVBScanEvent(eDVBScanEvent::eventScanGotBAT));
}

int eDVBScanController::handleSDT(eTransponder *&transponder, const SDT *sdt)
{

	eTransponder *old=0;
// update tsid / onid
	if (transponder->transport_stream_id != sdt->transport_stream_id || transponder->original_network_id != sdt->original_network_id)
	{
		changedTransponder.push_back(*transponder);
		
		transponder->transport_stream_id=sdt->transport_stream_id;
		transponder->original_network_id=sdt->original_network_id;

		old = transponder;
	}

		// ok we found the transponder, it seems to be valid
	eTransponder &real=dvb.settings->transponderlist->createTransponder(eTransportStreamID(sdt->transport_stream_id), eOriginalNetworkID(sdt->original_network_id));
	real=*transponder;

		// and we continue working on that
	transponder=&real;
	transponder->state=eTransponder::stateOK;

	if (old) // scan for dupe TPs after TSID / ONID Update
	for (std::list<eTransponder>::iterator it(knownTransponder.begin()); it != knownTransponder.end(); it++)
		if (old != &(*it) && *transponder == *it)
//		{
//			eDebug("set TP in hanldeSDT to stateError");
			it->state=eTransponder::stateError;
//		}

	currentONID=sdt->original_network_id;

	int known=0;

	if (knownNetworks.count(sdt->original_network_id))
		known=1;

		// insert the services
	dvb.settings->transponderlist->handleSDT(sdt);
	
	return known;

}

void eDVBScanController::setUseONIT(int useonit)
{
	if (useonit)
		flags|=flagUseONIT;
	else
		flags&=~flagUseONIT;
}

void eDVBScanController::setUseBAT(int usebat)
{
	if (usebat)
		flags|=flagUseBAT;
	else
		flags&=~flagUseBAT;
}

void eDVBScanController::setNetworkSearch(int networksearch)
{
	if (networksearch)
		flags|=flagNetworkSearch;
	else
		flags&=~flagNetworkSearch;
}

void eDVBScanController::setClearList(int clearlist)
{
	if (clearlist)
		flags|=flagClearList;
	else
		flags&=~flagClearList;
}

void eDVBScanController::setSkipKnownNIT(int skip)
{
	if (skip)
		flags|=flagSkipKnownNIT;
	else
		flags&=~flagSkipKnownNIT;
}

void eDVBScanController::setSkipOtherOrbitalPositions(int skipOtherOP)
{
	if (skipOtherOP)
		flags|=flagSkipOtherOrbitalPositions;
	else
		flags&=~flagSkipOtherOrbitalPositions;

}

void eDVBScanController::start()
{
	dvb.event(eDVBScanEvent(eDVBScanEvent::eventScanBegin));
}
