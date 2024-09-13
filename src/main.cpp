#include <Arduino.h>

#include "appSettings.h"
#include "stateMan.h"
#include "ioMan.h"
#include "logging.h"
#include "storage.h"
#include "comLineIf.h"


/********************************************** ENUMERATION ****************************************/

/************************************* STRUCTURE **************************************/


/******************************** Function prototype ************************************/


/**
 * @brief Setup the application
 */
void setup()
{
    /* Initialize with log level and log output */
    Serial.begin( SERIAL_BAUD_RATE );

    logging_setup();

    Log.noticeln( "Door control application %s", GIT_VERSION_STRING );
    Log.noticeln( "Starting ... " );

    comLineIf_setup();

    stateMan_setup();

    Log.noticeln( "... Done" );
}


/**
 * @brief The main loop of the application
 */
void loop()
{
    comLineIf_process();
    stateMan_process();
}
