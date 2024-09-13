/**
 * \file    logging.c
 * \brief   Source file for logging

 * \author  Mathias Buder
 * \date    2024-09-13

 *  Copyright (c) 2024 Mathias Buder
 */

#include <ArduinoLog.h>
#include <EEPROM.h>

#include "logging.h"

/******************************** Function prototype ************************************/


/**************************** Static Function prototype *********************************/


/******************************** Global variables **************************************/

static settings_t appSettings = {
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


    settings_t eepromSettings;
void appSettings_setup( void )
{
    appSettings_loadSettings( &eepromSettings );
}


/**
 * @brief Retrieves the application settings.
 *
 * This function returns a pointer to the application's settings structure.
 *
 * @return A pointer to the settings_t structure containing the application settings.
 */
settings_t* appSettings_getSettings( void )
{
    return &appSettings;
}


/**
 * @brief Loads the application settings from EEPROM into the settings structure.
 *
 * This function reads the settings data from EEPROM starting at address 0 and
 * stores it into the settings structure. It iterates through each byte of the
 * settings structure and reads the corresponding byte from EEPROM.
 *
 * @note Ensure that the EEPROM library is properly included and initialized before
 * calling this function.
 */
void appSettings_loadSettings( settings_t* settings )
{
    uint16_t address = 0;
    uint8_t* ptr     = (uint8_t*) settings;

    for ( uint8_t i = 0; i < sizeof( settings_t ); i++ )
    {
        *ptr = EEPROM.read( address );
        ptr++;
        address++;
    }
}

/**
 * @brief Saves the application settings to EEPROM.
 * 
 * This function writes the current settings to the EEPROM starting from address 0.
 * It iterates over each byte of the settings structure and writes it to the EEPROM.
 * 
 * @note Ensure that the settings structure is properly defined and initialized before calling this function.
 */
void appSettings_saveSettings( settings_t* settings )
{
    uint16_t address = 0;
    uint8_t* ptr     = (uint8_t*) settings;

    for ( uint8_t i = 0; i < sizeof( settings_t ); i++ )
    {
        EEPROM.write( address, *ptr );
        ptr++;
        address++;
    }
}