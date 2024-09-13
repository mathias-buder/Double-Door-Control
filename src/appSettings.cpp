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

#define EEPROM_SETTINGS_ADDRESS     0                                   /*!< The EEPROM address where the settings are stored */
#define EEPROM_EMPTY_CRC            18446744073709551615ULL             /*!< The initial value of the CRC in the EEPROM */

/******************************** Function prototype ************************************/


/**************************** Static Function prototype *********************************/
static uint64_t appSettings_calculateCrc( settings_t* settings );
static void     appSettings_writeCrc( settings_t* settings );
static uint64_t appSettings_readCrc( void );
static void     appSettings_loadSettings( settings_t* settings );

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


/**
 * @brief Initializes the application settings.
 *
 * This function sets up the application settings by reading the CRC value from the EEPROM.
 * If no settings are found in the EEPROM (indicated by a specific empty CRC value), it uses
 * the default settings. If settings are found, it loads them and verifies their integrity
 * by comparing the CRC value from the EEPROM with a newly calculated CRC value. If the CRC
 * values match, the settings are copied to the global variable. If there is a CRC mismatch,
 * it falls back to using the default settings.
 */
void appSettings_setup( void )
{
    /* Get the CRC value from the EEPROM */
    uint64_t crcFromEeprom = appSettings_readCrc();

    if ( crcFromEeprom == EEPROM_EMPTY_CRC )
    {
        Log.noticeln( "%s: No settings found in EEPROM. Using default settings.", __func__ );
        return;
    }
    else
    {
        /* Local variable to store the settings read from the EEPROM */
        settings_t settings;

        /* Load the settings from the EEPROM */
        appSettings_loadSettings( &settings );

        /* Calculate the CRC value for the settings */
        uint64_t crc = appSettings_calculateCrc( &settings );

        /* Check if the CRC value in the EEPROM matches the calculated CRC value */
        if ( crcFromEeprom != crc )
        {
            Log.warningln( "%s: CRC mismatch. Using default settings.", __func__ );
        }
        else
        {
            /* Copy the settings to the global variable */
            appSettings = settings;
            Log.noticeln( "%s: Settings loaded from EEPROM.", __func__ );
        }
    }
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
    Log.verboseln( "%s: Settings fetched", __func__ );
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
static void appSettings_loadSettings( settings_t* settings )
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

    Log.verboseln( "%s: Settings fetched from EEPROM", __func__ );
}


/**
 * @brief Saves the application settings to EEPROM.
 *
 * This function writes the provided settings structure to the EEPROM
 * starting at a predefined address. It also updates the CRC value in
 * the EEPROM to ensure data integrity.
 *
 * @param settings Pointer to the settings structure to be saved. If the
 *                 pointer is NULL, the function logs an error and returns
 *                 without performing any operation.
 */
void appSettings_saveSettings( void )
{
    uint16_t address = EEPROM_SETTINGS_ADDRESS;
    uint8_t* ptr     = (uint8_t*) &appSettings;

    for ( uint8_t i = 0; i < sizeof( settings_t ); i++ )
    {
        EEPROM.write( address, *ptr );
        ptr++;
        address++;
    }

    /* Update the CRC value in the EEPROM */
    appSettings_writeCrc( &appSettings );

    Log.noticeln( "%s: Settings saved to EEPROM", __func__ );
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

    Log.verboseln( "%s: CRC %l calculated", __func__, crc );

    return crc;
}


/**
 * @brief Writes the CRC value for the given settings to the EEPROM memory.
 *
 * This function calculates the CRC value for the provided settings and writes
 * it to the end of the EEPROM memory. The CRC value is used to ensure data
 * integrity.
 *
 * @param settings Pointer to the settings structure for which the CRC is calculated.
 */
static void appSettings_writeCrc( settings_t* settings )
{
    /* Calculate the CRC value for the settings */
    uint64_t crc = appSettings_calculateCrc( settings );

    /* Write the CRC value to the end of the EEPROM memory */
    uint16_t address = EEPROM.length() - sizeof( uint64_t ) - 1;
    EEPROM.put( address, crc );

    Log.verboseln( "%s: CRC %l written to EEPROM", __func__, crc );
}



/**
 * @brief Reads the CRC value from the end of the EEPROM memory.
 *
 * This function calculates the address of the CRC value stored at the end of the EEPROM memory,
 * reads the CRC value from that address, and returns it.
 *
 * @return The CRC value read from the EEPROM memory.
 */
static uint64_t appSettings_readCrc( void )
{
    uint64_t crc = 0;

    /* Read the CRC value from the end of the EEPROM memory */
    uint16_t address = EEPROM.length() - sizeof( uint64_t ) - 1;
    EEPROM.get( address, crc );

    Log.verboseln( "%s: CRC %l read from EEPROM", __func__, crc );

    return crc;
}