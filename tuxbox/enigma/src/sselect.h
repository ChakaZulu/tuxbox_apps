#ifndef __sselect_h
#define __sselect_h

#include <apps/enigma/bselect.h>
#include <core/gui/listbox.h>
#include <core/dvb/edvb.h>
#include <core/dvb/epgcache.h>

class eListboxEntryService: public eListBoxEntry
{
	friend class eServiceSelector;
	friend class eListBox<eListboxEntryService>;
	eString sort;
public:
	eService *service;
	eBouquet *bouquet;
	eListboxEntryService(eService *service, eListBox<eListboxEntryService> *listbox);
	eListboxEntryService(eBouquet *bouquet, eListBox<eListboxEntryService> *listbox);
	~eListboxEntryService();

	bool operator < ( const eListboxEntryService& e) const
	{
			return sort < e.sort;
	}

protected:
	void redraw(gPainter *rc, const eRect& rect, const gColor& coActive, const gColor& coNormal, bool highlited) const
	{
			rc->setForegroundColor(highlited?coActive:coNormal);
			rc->setFont(listbox->getEntryFnt());

			if ((coNormal != -1 && !highlited) || (highlited && coActive != -1))
					rc->fill(rect);

			eString descr;

			EITEvent *e=eEPGCache::getInstance()->lookupCurrentEvent(service->original_network_id, service->service_id);
			if (e)
			{
				for (ePtrList<Descriptor>::iterator d(e->descriptor); d != e->descriptor.end(); ++d)
				{
					Descriptor *descriptor=*d;
					if (descriptor->Tag()==DESCR_SHORT_EVENT)
					{
						ShortEventDescriptor *ss=(ShortEventDescriptor*)descriptor;
						descr+=" (";
						descr+=ss->event_name;
						descr+=")";
						break;
					}
				}
				delete e;
			}

			rc->renderText(rect, descr?sort+descr:sort);
			
			eWidget* p = listbox->getParent();			
			if (highlited && p && p->LCDElement)
				p->LCDElement->setText(sort);
	}

};

class eServiceSelector: public eListBoxWindow<eListboxEntryService>
{
	eService *result, *selected;
	eBouquetSelector* pbs;
protected:
	int eventHandler(const eWidgetEvent &event);
private:
	void fillServiceList();
	void entrySelected(eListboxEntryService *entry);
	void selchanged(eListboxEntryService *entry);
public:
	eServiceSelector();
	~eServiceSelector();
	void useBouquet(eBouquet *bouquet);
	eService *choose(eService *current=0, int irc=-1);
	eService *next();
	eService *prev();
};

#endif
