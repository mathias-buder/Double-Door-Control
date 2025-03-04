/**
 * \file    stateManagement.h
 * \brief   Header file for stateManagement

 * \author  Mathias Buder
 * \date    2024-09-13

 *  Copyright (c) 2024 Mathias Buder
 */

#ifndef STATEMANAGEMENT_H
#define STATEMANAGEMENT_H

#include <Arduino.h>
#include "hsm.h"
#include "appSettings.h"
#include "ioMan.h"

/************************************ ENUMERATION *************************************/

/**
 * @brief Enumeration of the door control state
 */
typedef enum
{
    DOOR_CONTROL_STATE_INIT,            /*!< Initializing the state machine */
    DOOR_CONTROL_STATE_IDLE,            /*!< The state machine is in idle state */
    DOOR_CONTROL_STATE_FAULT,           /*!< The state machine is in fault state */
    DOOR_CONTROL_STATE_DOOR_1_UNLOCKED, /*!< The door 1 is unlocked */
    DOOR_CONTROL_STATE_DOOR_1_OPEN,     /*!< The door 1 is open */
    DOOR_CONTROL_STATE_DOOR_2_UNLOCKED, /*!< The door 2 is unlocked */
    DOOR_CONTROL_STATE_DOOR_2_OPEN      /*!< The door 2 is open */
} door_control_state_t;

/**
 * @brief Enumeration of the door control event
 */
typedef enum
{
    DOOR_CONTROL_EVENT_INIT_DONE = 1,         /*!< The initialization is done successfully */
    DOOR_CONTROL_EVENT_DOOR_1_UNLOCK,         /*!< The door 1 is unlocked */
    DOOR_CONTROL_EVENT_DOOR_1_UNLOCK_TIMEOUT, /*!< The door 1 is unlocked timeout */
    DOOR_CONTROL_EVENT_DOOR_1_OPEN,           /*!< The door 1 is open */
    DOOR_CONTROL_EVENT_DOOR_1_CLOSE,          /*!< The door 1 is closed */
    DOOR_CONTROL_EVENT_DOOR_1_OPEN_TIMEOUT,   /*!< The door 1 is open timeout */
    DOOR_CONTROL_EVENT_DOOR_2_UNLOCK,         /*!< The door 2 is unlocked */
    DOOR_CONTROL_EVENT_DOOR_2_UNLOCK_TIMEOUT, /*!< The door 2 is unlocked timeout */
    DOOR_CONTROL_EVENT_DOOR_2_OPEN,           /*!< The door 2 is open */
    DOOR_CONTROL_EVENT_DOOR_2_CLOSE,          /*!< The door 2 is closed */
    DOOR_CONTROL_EVENT_DOOR_2_OPEN_TIMEOUT,   /*!< The door 2 is open timeout */
    DOOR_CONTROL_EVENT_DOOR_1_2_OPEN,         /*!< The door 1 and 2 are open */
    DOOR_CONTROL_EVENT_DOOR_1_2_CLOSE         /*!< The door 1 and 2 are closed */
} door_control_event_t;


/************************************* STRUCTURE **************************************/

/**
 * @brief The timer structure
 * @details The timer structure is used to hold the door 1/2 open timeout timers
 */
typedef struct
{
    void ( *handler )( uint32_t time );   //!< The handler function that is called when the timer expires
    uint32_t timeout;                     //!< The timeout after which the handler is called @unit ms
    uint64_t timeReference;               //!< The time reference when the timer is started @unit ms
} door_timer_t;

/**
 * @brief The door control state machine
 * @details The door control state machine is used to control the door 1 and 2
 */
typedef struct
{
    state_machine_t machine;                         /*!< Abstract state machine */
    door_timer_t    doorTimer[DOOR_TIMER_TYPE_SIZE]; /*!< The door timers */
} door_control_t;



/******************************** Function prototype ************************************/

void stateMan_setup( void );
void stateMan_process( void );
void stateMan_setDoorTimer( door_timer_type_t timerType, uint32_t timeout );

#endif // STATEMANAGEMENT_H