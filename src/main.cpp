#include <Arduino.h>

#include "stateMan.h"
#include "ioMan.h"
#include "comLineIf.h"
#include "logging.h"



/**
 * @brief Setup the application
 */
void setup()
{
    /* Initialize serial communication and logging */
    Serial.begin( SERIAL_BAUD_RATE );
    logging_setup();
    Log.noticeln( "Door control application %s", GIT_VERSION_STRING );
    Log.noticeln( "Starting ... " );

    /* Initialize command line interface, input/output management and state management */
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
    /* Process the command line interface, and state management */
    comLineIf_process();
    stateMan_process();
}
