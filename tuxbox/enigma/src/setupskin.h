#ifndef __apps_enigma_setupskin_h
#define __apps_enigma_setupskin_h

#include <lib/gui/listbox.h>
#include <lib/gui/statusbar.h>

class eButton;

class eListBoxEntrySkin: public eListBoxEntryText
{
	friend class eSkinSetup;
	friend class eListBox<eListBoxEntrySkin>;
	eString esml;
public:
	eListBoxEntrySkin(eListBox<eListBoxEntrySkin> *parent, eString name, eString esml)
	:eListBoxEntryText((eListBox<eListBoxEntryText>*)parent, name), esml(esml)
	{
	}

	const eString &getESML() const { return esml; };
};

class eSkinSetup: public eWindow
{
	eButton *baccept, *breject;
	eListBox<eListBoxEntrySkin> *lskins;
	eStatusBar* statusbar;
	void loadSkins();
	void accept();
	void skinSelected(eListBoxEntrySkin *l);
	int eventHandler(const eWidgetEvent &event);
public:
	eSkinSetup();
	~eSkinSetup();
};

#endif
