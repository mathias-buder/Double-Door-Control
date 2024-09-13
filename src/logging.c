/**
 * \file    logging.c
 * \brief   Source file for logging

 * \author  Mathias Buder
 * \date    2024-09-13

 *  Copyright (c) 2024 Mathias Buder
 */

#include <ArduinoLog.h>
#include "logging.h"



void logging_setup( void )
{
    /* Initialize with log level and log output */
    Serial.begin( SERIAL_BAUD_RATE );
    Log.begin( DEFAULT_LOG_LEVEL, &Serial );
    Log.noticeln( "Door control application %s", GIT_VERSION_STRING );
    Log.noticeln( "Starting ... " );
}


/**
 * @brief Convert the event to string
 * 
 * @param event - The event to convert
 * @return String - The string representation of the event
 */
void eventLogger( uint32_t stateMachine, uint32_t state, uint32_t event )
{
    (void) stateMachine;
    static uint32_t lastEvent = 0;
    static uint32_t lastState = 0;

    /* Only log if the event and state are changed */
    if (    ( lastEvent != event )
         && ( lastState != state ) )
    {
        Log.noticeln( "%s: Event: %s, State: %s", __func__,
                    eventToString( (door_control_event_t) event ).c_str(),
                    stateToString( (door_control_state_t) state ).c_str() );
    }

    /* Save the last event and state */
    lastEvent = event;
    lastState = state;
}


/**
 * @brief Convert the state to string
 * 
 * @param state - The state to convert
 * @return String - The string representation of the state
 */
void resultLogger(uint32_t state, state_machine_result_t result)
{
    static uint32_t lastState = 0;

    /* Only log if the state is changed */
    if ( lastState != state )
    {
        Log.noticeln( "%s: Result: %s, Current state: %s", __func__,
                                                            resultToString( result ).c_str(),
                                                            stateToString( (door_control_state_t) state ).c_str() );
    }

    /* Save the last state */
    lastState = state;
}


/**
 * @brief Convert the state to string
 * 
 * @param state - The state to convert
 * @return String - The string representation of the state
 */
String stateToString( door_control_state_t state )
{
    switch ( state )
    {
    case DOOR_CONTROL_STATE_INIT:
        return "DOOR_CONTROL_STATE_INIT";
    case DOOR_CONTROL_STATE_IDLE:
        return "DOOR_CONTROL_STATE_IDLE";
    case DOOR_CONTROL_STATE_FAULT:
        return "DOOR_CONTROL_STATE_FAULT";
    case DOOR_CONTROL_STATE_DOOR_1_UNLOCKED:
        return "DOOR_CONTROL_STATE_DOOR_1_UNLOCKED";
    case DOOR_CONTROL_STATE_DOOR_1_OPEN:
        return "DOOR_CONTROL_STATE_DOOR_1_OPEN";
    case DOOR_CONTROL_STATE_DOOR_2_UNLOCKED:
        return "DOOR_CONTROL_STATE_DOOR_2_UNLOCKED";
    case DOOR_CONTROL_STATE_DOOR_2_OPEN:
        return "DOOR_CONTROL_STATE_DOOR_2_OPEN";
    default:
        return "UNKNOWN";
    }
}


/**
 * @brief Convert the event to string
 * 
 * @param event - The event to convert
 * @return String - The string representation of the event
 */
String eventToString( door_control_event_t event )
{
    Log.verboseln( "%s: Event: %d", __func__, event );

    switch ( event )
    {
    case DOOR_CONTROL_EVENT_INIT_DONE:
        return "DOOR_CONTROL_EVENT_INIT_DONE";
    case DOOR_CONTROL_EVENT_DOOR_1_UNLOCK:
        return "DOOR_CONTROL_EVENT_DOOR_1_UNLOCK";
    case DOOR_CONTROL_EVENT_DOOR_1_UNLOCK_TIMEOUT:
        return "DOOR_CONTROL_EVENT_DOOR_1_UNLOCK_TIMEOUT";
    case DOOR_CONTROL_EVENT_DOOR_1_OPEN:
        return "DOOR_CONTROL_EVENT_DOOR_1_OPEN";
    case DOOR_CONTROL_EVENT_DOOR_1_CLOSE:
        return "DOOR_CONTROL_EVENT_DOOR_1_CLOSE";
    case DOOR_CONTROL_EVENT_DOOR_1_OPEN_TIMEOUT:
        return "DOOR_CONTROL_EVENT_DOOR_1_OPEN_TIMEOUT";
    case DOOR_CONTROL_EVENT_DOOR_2_UNLOCK:
        return "DOOR_CONTROL_EVENT_DOOR_2_UNLOCK";
    case DOOR_CONTROL_EVENT_DOOR_2_UNLOCK_TIMEOUT:
        return "DOOR_CONTROL_EVENT_DOOR_2_UNLOCK_TIMEOUT";
    case DOOR_CONTROL_EVENT_DOOR_2_OPEN:
        return "DOOR_CONTROL_EVENT_DOOR_2_OPEN";
    case DOOR_CONTROL_EVENT_DOOR_2_CLOSE:
        return "DOOR_CONTROL_EVENT_DOOR_2_CLOSE";
    case DOOR_CONTROL_EVENT_DOOR_2_OPEN_TIMEOUT:
        return "DOOR_CONTROL_EVENT_DOOR_2_OPEN_TIMEOUT";
    case DOOR_CONTROL_EVENT_DOOR_1_2_OPEN:
        return "DOOR_CONTROL_EVENT_DOOR_1_2_OPEN";
    case DOOR_CONTROL_EVENT_DOOR_1_2_CLOSE:
        return "DOOR_CONTROL_EVENT_DOOR_1_2_CLOSE";
    default:
        return "UNKNOWN";
    }
}


/**
 * @brief Convert the result to string
 * 
 * @param result - The result to convert
 * @return String - The string representation of the result
 */
String resultToString( state_machine_result_t result )
{
    Log.verboseln( "%s: Result: %d", __func__, result );

    switch ( result )
    {
    case EVENT_HANDLED:
        return "EVENT_HANDLED";
    case EVENT_UN_HANDLED:
        return "EVENT_UN_HANDLED";
    case TRIGGERED_TO_SELF:
        return "TRIGGERED_TO_SELF";
    default:
        return "UNKNOWN";
    }
}


/**
 * @brief Convert the input/output to string
 * 
 * @param sensor - The input/output to convert
 * @return String - The string representation of the input/output
 */
String ioToString( io_t io )
{
    Log.verboseln( "%s: IO: %d", __func__, io );

    switch ( io )
    {
    case IO_BUTTON_1:
        return "IO_BUTTON_1";
    case IO_BUTTON_2:
        return "IO_BUTTON_2";
    case IO_SWITCH_1:
        return "IO_SWITCH_1";
    case IO_SWITCH_2:
        return "IO_SWITCH_2";
    case IO_MAGNET_1:
        return "IO_MAGNET_1";
    case IO_MAGNET_2:
        return "IO_MAGNET_2";
    case IO_LED_1_R:
        return "IO_LED_1_R";
    case IO_LED_1_G:
        return "IO_LED_1_G";
    case IO_LED_1_B:
        return "IO_LED_1_B";
    case IO_LED_2_R:
        return "IO_LED_2_R";
    case IO_LED_2_G:
        return "IO_LED_2_G";
    case IO_LED_2_B:
        return "IO_LED_2_B";
    default:
        return "UNKNOWN";
    }
}


/**
 * @brief Convert the timer type to string
 * 
 * @param timerType - The timer type to convert
 * @return String - The string representation of the timer type
 */
String timerTypeToString( door_timer_type_t timerType )
{
    Log.verboseln( "%s: Timer type: %d", __func__, timerType );

    switch ( timerType )
    {
    case DOOR_TIMER_TYPE_OPEN:
        return "DOOR_TIMER_TYPE_OPEN";
    case DOOR_TIMER_TYPE_UNLOCK:
        return "DOOR_TIMER_TYPE_UNLOCK";
    default:
        return "UNKNOWN";
    }
}


/**
 * @brief Convert the input state to string
 * 
 * @param state - The input state to convert
 * @return String - The string representation of the input state
 */
String inputStateToString( input_state_t state )
{
    Log.verboseln( "%s: State: %d", __func__, state );

    switch ( state )
    {
    case INPUT_STATE_INACTIVE:
        return "INPUT_STATE_INACTIVE";
    case INPUT_STATE_ACTIVE:
        return "INPUT_STATE_ACTIVE";
    default:
        return "UNKNOWN";
    }
}


String logLevelToString( uint8_t level )
{
    Log.verboseln( "%s: Level: %d", __func__, level );

    switch ( level )
    {
    case LOG_LEVEL_SILENT:
        return "LOG_LEVEL_SILENT";
    case LOG_LEVEL_FATAL:
        return "LOG_LEVEL_FATAL";
    case LOG_LEVEL_ERROR:
        return "LOG_LEVEL_ERROR";
    case LOG_LEVEL_WARNING:
        return "LOG_LEVEL_WARNING";
    case LOG_LEVEL_NOTICE:
        return "LOG_LEVEL_NOTICE";
    case LOG_LEVEL_TRACE:
        return "LOG_LEVEL_TRACE";
    case LOG_LEVEL_VERBOSE:
        return "LOG_LEVEL_VERBOSE";
    default:
        return "UNKNOWN";
    }
}

