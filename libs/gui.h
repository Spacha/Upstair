 /**************************************************************
  * 
  *   _   _           _        _        ____    ___  
  *  | | | |_ __  ___| |_ __ _(_)_ __  |___ \  / _ \ 
  *  | | | | '_ \/ __| __/ _` | | '__|   __) || | | |
  *  | |_| | |_) \__ \ || (_| | | |     / __/ | |_| |
  *   \___/| .__/|___/\__\__,_|_|_|    |_____(_)___/ 
  *        |_| Miika Sikala, Oulun Yliopisto, 2018
  * 
  * 
  * ************************************************************
  * THE BEER-WARE LICENSE
  * This file is part of Upstair 2.0 software. As long as you retain this notice you
  * can do whatever you want with this stuff. If we meet some day, and you think
  * this stuff is worth it, you can buy me a beer in return.
  * 
  * Miika Sikala
  * ************************************************************
  */
  
#ifndef UPSTAIR_GUI_H
#define UPSTAIR_GUI_H

/* Standard libs */
#include <inttypes.h>

/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>

#include <ti/mw/display/Display.h>
#include <ti/mw/display/DisplayExt.h>

#include "upstair.h"
#include "bitmaps/gui.h"

#define GUI_CONTENT_Y 18    // starting coordinate of the content (y)


/* Views */
typedef enum {
    VW_MAIN,            // home screen
    VW_MENU,            // main menu
    VW_MSGS,            // received messages
    VW_STATS,           // statistics
    VW_SETTINGS,        // settings
    VW_GAME,            // the Game (never finished, sadly)
    VW_NEW_MSG,         // view recently received message
    VW_CONFM_SHUTDOWN   // device shutdown confirmation
} View;


/* Public functions */

uint8_t GUI_menuPos();
void GUI_initDisplay();
void GUI_clearDisplay();
void GUI_closeDisplay();
void GUI_moveMenuCursor(uint8_t itemCount);
void GUI_drawDynamics(View *view, float shakiness);
void GUI_updateScreen(
    Activity *activity,
    View *view,
    uint8_t batteryLevel,
    uint16_t score,
    
    char msgs[MSGS_MAX_COUNT][MAX_TEXT_LEN],
    uint8_t msgCount,
    uint8_t *newMsg
);
void GUI_changeView(View *view, int newView, State *state);


/* View renderers */

void GUI_mainView(Activity activity, uint16_t score);
void GUI_menuView();
void GUI_messagesView(char msgs[MSGS_MAX_COUNT][MAX_TEXT_LEN], uint8_t msgCount);
void GUI_statsView(uint16_t score);
void GUI_settingsView();
void GUI_confmShutdownView();


/* Other functions */

void drawButton(int icon, uint8_t pos);
void drawBatteryIndicator(uint8_t batteryLevel);
void drawScore(uint16_t score);

#endif /* UPSTAIR_GUI_H */
