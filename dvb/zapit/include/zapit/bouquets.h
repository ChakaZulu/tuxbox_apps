/*
 * $Id: bouquets.h,v 1.59 2005/02/01 18:16:05 thegoodguy Exp $
 */

#ifndef __bouquets_h__
#define __bouquets_h__

#include <algorithm>
#include <cstdio>
#include <functional>
#include <map>
#include <vector>
#include <string.h>
#include <ctype.h>

#include <inttypes.h>
#include <zapit/client/zapitclient.h>

#include "channel.h"
#include "xmlinterface.h"

using namespace std;

typedef map<t_channel_id, CZapitChannel> tallchans;
typedef tallchans::iterator tallchans_iterator;

typedef vector<CZapitChannel*> ChannelList;

/*
 * Struct for channel comparison by channel names 
 *
 * TODO:
 * Channel names are not US-ASCII, but UTF-8 encoded.
 * Hence we need a compare function that considers the whole unicode charset.
 * For instance all countless variants of the letter a have to be regarded as the same letter.
 */
struct CmpChannelByChName: public binary_function <const CZapitChannel * const, const CZapitChannel * const, bool>
{
	static bool comparetolower(const char a, const char b)
		{
			return tolower(a) < tolower(b);
		};
	
	bool operator() (const CZapitChannel * const c1, const CZapitChannel * const c2)
		{
			return std::lexicographical_compare(c1->getName().begin(), c1->getName().end(), c2->getName().begin(), c2->getName().end(), comparetolower);
		};
};


class CBouquet
{
 public:
	std::string Name;
	bool        bHidden;
	bool        bLocked;

	ChannelList radioChannels;
	ChannelList tvChannels;

	inline CBouquet(const std::string name) { Name = name; bHidden = false; bLocked = false; }

	void addService(CZapitChannel* newChannel);

	void removeService(CZapitChannel* oldChannel);
	void removeService(const t_channel_id channel_id, unsigned char serviceType = ST_RESERVED) { removeService(getChannelByChannelID(channel_id, serviceType)); }
	
	void moveService (const unsigned int oldPosition, const unsigned int newPosition, const unsigned char serviceType);
	
	size_t recModeRadioSize(const transponder_id_t transponder_id);
	size_t recModeTVSize   (const transponder_id_t transponder_id);
	CZapitChannel* getChannelByChannelID(const t_channel_id channel_id, const unsigned char serviceType = ST_RESERVED);
};

typedef std::vector<CBouquet *> BouquetList;

class CBouquetManager
{
 private:
	CBouquet * remainChannels;

	void makeRemainingChannelsBouquet(void);
	void parseBouquetsXml            (const xmlNodePtr root);

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
				int getLowestChannelNumberWithChannelID(const t_channel_id channel_id);
				int getNrofFirstChannelofBouquet(const unsigned int bouquet_nr);
				bool EndOfChannels() { return (c == -2); };
		};

		ChannelIterator tvChannelsBegin() { return ChannelIterator(this, true); };
		ChannelIterator radioChannelsBegin() { return ChannelIterator(this, false); };

		BouquetList Bouquets;

		void saveBouquets(void);
		void saveBouquets(const CZapitClient::bouquetMode bouquetMode, const char * const providerName);
		void loadBouquets(bool ignoreBouquetFile = false);
		void renumServices();

		CBouquet* addBouquet(const std::string & name);
		void deleteBouquet(const unsigned int id);
		void deleteBouquet(const CBouquet* bouquet);
		int  existsBouquet(char const * const name);
		void moveBouquet(const unsigned int oldId, const unsigned int newId);
		bool existsChannelInBouquet(unsigned int bq_id, const t_channel_id channel_id);

		void clearAll();

		CZapitChannel* findChannelByChannelID(const t_channel_id channel_id);

};

#endif /* __bouquets_h__ */
