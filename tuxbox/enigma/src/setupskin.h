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
protected:
	int eventHandler(const eWidgetEvent &event);
};

class eSkinSetup: public eWindow
{
	eButton *baccept;
	eLabel  *lb3;
	eListBox<eListBoxEntrySkin> *lskins;
	eStatusBar* statusbar;
	void loadSkins();
	void accept();
	void skinSelected(eListBoxEntrySkin *l);
	void skinchanged(eListBoxEntrySkin *l);

protected:
	int eventHandler(const eWidgetEvent &event);
public:
	eSkinSetup();
	~eSkinSetup();
};

#endif
