#ifndef COMMANDCODING_H
#define COMMANDCODING_H


// Classes
enum
{
    NOTHING,
    OSD,
    HARDWARE,
    RC,
    SETTINGS,
    CONTROL,
    SCAN,
    CHANNELS,
    UPDATE,
    TIMER,
    PLUGINS,
    EIT,
    FB,
    IF,
    ELSE,
    IR,
	SDT
};

// Commands
enum
{
    C_direct, // 0
    C_Wait, // 1
    C_Menu, // 2
    C_Update, // 3
    C_Timers, // 4
    C_Add, // 5
    C_Set, // 6
    C_Scan, // 7
    C_Settings, // 8
    C_Plugins, // 9
    C_Save, // 10
    C_Load, // 11
    C_Zap, // 12
    C_Mode, // 13
    C_Switch, // 14
    C_Show, // 15
    C_Hide, // 16
    C_Channellist, // 17
    C_Perspectives, // 18
    C_Dump, // 19
    C_Shutdown, // 20
    C_Read, // 21
    C_Var, // 22
    C_Sub, // 23
    C_Send, // 24
    C_Fillbox, // 25
    C_Puttext, // 26
    C_Get // 27
};

#endif
