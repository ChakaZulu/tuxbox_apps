#include <lib/gui/textinput.h>
#include <lib/gui/guiactions.h>
#include <lib/gui/numberactions.h>
#include <lib/gui/eskin.h>
#include <lib/gui/ewindow.h>
#include <lib/system/init.h>
#include <lib/gdi/font.h>

eTextInputField::eTextInputField( eWidget *parent, eLabel *descr, const char *deco )
	:eButton( parent, descr, 1, deco), maxChars(0), lastKey(-1), editMode(false), nextCharTimer(eApp),
	useableChars("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 +-.,:?!\"';_*/()<=>%#@&")
{
	CONNECT( nextCharTimer.timeout, eTextInputField::nextChar );
	addActionMap(&i_numberActions->map);
	flags=0;
	align=eTextPara::dirLeft;
}

void eTextInputField::nextChar()
{
	if ( curPos+1 < maxChars )
	{
		++curPos;
		eLabel::invalidate();
		drawCursor();
		if ( curPos > text.length() )
			text+=' ';
	}
	lastKey=-1;
}

const char *keys[12] = {
		"+0-.,:?!\"';_", // 0
		" 1",        // 1
		"abc2ABC",   // 2
		"def3DEF",   // 3
		"ghi4GHI",   // 4
		"jkl5JKL",   // 5
		"mno6MNO",   // 6
		"pqrs7PQRS", // 7
		"tuv8TUV",   // 8
		"wxyz9WXYZ", // 9
		"*/()<=>%",  // <
		"#@&`"       // >
};

void eTextInputField::setUseableChars( const char* uchars )
{
	useableChars=uchars;
}

void eTextInputField::drawCursor()
{
	eRect rc;
	rc.setTop(crect.bottom()-4);
	rc.setHeight( 3 );
	if ( para )
	{
		if ( text.length() )  // text exist?
		{
			if ( text.length() > curPos) // before or on the last char?
			{
				const eRect &bbox = para->getGlyphBBox(curPos);
				if ( !bbox.width() )  // Space
				{
					if (curPos)  // char before space?
					{
						const eRect &bbBefore = para->getGlyphBBox(curPos-1);
						rc.setLeft( bbBefore.right()+2 );
					}
					if ( text.length() > curPos+1) // char after space ?
					{
						const eRect &bbAfter = para->getGlyphBBox(curPos+1);
						rc.setRight( bbAfter.left()-2 );
					}
					else  // no char behind Space
						rc.setWidth( 10 );
				}
				else
				{
					rc.setLeft( bbox.left() );
					rc.setWidth( bbox.width() );
				}
			}
			else // we are behind the last character
			{
				const eRect &bbox = para->getGlyphBBox(text.length()-1);
				rc.setLeft( bbox.right() + ( ( curPos-text.length() ) * 10 ) + 2 );
				rc.setWidth( 10 );
			}
		}
		else  //  no one character in text
		{
			rc.setLeft( 2 );
			rc.setWidth( 10 );
		}
		rc.moveBy( (deco_selected?crect_selected.left():crect.left())+1, 0 );
		gPainter *painter = getPainter( deco_selected?crect_selected:crect );
		painter->clip( rc );
		painter->setForegroundColor( getForegroundColor() );
		painter->setBackgroundColor( getBackgroundColor() );
		painter->fill( rc );
		painter->clippop();
		delete painter;
	}
}

int eTextInputField::eventHandler( const eWidgetEvent &event )
{
	switch (event.type)
	{
		case eWidgetEvent::changedText:
			if ( maxChars < text.length() )
				maxChars = text.length();
			return eButton::eventHandler( event );
		break;
		case eWidgetEvent::evtAction:
		{
			int key = -1;
			if (event.action == &i_cursorActions->up && editMode && text.length()<maxChars)
			{
				text.insert( curPos, " ");
				eLabel::invalidate();
				drawCursor();
			}
			else if (event.action == &i_cursorActions->down && editMode && text.length())
			{
				text.erase( curPos, 1 );
				eLabel::invalidate();
				drawCursor();
			}
			else if (event.action == &i_cursorActions->left && editMode )
			{
				nextCharTimer.stop();
				if ( curPos > 0 )
				{
					--curPos;
					eLabel::invalidate();
					drawCursor();
					lastKey=-1;
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
					setHelpText(oldHelpText);
					while ( text[text.length()-1] == ' ' )
						text.erase( text.length()-1 );

					eButton::invalidate();  // remove the underline
					editMode=false;
					eWindow::globalCancel(eWindow::ON);
				}
				else
				{
					oldHelpText=helptext;
					oldText=text;
					setHelpText(_("press ok to leave edit mode"));
					editMode=true;
					curPos=0;
					drawCursor();
					eWindow::globalCancel(eWindow::OFF);
				}
			}
			else if ( editMode && event.action == &i_cursorActions->cancel)
			{
				nextCharTimer.stop();
				editMode=false;
				setText(oldText,false);
				invalidate();
				eWindow::globalCancel(eWindow::ON);
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
			if ( key != lastKey && nextCharTimer.isActive() )
			{
				nextCharTimer.stop();
				nextChar();
			}
			if ( editMode && key != -1 )
			{
				char newChar = 0;
				
				if ( key == lastKey )
				{
					char *oldkey = strchr( keys[key], text[curPos] );
					newChar = oldkey?keys[key][oldkey-keys[key]+1]:0;
				}

				if (!newChar)
				{
					newChar = keys[key][0];
				}
				char testChar = newChar;
				do
				{
					if ( strchr( useableChars.c_str(), newChar ) ) // char useable?
					{
						if ( curPos == text.length() )
							text += newChar;
						else
							text[curPos] = newChar;
						eLabel::invalidate();
						drawCursor();
						nextCharTimer.start(750,true);
						break;
					}
					else
					{
						nextCharTimer.stop();
						char *oldkey = strchr( keys[key], newChar );
						newChar=oldkey?keys[key][oldkey-keys[key]+1]:0;
						if (!newChar)
							newChar=keys[key][0];
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

