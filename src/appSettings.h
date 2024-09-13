/**
 * \file    appSettings.h
 * \brief   Header file for containing all application settings

 * \author  Mathias Buder
 * \date    2024-09-13

 *  Copyright (c) 2024 Mathias Buder
 */

#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include "ioMan.h"


/***************************************************************************************************/
/*                                           MACRO DEFINITIONS                                      */
/***************************************************************************************************/
#define AUX( x ) #x
#define STRINGIFY( x ) AUX( x )

/***************************************************************************************************/
/*                                           PIN CONFIGURATION                                     */
/***************************************************************************************************/
/* DOOR 1 */
/***************************************************************************************************/
#define RBG_LED_1_R                     7              /*!< Pin for the red LED of the RGB-LED */
#define RBG_LED_1_G                     6              /*!< Pin for the green LED of the RGB-LED */
#define RBG_LED_1_B                     5              /*!< Pin for the blue LED of the RGB-LED */
#define DOOR_1_BUTTON                   2              /*!< Pin for the button of the door */
#define DOOR_1_SWITCH                   3              /*!< Pin for the switch of the door */
#define DOOR_1_MAGNET                   4              /*!< Pin for the magnet of the door */

/***************************************************************************************************/
/* DOOR 2 */
/***************************************************************************************************/
#define RBG_LED_2_R                     13             /*!< Pin for the red LED of the RGB-LED */
#define RBG_LED_2_G                     12             /*!< Pin for the green LED of the RGB-LED */
#define RBG_LED_2_B                     11             /*!< Pin for the blue LED of the RGB-LED */
#define DOOR_2_BUTTON                   10             /*!< Pin for the button of the door */
#define DOOR_2_SWITCH                   9              /*!< Pin for the switch of the door */
#define DOOR_2_MAGNET                   8              /*!< Pin for the magnet of the door */
/***************************************************************************************************/
/*                                        GENERAL CONFIGURATION                                    */
/***************************************************************************************************/
#define DEBOUNCE_DELAY_DOOR_BUTTON_1    100            /*!< Debounce delay for the door button @unit ms*/
#define DEBOUNCE_DELAY_DOOR_BUTTON_2    100            /*!< Debounce delay for the door button @unit ms*/
#define DEBOUNCE_DELAY_DOOR_SWITCH_1    100            /*!< Debounce delay for the door switch @unit ms*/
#define DEBOUNCE_DELAY_DOOR_SWITCH_2    100            /*!< Debounce delay for the door switch @unit ms*/
#define DEBOUNCE_STABLE_TIMEOUT         300            /*!< Timeout for the debounce stable state @unit ms */

#define SERIAL_BAUD_RATE                115200         /*!< Baud rate of the serial communication @unit bps */
#define DEFAULT_LOG_LEVEL               LOG_LEVEL_INFO /*!< Default log level */

#define LED_BLINK_INTERVAL              500            /*!< Interval of the led blink @unit ms */
#define DOOR_UNLOCK_TIMEOUT             5              /*!< Timeout for the door unlock ( 0 = disabled ) @unit s */
#define DOOR_OPEN_TIMEOUT               600            /*!< Timeout for the door open ( 0 = disabled ) @unit s */



/************************************ ENUMERATION *************************************/



/************************************* STRUCTURE **************************************/

typedef struct
{
    uint8_t  doorUnlockTimeout;            /*!< The door unlock timeout */
    uint16_t doorOpenTimeout;              /*!< The door open timeout */
    uint16_t ledBlinkInterval;             /*!< The led blink interval */
    uint16_t debounceDelay[IO_INPUT_SIZE]; /*!< The debounce delay of the inputs */
} settings_t;


/******************************** Function prototype ************************************/

void        appSettings_setup( void );
settings_t* appSettings_getSettings( void );
void        appSettings_loadSettings( settings_t* settings );
void        appSettings_saveSettings( settings_t* settings );

#endif  // APPSETTINGS_H