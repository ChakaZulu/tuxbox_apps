#ifndef __bouqueteditor_bouquets__
#define __bouqueteditor_bouquets__

#include "driver/framebuffer.h"
#include "driver/fontrenderer.h"
#include "menue.h"
#include <zapitclient.h>

#include <string>
#include <vector>

using namespace std;

	class CBEBouquetWidget : public CMenuWidget
	{
		enum state_
		{
			beDefault,
			beMoving
		} state;

		unsigned int		selected;
		unsigned int		origPosition;
		unsigned int		newPosition;

		unsigned int		liststart;
		unsigned int		listmaxshow;
		unsigned int		numwidth;
		int					fheight; // Fonthoehe Bouquetlist-Inhalt
		int					theight; // Fonthoehe Bouquetlist-Titel

		int 				ButtonHeight;
		//string				name;

		int		width;
		int		height;
		int		x;
		int		y;

		void paintItem(int pos);
		void paint();
		void paintHead();
		void paintFoot();
		void hide();

		void deleteBouquet();
		void addBouquet();
		void beginMoveBouquet();
		void finishMoveBouquet();
		void cancelMoveBouquet();
		void internalMoveBouquet( unsigned int fromPosition, unsigned int toPosition);
		void renameBouquet();

		string inputName( string defaultName, string caption);

	public:
		CBEBouquetWidget();

		CZapitClient::BouquetList	Bouquets;
		int exec(CMenuTarget* parent, string actionKey);
	};

#endif

