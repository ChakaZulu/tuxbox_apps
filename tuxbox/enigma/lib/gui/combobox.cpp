#include <lib/gui/combobox.h>
#include <lib/gdi/font.h>

eComboBox::eComboBox( eWidget* parent, int OpenEntries, eLabel* desc, const char *deco )
	:eButton(parent, desc, 1, deco), listbox(0), button( this, desc, 0), pm(0), entries(OpenEntries)
{
	align=eTextPara::dirLeft;
	button.loadDeco();
	button.setBlitFlags(BF_ALPHATEST);
	listbox.hide();
	listbox.setDeco("eComboBox.listbox");
	listbox.loadDeco();
	pm=eSkin::getActive()->queryImage("eComboBox.arrow");
	button.setPixmap(pm);
	CONNECT( selected, eComboBox::onOkPressed );
	CONNECT( listbox.selected, eComboBox::onEntrySelected );
	CONNECT( listbox.selchanged, eComboBox::onSelChanged );
//	addActionMap(&i_cursorActions->map);
}

void eComboBox::onOkPressed()
{
	if ( flags & flagShowEntryHelp)
	{
		oldHelpText=helptext;
		setHelpText( listbox.getCurrent()->getHelpText() );
	}
	if ( flags & flagSorted )
		listbox.sort();
	parent->setFocus( &listbox );
	listbox.show();
}

int eComboBox::setProperty( const eString& prop, const eString& val )
{
	if ( prop == "sorted" )
		flags |= flagSorted;
	else if (prop == "openEntries" )
		entries = atoi( val.c_str() );
	else if (prop == "showEntryHelp" )
		flags |= flagShowEntryHelp;
	else
		return eButton::setProperty( prop, val);
	return 0;
}

int eComboBox::eventHandler( const eWidgetEvent& event )
{
	switch (event.type)
	{
		case eWidgetEvent::evtAction:
			if (event.action == &i_cursorActions->cancel)
				;
			else
				return eButton::eventHandler( event );
		break;

		case eWidgetEvent::changedPosition:
		case eWidgetEvent::changedSize:
		{
			eListBoxEntryText* cur = listbox.getCurrent();

			if (deco)
				button.resize( eSize(25, crect.height()) );
			listbox.resize( eSize( getSize().width(), eListBoxEntryText::getEntryHeight()*entries+listbox.getDeco().borderBottom+listbox.getDeco().borderTop ) );
			button.move( ePoint( crect.right()-25, crect.top() ) );		
			button.pixmap_position = ePoint( (button.getSize().width() - pm->x) / 2, (button.getSize().height() - pm->y) / 2 );
			ePoint pt = getAbsolutePosition();
			if ( pt.y() + getSize().height() + listbox.getSize().height() > 520)
				listbox.move( ePoint( pt.x(), pt.y()-listbox.getSize().height() ) );
			else
				listbox.move( ePoint( pt.x(), pt.y()+getSize().height() ) );

      if (cur)
				listbox.setCurrent(cur);
		}
		default:
			return eButton::eventHandler( event );	
	}
	return 1;	
}

void eComboBox::onEntrySelected( eListBoxEntryText* e)
{
	listbox.hide();	
	if (flags & flagShowEntryHelp)
		setHelpText( oldHelpText );

	if (e && button.getText() != e->getText() )
	{
		setText( e->getText() );
		setFocus( this );
		/* emit */ selchanged_id(this, e);
		/* emit */ selchanged(e);
	}
	else
		setFocus( this );
}

void eComboBox::onSelChanged(eListBoxEntryText* le)
{
	if (flags & flagShowEntryHelp )	
		setHelpText( le->getHelpText() );
	if ( parent->getFocus() == &listbox )
	{
		if ( LCDTmp )
			LCDTmp->setText( le->getText() );
		else if ( parent->LCDElement )
			parent->LCDElement->setText( le->getText() );
	}
}

void eComboBox::removeEntry( eListBoxEntryText* le )
{
	if (le)
	{
		listbox.remove(le);
		if ( flags & flagSorted )
			listbox.sort();
	}
}

void eComboBox::removeEntry( int num )
{
	if ( listbox.getCount() >= num)
	{
		setCurrent(	num );
	  listbox.remove( listbox.getCurrent() );
		if ( flags & flagSorted )
			listbox.sort();
	}
}

void eComboBox::removeEntry( void* key )
{
	setCurrent(key);
	if (listbox.getCurrent() && key == listbox.getCurrent()->getKey() )
	{
		listbox.remove( listbox.getCurrent() );
		if ( flags & flagSorted )
			listbox.sort();
	}
}

int eComboBox::setCurrent( eListBoxEntryText* le )
{
	if (!le)
		return E_INVALID_ENTRY;

	int err;
	if( (err = listbox.setCurrent( le )) )
		return err;

	setText( listbox.getCurrent()->getText() );

	return OK;
}

struct selectEntryByNum: public std::unary_function<const eListBoxEntryText&, void>
{
	int num;
	eListBox<eListBoxEntryText>* lb;

	selectEntryByNum(int num, eListBox<eListBoxEntryText> *lb): num(num), lb(lb)
	{
	}

	bool operator()(const eListBoxEntryText& le)
	{
		if (!num--)
		{
			lb->setCurrent(&le);
	 		return 1;
		}
		return 0;
	}
};

int eComboBox::setCurrent( int num )
{
	eListBoxEntryText* cur = listbox.getCurrent();

	if ( num > listbox.getCount() )
		return E_INVALID_ENTRY;

	int err;
	if ( (err=listbox.forEachEntry( selectEntryByNum(num, &listbox ) ) ) )
	{
		if ( cur == listbox.getCurrent() )	
			return E_ALLREADY_SELECTED;
		else
			return E_COULDNT_FIND;
	}

	setText( listbox.getCurrent()->getText() );

	return err;  // normal in err is now OK
}

struct selectEntryByKey: public std::unary_function<const eListBoxEntryText&, void>
{
	void* key;
	eListBox<eListBoxEntryText>* lb;

	selectEntryByKey(void *key, eListBox<eListBoxEntryText> *lb):key(key), lb(lb)
	{
	}

	bool operator()(const eListBoxEntryText& le)
	{
		if ( le.getKey() == key )
		{
			lb->setCurrent(&le);
			return 1;
		}

		return 0;
	}
};

int eComboBox::setCurrent( void* key )
{
	if (!listbox.getCount())
		return E_INVALID_ENTRY;

	eListBoxEntryText* cur = listbox.getCurrent();

	int err;
	if ( (err=listbox.forEachEntry( selectEntryByKey(key, &listbox ) ) ) )
	{
		if ( cur == listbox.getCurrent() )	
			return E_ALLREADY_SELECTED;
		else
			return E_COULDNT_FIND;
	}

	setText( listbox.getCurrent()->getText() );

	return err;  // normal in err is now OK
}

eListBoxEntryText* eComboBox::getCurrent()
{
	return listbox.getCurrent();
}
