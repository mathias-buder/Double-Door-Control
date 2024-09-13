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

/************************************ ENUMERATION *************************************/


/************************************* STRUCTURE **************************************/


/******************************** Function prototype ************************************/

void   logging_setup( void );
void   logging_eventLogger( uint32_t stateMachine, uint32_t state, uint32_t event );
void   logging_resultLogger( uint32_t state, state_machine_result_t result );
String logging_stateToString( door_control_state_t state );
String logging_inputStateToString( input_state_t state );
String logging_eventToString( door_control_event_t event );
String logging_resultToString( state_machine_result_t result );
String logging_ioToString( io_t io );
String logging_timerTypeToString( door_timer_type_t timerType );
String logging_logLevelToString( uint8_t level );

#endif  // LOGGING_H