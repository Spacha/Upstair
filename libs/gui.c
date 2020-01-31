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
  
#include "libs/gui.h"



/*******************************
 *        DEFINITIONS          *
 ******************************/

/* Display */
Display_Handle hDisplay;

/* Gfx context */
tContext *pContext;

/* Button icons */
enum ButtonIcon {
    ICON_MENU,
    ICON_POWER,
    ICON_BACK,
    ICON_SELECT,
    ICON_DOWN,
    ICON_EMPTY,
};


/* Presets */
uint8_t menuPos = 0;            // holds the menu cursor position
uint8_t settingsMenuPos = 0;    // holds the settings menu cursor position
uint8_t forceScrClear = 0;      // if this is set to true, screen is forced to update



/*******************************
 *          FUNCTIONS          *
 ******************************/


/**
 * Getter for menu position.
 */
uint8_t GUI_menuPos() {
    return menuPos;
}

/**
 * Getter for settings menu position.
 */
uint8_t GUI_settingsMenuPos() {
    return settingsMenuPos;
}

/**
 * Initialize display and gfx context.
 */
void GUI_initDisplay() {
    
    Display_Params displayParams;
	displayParams.lineClearMode = DISPLAY_CLEAR_BOTH;
    Display_Params_init(&displayParams);

    hDisplay = Display_open(Display_Type_LCD, &displayParams);
    if (hDisplay == NULL) {
        System_abort("Error initializing Display\n");
    }
    
    // create context for drawing graphic
    pContext = DisplayExt_getGrlibContext(hDisplay);
    if (pContext == NULL) {
        System_abort("Could not create a context for display\n");
    }
    
}


/**
 * Clear display.
 */
void GUI_clearDisplay() {
    
    Display_clear(hDisplay);
    
}


/**
 * Clear and close display.
 */
void GUI_closeDisplay() {
    
    Display_clear(hDisplay);
    Display_close(hDisplay);
    
}


/**
 * Move menu cursor one step forward and loop back after last item.
 * 
 * @itemCount   How many items there are in the menu
 */
void GUI_moveMenuCursor(uint8_t itemCount) {
    
    // Move menu cursor
    if (menuPos < itemCount-1) {
        menuPos += 1;
    } else {
        menuPos = 0;
    }
    
    // update screen so no ghosts of menu items remain
    forceScrClear = 1;
    
}


/**
 * Update the screen and redraw everything if necessary.
 * 
 * @activity        Handle to current ativity
 * @view            Handle to current view
 * @batteryLevel    Current battery level reading
 * @score           Current score
 * @msgs            Array containing received messages
 * @msgCount        Amount of messages in array @msgs
 * @newMsg          True/false if there are unread messages
 */
void GUI_updateScreen(Activity *activity, View *view, uint8_t batteryLevel, uint16_t score, char msgs[MSGS_MAX_COUNT][MAX_TEXT_LEN], uint8_t msgCount, uint8_t *newMsg) {
    
    // clear screen before updating if forced
    if (forceScrClear) {
        GUI_clearDisplay();
        forceScrClear = 0;
    }
    
    /* Things to be drawn in every view */
    
    // Battery level indicator
    drawBatteryIndicator(batteryLevel);
    
    // New (unread) messages waiting: draw a message icon to top
    if (*newMsg) {
        GrImageDraw(pContext, &icon_messages, 22, -1);
    }
    
    /* View-specific drawings */
    switch(*view) {
        
        case VW_MAIN:
            GUI_mainView(*activity, score);
            break;
            
        case VW_MENU:
            GUI_menuView();
            break;
            
        case VW_MSGS:
            GUI_messagesView(msgs, msgCount);
            break;
            
        case VW_STATS:
            GUI_statsView(score);
            break;
            
        case VW_SETTINGS:
            GUI_settingsView();
            break;
            
        case VW_CONFM_SHUTDOWN:
            GUI_confmShutdownView();
            break;
    }
    
    
    /* These will be drawn on top of everything else */
    
    // The vertical line on the right side
    GrLineDraw(pContext, SCREEN_W-17, 0, SCREEN_W-17, SCREEN_H);
    
    // Flush graphics from the buffer
    GrFlush(pContext);
}


/**
 * Change view.
 * 
 * @view        Handle to current view
 * @newView     The view to be changed to
 * @state       Handle to State
 */
void GUI_changeView(View *view, int newView, State *state) {
    
    *view = newView;
    
    forceScrClear = 1;
    *state = ST_UPDATE_SCR;     // update screen after view has changed
    
}



/******************************
 *           VIEWS            *
 *****************************/


/**
 * Draws the main/home screen including activity icon in the middle and
 * the score reading on the bottom.
 * 
 * @activity        Handle to Activity
 * @score           Current score.
 */
void GUI_mainView(Activity activity, uint16_t score) {

    // Show activity icon on the screen
    
    if (activity == ACT_STAIRS) {
        GrImageDraw(pContext, &img_stairs_up, 20, 17);
    } else {
        GrImageDraw(pContext, &img_standing, 20, 17);
    }
    
    // draw the price and current score
    drawScore(score);
    
    // GUI elements are drawn last so they are always on top
    drawButton(ICON_MENU, 1);
    drawButton(ICON_POWER, 2);
    
}


/**
 * Draw the main menu.
 */
void GUI_menuView() {
    
    // array containing pointers to the menu icons
    // these are ordered as they are in the menu by default (active is second etc.)
    tImage *menuIcons[MAIN_MENU_LEN] = {
        &icon_settings,
        &icon_home,
        &icon_messages,
        &icon_stats,
    };
    
    char labels[MAIN_MENU_LEN][MAX_TEXT_LEN] = {
        "Settings",
        "Home",
        "Messages",
        "Stats"
    };
    
    uint8_t i;
    uint8_t lineH = 20;     // 16px for the icon, 2px for top and bottom as padding
    uint8_t itemPos = 0;    // used in calculating menu order
    uint8_t linePosY = 0;   // actual line position on the screen
    
    
    // this box highlights the active selection
    tRectangle activeRect = { 2, 24 + lineH - 2, SCREEN_W-17, 24 + 2*lineH - 4 };
    
    for(i=0; i < MAIN_MENU_LEN; i++) {
        
        // menu drawing order
        itemPos = i + menuPos;
        if (itemPos >= MAIN_MENU_LEN) {
            itemPos -= MAIN_MENU_LEN;
        }
        
        linePosY = 24 + lineH*i;
        
        // no need to draw stuff 'outside of the screen'
        if (linePosY > SCREEN_H) {
            continue;
        }
        
        // draw the menu item box and icon
        GrStringDraw(pContext, labels[itemPos], -1, 24, linePosY + 4, 0);
        GrRectDraw(pContext, &activeRect);
        GrImageDraw(pContext, menuIcons[itemPos], 4, linePosY);
        
    }

    // GUI elements are drawn last so they are always on top
    drawButton(ICON_DOWN, 1);
    drawButton(ICON_SELECT, 2);
    
}


/**
 * Draw the messages view including recently received messages.
 * 
 * @messages    Array containing the received messages
 * @msgCount    Amount of these messages
 */
void GUI_messagesView(char messages[MSGS_MAX_COUNT][MAX_TEXT_LEN], uint8_t msgCount) {
    
    GrStringDraw(pContext, "MESSAGES", -1, 4, GUI_CONTENT_Y, 0);
    
    // if no messages
    if (msgCount == 0) {
        
        GrStringDraw(pContext, "No messages", -1, 4, GUI_CONTENT_Y + 16, 0);
        
    } else {
        
        // if there are messages to show...
        
        char msg[MAX_TEXT_LEN];
    
        uint8_t i;
        for(i=0; i < msgCount; i++) {
            
            sprintf(msg, "%s", messages[i]);
            GrStringDraw(pContext, msg, -1, 4, GUI_CONTENT_Y+12 + (7+8)*i+4, 1);
            
            // draw a horizontal separator between messages
            if (i > 0) {
                GrLineDrawH(pContext, 4, SCREEN_W-17, GUI_CONTENT_Y+12 + (7+8)*i);
            }
        }
    }
    
    // GUI elements are drawn last so they are always on top
    drawButton(ICON_BACK, 1);
    drawButton(ICON_EMPTY, 2);
}


/**
 * Draw the statistics view.
 * 
 * This was supposed to show so much more stuff but sadly I ran out of time...
 * 
 * @score   Current score
 */
void GUI_statsView(uint16_t score) {
    char score_str[MAX_TEXT_LEN];
    
    GrStringDraw(pContext, "STATS", -1, 4, GUI_CONTENT_Y, 0);
    
    sprintf(score_str, "Score: %d", score);
    GrStringDraw(pContext, score_str, -1, 4, GUI_CONTENT_Y + 16, 1);
    
    // GUI elements are drawn last so they are always on top
    drawButton(ICON_BACK, 1);
    drawButton(ICON_EMPTY, 2);
}


/**
 * Draw the settings view.
 */
void GUI_settingsView() {
    tRectangle checkBox_empty = {   // checkbox (11 x 11)
        SCREEN_W-17-11-2,           // x1
        GUI_CONTENT_Y + 16-2,       // y1
        SCREEN_W-17-2,              // x2
        GUI_CONTENT_Y + 16+11-2     // y2
    };
    
    tRectangle activeRect = {
        2,
        GUI_CONTENT_Y + 16 - 2,
        SCREEN_W-17,
        GUI_CONTENT_Y + 16+7 + 2
    };
    
    GrStringDraw(pContext, "SETTINGS", -1, 4, GUI_CONTENT_Y, 0);
    
    
    // From now on, this is hard-coded (lack of time)
    
    // if checked, draw check mark
    if (autoSleep) {
        GrImageDraw(pContext, &icon_select, SCREEN_W-17-9-2-4, GUI_CONTENT_Y + 16-1-3);
    } else {
        GrImageDraw(pContext, &icon_empty, SCREEN_W-17-9-2-4, GUI_CONTENT_Y + 16-1-3);
    }
    
    GrStringDraw(pContext, "Auto-sleep", -1, 4, GUI_CONTENT_Y + 16, 1);

    GrRectDraw(pContext, &checkBox_empty);
    
    // if autoSleep option active...
    if (settingsMenuPos == 0) {
        GrRectDraw(pContext, &activeRect);
    }
    
    // GUI elements are drawn last so they are always on top
    drawButton(ICON_BACK, 1);
    drawButton(ICON_SELECT, 2);
    
}


/**
 * Confirm shutdown. This is the view that asks: "Are you sure?"
 */
void GUI_confmShutdownView() {
    
    GrStringDrawCentered(pContext, "Are you", -1, (SCREEN_W-17)/2, 30, 0);
    GrStringDrawCentered(pContext, "sure?", -1, (SCREEN_W-17)/2, 39, 0);
    GrImageDraw(pContext, &icon_power, (SCREEN_W-17)/2-8, 52);
    
    // GUI elements are drawn last so they are always on top
    drawButton(ICON_BACK, 1);
    drawButton(ICON_SELECT, 2);
}


/* Smaller parts... */

/**
 * Draw button icons.
 * 
 * @icon        The icon to draw
 * @pos         1 => upper position, 2 => lower position
 * 
 */
void drawButton(int icon, uint8_t pos) {

    uint8_t empty = 0;
    
    // draw the button icon (upper or lower corner depending on pos)
    switch(icon) {
        
        case ICON_MENU:
        
            GrImageDraw(pContext, &icon_menu, SCREEN_W-16, (pos==2) ? (SCREEN_H-16):1);
            break;
            
        case ICON_POWER:
        
            GrImageDraw(pContext, &icon_power, SCREEN_W-16, (pos==2) ? (SCREEN_H-16):1);
            break;
            
        case ICON_BACK:
        
            GrImageDraw(pContext,  &icon_back, SCREEN_W-16, (pos==2) ? (SCREEN_H-16):1);
            break;
            
        case ICON_SELECT:
        
            GrImageDraw(pContext, &icon_select, SCREEN_W-16, (pos==2) ? (SCREEN_H-16):1);
            break;
            
        case ICON_DOWN:
        
            GrImageDraw(pContext, &icon_down, SCREEN_W-16, (pos==2) ? (SCREEN_H-16):1);
            break;
            
        default:
        
            // By default, show no icon
            GrImageDraw(pContext, &icon_empty, SCREEN_W-16, (pos==2) ? (SCREEN_H-16):1);
            empty = 1;
            break;
    }
    
    // draw the button borderline (horizontal), except for empty icon
    if (!empty) {
        GrLineDrawH(pContext,  SCREEN_W-17, SCREEN_W, (pos==2) ? (SCREEN_H-17) : 17);
    }
}

/**
 * Draw battery level indicator to upper left corner.
 * 
 * This could be improved: make a constant BAT_MAX_VOLTAGE and compare the
 * current voltage to that. This is too hard-coded but works just fine.
 * 
 * @batteryLevel    Current battery voltage.
 */
void drawBatteryIndicator(uint8_t batteryLevel) {
    
    // these are the 'bars' of the battery icon
    tRectangle bars[3][4] = {   // coords of the 3 x 6 boxes
        { 4,  4,  6, 9 },
        { 8,  4, 10, 9 },
        { 12, 4, 14, 9 }
    };
    
    // draw the base icon of the battery
    GrImageDraw(pContext, &icon_battery, 0, 0);
    
    // draw the bars to the battery icon
    uint8_t i;
    for(i=0; i < batteryLevel; i++) {
        
        // if batteryLevel is bigger than 3 for some reason, break
        if (i > 2) {
            break;
        }
        
        // this is my hax to fill those rects since GrRectFill() didn't work :(
        GrRectDraw(pContext, bars[i]);
        GrLineDrawV(pContext, 5+i*4, 5, 8);
    }
}


/**
 * Draw current score and a price icon to the bottom left corner of the screen.
 * 
 * @score   Current score.
 */
void drawScore(uint16_t score) {
    char score_str[8];
    
    sprintf(score_str, "%d", score);
    
    // 2px margin to corner
    GrImageDraw(pContext, &icon_price, 0, SCREEN_H-18);
    GrStringDraw(pContext, score_str, -1, 2+12+2, SCREEN_H-2-11, 1);
}
