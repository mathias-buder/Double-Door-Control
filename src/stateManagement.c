/**
 * \file    stateManagement.c
 * \brief   Source file for stateManagement

 * \author  Mathias Buder
 * \date    2024-09-13

 *  Copyright (c) 2024 Mathias Buder
 */

#include "stateManagement.h"
#include "hsm.h"



/**
 * @brief The door control state machine
 * @details The door control state machine is used to control the door 1 and 2
 */
typedef struct
{
    state_machine_t machine;                         /*!< Abstract state machine */
    door_timer_t    doorTimer[DOOR_TIMER_TYPE_SIZE]; /*!< The door timers */
} door_control_t;


/**
 * @brief The state machine for the door control
 * @details The state machine is defined as an array of states and its handlers
 */
static const state_t doorControlStates[] = {

    [DOOR_CONTROL_STATE_INIT] = {
        .Handler = initHandler,
        .Entry   = initEntryHandler,
        .Exit    = NULL,
        .Id      = DOOR_CONTROL_STATE_INIT
    },

    [DOOR_CONTROL_STATE_IDLE] = {
        .Handler = idleHandler,
        .Entry   = idleEntryHandler,
        .Exit    = idleExitHandler,
        .Id      = DOOR_CONTROL_STATE_IDLE
    },

    [DOOR_CONTROL_STATE_FAULT] = {
        .Handler = faultHandler,
        .Entry   = faultEntryHandler,
        .Exit    = faultExitHandler,
        .Id      = DOOR_CONTROL_STATE_FAULT
    },

    [DOOR_CONTROL_STATE_DOOR_1_UNLOCKED] = {
        .Handler = door1UnlockHandler,
        .Entry   = door1UnlockEntryHandler,
        .Exit    = door1UnlockExitHandler,
        .Id      = DOOR_CONTROL_STATE_DOOR_1_UNLOCKED
    },

    [DOOR_CONTROL_STATE_DOOR_1_OPEN] = {
        .Handler = door1OpenHandler,
        .Entry   = door1OpenEntryHandler,
        .Exit    = door1OpenExitHandler,
        .Id      = DOOR_CONTROL_STATE_DOOR_1_OPEN
    },

    [DOOR_CONTROL_STATE_DOOR_2_UNLOCKED] = {
        .Handler = door2UnlockHandler,
        .Entry   = door2UnlockEntryHandler,
        .Exit    = door2UnlockExitHandler,
        .Id      = DOOR_CONTROL_STATE_DOOR_2_UNLOCKED
    },

    [DOOR_CONTROL_STATE_DOOR_2_OPEN] = {
        .Handler = door2OpenHandler,
        .Entry   = door2OpenEntryHandler,
        .Exit    = door2OpenExitHandler,
        .Id      = DOOR_CONTROL_STATE_DOOR_2_OPEN
    }
};

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

/**
 * @brief The array of state machines
 * @details The array of state machines is used to dispatch the event to the state machines.
 */
state_machine_t* const stateMachines[] = {(state_machine_t*) &doorControl};