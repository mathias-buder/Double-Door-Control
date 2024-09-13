/**
 * \file    logging.c
 * \brief   Source file for logging

 * \author  Mathias Buder
 * \date    2024-09-13

 *  Copyright (c) 2024 Mathias Buder
 */

#include <ArduinoLog.h>
#include "logging.h"

/******************************** Function prototype ************************************/


/**************************** Static Function prototype *********************************/


/******************************** Global variables **************************************/

settings_t settings = {
    .doorUnlockTimeout = DOOR_UNLOCK_TIMEOUT,
    .doorOpenTimeout   = DOOR_OPEN_TIMEOUT,
    .ledBlinkInterval  = LED_BLINK_INTERVAL,
    .debounceDelay     = {
                            DEBOUNCE_DELAY_DOOR_BUTTON_1,
                            DEBOUNCE_DELAY_DOOR_BUTTON_2,
                            DEBOUNCE_DELAY_DOOR_SWITCH_1,
                            DEBOUNCE_DELAY_DOOR_SWITCH_2
                        }
};


/******************************** Function definition ************************************/