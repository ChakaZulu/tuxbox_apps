#include <wizard_scan.h>
#include <rotorconfig.h>
#include <lib/dvb/frontend.h>
#include <lib/gdi/font.h>
#include <lib/gui/listbox.h>
#include <lib/system/init_num.h>
#include <lib/system/info.h>
#include <lib/system/econfig.h>
#include <satconfig.h>
#include <scan.h>
#include <enigma_scan.h>
#include <enigma_main.h>

class eDiseqcChoice: public eListBoxEntry
{
	friend class eListBox<eDiseqcChoice>;
	gPixmap *pixmap;
	eTextPara *para;
	static gFont font;
	int choice;
	eString text;
	int yOffs;
public:
	enum { none, simple, complex };
	eDiseqcChoice(eListBox<eDiseqcChoice>* lb, int choice)
		: eListBoxEntry( (eListBox<eListBoxEntry>*)lb), choice(choice)
	{
		pixmap=eSkin::getActive()->queryImage(eString().sprintf("diseqc_%d", choice));
		if (!font.pointSize)
			font = eSkin::getActive()->queryFont("eListBox.EntryText.normal");
		para=0;
	}
	
	~eDiseqcChoice()
	{	
		delete para;
	}
	
	int getDiseqcChoice() const
	{
		return choice;
	}
protected:
	static int getEntryHeight()
	{
		return 150;
	}
	
	const eString& redraw(gPainter *rc, const eRect& rect, gColor coActiveB, gColor coActiveF, gColor coNormalB, gColor coNormalF, int state )
	{
		drawEntryRect( rc, rect, coActiveB, coActiveF, coNormalB, coNormalF, state );

		if (!para)
		{
			para = new eTextPara( eRect( rect.left(), 0, rect.width(), rect.height() ) );
			para->setFont(font);
			para->renderString(text);
			para->realign(eTextPara::dirCenter);
			yOffs = ((rect.height() - para->getBoundBox().height()) / 2) - para->getBoundBox().top();
		}
		rc->renderPara(*para, ePoint(0, rect.top() + yOffs ) );
		if (pixmap)
			rc->blit(*pixmap, ePoint(rect.left()+15, rect.top()+15));

		return text;
	}
};

gFont eDiseqcChoice::font;

eWizardSelectDiseqc::eWizardSelectDiseqc()
{
	diseqclist=new eListBox<eDiseqcChoice>(this);
	diseqclist->setName("choices");
	diseqclist->setColumns(3);
	
	description=new eLabel(this);
	description->setName("description");

	if (eSkin::getActive()->build(this, "eWizardDiseqc"))
		eFatal("skin load of \"eWizardDiseqc\" failed");
		
	eDiseqcChoice *current;
	current=new eDiseqcChoice(diseqclist, eDiseqcChoice::none);
	new eDiseqcChoice(diseqclist, eDiseqcChoice::simple);
	new eDiseqcChoice(diseqclist, eDiseqcChoice::complex);
	CONNECT(diseqclist->selchanged, eWizardSelectDiseqc::selchanged);
	CONNECT(diseqclist->selected, eWizardSelectDiseqc::selected);
	selchanged(current);
}

int eWizardSelectDiseqc::run()
{
	eWizardSelectDiseqc *wizard=new eWizardSelectDiseqc();
	wizard->show();
	int res=wizard->exec();
	wizard->hide();
	return res;
}

void eWizardSelectDiseqc::selected(eDiseqcChoice *choice)
{
	if (!choice)
		close(-1);
	else
		close(choice->getDiseqcChoice());
}

void eWizardSelectDiseqc::selchanged(eDiseqcChoice *choice)
{
	if (!choice)
		return;
	switch(choice->getDiseqcChoice())
	{
	case 0:
		description->setText(_("Direct connection to one LNB"));
		break;
	case 1:
		description->setText(_("Simple DiSEqC (2 LNBs/satellites)"));
		break;
	case 2:
		description->setText(_("Complex configuration (including DiSEqC 1.2)"));
		break;
	}
}

class eWizardScanInit
{
public:
	eWizardScanInit()
	{
		int res = 0;
		int diseqc=0;
		eConfig::getInstance()->getKey("/elitedvb/wizards/diseqc", diseqc);
		if (diseqc)
			return;
		if ( eSystemInfo::getInstance()->getFEType() == eSystemInfo::feSatellite )
		{
// gotos considered harmless.. :)
again_wizard: 
			res=eWizardSelectDiseqc::run();

			if (res >= 0)
			{
				eSatelliteConfigurationManager satconfig;
				
				switch (res)
				{
				case 0:
					satconfig.extSetComplexity(0); // single lnb
					break;
				case 1:
					satconfig.extSetComplexity(1); // diseqc 1.0
					break;
				case 2:
					satconfig.extSetComplexity(3); // diseqc 1.2
					break;
				}
				do
				{
					satconfig.show();
					res=satconfig.exec();
					satconfig.hide();

					if (res != 1)
						goto again_wizard;

					{
						eLNB *l=eZapScan::getRotorLNB(1);
						if (l)
						{
							RotorConfig c(l);
							c.show();
							c.exec();
							c.hide();
						}
					}

					{
						TransponderScan scan(0, 0, TransponderScan::stateMenu);
						res=scan.exec();
					}
				}
				while (res);

			}
		}
		else do // cable or dvb-t
		{
			TransponderScan scan(0, 0, TransponderScan::stateMenu);
			res=scan.exec();
		}
		while ( res );

		eZapMain::getInstance()->showServiceSelector( eServiceSelector::dirFirst, eZapMain::pathAll );
		diseqc=1;
		eConfig::getInstance()->setKey("/elitedvb/wizards/diseqc", diseqc);
	}
};

eAutoInitP0<eWizardScanInit> init_eWizardScanInit(eAutoInitNumbers::wizard+3, "wizard: scan");
