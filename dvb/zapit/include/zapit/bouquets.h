/*
 * $Id: bouquets.h,v 1.15 2002/04/06 11:26:11 obi Exp $
 */

#ifndef __bouquets_h__
#define __bouquets_h__

#include <algorithm>
#include <functional>
#include <vector>
#include <stdio.h>

#include "getservices.h"
#include "xml/xmltree.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#define CONFIGDIR "/var/tuxbox/config/zapit"
#endif

using namespace std;

typedef vector<channel*> ChannelList;

/* struct for comparing channels by channel number*/
struct CmpChannelByChNr: public binary_function< channel* , channel* , bool>
{
	bool operator()(channel*  c1, channel*  c2)
	{
		return (c1->chan_nr < c2->chan_nr);
	};
};

/* struct for comparing channels by channel name*/
struct CmpChannelByChName: public binary_function< channel* , channel* , bool>
{
	bool operator()(channel*  c1, channel*  c2)
	{
		return (c1->name < c2->name);
	};
};


class CBouquet
{
	private:
		channel* getChannelByName(char* serviceName, uint serviceType = 0);

	public:
		string Name;
		bool   bHidden;
		bool   bLocked;

		ChannelList radioChannels;
		ChannelList tvChannels;

		CBouquet( string name) { Name=name; bHidden = false; bLocked = false;}
		CBouquet( const CBouquet& bouquet);

		~CBouquet();

		void addService( channel* newChannel);

		void removeService( channel* oldChannel);
		void removeService( char* serviceName, uint serviceType = 0)	{removeService( getChannelByName( serviceName, serviceType));}
		void removeService( uint onidSid, uint serviceType = 0)			{removeService( getChannelByOnidSid( onidSid, serviceType));}

		void moveService(  char* serviceName, uint newPosition, uint serviceType);
//		void moveService(  uint onidSid, uint newPosition);
		void moveService(  uint oldPosition, uint newPosition, uint serviceType);

		channel* getChannelByOnidSid(uint onidSid, uint serviceType = 0);
};

typedef vector<CBouquet*> BouquetList;

class CBouquetManager
{
	private:
		CBouquet* remainChannels;
		void makeRemainingChannelsBouquet(unsigned int tvChanNr, unsigned int radioChanNr, string strTitle);
		void parseBouquetsXml(XMLTreeNode *root);
		string convertForXML( string s);
		void storeBouquets();
	public:
		class tvChannelIterator
		{
			private:
				CBouquetManager* Owner;
				int b;
				int c;
			public:
				tvChannelIterator(CBouquetManager* owner, int B=0, int C=0) { Owner = owner; b=B;c=C;};
				tvChannelIterator operator ++(int);
				bool operator != (const tvChannelIterator& it) const;
				bool operator == (const tvChannelIterator& it) const;
				channel* operator *();
			friend class CBouquetManager;
		};

		tvChannelIterator tvChannelsBegin();
		tvChannelIterator tvChannelsEnd(){ return tvChannelIterator(this, -1, -1);};
		tvChannelIterator tvChannelsFind( unsigned int onid_sid);

		class radioChannelIterator
		{
			private:
				CBouquetManager* Owner;
				int b;
				int c;
			public:
				radioChannelIterator(CBouquetManager* owner, int B=0, int C=0) { Owner = owner; b=B;c=C;};
				radioChannelIterator operator ++(int);
				bool operator != (const radioChannelIterator& it) const;
				bool operator == (const radioChannelIterator& it) const;
				channel* operator *();
			friend class CBouquetManager;
		};

		radioChannelIterator radioChannelsBegin();
		radioChannelIterator radioChannelsEnd(){ return radioChannelIterator(this, -1, -1);};
		radioChannelIterator radioChannelsFind( unsigned int onid_sid);

		BouquetList Bouquets;
		BouquetList storedBouquets;

		void saveBouquets();
		void loadBouquets( bool ignoreBouquetFile = false);
		void restoreBouquets();
		void renumServices();

		CBouquet* addBouquet( string name);
		void deleteBouquet( uint id);
		void deleteBouquet( string name);
		int  existsBouquet( string name);
		void moveBouquet( uint oldId, uint newId);
		bool existsChannelInBouquet( unsigned int bq_id, unsigned int onid_sid);

		void saveAsLast( uint BouquetId, uint channelNr);
		void getLast( uint* BouquetId, uint* channelNr);
		void clearAll();
		void onTermination();
		void onStart();

		channel* copyChannelByOnidSid( unsigned int onid_sid);

};

//extern CBouquetManager* BouquetManager;

#endif /* __bouquets_h__ */
