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



static state_machine_result_t initHandler( state_machine_t* const pState, const uint32_t event );
static state_machine_result_t initEntryHandler( state_machine_t* const pState, const uint32_t event );
/* static state_machine_result_t initExitHandler( state_machine_t* const pState, const uint32_t event ); */

static state_machine_result_t idleHandler( state_machine_t* const pState, const uint32_t event );
static state_machine_result_t idleEntryHandler( state_machine_t* const pState, const uint32_t event );
static state_machine_result_t idleExitHandler( state_machine_t* const pState, const uint32_t event );

static state_machine_result_t faultHandler( state_machine_t* const pState, const uint32_t event );
static state_machine_result_t faultEntryHandler( state_machine_t* const pState, const uint32_t event );
static state_machine_result_t faultExitHandler( state_machine_t* const pState, const uint32_t event );

static state_machine_result_t door1UnlockHandler( state_machine_t* const pState, const uint32_t event );
static state_machine_result_t door1UnlockEntryHandler( state_machine_t* const pState, const uint32_t event );
static state_machine_result_t door1UnlockExitHandler( state_machine_t* const pState, const uint32_t event );
static state_machine_result_t door1OpenHandler( state_machine_t* const pState, const uint32_t event );
static state_machine_result_t door1OpenEntryHandler( state_machine_t* const pState, const uint32_t event );
static state_machine_result_t door1OpenExitHandler( state_machine_t* const pState, const uint32_t event );

static state_machine_result_t door2UnlockHandler( state_machine_t* const pState, const uint32_t event );
static state_machine_result_t door2UnlockEntryHandler( state_machine_t* const pState, const uint32_t event );
static state_machine_result_t door2UnlockExitHandler( state_machine_t* const pState, const uint32_t event );
static state_machine_result_t door2OpenHandler( state_machine_t* const pState, const uint32_t event );
static state_machine_result_t door2OpenEntryHandler( state_machine_t* const pState, const uint32_t event );
static state_machine_result_t door2OpenExitHandler( state_machine_t* const pState, const uint32_t event );

static void                   faultBlinkLedIsrHandler( void );
static void                   door1BlinkLedIsrHandler( void );
static void                   door2BlinkLedIsrHandler( void );
static void                   generateEvent( door_control_t* const pDoorControl );
static void                   processTimers( door_control_t* const pDoorControl );

void                          eventLogger( uint32_t stateMachine, uint32_t state, uint32_t event );
void                          resultLogger( uint32_t state, state_machine_result_t result );


/**
 * @brief The door control state machine
 * @details The door control state machine is defined with the state machine and the door timers.
 *          The state machine is initialized with the init-state and no event.
 */
door_control_t doorControl = {
    .machine = { NULL, NULL },
    .doorTimer = {
                    { doorUnlockTimeoutHandler, (uint32_t) DOOR_UNLOCK_TIMEOUT * 1000, 0 },
                    { doorOpenTimeoutHandler,   (uint32_t) DOOR_OPEN_TIMEOUT   * 1000, 0 }
                }
};


#endif // STATEMANAGEMENT_H