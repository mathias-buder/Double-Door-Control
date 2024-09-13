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


/*************************************** Defines ****************************************/

#define EEPROM_SETTINGS_ADDRESS     0 /*!< The EEPROM address where the settings are stored */
#define EEPROM_EMPTY_CRC            10737431656552524628 /*!< The initial value of the CRC in the EEPROM */

/******************************** Function prototype ************************************/


/**************************** Static Function prototype *********************************/
static uint64_t appSettings_calculateCrc( settings_t* settings );

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


void appSettings_setup( void )
{
    settings_t eepromSettings;
    appSettings_loadSettings( &eepromSettings );
    uint64_t eepromCrc = appSettings_calculateCrc( &eepromSettings );
    uint64_t appCrc    = appSettings_calculateCrc( &appSettings );
    Log.noticeln( "EEPROM CRC: %u", eepromCrc );
    Log.noticeln( "APP CRC: %u", appCrc );
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
 * @brief Loads the application settings from EEPROM into the provided settings structure.
 *
 * This function reads the settings data stored in EEPROM and populates the provided
 * settings structure with this data. It reads byte-by-byte from the EEPROM starting
 * from address 0.
 *
 * @param settings Pointer to the settings structure where the loaded data will be stored.
 *                 If this pointer is NULL, the function logs an error and returns immediately.
 */
void appSettings_loadSettings( settings_t* settings )
{
    if ( settings == NULL )
    {
        Log.error( "%s: settings is NULL", __func__ );
        return;
    }

    uint16_t address = EEPROM_SETTINGS_ADDRESS;
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
 * This function writes the provided settings structure to EEPROM memory.
 * It iterates over each byte of the settings structure and writes it to
 * the EEPROM starting from address 0.
 *
 * @param settings Pointer to the settings structure to be saved. If the
 *                 pointer is NULL, the function logs an error and returns.
 */
void appSettings_saveSettings( settings_t* settings )
{
    if ( settings == NULL )
    {
        Log.error( "%s: settings is NULL", __func__ );
        return;
    }

    uint16_t address = EEPROM_SETTINGS_ADDRESS;
    uint8_t* ptr     = (uint8_t*) settings;

    for ( uint8_t i = 0; i < sizeof( settings_t ); i++ )
    {
        EEPROM.write( address, *ptr );
        ptr++;
        address++;
    }
}

/**
 * @brief Calculates the CRC-64 checksum for the given settings.
 *
 * This function computes the CRC-64 checksum using the polynomial 0xC96C5795D7870F42.
 * It iterates over each byte of the settings structure and updates the CRC value accordingly.
 *
 * @param settings Pointer to the settings structure for which the CRC is to be calculated.
 * @return The calculated CRC-64 checksum.
 */
static uint64_t appSettings_calculateCrc( settings_t* settings )
{
    uint64_t crc = 0xFFFFFFFF;
    uint8_t* ptr = (uint8_t*) settings;

    for ( uint8_t i = 0; i < sizeof( settings_t ); i++ )
    {
        crc = crc ^ *ptr;
        for ( uint8_t j = 0; j < 8; j++ )
        {
            if ( crc & 1 )
            {
                crc = ( crc >> 1 ) ^ 0xC96C5795D7870F42; /*!< Polynomial used in CRC-64-ISO */
            }
            else
            {
                crc = crc >> 1;
            }
        }
        ptr++;
    }

    return crc;
}