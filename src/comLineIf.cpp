/**
 * \file    cli.c
 * \brief   Source file for command line interface

 * \author  Mathias Buder
 * \date    2024-09-13

 *  Copyright (c) 2024 Mathias Buder
 */

#include <Arduino.h>
#include <SimpleCLI.h>
#include <TimerOne.h>

#include "comLineIf.h"
#include "appSettings.h"
#include "logging.h"
#include "ioMan.h"


static void comLineIf_cmdGetInfoCb( cmd* pCommand );
static void comLineIf_cmdSetLogLevelCb( cmd* pCommand );
static void comLineIf_cmdSetTimerCb( cmd* pCommand );
static void comLineIf_cmdSetDebounceDelayCb( cmd* pCommand );
static void comLineIf_cmdHelpCb( cmd* pCommand );
static void comLineIf_cmdGetInputStateCb( cmd* pCommand );
static void comLineIf_cmdErrorCb( cmd_error* pError );

static SimpleCLI cli;                 /*!< The command line interface */
static Command   cmdGetInfo;          /*!< Get software information */
static Command   cmdSetLogLevel;      /*!< Set log level */
static Command   cmdSetTimer;         /*!< Set all timers */
static Command   cmdSetDebounceDelay; /*!< Setall debounce delays */
static Command   cmdGetInputState;    /*!< Get the state of all inputs */
static Command   cmdHelp;             /*!< Pint the help */

void comLineIf_setup( void )
{
    /* Initialize the command line interface */
    cmdGetInfo = cli.addSingleArgCmd( "info", comLineIf_cmdGetInfoCb ); /*!< Get software information */
    cmdGetInfo.setDescription( "Get software information" );

    cmdSetLogLevel = cli.addSingleArgCmd( "log", comLineIf_cmdSetLogLevelCb ); /*!< Set log level */
    cmdSetLogLevel.setDescription( "Set the log level: log <level (0..6)>" );

    cmdSetTimer = cli.addCmd( "timer", comLineIf_cmdSetTimerCb );       /*!< Set timer */
    cmdSetTimer.addArg( "u", STRINGIFY( DOOR_UNLOCK_TIMEOUT ) ); /*!< Unlock timeout */
    cmdSetTimer.addArg( "o", STRINGIFY( DOOR_OPEN_TIMEOUT ) );   /*!< Open timeout */
    cmdSetTimer.addArg( "b", STRINGIFY( LED_BLINK_INTERVAL ) );  /*!< LED blink interval */
    cmdSetTimer.setDescription( "Set the timer. timer -u <unlock timeout (s)> -o <open timeout (min)> -b <blink interval (ms)>" );

    cmdSetDebounceDelay = cli.addCmd( "dbc", comLineIf_cmdSetDebounceDelayCb ); /*!< Set debounce time */
    cmdSetDebounceDelay.addArg( "i" );                                   /*!< Input index */
    cmdSetDebounceDelay.addArg( "t" );                                   /*!< Debounce time @unit ms */
    cmdSetDebounceDelay.setDescription( "Set the debounce time. dbc -i <input index (0..3)> -t <debounce time (ms)>" );

    cmdGetInputState = cli.addSingleArgCmd( "inputs", comLineIf_cmdGetInputStateCb ); /*!< Get the input state */
    cmdGetInputState.setDescription( "Get the input state of all buttons and switches" );

    cmdHelp = cli.addCmd( "help", comLineIf_cmdHelpCb ); /*!< Help */
    cmdHelp.setDescription( "Show the help" );

    cli.setOnError( comLineIf_cmdErrorCb );

}


void comLineIf_process( void )
{
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
}



static void comLineIf_cmdGetInfoCb( cmd* pCommand )
{
    Serial.println( "----------------------------------" );
    Serial.println( "Door Control System Information   " );
    Serial.println( "----------------------------------" );

    /* Output software version string */
    Serial.println( "Version: " + String( GIT_VERSION_STRING ) );

    /* Output the build date and time */
    Serial.println( "Build date: " + String( __DATE__ ) + " " + String( __TIME__ ) );

    /* Output current log level */
    Serial.println( "Log level: " + logging_logLevelToString( Log.getLevel() ) );

    /* Output the all times and timeouts */
    Serial.println( "Door unlock timeout: " + String( settings.doorUnlockTimeout ) + " s" );
    Serial.println( "Door open timeout: " + String( settings.doorOpenTimeout ) + " min" );
    Serial.println( "Led blink interval: " + String( settings.ledBlinkInterval ) + " ms" );

    for ( uint8_t i = 0; i < IO_INPUT_SIZE; i++ )
    {
        Serial.println( "Debounce delay " + logging_ioToString( (io_t) i ) + ": " + String( settings.debounceDelay[i] ) + " ms" );
    }

    Serial.println( "----------------------------------" );
}


/**
 * @brief CLI command callback for setting the log level
 * 
 * @param pCommand - The command
 */
static void comLineIf_cmdSetLogLevelCb( cmd* pCommand )
{
    Command cmd( pCommand );
    Argument arg = cmd.getArgument();
    if ( !arg.isSet() )
    {
        Log.errorln( "%s: No log level specified, remaining at %s.", __func__, logging_logLevelToString( Log.getLevel() ).c_str() );
        return;
    }

    Log.noticeln( "Setting log level from %s to %s", logging_logLevelToString( Log.getLevel() ).c_str(), logging_logLevelToString( arg.getValue().toInt() ).c_str() );
    Log.setLevel( arg.getValue().toInt() );
}


/**
 * @brief CLI command callback for setting the timer values
 * 
 * @param pCommand - The command
 */
static void comLineIf_cmdSetTimerCb( cmd* pCommand )
{
    Command  cmd( pCommand );

    Argument argUnlock = cmd.getArgument( "u" );
    if ( argUnlock.isSet() )
    {
        settings.doorUnlockTimeout = argUnlock.getValue().toInt();
        stateMan_setDoorTimer( DOOR_TIMER_TYPE_UNLOCK, settings.doorUnlockTimeout );
        Log.noticeln( "%s: Door unlock timeout set to %d s", __func__, settings.doorUnlockTimeout );
    }

    Argument argOpen = cmd.getArgument( "o" );
    if ( argOpen.isSet() )
    {
        settings.doorOpenTimeout = argOpen.getValue().toInt();
        stateMan_setDoorTimer( DOOR_TIMER_TYPE_OPEN, settings.doorOpenTimeout );
        Log.noticeln( "%s: Door open timeout set to %d min", __func__, settings.doorOpenTimeout );
    }

    Argument argBlink = cmd.getArgument( "b" );
    if ( argBlink.isSet() )
    {
        settings.ledBlinkInterval = argBlink.getValue().toInt();
        Timer1.setPeriod( ( (uint32_t) 2000 ) * ( (uint32_t) settings.ledBlinkInterval ) );
        Log.noticeln( "%s: Led blink interval set to %d ms", __func__, settings.ledBlinkInterval );
    }
}


/**
 * @brief CLI command callback for setting the debounce delay
 * 
 * @param pCommand - The command
 */
static void comLineIf_cmdSetDebounceDelayCb( cmd* pCommand )
{
    Command cmd( pCommand );

    Argument argInputIdx = cmd.getArgument( "i" );
    uint8_t  inputIdx    = argInputIdx.getValue().toInt();

    if ( inputIdx < IO_INPUT_SIZE )
    {
        settings.debounceDelay[inputIdx] = cmd.getArgument( "t" ).getValue().toInt();
        ioMan_setDebounceDelay( (io_t) inputIdx, settings.debounceDelay[inputIdx] );
        Log.noticeln( "%s: Debounce delay for input %s set to %d ms", __func__, logging_ioToString( (io_t) inputIdx ).c_str(), settings.debounceDelay[inputIdx] );
    }
    else
    {
        Log.errorln( "%s: Invalid input index: %d", __func__, inputIdx );
    }
}


/**
 * @brief CLI command callback for getting the input state
 * 
 * @param pCommand - The command
 */
static void comLineIf_cmdGetInputStateCb( cmd* pCommand )
{
    Serial.println( "----------------------------------" );
    Serial.println( "Input State" );
    Serial.println( "----------------------------------" );

    for ( uint8_t i = 0; i < IO_INPUT_SIZE; i++ )
    {
        input_status_t inputState = ioMan_getDoorState( (io_t) i );
        Serial.println( logging_ioToString( (io_t) i ) + ": " + logging_inputStateToString( inputState.state ) );
    }

    Serial.println( "----------------------------------" );
}


/**
 * @brief CLI command callback for printing help
 * 
 * @param pCommand - The command
 */
static void comLineIf_cmdHelpCb( cmd* pCommand )
{
    Serial.println( "Help:" );
    Serial.println( "--------------------------------------------" );
    Serial.println( cli.toString() );
}


/**
 * @brief CLI error callback
 * 
 * @param pError - The error
 */
static void comLineIf_cmdErrorCb( cmd_error* pError )
{
    CommandError error( pError );

    Log.noticeln( "%s: %s", __func__, error.toString().c_str() );

    if ( error.hasCommand() )
    {
        Log.noticeln( "Did you mean: %s", error.getCommand().toString().c_str() );
    }
    else
    {
        Log.noticeln( "Available commands:" );
        Serial.println( "\n" + cli.toString() );
    }
}