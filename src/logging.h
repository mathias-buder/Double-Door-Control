/**
 * \file    logging.h
 * \brief   Header file for logging

 * \author  Mathias Buder
 * \date    2024-09-13

 *  Copyright (c) 2024 Mathias Buder
 */

#ifndef LOGGING_H
#define LOGGING_H


#include <ArduinoLog.h>

#include "hsm.h"
#include "stateMan.h"

void eventLogger( uint32_t stateMachine, uint32_t state, uint32_t event );
void resultLogger(uint32_t state, state_machine_result_t result);

String stateToString( door_control_state_t state );
String inputStateToString( input_state_t state );
String eventToString( door_control_event_t event );
String resultToString( state_machine_result_t result );
String ioToString( io_t io );
String timerTypeToString( door_timer_type_t timerType );
String logLevelToString( uint8_t level );

#endif  // LOGGING_H