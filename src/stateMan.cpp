/**
 * \file    stateManagement.c
 * \brief   Source file for stateManagement

 * \author  Mathias Buder
 * \date    2024-09-13

 *  Copyright (c) 2024 Mathias Buder
 */

#include <TimerOne.h>

#include "stateMan.h"
#include "ioMan.h"
#include "logging.h"


/**************************** Static Function prototype *********************************/

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
static void                   doorOpenTimeoutHandler( uint32_t time );
static void                   doorUnlockTimeoutHandler( uint32_t time );


/******************************** Global variables ************************************/

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

/**************************** Static Function prototype *********************************/

static void stateMan_generateEvent( door_control_t* const pDoorControl );
static void stateMan_processTimers( door_control_t* const pDoorControl );


/******************************** Function definition ************************************/


/**
 * @brief Sets up the state manager.
 *
 * This function initializes the LED blink timer with an interval specified
 * by the settings.ledBlinkInterval. It also initializes the state machine
 * by switching to the initial state defined in doorControlStates.
 */
void stateMan_setup( void )
{
    Log.noticeln( "%s: Setting up the state manager", __func__ );

    /* Initialize the led blink timer */
    Timer1.initialize( ( (uint32_t) 2000 ) * ( (uint32_t) appSettings_getSettings()->ledBlinkInterval) );

    /* Initialize the state machine */
    switch_state( &doorControl.machine, &doorControlStates[DOOR_CONTROL_STATE_INIT] );
}


/**
 * @brief Processes the state management for the door control system.
 *
 * This function performs the following tasks:
 * 1. Generates and processes events related to the door control.
 * 2. Processes door open timers.
 * 3. Dispatches the event to the state machine and logs an error if the event is not handled.
 */
void stateMan_process( void )
{
    /* Generate/Process events */
    stateMan_generateEvent( &doorControl );

    /* Process door open timers */
    stateMan_processTimers( &doorControl );

    /* Dispatch the event to the state machine */
    if ( dispatch_event( stateMachines, 1, logging_eventLogger, logging_resultLogger ) == EVENT_UN_HANDLED )
    {
        Log.errorln( "Event is not handled" );
    }
}


/**
 * @brief Handler for the init state entry
 * 
 * @param pState - The state machine
 * @param event - The event
 * @return state_machine_result_t - The result of the handler
 */
static state_machine_result_t initEntryHandler( state_machine_t* const pState, const uint32_t event )
{
    Log.verboseln("%s: Event %s", __func__, logging_eventToString( (door_control_event_t) event ).c_str() );

    /* Initialize both doors to locked */
    ioMan_setDoorState( DOOR_TYPE_DOOR_1, LOCK_STATE_LOCKED );
    ioMan_setDoorState( DOOR_TYPE_DOOR_2, LOCK_STATE_LOCKED );

    /* Check whether the door switches are debouncing. This "waiting" mechanism is only used
     * for the initialization as the door switches are checked here in an one-shot manner. If
     * the switches are not stable within the timeout, the state machine switches to the fault state.
     */
    input_status_t door1SwitchStatus, door2SwitchStatus;
    uint64_t        currentTime = millis();

    do {
        door1SwitchStatus = ioMan_getDoorState( IO_SWITCH_1 );
        door2SwitchStatus = ioMan_getDoorState( IO_SWITCH_2 );

        if ( ( millis() - currentTime ) >= DEBOUNCE_STABLE_TIMEOUT )
        {
            Log.errorln( "Door switches wheren't stable within %d ms", DEBOUNCE_STABLE_TIMEOUT );

            /* Switch to fault state */
            return switch_state( pState, &doorControlStates[DOOR_CONTROL_STATE_FAULT] );
        }
    }
    while (    ( door1SwitchStatus.debounce == INPUT_DEBOUNCE_UNSTABLE )
            || ( door2SwitchStatus.debounce == INPUT_DEBOUNCE_UNSTABLE ) );
    

     /* Get pointer to the current event */
    event_t** pCurrentEvent = &pState->event;

    if (    ( door1SwitchStatus.state == INPUT_STATE_ACTIVE )
         && ( door2SwitchStatus.state == INPUT_STATE_ACTIVE) )
    {
        pushEvent( pCurrentEvent, DOOR_CONTROL_EVENT_DOOR_1_2_CLOSE );
    }

    if ( door1SwitchStatus.state == INPUT_STATE_INACTIVE )
    {
        pushEvent( pCurrentEvent, DOOR_CONTROL_EVENT_DOOR_1_OPEN );
    }

    if ( door2SwitchStatus.state == INPUT_STATE_INACTIVE )
    {
        pushEvent( pCurrentEvent, DOOR_CONTROL_EVENT_DOOR_2_OPEN );
    }

    return EVENT_HANDLED;
}



/**
 * @brief Handler for the init state
 * 
 * @param pState - The state machine
 * @param event - The event
 * @return state_machine_result_t - The result of the handler
 */
static state_machine_result_t initHandler( state_machine_t* const pState, const uint32_t event )
{
    Log.verboseln("%s: Event %s", __func__, logging_eventToString( (door_control_event_t) event ).c_str() );

    switch ( event )
    {
    case DOOR_CONTROL_EVENT_DOOR_1_2_CLOSE:
        switch_state( pState, &doorControlStates[DOOR_CONTROL_STATE_IDLE] );
        break;
    case DOOR_CONTROL_EVENT_DOOR_1_OPEN:
    case DOOR_CONTROL_EVENT_DOOR_2_OPEN:
        switch_state( pState, &doorControlStates[DOOR_CONTROL_STATE_FAULT] );
        break;
    default:
       break;
    }

    return EVENT_HANDLED;
}


/*
static state_machine_result_t initExitHandler( state_machine_t* const pState )
{
    Log.verboseln("%s: Event %s", __func__, logging_eventToString( (door_control_event_t) event ).c_str() );
    return EVENT_HANDLED;
}
*/


/**
 * @brief Handler for the idle state entry
 * 
 * @param pState - The state machine
 * @param event - The event
 * @return state_machine_result_t - The result of the handler
 */
static state_machine_result_t idleEntryHandler( state_machine_t* const pState, const uint32_t event )
{
    Log.verboseln( "%s: Event %s", __func__, logging_eventToString( (door_control_event_t) event ).c_str() );

    /* Set both door leds to white */
    ioMan_setLed( true, DOOR_TYPE_DOOR_1, LED_COLOR_WHITE );
    ioMan_setLed( true, DOOR_TYPE_DOOR_2, LED_COLOR_WHITE );

    return EVENT_HANDLED;
}


/**
 * @brief Handler for the idle state
 * 
 * @param pState - The state machine
 * @param event - The event
 * @return state_machine_result_t - The result of the handler
 */
static state_machine_result_t idleHandler( state_machine_t* const pState, const uint32_t event )
{
    Log.verboseln( "%s: Event %s", __func__, logging_eventToString( (door_control_event_t) event ).c_str() );

    /* Process the event */
    switch ( event )
    {
        case DOOR_CONTROL_EVENT_DOOR_1_UNLOCK:
            return switch_state( pState, &doorControlStates[DOOR_CONTROL_STATE_DOOR_1_UNLOCKED] );
        case DOOR_CONTROL_EVENT_DOOR_2_UNLOCK:
            return switch_state( pState, &doorControlStates[DOOR_CONTROL_STATE_DOOR_2_UNLOCKED] );
        case DOOR_CONTROL_EVENT_DOOR_1_OPEN:
        case DOOR_CONTROL_EVENT_DOOR_2_OPEN:
        case DOOR_CONTROL_EVENT_DOOR_1_2_OPEN:
            return switch_state( pState, &doorControlStates[DOOR_CONTROL_STATE_FAULT] );
        default:
            break;
    }

    /* Get the state of the door buttons. The debounce state isn't used here,
     * but it is necessary to call the function.
     */
    input_state_t door1Button = ioMan_getDoorState( IO_BUTTON_1 ).state;
    input_state_t door2Button = ioMan_getDoorState( IO_BUTTON_2 ).state;

    /* XOR-logic to allow only one door to be open */
    if (    ( door1Button == INPUT_STATE_ACTIVE )
         && ( door2Button == INPUT_STATE_INACTIVE ) )
    {
        pushEvent( &pState->event, DOOR_CONTROL_EVENT_DOOR_1_UNLOCK );
    }
    else if (    ( door1Button == INPUT_STATE_INACTIVE )
              && ( door2Button == INPUT_STATE_ACTIVE ) )
    {
        pushEvent( &pState->event, DOOR_CONTROL_EVENT_DOOR_2_UNLOCK );
    }
    else
    {
        ioMan_setDoorState( DOOR_TYPE_DOOR_1, LOCK_STATE_LOCKED );
        ioMan_setDoorState( DOOR_TYPE_DOOR_2, LOCK_STATE_LOCKED );
    }

    return EVENT_HANDLED;
}


/**
 * @brief Handler for the idle state exit
 * 
 * @param pState - The state machine
 * @param event - The event
 * @return state_machine_result_t - The result of the handler
 */
static state_machine_result_t idleExitHandler( state_machine_t* const pState, const uint32_t event )
{
    Log.verboseln("%s: Event %s", __func__, logging_eventToString( (door_control_event_t) event ).c_str() );

    /* Set both door leds to off */
    ioMan_setLed( false, DOOR_TYPE_DOOR_1, LED_COLOR_SIZE );
    ioMan_setLed( false, DOOR_TYPE_DOOR_2, LED_COLOR_SIZE );

    return EVENT_HANDLED;
}


/**
 * @brief Handler for the fault state entry
 * 
 * @param pState - The state machine
 * @param event - The event
 * @return state_machine_result_t - The result of the handler
 */
static state_machine_result_t faultEntryHandler( state_machine_t* const pState, const uint32_t event )
{
    Log.verboseln("%s: Event %s", __func__, logging_eventToString( (door_control_event_t) event ).c_str() );

    Timer1.attachInterrupt( faultBlinkLedIsrHandler );
    Timer1.start();

    return EVENT_HANDLED;
}


/**
 * @brief Handler for the fault state
 * 
 * @param pState - The state machine
 * @param event - The event
 * @return state_machine_result_t - The result of the handler
 */
static state_machine_result_t faultHandler( state_machine_t* const pState, const uint32_t event )
{
    Log.verboseln("%s: Event %s", __func__, logging_eventToString( (door_control_event_t) event ).c_str() );

        switch ( event )
    {
        case DOOR_CONTROL_EVENT_DOOR_1_2_CLOSE:
            switch_state( pState, &doorControlStates[DOOR_CONTROL_STATE_IDLE] );
            break;
        default:
            break;
    }

    return EVENT_HANDLED;
}


/**
 * @brief Handler for the fault state exit
 * 
 * @param pState - The state machine
 * @param event - The event
 * @return state_machine_result_t - The result of the handler
 */
static state_machine_result_t faultExitHandler( state_machine_t* const pState, const uint32_t event )
{
    Log.verboseln("%s: Event %s", __func__, logging_eventToString( (door_control_event_t) event ).c_str() );

    Timer1.stop();
    Timer1.detachInterrupt();
    ioMan_setLed( false, DOOR_TYPE_DOOR_1, LED_COLOR_SIZE );
    ioMan_setLed( false, DOOR_TYPE_DOOR_2, LED_COLOR_SIZE );

    return EVENT_HANDLED;
}


/**
 * @brief Handler for the fault led blink
 */
static void faultBlinkLedIsrHandler( void )
{
    static uint8_t ledState = false;
    ledState                = !ledState;
    ioMan_setLed( ledState, DOOR_TYPE_DOOR_1, LED_COLOR_MAGENTA );
    ioMan_setLed( ledState, DOOR_TYPE_DOOR_2, LED_COLOR_MAGENTA );
}


/**
 * @brief Handler for the door 1 unlock entry
 * 
 * @param pState - The state machine
 * @param event - The event
 * @return state_machine_result_t - The result of the handler
 */
static state_machine_result_t door1UnlockEntryHandler( state_machine_t* const pState, const uint32_t event )
{
    Log.verboseln("%s: Event %s", __func__, logging_eventToString( (door_control_event_t) event ).c_str() );

    /* Unlock the door and start led blink */
    ioMan_setDoorState( DOOR_TYPE_DOOR_1, LOCK_STATE_UNLOCKED );
    Timer1.attachInterrupt( door1BlinkLedIsrHandler );
    Timer1.start();

    doorControl.doorTimer[DOOR_TIMER_TYPE_UNLOCK].timeReference = millis();

    return EVENT_HANDLED;
}


/**
 * @brief Handler for the door 1 unlock
 * 
 * @param pState - The state machine
 * @param event - The event
 * @return state_machine_result_t - The result of the handler
 */
static state_machine_result_t door1UnlockHandler( state_machine_t* const pState, const uint32_t event )
{
    Log.verboseln("%s: Event %s", __func__, logging_eventToString( (door_control_event_t) event ).c_str() );

    switch ( event )
    {
    case DOOR_CONTROL_EVENT_DOOR_1_UNLOCK_TIMEOUT:
        switch_state( pState, &doorControlStates[DOOR_CONTROL_STATE_IDLE] );
        break;
    case DOOR_CONTROL_EVENT_DOOR_1_OPEN:
        switch_state( pState, &doorControlStates[DOOR_CONTROL_STATE_DOOR_1_OPEN] );
        break;
    default:
        break;
    }

    return EVENT_HANDLED;
}

/**
 * @brief Handler for the door 1 unlock exit
 * 
 * @param pState - The state machine
 * @param event - The event
 * @return state_machine_result_t - The result of the handler
 */
static state_machine_result_t door1UnlockExitHandler( state_machine_t* const pState, const uint32_t event )
{
    Log.verboseln("%s: Event %s", __func__, logging_eventToString( (door_control_event_t) event ).c_str() );

    /* Only lock the door and disable the led blink if we move back to the idle state */
    const state_t* pNextState = pState->State;
    if ( pNextState->Id == DOOR_CONTROL_STATE_IDLE )
    {
        /* Lock the door */
        ioMan_setDoorState( DOOR_TYPE_DOOR_1, LOCK_STATE_LOCKED );

        /* Stop the led blink and disable leds */
        Timer1.stop();
        Timer1.detachInterrupt();
        ioMan_setLed( false, DOOR_TYPE_DOOR_1, LED_COLOR_SIZE );
        ioMan_setLed( false, DOOR_TYPE_DOOR_2, LED_COLOR_SIZE );
    }

    /* Reset the door unlock timer */
    doorControl.doorTimer[DOOR_TIMER_TYPE_UNLOCK].timeReference = 0;

    return EVENT_HANDLED;
}


/**
 * @brief Handler for the door 1 open entry
 * 
 * @param pState - The state machine
 * @param event - The event
 * @return state_machine_result_t - The result of the handler
 */
static state_machine_result_t door1OpenEntryHandler( state_machine_t* const pState, const uint32_t event )
{
    Log.verboseln("%s: Event %s", __func__, logging_eventToString( (door_control_event_t) event ).c_str() );

    /* Start the door open timer */
    doorControl.doorTimer[DOOR_TIMER_TYPE_OPEN].timeReference = millis();

    return EVENT_HANDLED;
}


/**
 * @brief Handler for the door 1 open
 * 
 * @param pState - The state machine
 * @param event - The event
 * @return state_machine_result_t - The result of the handler
 */
static state_machine_result_t door1OpenHandler( state_machine_t* const pState, const uint32_t event )
{
    Log.verboseln("%s: Event %s", __func__, logging_eventToString( (door_control_event_t) event ).c_str() );

    switch ( event )
    {
    case DOOR_CONTROL_EVENT_DOOR_1_CLOSE:
        switch_state( pState, &doorControlStates[DOOR_CONTROL_STATE_IDLE] );
        break;
    case DOOR_CONTROL_EVENT_DOOR_2_OPEN:
        switch_state( pState, &doorControlStates[DOOR_CONTROL_STATE_FAULT] );
        break;
    default:
        break;
    }
    return EVENT_HANDLED;
}


/**
 * @brief Handler for the door 1 open exit
 * 
 * @param pState - The state machine
 * @param event - The event
 * @return state_machine_result_t - The result of the handler
 */
static state_machine_result_t door1OpenExitHandler( state_machine_t* const pState, const uint32_t event )
{
    Log.verboseln("%s: Event %s", __func__, logging_eventToString( (door_control_event_t) event ).c_str() );

    /* Lock the door */
    ioMan_setDoorState( DOOR_TYPE_DOOR_1, LOCK_STATE_LOCKED );

    /* Stop the led blink */
    Timer1.stop();
    Timer1.detachInterrupt();
    ioMan_setLed( false, DOOR_TYPE_DOOR_1, LED_COLOR_SIZE );
    ioMan_setLed( false, DOOR_TYPE_DOOR_2, LED_COLOR_SIZE );

    /* Reset the door open timer */
    doorControl.doorTimer[DOOR_TIMER_TYPE_OPEN].timeReference = 0;

    return EVENT_HANDLED;
}


/**
 * @brief Handler for the door 1 led blink
 */
static void door1BlinkLedIsrHandler( void )
{
    static uint8_t ledState = false;
    ledState                = !ledState;
    ioMan_setLed( ledState, DOOR_TYPE_DOOR_1, LED_COLOR_GREEN );
    ioMan_setLed( ledState, DOOR_TYPE_DOOR_2, LED_COLOR_RED );
}


/**
 * @brief Handler for the door 2 unlock entry
 * 
 * @param pState - The state machine
 * @param event - The event
 * @return state_machine_result_t - The result of the handler
 */
static state_machine_result_t door2UnlockEntryHandler( state_machine_t* const pState, const uint32_t event )
{
    Log.verboseln("%s: Event %s", __func__, logging_eventToString( (door_control_event_t) event ).c_str() );

    /* Unlock the door and start led blink */
    ioMan_setDoorState( DOOR_TYPE_DOOR_2, LOCK_STATE_UNLOCKED );
    Timer1.attachInterrupt( door2BlinkLedIsrHandler );
    Timer1.start();

    doorControl.doorTimer[DOOR_TIMER_TYPE_UNLOCK].timeReference = millis();

    return EVENT_HANDLED;
}


/**
 * @brief Handler for the door 2 unlock
 * 
 * @param pState - The state machine
 * @param event - The event
 * @return state_machine_result_t - The result of the handler
 */
static state_machine_result_t door2UnlockHandler( state_machine_t* const pState, const uint32_t event )
{
    Log.verboseln("%s: Event %s", __func__, logging_eventToString( (door_control_event_t) event ).c_str() );

    switch ( event )
    {
    case DOOR_CONTROL_EVENT_DOOR_2_UNLOCK_TIMEOUT:
        switch_state( pState, &doorControlStates[DOOR_CONTROL_STATE_IDLE] );
        break;
    case DOOR_CONTROL_EVENT_DOOR_2_OPEN:
        switch_state( pState, &doorControlStates[DOOR_CONTROL_STATE_DOOR_2_OPEN] );
        break;
    default:
        break;
    }

    return EVENT_HANDLED;
}

/**
 * @brief Handler for the door 2 unlock exit
 * 
 * @param pState - The state machine
 * @param event - The event
 * @return state_machine_result_t - The result of the handler
 */
static state_machine_result_t door2UnlockExitHandler( state_machine_t* const pState, const uint32_t event )
{
    Log.verboseln("%s: Event %s", __func__, logging_eventToString( (door_control_event_t) event ).c_str() );

    /* Only lock the door and disable the led blink if we move back to the idle state */
    const state_t* pNextState = pState->State;
    if ( pNextState->Id == DOOR_CONTROL_STATE_IDLE )
    {
        /* Lock the door */
        ioMan_setDoorState( DOOR_TYPE_DOOR_2, LOCK_STATE_LOCKED );

        /* Stop the led blink and disable leds */
        Timer1.stop();
        Timer1.detachInterrupt();
        ioMan_setLed( false, DOOR_TYPE_DOOR_1, LED_COLOR_SIZE );
        ioMan_setLed( false, DOOR_TYPE_DOOR_2, LED_COLOR_SIZE );
    }

    /* Reset the door unlock timer */
    doorControl.doorTimer[DOOR_TIMER_TYPE_UNLOCK].timeReference = 0;

    return EVENT_HANDLED;
}


/**
 * @brief Handler for the door 2 open entry
 * 
 * @param pState - The state machine
 * @param event - The event
 * @return state_machine_result_t - The result of the handler
 */
static state_machine_result_t door2OpenEntryHandler( state_machine_t* const pState, const uint32_t event )
{
    Log.verboseln("%s: Event %s", __func__, logging_eventToString( (door_control_event_t) event ).c_str() );

    /* Unlock the door */
    ioMan_setDoorState( DOOR_TYPE_DOOR_2, LOCK_STATE_UNLOCKED );

    Timer1.attachInterrupt( door2BlinkLedIsrHandler );
    Timer1.start();

    /* Start the door open timer */
    doorControl.doorTimer[DOOR_TIMER_TYPE_OPEN].timeReference = millis();

    return EVENT_HANDLED;
}


/**
 * @brief Handler for the door 2 open
 * 
 * @param pState - The state machine
 * @param event - The event
 * @return state_machine_result_t - The result of the handler
 */
static state_machine_result_t door2OpenHandler( state_machine_t* const pState, const uint32_t event )
{
    Log.verboseln("%s: Event %s", __func__, logging_eventToString( (door_control_event_t) event ).c_str() );

    switch ( event )
    {
    case DOOR_CONTROL_EVENT_DOOR_2_CLOSE:
        switch_state( pState, &doorControlStates[DOOR_CONTROL_STATE_IDLE] );
        break;
    case DOOR_CONTROL_EVENT_DOOR_1_OPEN:
        switch_state( pState, &doorControlStates[DOOR_CONTROL_STATE_FAULT] );
        break;
    default:
        break;
    }
    return EVENT_HANDLED;
}


/**
 * @brief Handler for the door 2 open exit
 * 
 * @param pState - The state machine
 * @param event - The event
 * @return state_machine_result_t - The result of the handler
 */
static state_machine_result_t door2OpenExitHandler( state_machine_t* const pState, const uint32_t event )
{
    Log.verboseln("%s: Event %s", __func__, logging_eventToString( (door_control_event_t) event ).c_str() );

    /* Lock the door */
    ioMan_setDoorState( DOOR_TYPE_DOOR_2, LOCK_STATE_LOCKED );

    /* Stop the led blink */
    Timer1.stop();
    Timer1.detachInterrupt();
    ioMan_setLed( false, DOOR_TYPE_DOOR_1, LED_COLOR_SIZE );
    ioMan_setLed( false, DOOR_TYPE_DOOR_2, LED_COLOR_SIZE );

    /* Reset the door open timer */
    doorControl.doorTimer[DOOR_TIMER_TYPE_OPEN].timeReference = 0;

    return EVENT_HANDLED;
}



/**
 * @brief Interrupt Service Routine (ISR) handler for blinking LEDs on door 2.
 *
 * This function toggles the state of two LEDs each time it is called. The red LED
 * on door 1 and the green LED on door 2 will be toggled between on and off states.
 *
 * The function uses a static variable to keep track of the current state of the LEDs.
 * Each time the function is called, the state is inverted and the LEDs are updated
 * accordingly.
 *
 * @note This function is intended to be used as an ISR handler and should be kept
 *       as short and efficient as possible.
 */
static void door2BlinkLedIsrHandler( void )
{
    static uint8_t ledState = false;
    ledState                = !ledState;
    ioMan_setLed( ledState, DOOR_TYPE_DOOR_1, LED_COLOR_RED );
    ioMan_setLed( ledState, DOOR_TYPE_DOOR_2, LED_COLOR_GREEN );
}



/**
 * @brief Handles the timeout event for the door open state.
 *
 * This function is called when the door open timeout occurs. It logs the event
 * and switches the state machine to the fault state if the door is not closed
 * within the specified time.
 *
 * @param time The time elapsed since the door was opened.
 */
static void doorOpenTimeoutHandler( uint32_t time )
{
    Log.verboseln("%s: Time: %d", __func__, time );

    /* Switch to the fault state if the door is not closed in time */
    switch_state( &doorControl.machine, &doorControlStates[DOOR_CONTROL_STATE_FAULT] );
}


/**
 * @brief Handles the timeout event for door unlocking.
 *
 * This function is called when the door unlock timeout occurs. It logs the 
 * event with the provided time and switches the state machine back to the 
 * idle state if the door is not opened in time by pushing the appropriate 
 * timeout events.
 *
 * @param time The time at which the timeout event occurred.
 */
static void doorUnlockTimeoutHandler( uint32_t time )
{
    Log.verboseln("%s: Time: %d", __func__, time );

    /* Switch back to the idle state if the door is not opened in time */
    pushEvent( &doorControl.machine.event, DOOR_CONTROL_EVENT_DOOR_1_UNLOCK_TIMEOUT );
    pushEvent( &doorControl.machine.event, DOOR_CONTROL_EVENT_DOOR_2_UNLOCK_TIMEOUT );
}



/**
 * @brief Generates events based on the state of door switches.
 *
 * This function reads the state of two door switches and generates corresponding events
 * for the door control state machine. It logs the state of the door switches and pushes
 * events to the event queue based on the following conditions:
 * 
 * - If both door switches are inactive, it generates a DOOR_CONTROL_EVENT_DOOR_1_2_OPEN event.
 * - If both door switches are active, it generates a DOOR_CONTROL_EVENT_DOOR_1_2_CLOSE event.
 * - If door 1 switch is active, it generates a DOOR_CONTROL_EVENT_DOOR_1_CLOSE event.
 * - If door 1 switch is inactive, it generates a DOOR_CONTROL_EVENT_DOOR_1_OPEN event.
 * - If door 2 switch is active, it generates a DOOR_CONTROL_EVENT_DOOR_2_CLOSE event.
 * - If door 2 switch is inactive, it generates a DOOR_CONTROL_EVENT_DOOR_2_OPEN event.
 *
 * @param pDoorControl Pointer to the door control structure.
 */
static void stateMan_generateEvent( door_control_t* const pDoorControl )
{
    /* Read the state of both door switches */
    input_state_t door1Switch = ioMan_getDoorState( IO_SWITCH_1 ).state;
    input_state_t door2Switch = ioMan_getDoorState( IO_SWITCH_2 ).state;

    Log.verboseln( "%s: Door 1 switch: %s, Door 2 switch: %s", __func__, 
                    logging_inputStateToString( door1Switch ).c_str(),
                    logging_inputStateToString( door2Switch ).c_str() );

    /* Get pointer to the current event */
    event_t** pCurrentEvent = &pDoorControl->machine.event;

    /* Generate the event */
    if (    ( door1Switch == INPUT_STATE_INACTIVE )
         && ( door2Switch == INPUT_STATE_INACTIVE ) )
    {
        pushEvent( pCurrentEvent, DOOR_CONTROL_EVENT_DOOR_1_2_OPEN );
    }

    if (    ( door1Switch == INPUT_STATE_ACTIVE )
         && ( door2Switch == INPUT_STATE_ACTIVE ) )
    {
        pushEvent( pCurrentEvent, DOOR_CONTROL_EVENT_DOOR_1_2_CLOSE );
    }

    if ( door1Switch == INPUT_STATE_ACTIVE )
    {
        pushEvent( pCurrentEvent, DOOR_CONTROL_EVENT_DOOR_1_CLOSE );
    }

    if ( door1Switch == INPUT_STATE_INACTIVE )
    {
        pushEvent( pCurrentEvent, DOOR_CONTROL_EVENT_DOOR_1_OPEN );
    }

    if ( door2Switch == INPUT_STATE_ACTIVE )
    {
        pushEvent( pCurrentEvent, DOOR_CONTROL_EVENT_DOOR_2_CLOSE );
    }

    if ( door2Switch == INPUT_STATE_INACTIVE )
    {
        pushEvent( &pDoorControl->machine.event, DOOR_CONTROL_EVENT_DOOR_2_OPEN );
    }
}



/**
 * @brief Processes the door control timers and handles timer expiration.
 *
 * This function checks the status of each door timer in the door control structure.
 * If a timer is running and has expired, it calls the associated handler and resets the timer.
 * It also logs the remaining time for each running timer.
 *
 * @param pDoorControl Pointer to the door control structure containing the timers.
 */
static void stateMan_processTimers( door_control_t* const pDoorControl )
{
    Log.verboseln( "%s", __func__ );

    /* Get the current time */
    uint64_t currentTime = millis();

    /* Loop through all door timers */
    for ( uint8_t i = 0; i < DOOR_TYPE_SIZE; i++ )
    {
        /* Check if the timer is running */
        if ( pDoorControl->doorTimer[i].timeReference != 0 )
        {
            /* Check if the timer has expired */
            if ( ( currentTime - pDoorControl->doorTimer[i].timeReference ) >= pDoorControl->doorTimer[i].timeout )
            {
                /* Call the timer handler */
                pDoorControl->doorTimer[i].handler( currentTime );
                /* Reset the timer */
                pDoorControl->doorTimer[i].timeReference = 0;
                return;
            }

            /* Calculate remaining time */
            String remainingTime = String( ( pDoorControl->doorTimer[i].timeout - ( currentTime - pDoorControl->doorTimer[i].timeReference ) ) / 1000.0F );
            Log.noticeln( "%s: %s", logging_timerTypeToString( (door_timer_type_t) i ).c_str(), remainingTime.c_str() );
        }
    }
}


/**
 * @brief Sets the door timer based on the specified timer type and timeout value.
 *
 * This function configures the timeout for a specific door timer type. The timeout
 * value is converted to milliseconds or minutes depending on the timer type.
 *
 * @param timerType The type of the door timer to set. Must be a value of type `door_timer_type_t`.
 *                  Valid values are:
 *                  - DOOR_TIMER_TYPE_UNLOCK: Sets the unlock timer (timeout in seconds).
 *                  - DOOR_TIMER_TYPE_OPEN: Sets the open timer (timeout in minutes).
 * @param timeout The timeout value for the specified timer type. The unit of this value
 *                depends on the timer type:
 *                - For DOOR_TIMER_TYPE_UNLOCK, the timeout is in seconds.
 *                - For DOOR_TIMER_TYPE_OPEN, the timeout is in minutes.
 *
 * @note If an invalid timer type is provided, the function logs an error and returns without
 *       making any changes.
 */
void stateMan_setDoorTimer( door_timer_type_t timerType, uint32_t timeout )
{
    if ( timerType >= DOOR_TIMER_TYPE_SIZE )
    {
        Log.errorln( "%s: Invalid timer type", __func__ );
        return;
    }

    switch ( timerType )
    {
    case DOOR_TIMER_TYPE_UNLOCK:
        doorControl.doorTimer[DOOR_TIMER_TYPE_UNLOCK].timeout = ( (uint32_t)timeout * 1000 );
        break;
    case DOOR_TIMER_TYPE_OPEN:
        doorControl.doorTimer[DOOR_TIMER_TYPE_OPEN].timeout = ( (uint32_t) timeout ) * 60000;
        break;
    default:
        break;
    }
}
