#ifndef __RBXINPUT_H__
#define __RBXINPUT_H__

#include <rcinput.h>
///this an workaround wrapper, which maps input to RADIOBOX's event types
/// may be the correct way ist use only RC codes, but i like the possibility to
/// configure key mapping via user defined config file
class CRBXInput
{
public:

	static CRBXInput* getInstance() 
		{ static CRBXInput* inp = new CRBXInput(); return inp; }
	
	enum KEYS
	{
		UNKNOWN = 0, //do not define any keys before UNKNOWN!
		POWER,
		OPEN,
		TITLE,
		DISPLAY,
		SELECT,
		MENU,
		ZOOM,
		RETURN,
		UP,
		LEFT,
		RIGHT,
		DOWN,
		PLAY,
		STOP,
		PREV,
		NEXT,
		REW,
		FF,
		SUBTITLE,
		AUDIO,
		ANGLE,
		SEARCH,
		PROGRAM,
		REPEAT,
		AB,
		TIME,
		ONE,
		TWO,
		THREE,
		CLEAR,
		FOUR,
		FIVE,
		SIX,
		TEN,
		SEVEN,
		EIGHT,
		NINE,
		ZERO,
		MUTE,
		PLUS,
		MINUS,
		RED,
		BLUE,
		YELLOW,
		GREEN,
		NOKEY
	};

	void ReadKeys( KEYS& _key, bool& _keypressed );
	void WriteTranslations();

private:

	CRBXInput();
	~CRBXInput();

// RC INput interface for event comm channel
	CRCInput	rcinput;

// Lircd interface 
	int lircd;
	void ReadFromLircd( KEYS& _key );
	void OpenLircd();

	struct _trpair
	{
		KEYS	key;
		int		rc;
	};

	std::list<_trpair> translations;

	//Native RC
	void LoadNativeTranslations();
	void SetDefaultTranslations();

	//Native RC
	KEYS TranslateKey( int _key );

	//LIRC RC
	KEYS TranslateKey( std::string _key );

	const char* GetKeyName( KEYS _key );

/*****************************/
	
};


#endif /* __RBXINPUT_H__ */

