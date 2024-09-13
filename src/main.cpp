#include <Arduino.h>

#include "stateMan.h"
#include "ioMan.h"
#include "comLineIf.h"
#include "logging.h"


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

    ioMan_Setup();
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
