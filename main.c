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
  * This file is part of Upstair 2.0 software. As long as you retain this notice
  * you can do whatever you want with this stuff. If we meet some day, and you
  * think this stuff is worth it, you can buy me a beer in return.
  * 
  * Miika Sikala
  * ************************************************************
  */
  
/* Standard libs */
#include <inttypes.h>
#include <time.h>

/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Clock.h>

/* TI-RTOS Header files */
#include <ti/drivers/I2C.h>
#include <ti/drivers/PIN.h>
#include <ti/drivers/i2c/I2CCC26XX.h>
#include <ti/drivers/pin/PINCC26XX.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26XX.h>

/* Board Header files */
#include "Board.h"

/* Application specific */
#include "upstair.h"

/* Libraries */
#include "wireless/comm_lib.h"
#include "libs/gui.h"

/* Task stacks */
#define STACKSIZE 2048
Char mainTaskStack[STACKSIZE];
Char commTaskStack[STACKSIZE];



/*******************************
 *        DEFINITIONS          *
 ******************************/


View view = VW_MAIN;
Activity activity = ACT_IDLE;
State state = ST_IDLE;

uint8_t batteryLevel = 0;                   // holds the battery level in volts
uint16_t score = 0;                         // user's activity points
uint8_t newMsg = 0;                         // whether or not there is an unread message

char msgs[MSGS_MAX_COUNT][MAX_TEXT_LEN];    // holds latest received messages
uint8_t msgCount = 0;
uint8_t msgCooldown = 0;                    // this prevents sending too many messages in short period

uint32_t sleepCounter = 0;
uint8_t autoSleep = 0;



/******************************
 *      STEP DETECTION        *
 *****************************/


////////////////////////////////////////////////////////////////////////////////
// ALGORITHM BEHAVIOUR
////////////////////////////////////////////////////////////////////////////////

// Tweak these values to calibrate the step detector.


float treshold          = 0.19;     // how small/big vibrations are counted

uint8_t stairsLimit     = 5;        // if shakiness rises above this --> STAIRS
uint8_t idleLimit       = 3;        // if shakiness drops below this --> IDLE

uint8_t maxShakiness    = 7;        // the greatest possible shakiness
uint8_t minShakiness    = 0;        // the lowest possible shakiness

float shakinessRise     = 0.7;      // how fast shakiness rises
float shakinessDrop     = 0.5;      // how fast shakiness drops back

////////////////////////////////////////////////////////////////////////////////

float shakiness = 0;

uint32_t sampleCount = 0;

// moving average samples for each axis
float ma_samples_x[MA_N];
float ma_samples_y[MA_N];
float ma_samples_z[MA_N];

// moving average values for each axis
float movingAvg_x;
float movingAvg_y;
float movingAvg_z;



/*******************************
 *       CONFIGURE I/O         *
 ******************************/


// Button 1
static PIN_Handle hButton1;
static PIN_State sButton1;
PIN_Config cButton1[] = {
    Board_BUTTON0 | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE,
    PIN_TERMINATE
};

// Button 2
static PIN_Handle hButton2;
static PIN_State sButton2;
PIN_Config cButton2[] = {
    Board_BUTTON1   | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE,
    PIN_TERMINATE
};

// Button 2 is also configured as power button
PIN_Config cPowerWake[] = {
    Board_BUTTON1 | PIN_INPUT_EN | PIN_PULLUP | PINCC26XX_WAKEUP_NEGEDGE,
    PIN_TERMINATE
};


// Leds
static PIN_Handle hLed;
static PIN_State sLed;
PIN_Config cLed[] = {
    Board_LED0 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    PIN_TERMINATE
};

// MPU
static PIN_Handle hMpuPin;
static PIN_State sMpuPin;
static PIN_Config cMpuPin[] = {
    Board_MPU_POWER  | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    PIN_TERMINATE
};

// MPU9250 uses its own I2C interface
static const I2CCC26XX_I2CPinCfg i2cMPUCfg = {
    .pinSDA = Board_I2C0_SDA1,
    .pinSCL = Board_I2C0_SCL1
};



/*******************************
 *          FUNCTIONS          *
 ******************************/


void array_pop(float *arr, float *el, uint16_t len);
float array_sum(float *arr);

uint8_t tresholdExceeded(float ax, float ay, float az);
void readSensors(I2C_Handle *i2c, I2C_Handle *i2cMPU, I2C_Params *i2cParams, I2C_Params *i2cMPUParams, float *results);
void detectStep(float *rawData);
void resetAutoSleep();

void sendInspireMsg();
void readBattery(uint8_t *batteryLevel);
void shutDown();


/**
 * Shifts every element in given array to left and adds @el to the end of
 * the array. The first element is popped out.
 * 
 * NOTE: @arr and @el are type of float!
 * 
 * @arr     The array we are popping
 * @el      Element to be added to the end of @arr
 * @len     Length of @arr
 */
void array_pop(float *arr, float *el, uint16_t len) {
    
    // shift every element one element forward
    uint16_t i;
    for(i=0; i < len-1; i++) {
	    arr[i] = arr[i+1];
	}
	
	// add new element to the end of the array
	arr[len-1] = *el;
}


/**
 * Sum all elements in given array and return the sum.
 * 
 * @arr     Array containing the elements we want to be summed.
 * @return  (Float) sum of the elements.
 */
float array_sum(float *arr) {
	float sum = 0;
	unsigned int i;

	for(i=0; i < MA_N; i++) {
		sum += arr[i];
	}

	return sum;
}


/**
 * Part of stepDetection.
 * 
 * Returns 1 if treshold is exceeded on any axis.
 */
uint8_t tresholdExceeded(float ax, float ay, float az) {
    
    return ((fabs(movingAvg_x - ax) > treshold || fabs(movingAvg_y - ay) > treshold || fabs(movingAvg_z - az) > treshold));

}


/**
 * Read sensors.
 */
void readSensors(I2C_Handle *i2c, I2C_Handle *i2cMPU, I2C_Params *i2cParams, I2C_Params *i2cMPUParams, float *results) {
    
    // Ask data from MPU...
    
    *i2cMPU = I2C_open(Board_I2C, i2cMPUParams);
    if (*i2cMPU == NULL) {
        System_abort("Error Initializing I2CMPU\n");
    }
    
    // Accelometer/gyro: ax, ay, az, gx, gy, gz
    mpu9250_get_data(&i2cMPU,
        &results[0],
        &results[1],
        &results[2],
        &results[3],
        &results[4],
        &results[5]
    );
    
    I2C_close(*i2cMPU);
}


/**
 * Detect steps from accelometer data.
 * 
 * @rawData     Array containing the raw accelometer data from sensor
 */
void detectStep(float *rawData) {
    float ax = rawData[0];
    float ay = rawData[1];
    float az = rawData[2];
    
    // increase sampleCount - how many data pieces we have got in total
    ++sampleCount;
    
    // add new sample to moving average samples, oldest one pops out!
    array_pop(ma_samples_x, &ax, MA_N);
    array_pop(ma_samples_y, &ay, MA_N);
    array_pop(ma_samples_z, &az, MA_N);
    
    // if we have gained enough data so the sample list is full
    if (sampleCount >= MA_N-1) {
        
        // TODO: update all of these at once!!
        // calculate the moving average for each axis individually
        movingAvg_x = array_sum(ma_samples_x) / MA_N;
        movingAvg_y = array_sum(ma_samples_y) / MA_N;
        movingAvg_z = array_sum(ma_samples_z) / MA_N;
        
        // Adjust shakiness
        if(tresholdExceeded(ax, ay, az) && shakiness <= maxShakiness - shakinessRise) {
            
            shakiness += shakinessRise;
            resetAutoSleep();
            
            
        } else {
            
            // lower shakiness
            if(shakiness >= minShakiness + shakinessDrop) {
                
                shakiness -= shakinessDrop;
                
            }
        }
    }
    
    /* Handle result */
    
    if (shakiness >= stairsLimit) {
	    activity = ACT_STAIRS;
    } else if(shakiness < idleLimit) {
	    activity = ACT_IDLE;
	}
    
}

/**
 * Reset sleep counter.
 */
void resetAutoSleep() {
    if (autoSleep) {
        sleepCounter = AUTO_SLEEP_TIME;
    }
}

/**
 * Send an inspiring message to other devices on the range.
 */
void sendInspireMsg() {
    
    char msg[16] = "I'm so fit!";
    
    // send a message to SERVER via 6LoWPAN
    Send6LoWPAN(IEEE80154_BROADCAST, msg, strlen(msg));
    
}


/* Utility Functions */

/**
 * Read current battery voltage of the device.
 */
void readBattery(uint8_t *batteryLevel) {
    uint32_t batReg;
    
    // read the raw value from register
    batReg = HWREG(AON_BATMON_BASE + REG_BAT_OFFSET);
    
    // save the decimal part to the variable without rounding (not good)...
    *batteryLevel = (batReg >> 8);
}


/**
 * Shut down the device.
 */
void shutDown() {
    
    System_printf("Shutting down...\n");
    System_flush();
    
    GUI_closeDisplay();
    
    // Power off MPU
    PIN_setOutputValue(hMpuPin,Board_MPU_POWER, Board_MPU_POWER_OFF);
    
    Task_sleep(100000 / Clock_tickPeriod);

    PIN_close(hLed);
    PIN_close(hMpuPin);
    PIN_close(hButton2);

    // Set wakeup for button 2
    
    PINCC26XX_setWakeup(cPowerWake);
	Power_shutdown(NULL, 0);
}



/*******************************
 *          CALLBACKS          *
 ******************************/

 
/**
 * Callback function for BUTTON 1
 * 
 * Do stuff depending on stuff.
 * 
 */
Void button1Fxn(PIN_Handle handle, PIN_Id pinId) {
    
    // reset sleep counter
    resetAutoSleep();
    
    /* Button press event depents on the current view */
    switch(view) {
        
        case VW_MAIN:
        
            // Open uo the menu
            GUI_changeView(&view, VW_MENU, &state);
            break;
            
        case VW_MENU:
        
            // move menu cursor forward
            GUI_moveMenuCursor(MAIN_MENU_LEN);
            
            // update screen for pointer movement
            state = ST_UPDATE_SCR;
            break;
            
        case VW_MSGS:
        
            // Return to main menu
            GUI_changeView(&view, VW_MENU, &state);
            break;
            
        case VW_STATS:
        
            // Return to main menu
            GUI_changeView(&view, VW_MENU, &state);
            break;
            
        case VW_SETTINGS:
        
            // Return to main menu
            GUI_changeView(&view, VW_MENU, &state);
            break;
            
        case VW_CONFM_SHUTDOWN:
            
            // Return from confirmation view to main view
            GUI_changeView(&view, VW_MAIN, &state);
            break;
    }
    
}

/**
 * Callback function for BUTTON 2
 * 
 * Do stuff depending on stuff.
 * 
 */
Void button2Fxn(PIN_Handle handle, PIN_Id pinId) {
    
    // reset sleep counter
    resetAutoSleep();
    
    /* Button press event depents on the current view */
    switch(view) {
        
        case VW_MAIN:
        
            // switch the led on/off (for fun)
            PIN_setOutputValue(hLed, Board_LED0, !PIN_getOutputValue(Board_LED0));
            
            // Go to shut down confirmation view
            GUI_changeView(&view, VW_CONFM_SHUTDOWN, &state);
            break;
            
        case VW_MENU:
        
            // Select menu option based on cursor position
            switch(GUI_menuPos()) {
                
                case 0:
                
                    // Return to main screen
                    GUI_changeView(&view, VW_MAIN, &state);
                    break;
                    
                case 1:
                    
                    // Go to messages view
                    GUI_changeView(&view, VW_MSGS, &state);
                    break;
                    
                case 2:
                
                    // Go to statistics view
                    GUI_changeView(&view, VW_STATS, &state);
                    break;
                    
                case 3:
                
                    // Go to settings view
                    GUI_changeView(&view, VW_SETTINGS, &state);
                    break;
                    
            }
            
            break;
            
        case VW_SETTINGS:
        
            // change settings
            
            switch (GUI_settingsMenuPos()) {
                case 0:
                
                    // toggle timer
                    autoSleep = !autoSleep;
                    if (autoSleep) {
                        sleepCounter = AUTO_SLEEP_TIME;
                    }
            }
            break;
            
        case VW_CONFM_SHUTDOWN:
            
            // Turn off the device
            shutDown();
            break;
    }
}



/*******************************
 *           TASKS             *
 ******************************/


Void mainTask(UArg arg0, UArg arg1) {
    
    /*******************
     *   INIT THINGS   *
     ******************/
    
    /* LCD Display */
    
    GUI_initDisplay();
    
    
    /* Sensors */
    
    I2C_Handle      i2c;
    I2C_Params      i2cParams;
    I2C_Handle      i2cMPU;
	I2C_Params      i2cMPUParams;
	
	// General I2C
	I2C_Params_init(&i2cParams);
    i2cParams.bitRate = I2C_400kHz;
	
	// Custom I2C for MPU9250 sensor
	I2C_Params_init(&i2cMPUParams);
    i2cMPUParams.bitRate = I2C_400kHz;
    i2cMPUParams.custom = (uintptr_t)&i2cMPUCfg;
	
	// this always holds the lastest raw data samples
	float realTimeData[6];
	
	
	/* Init general I2C */
	
	i2c = I2C_open(Board_I2C0, &i2cParams);
    if (i2c == NULL) {
        System_abort("Error Initializing I2C\n");
    }
    
    // setup pressure sensor
    // bmp280_setup(&i2c);
    
    I2C_close(i2c);
    
    
    /* Init I2C for MPU */
    
    i2cMPU = I2C_open(Board_I2C, &i2cMPUParams);
    if (i2cMPU == NULL) {
        System_abort("Error Initializing I2CMPU\n");
    }
    
    // power on for MPU
    PIN_setOutputValue(hMpuPin, Board_MPU_POWER, Board_MPU_POWER_ON);

    // WAIT 100MS FOR THE SENSOR TO POWER UP
	Task_sleep(100000 / Clock_tickPeriod);
    System_printf("MPU9250: Power ON\n");
    System_flush();
    
    System_printf("MPU9250: Setup and calibration...\n");
	System_flush();

	mpu9250_setup(&i2cMPU);

	System_printf("MPU9250: Setup and calibration OK\n");
	System_flush();
	
	I2C_close(i2cMPU);
	
	
    
    /*****************
     *   MAIN LOOP   *
     ****************/
    
    // main loop's 'program counter'
    uint32_t loop = 0;
    
    while(1) {
        
        // These things we do every time...
        
        // read sensors
        if (loop % SAMPLE_RATE == 0) {
            state = ST_READ_SENSORS;
        }
        
        // refresh screen
        if (loop % FRAME_RATE == 0) {
            state = ST_UPDATE_SCR;
        }
        
        // read battery level
        if (loop % 15) {
            readBattery(&batteryLevel);
        }
        
        // Update scores (BETA) about once per second
        if (activity == ACT_STAIRS && loop % 20 == 0) {
            score += 1;
        }
        
        // if in 'messages' view, mark all messages as read
        if (view == VW_MSGS && newMsg) {
            newMsg = 0;
        }
        
        // if it's time to go to sleep, make it happen
        if (autoSleep && sleepCounter <= 0) {
            state = ST_SLEEP;
        }
        
        // This draws everything that needs to be updated on every loop
        // GUI_drawDynamics(&view, shakiness); (REMOVED)
        
        
        // These we do according to the current state...
        
        switch(state) {
                
            case ST_READ_SENSORS:
            
                // Read sensor data
                readSensors(&i2cParams, &i2c, &i2cMPU, &i2cMPUParams, &realTimeData);
                
                // Detect steps from 'real-time' data
                detectStep(&realTimeData);
                
                if (shakiness >= stairsLimit) {
            	    state = ST_SEND_MSG;    // send an inspirational message
            	} else {
            	    state = ST_IDLE;
            	}
            	
                break;
                
            case ST_SEND_MSG:
                
                // we don't want to keep sending messages constantly
                if (msgCooldown <= 0) {
                    
                    sendInspireMsg();
                    msgCooldown = SAMPLE_RATE * 80;
                    
                }
                
                state = ST_IDLE;
                break;
                
            case ST_UPDATE_SCR:
            
                // Update screen
                GUI_updateScreen(&activity, &view, batteryLevel, score, msgs, msgCount, &newMsg);
                
                state = ST_IDLE;
                break;
                
            case ST_SLEEP:
                
                // Go to sleep
                shutDown();
                
        }
        
        // maybe is not the most eloquent way...
        // anyway, decrementing the cooldown like a boss...
        if (msgCooldown > 0) {
            --msgCooldown;
        }
        
        // decrement sleep counter
        if (autoSleep && sleepCounter > 0) {
            --sleepCounter;
        }
        
        // give time for other tasks...
        Task_sleep((UInt)arg0);
        ++loop;
    }
    
}

/**
 * Task for receiving and handling messages coming through radio.
 */
Void commTask(UArg arg0, UArg arg1) {
    
    System_printf("CommTask\n");
    System_flush();
    
    uint16_t senderAddr;

    // Radio to receive mode
	int32_t result = StartReceive6LoWPAN();
	if(result != true) {
		System_abort("Wireless receive mode failed");
	}

    char receivedMsg[MAX_TEXT_LEN];

    while (1) {
        
        // No System_printf() here, apparently... It makes the whole thing stagger.
        
    	// if there are messages waiting
        if (GetRXFlag()) {

            // clear buffer from old stuff
            memset(msgs[msgCount], 0, 8);
            
            // read buffer to variable payload
            Receive6LoWPAN(&senderAddr, msgs[msgCount], MAX_TEXT_LEN);
            
            if (msgCount < MSGS_MAX_COUNT) {
                ++msgCount;
            }
            
            // set the 'unread messages' flag on
            newMsg = 1;
      }
    }
}



/*******************************
 *         MAIN TASK           *
 ******************************/


int main(void) {
    
    // Task variables
	Task_Handle hMainTask;
	Task_Params mainTaskParams;
	
	Task_Handle hCommTask;
	Task_Params commTaskParams;
    
    // Initialize board
    Board_initGeneral();
    Board_initI2C();
    Init6LoWPAN();
    
    
    /********************
     *   Init buttons   *
     *******************/

    // Button 1
	hButton1 = PIN_open(&sButton1, cButton1);
	if(!hButton1) {
		System_abort("Error initializing BUTTON 1\n");
	}
	if (PIN_registerIntCb(hButton1, &button1Fxn) != 0) {
		System_abort("Error registering power button callback function");
	}

    // Button 2
    hButton2 = PIN_open(&sButton2, cButton2);
    if(!hButton2) {
        System_abort("Error initializing BUTTON 2\n");
    }
    if(PIN_registerIntCb(hButton2, &button2Fxn) != 0) {
        System_abort("Error registering random button callback function");
    }
    
    
    /*****************************
     *   Init other components   *
     ****************************/

    // Leds
    hLed = PIN_open(&sLed, cLed);
    if(!hLed) {
        System_abort("Error initializing LED pin\n");
    }
    
    // Init MPU power pin
    hMpuPin = PIN_open(&sMpuPin, cMpuPin);
    if (hMpuPin == NULL) {
    	System_abort("MPU pin open failed!");
    }
    
    
    /******************
     *   Init tasks   *
     *****************/
     
     // Init Main Task
    
    Task_Params_init(&mainTaskParams);
    mainTaskParams.stackSize = STACKSIZE;
    mainTaskParams.stack = &mainTaskStack;
    mainTaskParams.priority=2;
    mainTaskParams.arg0 = MAIN_TASK_DELAY / Clock_tickPeriod;
    
    hMainTask = Task_create(mainTask, &mainTaskParams, NULL);
    if (hMainTask == NULL) {
    	System_abort("Error creating mainTask\n");
    }
    
    // Init Communication Task
    
    Task_Params_init(&commTaskParams);
    commTaskParams.stackSize = STACKSIZE;
    commTaskParams.stack = &commTaskStack;
    commTaskParams.priority=1;
    
    hCommTask = Task_create(commTask, &commTaskParams, NULL);
    if (hCommTask == NULL) {
        System_abort("Error creating commTask\n");
    }
    
    
    /************
     *   Boot   *
     ***********/
    
    System_printf("Board initiated, starting...\n");
    System_flush();
    
    // Start BIOS
    BIOS_start();
    
    return 0;
}
