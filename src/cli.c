/**
 * \file    cli.c
 * \brief   Source file for command line interface

 * \author  Mathias Buder
 * \date    2024-09-13

 *  Copyright (c) 2024 Mathias Buder
 */

#include <Arduino.h>
#include <SimpleCLI.h>

#include "cli.h"
#include "appSettings.h"


static void cliCmdGetInfoCb( cmd* pCommand );
static void cliCmdSetLogLevelCb( cmd* pCommand );
static void cliCmdSetTimerCb( cmd* pCommand );
static void cliCmdSetDebounceDelayCb( cmd* pCommand );
static void cliCmdHelpCb( cmd* pCommand );
static void cliCmdGetInputStateCb( cmd* pCommand );
static void cliErrorCb( cmd_error* pError );



static SimpleCLI cli;                    /*!< The command line interface */
static Command   cliCmdGetInfo;          /*!< Get software information */
static Command   cliCmdSetLogLevel;      /*!< Set log level */
static Command   cliCmdSetTimer;         /*!< Set all timers */
static Command   cliCmdSetDebounceDelay; /*!< Setall debounce delays */
static Command   cliCmdGetInputState;    /*!< Get the state of all inputs */
static Command   cliCmdHelp;             /*!< Pint the help */


void cliSetup( void )
{
    /* Initialize the command line interface */
    cliCmdGetInfo = cli.addSingleArgCmd( "info", cliCmdGetInfoCb ); /*!< Get software information */
    cliCmdGetInfo.setDescription( "Get software information" );

    cliCmdSetLogLevel = cli.addSingleArgCmd( "log", cliCmdSetLogLevelCb ); /*!< Set log level */
    cliCmdSetLogLevel.setDescription( "Set the log level: log <level (0..6)>" );

    cliCmdSetTimer = cli.addCmd( "timer", cliCmdSetTimerCb );       /*!< Set timer */
    cliCmdSetTimer.addArg( "u", STRINGIFY( DOOR_UNLOCK_TIMEOUT ) ); /*!< Unlock timeout */
    cliCmdSetTimer.addArg( "o", STRINGIFY( DOOR_OPEN_TIMEOUT ) );   /*!< Open timeout */
    cliCmdSetTimer.addArg( "b", STRINGIFY( LED_BLINK_INTERVAL ) );  /*!< LED blink interval */
    cliCmdSetTimer.setDescription( "Set the timer. timer -u <unlock timeout (s)> -o <open timeout (min)> -b <blink interval (ms)>" );

    cliCmdSetDebounceDelay = cli.addCmd( "dbc", cliCmdSetDebounceDelayCb ); /*!< Set debounce time */
    cliCmdSetDebounceDelay.addArg( "i" );                                   /*!< Input index */
    cliCmdSetDebounceDelay.addArg( "t" );                                   /*!< Debounce time @unit ms */
    cliCmdSetDebounceDelay.setDescription( "Set the debounce time. dbc -i <input index (0..3)> -t <debounce time (ms)>" );

    cliCmdGetInputState = cli.addSingleArgCmd( "inputs", cliCmdGetInputStateCb ); /*!< Get the input state */
    cliCmdGetInputState.setDescription( "Get the input state of all buttons and switches" );

    cliCmdHelp = cli.addCmd( "help", cliCmdHelpCb ); /*!< Help */
    cliCmdHelp.setDescription( "Show the help" );

    cli.setOnError( cliErrorCb );

}



static void cliCmdGetInfoCb( cmd* pCommand )
{
    Serial.println( "----------------------------------" );
    Serial.println( "Door Control System Information   " );
    Serial.println( "----------------------------------" );

    /* Output software version string */
    Serial.println( "Version: " + String( GIT_VERSION_STRING ) );

    /* Output the build date and time */
    Serial.println( "Build date: " + String( __DATE__ ) + " " + String( __TIME__ ) );

    /* Output current log level */
    Serial.println( "Log level: " + logLevelToString( Log.getLevel() ) );

    /* Output the all times and timeouts */
    Serial.println( "Door unlock timeout: " + String( settings.doorUnlockTimeout ) + " s" );
    Serial.println( "Door open timeout: " + String( settings.doorOpenTimeout ) + " min" );
    Serial.println( "Led blink interval: " + String( settings.ledBlinkInterval ) + " ms" );

    for ( uint8_t i = 0; i < IO_INPUT_SIZE; i++ )
    {
        Serial.println( "Debounce delay " + ioToString( (io_t) i ) + ": " + String( settings.debounceDelay[i] ) + " ms" );
    }

    Serial.println( "----------------------------------" );
}


/**
 * @brief CLI command callback for setting the log level
 * 
 * @param pCommand - The command
 */
static void cliCmdSetLogLevelCb( cmd* pCommand )
{
    Command cmd( pCommand );
    Argument arg = cmd.getArgument();
    if ( !arg.isSet() )
    {
        Log.errorln( "%s: No log level specified, remaining at %s.", __func__, logLevelToString( Log.getLevel() ).c_str() );
        return;
    }

    Log.noticeln( "Setting log level from %s to %s", logLevelToString( Log.getLevel() ).c_str(), logLevelToString( arg.getValue().toInt() ).c_str() );
    Log.setLevel( arg.getValue().toInt() );
}


/**
 * @brief CLI command callback for setting the timer values
 * 
 * @param pCommand - The command
 */
static void cliCmdSetTimerCb( cmd* pCommand )
{
    Command  cmd( pCommand );

    Argument argUnlock = cmd.getArgument( "u" );
    if ( argUnlock.isSet() )
    {
        settings.doorUnlockTimeout = argUnlock.getValue().toInt();
        doorControl.doorTimer[DOOR_TIMER_TYPE_UNLOCK].timeout = settings.doorUnlockTimeout * 1000;
        Log.noticeln( "%s: Door unlock timeout set to %d s", __func__, settings.doorUnlockTimeout );
    }

    Argument argOpen = cmd.getArgument( "o" );
    if ( argOpen.isSet() )
    {
        settings.doorOpenTimeout = argOpen.getValue().toInt();
        doorControl.doorTimer[DOOR_TIMER_TYPE_OPEN].timeout = ( (uint32_t) settings.doorOpenTimeout ) * 60000;
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
static void cliCmdSetDebounceDelayCb( cmd* pCommand )
{
    Command cmd( pCommand );

    Argument argInputIdx = cmd.getArgument( "i" );
    uint8_t  inputIdx    = argInputIdx.getValue().toInt();

    if ( inputIdx < IO_INPUT_SIZE )
    {
        settings.debounceDelay[inputIdx]             = cmd.getArgument( "t" ).getValue().toInt();
        buttonSwitchIoConfig[inputIdx].debounceDelay = settings.debounceDelay[inputIdx];
        Log.noticeln( "%s: Debounce delay for input %s set to %d ms", __func__, ioToString( (io_t) inputIdx ).c_str(), settings.debounceDelay[inputIdx] );
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
static void cliCmdGetInputStateCb( cmd* pCommand )
{
    Serial.println( "----------------------------------" );
    Serial.println( "Input State" );
    Serial.println( "----------------------------------" );

    for ( uint8_t i = 0; i < IO_INPUT_SIZE; i++ )
    {
        input_status_t inputState = getDoorIoState( (io_t) i );
        Serial.println( ioToString( (io_t) i ) + ": " + inputStateToString( inputState.state ) );
    }

    Serial.println( "----------------------------------" );
}


/**
 * @brief CLI command callback for printing help
 * 
 * @param pCommand - The command
 */
static void cliCmdHelpCb( cmd* pCommand )
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
static void cliErrorCb( cmd_error* pError )
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