#include <lib/base/estring.h>
#include <lib/gui/textinput.h>
#include <lib/gui/guiactions.h>
#include <lib/gui/numberactions.h>
#include <lib/gui/eskin.h>
#include <lib/gui/ewindow.h>
#include <lib/system/init.h>
#include <lib/system/econfig.h>
#include <lib/gdi/font.h>

eTextInputField::eTextInputField( eWidget *parent, eLabel *descr, const char *deco )
	:eButton( parent, descr, 1, deco), maxChars(0), lastKey(-1), editMode(false),
	editHelpText(_("press ok to leave edit mode, yellow=capslock")), nextCharTimer(eApp),
	useableChars("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
//						 " +-.,:?!\"';_*/()<=>%#@&"),
             "–—“”‘’÷◊ÿŸ⁄€‹›ﬁﬂ‡·‚„‰ÂÊÁËÈÍÎÏÌÓÔ∞±≤≥¥µ∂∑∏π∫ªºΩæø¿¡¬√ƒ≈∆«»… ÀÃÕŒœ"
						 " +-.,:?!\"';_*/()<=>%#@&"), capslock(0), editLabel(0)
{
	if ( eConfig::getInstance()->getKey("/ezap/rc/TextInputField/nextCharTimeout", nextCharTimeout ) )
		nextCharTimeout=0;
	CONNECT( nextCharTimer.timeout, eTextInputField::nextChar );
	addActionMap(&i_numberActions->map);
	flags=0;
	align=eTextPara::dirLeft;
}

void eTextInputField::updated()
{
	unsigned char c[4096];
	strcpy((char *)c,isotext.c_str());

	if ( editLabel )
		editLabel->setText(convertDVBUTF8(c,strlen((char*)c)));
	else
		text=convertDVBUTF8(c,strlen((char*)c));
	drawCursor();
}

void eTextInputField::nextChar()
{
	if ( curPos+1 < (int)maxChars )
	{
		lastKey=-1;
		++curPos;
		if ( curPos > (int)isotext.length()-1 )
			isotext+=' ';
		updated();
	}
}

//		"abc2ABC",   // 2
//		"def3DEF",   // 3
//		"ghi4GHIª",   // 4
//		"jkl5JKL",   // 5
//		"mno6MNO",   // 6
//		"pqrs7PQRS", // 7
//		"tuv8TUV",   // 8
//		"wxyz9WXYZ",   // 9

const char *keys[2][12] = {
// lowercase
	  { 	"+0-.,:?!\"';_",     // 0
		" 1#@&/()<=>%",      // 1
		"abc2–—“”",   // 2
		"def3‘’÷◊",   // 3
		"ghi4ÿŸ⁄€",   // 4
		"jkl5‹›ﬁﬂ",   // 5
		"mno6‡·‚„",   // 6
		"pqrs7‰ÂÊÁ",  // 7
		"tuv8ËÈÍÎ",   // 8
		"wxyz9ÌÓÔ",   // 9
		"*/()<=>%",          // <
		"#@&`" },            // >
// uppercase
	  { 	"+0-.,:?!\"';_",     // 0
		" 1#@&/()<=>%",      // 1
		"ABC2∞±≤≥",   // 2
		"DEF3¥µ∂∑",   // 3
		"GHI4∏π∫ª",   // 4
		"JKL5ºΩæø",   // 5
		"MNO6¿¡¬√",   // 6
		"PQRS7ƒ≈∆«",  // 7
		"TUV8»… À",   // 8
		"WXYZ9ÕŒœ",   // 9
		"*/()<=>%",          // <
		"#@&`" }             // >
};

void eTextInputField::setUseableChars( const char* uchars )
{
	useableChars=uchars;
}

void eTextInputField::setNextCharTimeout( unsigned int newtimeout )
{
	nextCharTimeout=newtimeout;
}

void eTextInputField::drawCursor()
{
//	eDebug("length = %d", isotext.length());
	if ( !cursorRect.isEmpty() )
		eWidget::invalidate(cursorRect);

	cursorRect.setTop((deco_selected?crect_selected.bottom():deco?crect.bottom():clientrect.bottom())-4);

	cursorRect.setHeight( 3 );
	if ( isotext.length() )  // text exist?
	{
		if ( (int)isotext.length() > curPos) // before or on the last char?
		{
			const eRect &bbox = editLabel->getPara()->getGlyphBBox(curPos);
			if ( !bbox.width() )  // Space
			{
				if (curPos)  // char before space?
				{
					const eRect &bbBefore = editLabel->getPara()->getGlyphBBox(curPos-1);
					cursorRect.setLeft( bbBefore.right()+2 );
				}
				if ( (int)isotext.length() > curPos+1) // char after space ?
				{
					const eRect &bbAfter = editLabel->getPara()->getGlyphBBox(curPos+1);
					cursorRect.setRight( bbAfter.left()-2 );
				}
				else  // no char behind Space
					cursorRect.setWidth( 10 );
			}
			else
			{
				cursorRect.setLeft( bbox.left() );
				cursorRect.setWidth( bbox.width() );
			}
		}
		else // we are behind the last character
		{
			const eRect &bbox = editLabel->getPara()->getGlyphBBox(isotext.length()-1);
			cursorRect.setLeft( bbox.right() + ( ( curPos-isotext.length() ) * 10 ) + 2 );
			cursorRect.setWidth( 10 );
		}
	}
	else  //  no one character in text
	{
		cursorRect.setLeft( 0 );
		cursorRect.setWidth( 10 );
	}
	eRect tmp = deco_selected?crect_selected:deco?crect:clientrect;
	if ( cursorRect.right() > scroll.top().second )
	{
		int newpos = scroll.top().first + cursorRect.left();
		scroll.push( std::pair<int,int>( newpos, newpos+tmp.width() ) );
		editLabel->move( ePoint( (-newpos)+tmp.left(), editLabel->getPosition().y() ) );
	}
	else if ( scroll.size() > 1 && cursorRect.left() < scroll.top().first )
	{
		scroll.pop();
		editLabel->move( ePoint( (-scroll.top().first)+tmp.left() , editLabel->getPosition().y() ) );
	}
	cursorRect.moveBy( (deco_selected?crect_selected.left():deco?crect.left():clientrect.left())-scroll.top().first+1, 0 );
	gPainter *painter = getPainter( eRect( ePoint(0,0), size ) );
	painter->setForegroundColor( getForegroundColor() );
	painter->setBackgroundColor( getBackgroundColor() );
	painter->fill( cursorRect );
	if(capslock)
	{
		if ( !capsRect.isEmpty() )
			eWidget::invalidate( capsRect );
		capsRect=cursorRect;
		capsRect.setTop(deco_selected?crect_selected.top():deco?crect.top():clientrect.top());
		capsRect.setHeight( 3 );
		painter->fill( capsRect );
	}
	if (deco_selected && have_focus)
		deco_selected.drawDecoration(painter, ePoint(width(), height()));
	else if (deco)
		deco.drawDecoration(painter, ePoint(width(), height()));

	delete painter;
}

int eTextInputField::eventHandler( const eWidgetEvent &event )
{
	if (editLabel)
		isotext=convertUTF8DVB(editLabel->getText());
	else
		isotext=convertUTF8DVB(text);
	switch (event.type)
	{
		case eWidgetEvent::changedText:
			if ( maxChars < isotext.length() )
				maxChars = isotext.length();
			return eButton::eventHandler( event );
		break;
		case eWidgetEvent::evtAction:
		{
			if ( curPos < 0 )
				curPos=0;
//			eDebug("curPos=%d, isotext.length=%d",curPos, isotext.length());
			int key = -1;
			if ( event.action == &i_cursorActions->capslock && editMode)
			{
				capslock^=1;
				if ( capslock )
					drawCursor();
				else if ( !capsRect.isEmpty() )
					eWidget::invalidate( capsRect );
			}
			else if ( (event.action == &i_cursorActions->up ||
				event.action == &i_cursorActions->down) && editMode )
			{
				lastKey=-1;
				nextCharTimer.stop();
				const char *pc1=useableChars.c_str();
				const char *pc2=strchr( pc1, isotext[curPos] );

				if( !pc2 || !pc2[0] )
					pc2=pc1;

				if(event.action == &i_cursorActions->up)
				{
					pc2++;
					if(!*pc2)
						pc2=pc1;
				}
				else
				{
					if(pc2==pc1)
						while(*pc2)
							pc2++;
					pc2--;
				}
				if ( isotext.length() )
					isotext[curPos] = *pc2;
				else
					isotext+=*pc2;
				updated();
			}
			else if (event.action == &i_cursorActions->insertchar && editMode)
			{
				if ( isotext.length() && isotext.length() < maxChars )
				{
					lastKey=-1;
					isotext.insert( curPos, " ");
					updated();
				}
			}
			else if (event.action == &i_cursorActions->deletechar && editMode)
			{
				if ( isotext.length() )
				{
					lastKey=-1;
					isotext.erase( curPos, 1 );
//					eDebug("curPos=%d, length=%d", curPos, text.length() );
					if ( (int)isotext.length() == curPos )
					{
//						eDebug("curPos--");
						--curPos;
					}
					updated();
				}
			}
			else if (event.action == &i_cursorActions->left && editMode )
			{
				nextCharTimer.stop();
				if ( curPos > 0 )
				{
					--curPos;
					lastKey=-1;
					updated();
				}
			}
			else if (event.action == &i_cursorActions->right && editMode)
			{
				nextCharTimer.stop();
				nextChar();
			}
			else if (event.action == &i_cursorActions->ok)
			{
				nextCharTimer.stop();
				if ( editMode )
				{
					editLabel->hide();
					setText(editLabel->getText());
					delete editLabel;
					editLabel=0;

					setHelpText(oldHelpText);

					while ( text.length() && text[text.length()-1] == ' ' )
						text.erase( text.length()-1 );

					editMode=false;
					/* emit */ selected();

					eWindow::globalCancel(eWindow::ON);
					break;
				}
				else
				{
					oldText=text;
					editMode=true;
					/* emit */ selected();
					capslock=0;
					while(scroll.size())
						scroll.pop();
					eRect tmp = deco_selected?crect_selected:deco?crect:clientrect;
					editLabel=new eLabel(this,0,0);
					editLabel->hide();
					editLabel->move(tmp.topLeft());
					scroll.push( std::pair<int,int>(0,tmp.width()) );
					eSize tmpSize=tmp.size();
					tmpSize.setWidth( tmp.width()*5 );
					editLabel->resize(tmpSize);
					editLabel->setText(text);
					oldHelpText=helptext;
					setText("");
					editLabel->show();
					setHelpText(editHelpText);
					curPos=0;
					drawCursor();
					eWindow::globalCancel(eWindow::OFF);
				}
			}
			else if ( editMode && event.action == &i_cursorActions->cancel )
			{
				delete editLabel;
				editLabel=0;
				nextCharTimer.stop();
				editMode=false;
				setText(oldText);
				setHelpText(oldHelpText);
				eWindow::globalCancel(eWindow::ON);
				break;
			}
			else if (event.action == &i_numberActions->key0)
				key=0;
			else if (event.action == &i_numberActions->key1)
				key=1;
			else if (event.action == &i_numberActions->key2)
				key=2;
			else if (event.action == &i_numberActions->key3)
				key=3;
			else if (event.action == &i_numberActions->key4)
				key=4;
			else if (event.action == &i_numberActions->key5)
				key=5;
			else if (event.action == &i_numberActions->key6)
				key=6;
			else if (event.action == &i_numberActions->key7)
				key=7;
			else if (event.action == &i_numberActions->key8)
				key=8;
			else if (event.action == &i_numberActions->key9)
				key=9;
			else if (event.action == &i_numberActions->keyExt1)
				key=10;
			else if (event.action == &i_numberActions->keyExt2)
				key=11;
			else
				return eButton::eventHandler( event );
			if ( key != lastKey && lastKey != -1 && key != -1 )
			{
				if ( nextCharTimer.isActive() )
					nextCharTimer.stop();
				nextChar();
			}
//			eDebug("editMode = %d, key = %d", editMode, key);
			if ( editMode && key != -1 )
			{
				char newChar = 0;
				
				if ( key == lastKey )
				{
					char *oldkey = strchr( keys[capslock][key], isotext[curPos] );
					newChar = oldkey?keys[capslock][key][oldkey-keys[capslock][key]+1]:0;
				}

				if (!newChar)
				{
					newChar = keys[capslock][key][0];
				}
//				eDebug("newChar = %d", newChar );
				char testChar = newChar;
				do
				{
					if ( strchr( useableChars.c_str(), newChar ) ) // char useable?
					{
						if ( curPos == (int)isotext.length() )
							isotext += newChar;
						else
							isotext[curPos] = newChar;
						updated();
						if(nextCharTimeout)
							nextCharTimer.start(nextCharTimeout,true);
						break;
					}
					else
					{
						nextCharTimer.stop();
						char *oldkey = strchr( keys[capslock][key], newChar );
						newChar=oldkey?keys[capslock][key][oldkey-keys[capslock][key]+1]:0;
						if (!newChar)
							newChar=keys[capslock][key][0];
					}
				}
				while( newChar != testChar );  // all chars tested.. and no char is useable..
				lastKey=key;
			}
		}
		break;

		default:
			return eButton::eventHandler( event );
		break;
	}
	return 1;
}

void eTextInputField::lostFocus()
{
	eWindow::globalCancel(eWindow::ON);
	eButton::lostFocus();
}

void eTextInputField::redrawWidget( gPainter *target, const eRect &area )
{
	eButton::redrawWidget( target, area );
}

static eWidget *create_eTextInputField(eWidget *parent)
{
	return new eTextInputField(parent);
}

class eTextInputFieldSkinInit
{
public:
	eTextInputFieldSkinInit()
	{
		eSkin::addWidgetCreator("eTextInputField", create_eTextInputField);
	}
	~eTextInputFieldSkinInit()
	{
		eSkin::removeWidgetCreator("eTextInputField", create_eTextInputField);
	}
};

eAutoInitP0<eTextInputFieldSkinInit> init_eTextInputFieldSkinInit(25, "eTextInputField");

