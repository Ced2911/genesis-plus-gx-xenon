/****************************************************************************
 * libwiigui Template
 * Tantric 2009
 *
 * menu.h
 * Menu flow routines - handles all menu logic
 ***************************************************************************/

#ifndef _MENU_H_
#define _MENU_H_

//#include <ogcsys.h>

void InitGUIThreads();
void MainMenu (int menuitem);

enum
{
	MENU_EXIT = -1,
	MENU_NONE,
        MENU_MAIN,
        MENU_OPTIONS,
        MENU_CHEATS,
        MENU_SAVE,
	MENU_SETTINGS,
	MENU_SETTINGS_FILE,
        MENU_IN_GAME,
	MENU_BROWSE_DEVICE,
        MENU_GAME_SAVE,
        MENU_GAME_LOAD,
        MENU_EMULATION
};

#endif
