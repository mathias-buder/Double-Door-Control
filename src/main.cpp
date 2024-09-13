#include <Arduino.h>

#include "appSettings.h"
#include "stateManagement.h"
#include "ioManagement.h"
#include "logging.h"
#include "storage.h"


/********************************************** ENUMERATION ****************************************/

/************************************* STRUCTURE **************************************/


/******************************** Function prototype ************************************/


static void                   doorOpenTimeoutHandler( uint32_t time );
static void                   doorUnlockTimeoutHandler( uint32_t time );



/**
 * @brief Setup the application
 */
void setup()
{
    /* Initialize with log level and log output */
    Serial.begin( SERIAL_BAUD_RATE );
    Log.begin( DEFAULT_LOG_LEVEL, &Serial );
    Log.noticeln( "Door control application %s", GIT_VERSION_STRING );
    Log.noticeln( "Starting ... " );
    /* Switch to the init state */
    switch_state( &doorControl.machine, &doorControlStates[DOOR_CONTROL_STATE_INIT] );

    Log.noticeln( "... Done" );
}


/**
 * @brief The main loop of the application
 */
void loop()
{
    /* Process the command line interface */
    if ( Serial.available() )
    {
        String input = Serial.readStringUntil( '\n' );
        cli.parse( input );

        // String testCommand( "timer -d 0 -u 5 -o 600 -b 500" );
        // String testCommand( "log" );
        // String testCommand1( "timer -u 30 -o 18 -b 180" );
        // cli.parse( testCommand1);
        // String testCommand2( "info" );
        // cli.parse( testCommand2);
        // String testCommand3( "time -f" );
        // cli.parse( testCommand3 );
        // String testCommand4( "dbc -i 3 -t 128" );
        // cli.parse( testCommand4);
        // String testCommand5( "dbc -t 300" );
        // cli.parse( testCommand5);
        // String testCommand6( "dbc -t 300" );
        // cli.parse( testCommand6);
    }

    /* Generate/Process events */
    generateEvent( &doorControl );

    /* Process door open timers */
    processTimers( &doorControl );

    /* Dispatch the event to the state machine */
    if ( dispatch_event( stateMachines, 1, eventLogger, resultLogger ) == EVENT_UN_HANDLED )
    {
        Log.errorln( "Event is not handled" );
    }
}
