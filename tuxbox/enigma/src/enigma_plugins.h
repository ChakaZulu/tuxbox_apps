#ifndef __enigma_plugins_h
#define __enigma_plugins_h

#include <core/gui/listbox.h>

class ePlugin: eListBoxEntry
{
	friend class eZapPlugins;
	friend class eListBox<ePlugin>;
public:
	int version;
	eString name, desc;
	eString depend, sopath, pluginname;
	bool needfb, needrc, needlcd, needvtxtpid, needoffsets, showpig;
	int posx, posy, sizex, sizey;
	int isback;
	ePlugin(eListBox<ePlugin> *parent, const char *cfgfile);

	bool operator < ( const ePlugin& e) const
	{
			return name < e.name;
	}

protected:
	void redraw(gPainter *rc, const eRect& rect, gColor coActiveB, gColor coActiveF, gColor coNormalB, gColor coNormalF, bool highlited) const
	{
		rc->setFont(listbox->getFont());

		if ((coNormalB != -1 && !highlited) || (highlited && coActiveB != -1))
		{
			rc->setForegroundColor(highlited?coActiveB:coNormalB);
			rc->fill(rect);
			rc->setBackgroundColor(highlited?coActiveB:coNormalB);
		} else
		{
			eWidget *w=listbox->getNonTransparentBackground();
			rc->setForegroundColor(w->getBackgroundColor());
			rc->fill(rect);
			rc->setBackgroundColor(w->getBackgroundColor());
		}

		rc->setForegroundColor(highlited?coActiveF:coNormalF);

		eString txt(isback?_("[back]"):name + " - " + desc);

		rc->renderText(rect, txt);
		
		eWidget* p = listbox->getParent();			
		if (highlited && p && p->LCDElement)
			p->LCDElement->setText(txt);
	}
};

class eZapPlugins: public Object
{
	eListBoxWindow<ePlugin> *window;
private:
	void selected(ePlugin *);
public:
	eZapPlugins(eWidget* lcdTitle=0, eWidget* lcdElement=0);
	void execPluginByName(const char* name);
	void execPlugin(ePlugin* plugin);
	~eZapPlugins();
	int exec();
};

#endif /* __enigma_plugins_h */
