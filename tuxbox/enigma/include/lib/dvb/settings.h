#ifndef __src_lib_dvb_settings_h
#define __src_lib_dvb_settings_h

#include <lib/dvb/dvb.h>
#include <libsig_comp.h>

class eDVB;

class eDVBSettings: public Object
{
	eDVB &dvb;
	ePtrList<eBouquet> bouquets;
	eTransponderList *transponderlist;
	int bouquetsChanged;
public:
	friend class sortinChannel;
	void removeDVBBouquets();
	void removeDVBBouquet(int bouquet_id);
	void renameDVBBouquet(int bouquet_id, eString& new_name);
	void addDVBBouquet(eDVBNamespace origin, const BAT *bat);
	eBouquet *getBouquet(int bouquet_id);
	eBouquet *getBouquet(eString& bouquet_name);
	eBouquet *createBouquet(int bouquet_id, eString bouquet_name);
	eBouquet *createBouquet(eString bouquet_name);
	int getUnusedBouquetID(int range);
	
	void revalidateBouquets();
	eTransponderList *getTransponders() { return transponderlist; }
	ePtrList<eBouquet> *getBouquets() { return &bouquets; }
	void sortInChannels();

	void saveServices();
	void loadServices();

	void saveBouquets();
	void loadBouquets();

	void clearList();
	void removeOrbitalPosition(int orbital_position);
	int importSatcoDX(eString line);

	void service_found( const eServiceReferenceDVB &, bool );
	void service_removed( const eServiceReferenceDVB &);

	eDVBSettings(eDVB &dvb);
	~eDVBSettings();
};

#endif
