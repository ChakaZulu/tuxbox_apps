/*
 * $Id: bouquets.h,v 1.30 2002/09/01 09:23:48 thegoodguy Exp $
 */

#ifndef __bouquets_h__
#define __bouquets_h__

#include <algorithm>
#include <functional>
#include <vector>
#include <stdio.h>

#include "channel.h"
#include "xml/xmltree.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#define CONFIGDIR "/var/tuxbox/config/zapit"
#endif

using namespace std;

typedef vector<CZapitChannel*> ChannelList;

/* struct for comparing channels by channel number*/
struct CmpChannelByChNr: public binary_function <CZapitChannel* , CZapitChannel* , bool>
{
	bool operator() (CZapitChannel* c1, CZapitChannel* c2)
	{
		return (c1->getChannelNumber() < c2->getChannelNumber());
	};
};

/* struct for comparing channels by channel name*/
struct CmpChannelByChName: public binary_function <CZapitChannel* , CZapitChannel* , bool>
{
	bool operator() (CZapitChannel*  c1, CZapitChannel*  c2)
	{
		return (c1->getName() < c2->getName());
	};
};


class CBouquet
{
	public:
		string Name;
		bool   bHidden;
		bool   bLocked;

		ChannelList radioChannels;
		ChannelList tvChannels;

		CBouquet(string name) { Name=name; bHidden = false; bLocked = false; }
		CBouquet(const CBouquet& bouquet);

		~CBouquet();

		void addService (CZapitChannel* newChannel);

		void removeService (CZapitChannel* oldChannel);
		void removeService (unsigned int onidSid, unsigned char serviceType = 0)			{removeService( getChannelByOnidSid( onidSid, serviceType));}

		void moveService (char* serviceName, unsigned int newPosition, unsigned char serviceType);
//		void moveService (unsigned int onidSid, unsigned int newPosition);
		void moveService (unsigned int oldPosition, unsigned int newPosition, unsigned char serviceType);

		int recModeRadioSize( unsigned int);
		int recModeTVSize( unsigned int);
		CZapitChannel* getChannelByOnidSid(unsigned int onidSid, unsigned char serviceType = 0);
};

typedef vector<CBouquet*> BouquetList;

class CBouquetManager
{
	private:
		CBouquet* remainChannels;
		void makeRemainingChannelsBouquet(unsigned int tvChanNr, unsigned int radioChanNr, string strTitle);
		void parseBouquetsXml(const XMLTreeNode *root, int &nChNrTv, int &nChNrRadio);
		string convertForXML( string s);
		void storeBouquets();
	public:
		CBouquetManager() { remainChannels = NULL; };
		class ChannelIterator
		{
			private:
				CBouquetManager* Owner;
				bool tv;           // true -> tvChannelIterator, false -> radioChannelIterator
				unsigned int b;
				int c;
				ChannelList* getBouquet() { return (tv ? &(Owner->Bouquets[b]->tvChannels) : &(Owner->Bouquets[b]->radioChannels)); };
			public:
				ChannelIterator(CBouquetManager* owner, const bool TV = true);
				ChannelIterator operator ++(int);
				CZapitChannel* operator *();
				ChannelIterator FindChannelNr(const unsigned int channel);
				bool EndOfChannels() { return (c == -2); };
		};

		ChannelIterator tvChannelsBegin() { return ChannelIterator(this, true); };
		ChannelIterator tvChannelsFind(const unsigned int onid_sid);

		ChannelIterator radioChannelsBegin() { return ChannelIterator(this, false); };
		ChannelIterator radioChannelsFind(const unsigned int onid_sid);

		BouquetList Bouquets;
		BouquetList storedBouquets;

		void saveBouquets();
		void loadBouquets( bool ignoreBouquetFile = false);
		void restoreBouquets();
		void renumServices();

		CBouquet* addBouquet( string name);
		void deleteBouquet(const unsigned int id);
		void deleteBouquet(const CBouquet* bouquet);
		int  existsBouquet( string name);
		void moveBouquet( unsigned int oldId, unsigned int newId);
		bool existsChannelInBouquet( unsigned int bq_id, unsigned int onid_sid);

		void saveAsLast( unsigned int BouquetId, unsigned int channelNr);
		void getLast( unsigned int* BouquetId, unsigned int* channelNr);
		void clearAll();
		void onTermination();
		void onStart();

		CZapitChannel* copyChannelByOnidSid(unsigned int onid_sid);

};

//extern CBouquetManager* BouquetManager;

#endif /* __bouquets_h__ */
