/*
	Neutrino-GUI  -   DBoxII-Project

	Diese GUI wurde von Grund auf neu programmiert und sollte nun vom
	Aufbau und auch den Ausbaumoeglichkeiten gut aussehen. Neutrino basiert
	auf der Client-Server Idee, diese GUI ist also von der direkten DBox-
	Steuerung getrennt. Diese wird dann von Daemons uebernommen.


	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "../global.h"
#include "../neutrino.h"
#include "../widget/channellist.h"
#include "../widget/hintbox.h"
#include "favorites.h"


//
// -- Add current channel to Favorites-Bouquet
// -- Return Status (bit-Status):A
// --    1 = Bouquet created
// --    2 = Channel added   (if not set, channel already in BQ)
// -- rasc
//

int CFavorites::addChannelToFavorites()

{

	unsigned int bouquet_id;
	unsigned int onid_sid;
	char         *fav_bouquetname;
	int          status = 0;


	// -- get Favorites Bouquetname  from Locales
	fav_bouquetname = (char *) g_Locale->getText("favorites.bouquetname").c_str();

	//
	// -- check if Favorite Bouquet exists: if not, create it.
	//
	bouquet_id = g_Zapit->existsBouquet (fav_bouquetname);
	if (!bouquet_id) {
		g_Zapit->addBouquet (fav_bouquetname);
	        bouquet_id = g_Zapit->existsBouquet (fav_bouquetname);
		status |= 1;
	}


	onid_sid = g_Zapit->getCurrentServiceID();
	//fprintf (stderr, "ADDFav: %08lx  (onid_sid)  bq_id: %u \n", onid_sid, bouquet_id);

	if ( ! g_Zapit->existsChannelInBouquet (bouquet_id,onid_sid) ) {
		g_Zapit->addChannelToBouquet(bouquet_id, onid_sid);
		status |= 2;
	}


	// -- tell zapit to save Boquets and reinit (if changed)
	if (status) {
		g_Zapit->saveBouquets();
		g_Zapit->reinitChannels();

		// -- same to Neutrino (keep current channel!)
		CNeutrinoApp *neutrino = CNeutrinoApp::getInstance();
		neutrino->channelsInit();
		neutrino->channelList->zapToOnidSid(onid_sid);
	}

	return status;
}





//
// -- Menue Handler Interface
// -- to fit the MenueClasses from McClean
// -- Add current channel to Favorites and display user messagebox
//

int CFavorites::exec(CMenuTarget* parent, string)
{
	int    res = menu_return::RETURN_EXIT_ALL;
	int    status;
	string str;


	if (parent)
	{
		parent->hide();
	}


	CHintBox* hintBox= new CHintBox(NULL, "favorites.bouquetname", g_Locale->getText("favorites.addchannel"), 380 );
	hintBox->paint();

	status = addChannelToFavorites();

	hintBox->hide();
	delete hintBox;

	// -- Display result
	
	str = "";
	if (status & 1)  str += g_Locale->getText("favorites.bqcreated");
	if (status & 2)  str += g_Locale->getText("favorites.chadded"); 
	else             str += g_Locale->getText("favorites.chalreadyinbq");

	if (status) str +=  g_Locale->getText("favorites.finalhint");

	ShowMsg ( "favorites.bouquetname", str, CMessageBox::mbrBack, CMessageBox::mbBack );

 
	return res;
}


