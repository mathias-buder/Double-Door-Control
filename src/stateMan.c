/**
 * \file    stateManagement.c
 * \brief   Source file for stateManagement

 * \author  Mathias Buder
 * \date    2024-09-13

 *  Copyright (c) 2024 Mathias Buder
 */

#include "stateMan.h"
#include "ioMan.h"


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
 * @brief The array of state machines
 * @details The array of state machines is used to dispatch the event to the state machines.
 */
state_machine_t* const stateMachines[] = {(state_machine_t*) &doorControl};




/**
 * @brief Handler for the init state entry
 * 
 * @param pState - The state machine
 * @param event - The event
 * @return state_machine_result_t - The result of the handler
 */
static state_machine_result_t initEntryHandler( state_machine_t* const pState, const uint32_t event )
{
    Log.verboseln("%s: Event %s", __func__, eventToString( (door_control_event_t) event ).c_str() );

    /* Initialize both doors to locked */
    setDoorState( DOOR_TYPE_DOOR_1, LOCK_STATE_LOCKED );
    setDoorState( DOOR_TYPE_DOOR_2, LOCK_STATE_LOCKED );

    /* Check whether the door switches are debouncing. This "waiting" mechanism is only used
     * for the initialization as the door switches are checked here in an one-shot manner. If
     * the switches are not stable within the timeout, the state machine switches to the fault state.
     */
    input_status_t door1SwitchStatus, door2SwitchStatus;
    uint64_t        currentTime = millis();

    do {
        door1SwitchStatus = getDoorIoState( IO_SWITCH_1 );
        door2SwitchStatus = getDoorIoState( IO_SWITCH_2 );

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
    Log.verboseln("%s: Event %s", __func__, eventToString( (door_control_event_t) event ).c_str() );

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
    Log.verboseln("%s: Event %s", __func__, eventToString( (door_control_event_t) event ).c_str() );
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
    Log.verboseln( "%s: Event %s", __func__, eventToString( (door_control_event_t) event ).c_str() );

    /* Set both door leds to white */
    setLed( true, DOOR_TYPE_DOOR_1, LED_COLOR_WHITE );
    setLed( true, DOOR_TYPE_DOOR_2, LED_COLOR_WHITE );

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
    Log.verboseln( "%s: Event %s", __func__, eventToString( (door_control_event_t) event ).c_str() );

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
    input_state_t door1Button = getDoorIoState( IO_BUTTON_1 ).state;
    input_state_t door2Button = getDoorIoState( IO_BUTTON_2 ).state;

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
        setDoorState( DOOR_TYPE_DOOR_1, LOCK_STATE_LOCKED );
        setDoorState( DOOR_TYPE_DOOR_2, LOCK_STATE_LOCKED );
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
    Log.verboseln("%s: Event %s", __func__, eventToString( (door_control_event_t) event ).c_str() );

    /* Set both door leds to off */
    setLed( false, DOOR_TYPE_DOOR_1, LED_COLOR_SIZE );
    setLed( false, DOOR_TYPE_DOOR_2, LED_COLOR_SIZE );

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
    Log.verboseln("%s: Event %s", __func__, eventToString( (door_control_event_t) event ).c_str() );

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
    Log.verboseln("%s: Event %s", __func__, eventToString( (door_control_event_t) event ).c_str() );

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
    Log.verboseln("%s: Event %s", __func__, eventToString( (door_control_event_t) event ).c_str() );

    Timer1.stop();
    Timer1.detachInterrupt();
    setLed( false, DOOR_TYPE_DOOR_1, LED_COLOR_SIZE );
    setLed( false, DOOR_TYPE_DOOR_2, LED_COLOR_SIZE );

    return EVENT_HANDLED;
}


/**
 * @brief Handler for the fault led blink
 */
static void faultBlinkLedIsrHandler( void )
{
    static uint8_t ledState = false;
    ledState                = !ledState;
    setLed( ledState, DOOR_TYPE_DOOR_1, LED_COLOR_MAGENTA );
    setLed( ledState, DOOR_TYPE_DOOR_2, LED_COLOR_MAGENTA );
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
    Log.verboseln("%s: Event %s", __func__, eventToString( (door_control_event_t) event ).c_str() );

    /* Unlock the door and start led blink */
    setDoorState( DOOR_TYPE_DOOR_1, LOCK_STATE_UNLOCKED );
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
    Log.verboseln("%s: Event %s", __func__, eventToString( (door_control_event_t) event ).c_str() );

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
    Log.verboseln("%s: Event %s", __func__, eventToString( (door_control_event_t) event ).c_str() );

    /* Only lock the door and disable the led blink if we move back to the idle state */
    const state_t* pNextState = pState->State;
    if ( pNextState->Id == DOOR_CONTROL_STATE_IDLE )
    {
        /* Lock the door */
        setDoorState( DOOR_TYPE_DOOR_1, LOCK_STATE_LOCKED );

        /* Stop the led blink and disable leds */
        Timer1.stop();
        Timer1.detachInterrupt();
        setLed( false, DOOR_TYPE_DOOR_1, LED_COLOR_SIZE );
        setLed( false, DOOR_TYPE_DOOR_2, LED_COLOR_SIZE );
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
    Log.verboseln("%s: Event %s", __func__, eventToString( (door_control_event_t) event ).c_str() );

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
    Log.verboseln("%s: Event %s", __func__, eventToString( (door_control_event_t) event ).c_str() );

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
    Log.verboseln("%s: Event %s", __func__, eventToString( (door_control_event_t) event ).c_str() );

    /* Lock the door */
    setDoorState( DOOR_TYPE_DOOR_1, LOCK_STATE_LOCKED );

    /* Stop the led blink */
    Timer1.stop();
    Timer1.detachInterrupt();
    setLed( false, DOOR_TYPE_DOOR_1, LED_COLOR_SIZE );
    setLed( false, DOOR_TYPE_DOOR_2, LED_COLOR_SIZE );

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
    setLed( ledState, DOOR_TYPE_DOOR_1, LED_COLOR_GREEN );
    setLed( ledState, DOOR_TYPE_DOOR_2, LED_COLOR_RED );
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
    Log.verboseln("%s: Event %s", __func__, eventToString( (door_control_event_t) event ).c_str() );

    /* Unlock the door and start led blink */
    setDoorState( DOOR_TYPE_DOOR_2, LOCK_STATE_UNLOCKED );
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
    Log.verboseln("%s: Event %s", __func__, eventToString( (door_control_event_t) event ).c_str() );

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
    Log.verboseln("%s: Event %s", __func__, eventToString( (door_control_event_t) event ).c_str() );

    /* Only lock the door and disable the led blink if we move back to the idle state */
    const state_t* pNextState = pState->State;
    if ( pNextState->Id == DOOR_CONTROL_STATE_IDLE )
    {
        /* Lock the door */
        setDoorState( DOOR_TYPE_DOOR_2, LOCK_STATE_LOCKED );

        /* Stop the led blink and disable leds */
        Timer1.stop();
        Timer1.detachInterrupt();
        setLed( false, DOOR_TYPE_DOOR_1, LED_COLOR_SIZE );
        setLed( false, DOOR_TYPE_DOOR_2, LED_COLOR_SIZE );
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
    Log.verboseln("%s: Event %s", __func__, eventToString( (door_control_event_t) event ).c_str() );

    /* Unlock the door */
    setDoorState( DOOR_TYPE_DOOR_2, LOCK_STATE_UNLOCKED );

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
    Log.verboseln("%s: Event %s", __func__, eventToString( (door_control_event_t) event ).c_str() );

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
    Log.verboseln("%s: Event %s", __func__, eventToString( (door_control_event_t) event ).c_str() );

    /* Lock the door */
    setDoorState( DOOR_TYPE_DOOR_2, LOCK_STATE_LOCKED );

    /* Stop the led blink */
    Timer1.stop();
    Timer1.detachInterrupt();
    setLed( false, DOOR_TYPE_DOOR_1, LED_COLOR_SIZE );
    setLed( false, DOOR_TYPE_DOOR_2, LED_COLOR_SIZE );

    /* Reset the door open timer */
    doorControl.doorTimer[DOOR_TIMER_TYPE_OPEN].timeReference = 0;

    return EVENT_HANDLED;
}


/**
 * @brief Handler for the door 2 led blink
 */
static void door2BlinkLedIsrHandler( void )
{
    static uint8_t ledState = false;
    ledState                = !ledState;
    setLed( ledState, DOOR_TYPE_DOOR_1, LED_COLOR_RED );
    setLed( ledState, DOOR_TYPE_DOOR_2, LED_COLOR_GREEN );
}


/**
 * @brief Handler for the door 1 open timeout
 * 
 * @param time - The time
 */
static void doorOpenTimeoutHandler( uint32_t time )
{
    Log.verboseln("%s: Time: %d", __func__, time );

    /* Switch to the fault state if the door is not closed in time */
    switch_state( &doorControl.machine, &doorControlStates[DOOR_CONTROL_STATE_FAULT] );
}


static void doorUnlockTimeoutHandler( uint32_t time )
{
    Log.verboseln("%s: Time: %d", __func__, time );

    /* Switch back to the idle state if the door is not opened in time */
    pushEvent( &doorControl.machine.event, DOOR_CONTROL_EVENT_DOOR_1_UNLOCK_TIMEOUT );
    pushEvent( &doorControl.machine.event, DOOR_CONTROL_EVENT_DOOR_2_UNLOCK_TIMEOUT );
}




/**
 * @brief Generate the event based on the door switches and buttons
 *
 * @param pDoorControl - Pointer to the door control
 */
static void generateEvent( door_control_t* const pDoorControl )
{
    /* Read the state of both door switches */
    input_state_t door1Switch = getDoorIoState( IO_SWITCH_1 ).state;
    input_state_t door2Switch = getDoorIoState( IO_SWITCH_2 ).state;

    Log.verboseln( "%s: Door 1 switch: %s, Door 2 switch: %s", __func__, 
                    inputStateToString( door1Switch ).c_str(),
                    inputStateToString( door2Switch ).c_str() );

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