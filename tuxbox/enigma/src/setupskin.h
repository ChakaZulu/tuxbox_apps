#ifndef __apps_enigma_setupskin_h
#define __apps_enigma_setupskin_h

#include <core/gui/listbox.h>

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
	void loadSkins();
	void accept();
	void skinSelected(eListBoxEntrySkin *l);
	int keyDown(int rc);
public:
	eSkinSetup();
	~eSkinSetup();
};

#endif
