#include <Arduino.h>

#include "stateMan.h"
#include "ioMan.h"
#include "comLineIf.h"
#include "logging.h"


/**
 * @brief Initializes the door control application.
 * 
 * This function sets up the necessary components for the door control application.
 * It performs the following tasks:
 * - Initializes serial communication and logging.
 * - Logs the application version and startup message.
 * - Initializes the command line interface.
 * - Sets up input/output management.
 * - Initializes state management.
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
 * @brief Main loop function that processes the command line interface and state management.
 * 
 * This function is called repeatedly and is responsible for:
 * - Processing the command line interface using `comLineIf_process()`.
 * - Managing the state using `stateMan_process()`.
 */
void loop()
{
    /* Process the command line interface, and state management */
    comLineIf_process();
    stateMan_process();
}
