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
		unsigned int		selected;
		unsigned int		liststart;
		unsigned int		listmaxshow;
		unsigned int		numwidth;
		int					fheight; // Fonthoehe Bouquetlist-Inhalt
		int					theight; // Fonthoehe Bouquetlist-Titel

		//string				name;

		int		width;
		int		height;
		int		x;
		int		y;

		void paintItem(int pos);
		void paint();
		void paintHead();
		void hide();

	public:
		CBEBouquetWidget();

		CZapitClient::BouquetList	Bouquets;
		int exec(CMenuTarget* parent, string actionKey);
	};

#endif

