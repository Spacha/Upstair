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
  
#ifndef UPSTAIR_H
#define UPSTAIR_H

#define IEEE80154_BROADCAST 0xFFFF      // address for broadcast
#define REG_BAT_OFFSET 0x00000028       // offset to battery voltage address

/* Device LCD dimensions */
#define SCREEN_W 96
#define SCREEN_H 96


/* PERFORMANCE */
#define MAIN_TASK_DELAY 50000           // 1000000/50000 = ~20 times/sec
#define SAMPLE_RATE 2                   // 20/2 = 10 times/sec
#define FRAME_RATE 3                    // 20/3 = 6.7 times/sec

#define AUTO_SLEEP_TIME 1800            // idle time before goung to sleep (in loops)

/* Views */
#define MAIN_MENU_LEN 4

/* Messages */
#define MSGS_MAX_COUNT 6
#define MAX_TEXT_LEN 16                 // how many characters fits to one line

/* Step detection */
#define MA_N 8                          // how many samples for moving average algorithm

extern uint8_t autoSleep;


/* Activity types (or exercises) we can recognize */
typedef enum { ACT_IDLE, ACT_STAIRS, ACT_ELEVATOR } Activity;


/* States of the finite state machine */
typedef enum {
    ST_IDLE,            // do nothing specific
    ST_READ_SENSORS,    // read sensors for new data
    ST_SEND_MSG,        // send an inspirational message
    ST_UPDATE_SCR,      // redraw screen
    ST_SLEEP            // go to sleep
} State;

#endif /* UPSTAIR_H */
