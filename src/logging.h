/**
 * \file    logging.h
 * \brief   Header file for logging

 * \author  Mathias Buder
 * \date    2024-09-13

 *  Copyright (c) 2024 Mathias Buder
 */

#ifndef LOGGING_H
#define LOGGING_H


#include <Arduino.h>
#include <ArduinoLog.h>

#include "hsm.h"



static String                 stateToString( door_control_state_t state );
static String                 inputStateToString( input_state_t state );
static String                 eventToString( door_control_event_t event );
static String                 resultToString( state_machine_result_t result );
static String                 ioToString( io_t io );
static String                 timerTypeToString( door_timer_type_t timerType );
static String                 logLevelToString( uint8_t level );

#endif  // LOGGING_H