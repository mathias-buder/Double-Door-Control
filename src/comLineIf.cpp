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


/******************************** Global variables ************************************/

static SimpleCLI cli;                 /*!< The command line interface */
static Command   cmdGetInfo;          /*!< Get software information */
static Command   cmdSetLogLevel;      /*!< Set log level */
static Command   cmdSetTimer;         /*!< Set all timers */
static Command   cmdSetDebounceDelay; /*!< Setall debounce delays */
static Command   cmdGetInputState;    /*!< Get the state of all inputs */
static Command   cmdHelp;             /*!< Pint the help */

/**************************** Static Function prototype *********************************/

static void comLineIf_cmdGetInfoCb( cmd* pCommand );
static void comLineIf_cmdSetLogLevelCb( cmd* pCommand );
static void comLineIf_cmdSetTimerCb( cmd* pCommand );
static void comLineIf_cmdSetDebounceDelayCb( cmd* pCommand );
static void comLineIf_cmdHelpCb( cmd* pCommand );
static void comLineIf_cmdGetInputStateCb( cmd* pCommand );
static void comLineIf_cmdErrorCb( cmd_error* pError );

/******************************** Function definition ************************************/


/**
 * @brief Sets up the command line interface with various commands.
 * 
 * This function initializes the command line interface by adding several commands:
 * - "info": Retrieves software information.
 * - "log": Sets the log level.
 * - "timer": Configures the timer with unlock timeout, open timeout, and LED blink interval.
 * - "dbc": Sets the debounce time for inputs.
 * - "inputs": Retrieves the state of all buttons and switches.
 * - "help": Displays the help information.
 * 
 * It also sets the error callback for the command line interface.
 */
void comLineIf_setup( void )
{
    Log.noticeln( "%s: Setting up the command line interface", __func__ );

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


/**
 * @brief Processes incoming serial commands.
 *
 * This function checks if there is any data available on the serial port.
 * If data is available, it reads the input string until a newline character
 * is encountered and then parses the input using the command line interface (CLI) parser.
 *
 * The function also contains commented-out test commands that can be used
 * for debugging or testing purposes.
 *
 * @note The function assumes that the `Serial` object and `cli` parser are
 * properly initialized and configured elsewhere in the code.
 */
void comLineIf_process( void )
{
    if ( Serial.available() )
    {
        String input = Serial.readStringUntil( '\n' );
        cli.parse( input );

        /* Uncomment the following lines to test the command line interface */
        // cli.parse( String( "info" ) );
        // cli.parse( String( "log" ) );
        // cli.parse( String( "timer -d 0 -u 5 -o 600 -b 500" ) );
        // cli.parse( String( "timer -u 30 -o 18 -b 180" ) );
        // cli.parse( String( "time -f" ) );
        // cli.parse( String( "dbc -i 3 -t 128" ) );
        // cli.parse( String( "dbc -t 300" ) );
        // cli.parse( String( "info" ) );
    }
}


/**
 * @brief Callback function to handle the 'Get Info' command.
 *
 * This function outputs various information about the Door Control System
 * to the serial interface. The information includes:
 * - Software version string
 * - Build date and time
 * - Current log level
 * - Door unlock timeout
 * - Door open timeout
 * - LED blink interval
 * - Debounce delays for each input
 *
 * @param pCommand Pointer to the command structure.
 */
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
    settings_t* settings = appSettings_getSettings();
    Serial.println( "Door unlock timeout: " + String( settings->doorUnlockTimeout ) + " s" );
    Serial.println( "Door open timeout: " + String( settings->doorOpenTimeout ) + " min" );
    Serial.println( "Led blink interval: " + String( settings->ledBlinkInterval ) + " ms" );

    for ( uint8_t i = 0; i < IO_INPUT_SIZE; i++ )
    {
        Serial.println( "Debounce delay " + logging_ioToString( (io_t) i ) + ": " + String( settings->debounceDelay[i] ) + " ms" );
    }

    Serial.println( "----------------------------------" );
}


/**
 * @brief Callback function to set the log level.
 *
 * This function is called to change the current log level based on the provided command argument.
 * If no log level is specified in the argument, it logs an error message and retains the current log level.
 * Otherwise, it updates the log level to the specified value and logs the change.
 *
 * @param pCommand Pointer to the command containing the log level argument.
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
 * @brief Callback function to set various timers based on command arguments.
 *
 * This function processes a command to set the door unlock timeout, door open timeout,
 * and LED blink interval. It retrieves the respective arguments from the command and
 * updates the corresponding settings and timers.
 *
 * @param pCommand Pointer to the command structure containing the arguments.
 *
 * The following command arguments are processed:
 * - "u": Sets the door unlock timeout in seconds.
 * - "o": Sets the door open timeout in minutes.
 * - "b": Sets the LED blink interval in milliseconds.
 *
 * Example usage:
 * - To set the door unlock timeout to 10 seconds: `cmd -u 10`
 * - To set the door open timeout to 5 minutes: `cmd -o 5`
 * - To set the LED blink interval to 500 milliseconds: `cmd -b 500`
 */
static void comLineIf_cmdSetTimerCb( cmd* pCommand )
{
    Command  cmd( pCommand );
    settings_t* settings = appSettings_getSettings();

    Argument argUnlock = cmd.getArgument( "u" );
    if ( argUnlock.isSet() )
    {
        settings->doorUnlockTimeout = argUnlock.getValue().toInt();
        stateMan_setDoorTimer( DOOR_TIMER_TYPE_UNLOCK, settings->doorUnlockTimeout );
        Log.noticeln( "%s: Door unlock timeout set to %d s", __func__, settings->doorUnlockTimeout );
    }

    Argument argOpen = cmd.getArgument( "o" );
    if ( argOpen.isSet() )
    {
        settings->doorOpenTimeout = argOpen.getValue().toInt();
        stateMan_setDoorTimer( DOOR_TIMER_TYPE_OPEN, settings->doorOpenTimeout );
        Log.noticeln( "%s: Door open timeout set to %d min", __func__, settings->doorOpenTimeout );
    }

    Argument argBlink = cmd.getArgument( "b" );
    if ( argBlink.isSet() )
    {
        settings->ledBlinkInterval = argBlink.getValue().toInt();
        Timer1.setPeriod( ( (uint32_t) 2000 ) * ( (uint32_t) settings->ledBlinkInterval ) );
        Log.noticeln( "%s: Led blink interval set to %d ms", __func__, settings->ledBlinkInterval );
    }
}



/**
 * @brief Callback function to set the debounce delay for a specified input.
 *
 * This function is called to set the debounce delay for a specific input index.
 * It retrieves the input index and debounce delay from the command arguments,
 * validates the input index, and updates the debounce delay setting if the index
 * is valid. It also logs the operation result.
 *
 * @param pCommand Pointer to the command structure containing the arguments.
 *
 * Command Arguments:
 * - "i": Input index (uint8_t)
 * - "t": Debounce delay time in milliseconds (int)
 *
 * Logs:
 * - Success: Logs the debounce delay set for the specified input.
 * - Error: Logs an error if the input index is invalid.
 */
static void comLineIf_cmdSetDebounceDelayCb( cmd* pCommand )
{
    Command cmd( pCommand );
    settings_t* settings = appSettings_getSettings();

    Argument argInputIdx = cmd.getArgument( "i" );
    uint8_t  inputIdx    = argInputIdx.getValue().toInt();


    if ( inputIdx < IO_INPUT_SIZE )
    {
        settings->debounceDelay[inputIdx] = cmd.getArgument( "t" ).getValue().toInt();
        ioMan_setDebounceDelay( (io_t) inputIdx, settings->debounceDelay[inputIdx] );
        Log.noticeln( "%s: Debounce delay for input %s set to %d ms", __func__, logging_ioToString( (io_t) inputIdx ).c_str(), settings->debounceDelay[inputIdx] );
    }
    else
    {
        Log.errorln( "%s: Invalid input index: %d", __func__, inputIdx );
    }
}


/**
 * @brief Callback function to get and print the input state.
 *
 * This function prints the state of each input to the serial output.
 * It iterates over all inputs, retrieves their state, and prints the
 * corresponding state as a string.
 *
 * @param pCommand Pointer to the command structure.
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
 * @brief Callback function to display help information for commands.
 *
 * This function prints out a help message to the serial output, including
 * a header and the string representation of the command line interface.
 *
 * @param pCommand Pointer to the command structure.
 */
static void comLineIf_cmdHelpCb( cmd* pCommand )
{
    Serial.println( "Help:" );
    Serial.println( "--------------------------------------------" );
    Serial.println( cli.toString() );
}



/**
 * @brief Callback function to handle command errors.
 *
 * This function is called when a command error occurs. It logs the error
 * message and, if a command is associated with the error, suggests the
 * correct command. If no command is associated with the error, it lists
 * all available commands.
 *
 * @param pError Pointer to the cmd_error structure containing error details.
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