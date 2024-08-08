#include <Arduino.h>
#include <ArduinoLog.h>
#include <TimerOne.h>

#include "hsm.h"

/****************************************************************************************/
/*                                    PIN CONFIGURATION                                 */
/****************************************************************************************/
/* DOOR 1 */
/****************************************************************************************/
#define RBG_LED_1_R         5   /*!< Pin for the red LED of the RGB-LED */
#define RBG_LED_1_G         6   /*!< Pin for the green LED of the RGB-LED */
#define RBG_LED_1_B         7   /*!< Pin for the blue LED of the RGB-LED */

#define DOOR_1_BUTTON       2   /*!< Pin for the button of the door */
#define DOOR_1_SWITCH       3   /*!< Pin for the switch of the door */
#define DOOR_1_MAGNET       4   /*!< Pin for the magnet of the door */

/****************************************************************************************/
/* DOOR 2 */
/****************************************************************************************/
#define RBG_LED_2_R         11  /*!< Pin for the red LED of the RGB-LED */
#define RBG_LED_2_G         12  /*!< Pin for the green LED of the RGB-LED */
#define RBG_LED_2_B         13  /*!< Pin for the blue LED of the RGB-LED */

#define DOOR_2_BUTTON       10  /*!< Pin for the button of the door */
#define DOOR_2_SWITCH       9   /*!< Pin for the switch of the door */
#define DOOR_2_MAGNET       8   /*!< Pin for the magnet of the door */

/****************************************************************************************/
#define DOOR_BUTTON_DEBOUNCE_DELAY  100 /*!< Debounce delay for the door button @unit ms*/




/************************************* ENUMERATION **************************************/

/**
 * @brief
 */
typedef enum
{
    DOOR_CONTROL_STATE_INIT,        /*!< Initializing the state machine */
    DOOR_CONTROL_STATE_IDLE,        /*!< The state machine is in idle state */
    DOOR_CONTROL_STATE_FAULT,       /*!< The state machine is in fault state */
    DOOR_CONTROL_STATE_DOOR_1_OPEN, /*!< The door 1 is open */
    DOOR_CONTROL_STATE_DOOR_2_OPEN  /*!< The door 2 is open */
} door_control_state_t;

/**
 * @brief
 */
typedef enum
{
    DOOR_CONTROL_EVENT_INIT_DONE = 1,       /*!< The initialization is done successfully */
    DOOR_CONTROL_EVENT_DOOR_1_OPEN,         /*!< The door 1 is open */
    DOOR_CONTROL_EVENT_DOOR_1_CLOSE,        /*!< The door 1 is closed */
    DOOR_CONTROL_EVENT_DOOR_1_OPEN_TIMEOUT, /*!< The door 1 is open timeout */
    DOOR_CONTROL_EVENT_DOOR_2_OPEN,         /*!< The door 2 is open */
    DOOR_CONTROL_EVENT_DOOR_2_CLOSE,        /*!< The door 2 is closed */
    DOOR_CONTROL_EVENT_DOOR_2_OPEN_TIMEOUT, /*!< The door 2 is open timeout */
    DOOR_CONTROL_EVENT_DOOR_1_2_OPEN,       /*!< The door 1 and 2 are open */
    DOOR_CONTROL_EVENT_DOOR_1_2_CLOSE       /*!< The door 1 and 2 are closed */
} door_control_event_t;

typedef enum
{
    DOOR_TYPE_DOOR_1, /*!< The door 1 */
    DOOR_TYPE_DOOR_2, /*!< The door 2 */
    DOOR_TYPE_SIZE    /*!< Number of doors */
} door_type_t;

typedef enum
{
    LOCK_STATE_CLOSED = LOW,    /*!< The door is closed */
    LOCK_STATE_OPEN   = HIGH    /*!< The door is open */
} lock_state_t;

typedef enum
{
    BUTTON_STATE_RELEASED = 0, /*!< The button is released */
    BUTTON_STATE_PRESSED        /*!< The button is pressed */
} button_state_t;


/************************************* STRUCTURE **************************************/

//! doorControl state machine
typedef struct
{
    state_machine_t machine;      //!< Abstract state machine

    // uint32_t Set_Time;    //! Set time of a doorControl
    // uint32_t Resume_Time; //!< Remaining time when the doorControl is paused
    // uint32_t Timer;       //!< Process timer
}door_control_t;




/******************************** Function prototype ************************************/

static state_machine_result_t initHandler( state_machine_t* const pState, const uint32_t event );
/* static state_machine_result_t initEntryHandler( state_machine_t* const pState, const uint32_t event ); */
/* static state_machine_result_t initExitHandler( state_machine_t* const pState, const uint32_t event ); */

static state_machine_result_t idleHandler( state_machine_t* const pState, const uint32_t event );
/* static state_machine_result_t idleEntryHandler( state_machine_t* const pState, const uint32_t event ); */
/* static state_machine_result_t idleExitHandler( state_machine_t* const pState, const uint32_t event ); */

static state_machine_result_t faultHandler( state_machine_t* const pState, const uint32_t event );
/* static state_machine_result_t faultEntryHandler( state_machine_t* const pState, const uint32_t event ); */
/* static state_machine_result_t faultExitHandler( state_machine_t* const pState, const uint32_t event ); */

static state_machine_result_t door1OpenHandler( state_machine_t* const pState, const uint32_t event );
/* static state_machine_result_t door1OpenEntryHandler( state_machine_t* const pState, const uint32_t event ); */
/* static state_machine_result_t door1OpenExitHandler( state_machine_t* const pState, const uint32_t event ); */

static state_machine_result_t door2OpenHandler( state_machine_t* const pState, const uint32_t event );
/* static state_machine_result_t door2OpenEntryHandler( state_machine_t* const pState, const uint32_t event ); */
/* static state_machine_result_t door2OpenExitHandler( state_machine_t* const pState, const uint32_t event ); */

static void           init( door_control_t* const pDoorControl, uint32_t processTime );
void                  eventLogger( uint32_t stateMachine, uint32_t state, uint32_t event );
void                  resultLogger( uint32_t state, state_machine_result_t result );
static void           getDoorState( const door_type_t door, button_state_t* const doorSwitch, button_state_t* const doorButton );
static void           setDoorState( const door_type_t door, const lock_state_t state );
static button_state_t getDoorButtonState( const door_type_t door );
static void           generateEvent( door_control_t* const pDoorControl );
String stateToString( door_control_state_t state );
String eventToString( door_control_event_t event );
String resultToString( state_machine_result_t result );

/******************************** Global variables ************************************/

static const state_t doorControlStates[] = {

    [DOOR_CONTROL_STATE_INIT] = {
        .Handler = initHandler,
        .Entry   = NULL,
        .Exit    = NULL,
        .Id      = DOOR_CONTROL_STATE_INIT
    },

    [DOOR_CONTROL_STATE_IDLE] = {
        .Handler = idleHandler,
        .Entry   = NULL,
        .Exit    = NULL,
        .Id      = DOOR_CONTROL_STATE_IDLE
    },

    [DOOR_CONTROL_STATE_FAULT] = {
        .Handler = faultHandler,
        .Entry   = NULL,
        .Exit    = NULL,
        .Id      = DOOR_CONTROL_STATE_FAULT
    },

    [DOOR_CONTROL_STATE_DOOR_1_OPEN] = {
        .Handler = door1OpenHandler, 
        .Entry   = NULL,
        .Exit    = NULL,
        .Id      = DOOR_CONTROL_STATE_DOOR_1_OPEN
    },

    [DOOR_CONTROL_STATE_DOOR_2_OPEN] = {
        .Handler = door2OpenHandler,
        .Entry   = NULL,
        .Exit    = NULL,
        .Id      = DOOR_CONTROL_STATE_DOOR_2_OPEN
    }
};

door_control_t         doorControl;                                            /*!< Instance of door_control_t */
state_machine_t* const stateMachines[] = { (state_machine_t* ) &doorControl }; /*!< Create and initialize the array of state machines. */

/**
 * @brief Setup Funktion
 * @details Wird einmalig beim Start des Programms ausgeführt und initialisiert
 * die Steuerung
 */
void setup()
{
    /* Initialize with log level and log output */
    Serial.begin( 115200 );
    Log.begin( LOG_LEVEL_INFO, &Serial );

    /* Initialize the doorControl */
    init( &doorControl, 0 );
}

/* ***************************************************************************************
 *                                     HAUPTPROGRAMM
 * **************************************************************************************/
void loop()
{
    /* Generate/Process the event */
    generateEvent( &doorControl );

    /* Dispatch the event to the state machine */
    if ( dispatch_event( stateMachines, 1, eventLogger, resultLogger ) == EVENT_UN_HANDLED )
    {
        Log.error( "Event is not handled" CR );
    }
}


static void init( door_control_t* const pDoorControl, uint32_t processTime )
{
    pDoorControl->machine.State = &doorControlStates[DOOR_CONTROL_STATE_INIT];

    // pDoorControl->Set_Time      = processTime;
    // pDoorControl->Resume_Time   = 0;

    /* initEntryHandler( (state_machine_t*) pDoorControl ); */
}


/*
static state_machine_result_t initEntryHandler( state_machine_t* const pState )
{
    Log.notice("Init Entry Handler" CR);
    return EVENT_HANDLED;
}
*/

static state_machine_result_t initHandler( state_machine_t* const pState, const uint32_t event )
{
    Log.verbose("%s: Event %s" CR, __func__, eventToString( (door_control_event_t) event ).c_str() );

    switch ( event )
    {
    case DOOR_CONTROL_EVENT_INIT_DONE:
        switch_state( pState, &doorControlStates[DOOR_CONTROL_STATE_IDLE] );
        break;
    case DOOR_CONTROL_EVENT_DOOR_1_2_OPEN:
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
    Log.notice("Init Exit Handler" CR);
    return EVENT_HANDLED;
}
*/

/*
static state_machine_result_t idleEntryHandler( state_machine_t* const pState )
{
    Log.notice("Idle Entry Handler" CR);
    return EVENT_HANDLED;
}
*/


static state_machine_result_t idleHandler( state_machine_t* const pState, const uint32_t event )
{
    Log.verbose( "%s: Event %s" CR, __func__, eventToString( (door_control_event_t) event ).c_str() );

    /* Process door buttons */
    door_type_t doors[DOOR_TYPE_SIZE] = {DOOR_TYPE_DOOR_1, DOOR_TYPE_DOOR_2};

    for ( uint8_t i = 0; i < DOOR_TYPE_SIZE; i++ )
    {
        if ( getDoorButtonState( doors[i] ) == BUTTON_STATE_PRESSED )
        {
            // Log.notice( "Door %d is pressed" CR, doors[i] );
            setDoorState( doors[i], LOCK_STATE_OPEN );
        }
        else if ( getDoorButtonState( doors[i] ) == BUTTON_STATE_RELEASED )
        {
            // Log.notice( "Door %d is released" CR, doors[i] );
            setDoorState( doors[i], LOCK_STATE_CLOSED );
        }
    }

    /* Process the event */
    switch ( event )
    {
        case DOOR_CONTROL_EVENT_DOOR_1_OPEN:
            switch_state( pState, &doorControlStates[DOOR_CONTROL_STATE_DOOR_1_OPEN] );
            break;
        case DOOR_CONTROL_EVENT_DOOR_2_OPEN:
            switch_state( pState, &doorControlStates[DOOR_CONTROL_STATE_DOOR_2_OPEN] );
            break;
        case DOOR_CONTROL_EVENT_DOOR_1_2_OPEN:
            switch_state( pState, &doorControlStates[DOOR_CONTROL_STATE_FAULT] );
            break;
        default:
            break;
    }

    return EVENT_HANDLED;
}

/*
static state_machine_result_t idleExitHandler( state_machine_t* const pState )
{
    Log.notice("Idle Exit Handler" CR);
    return EVENT_HANDLED;
}
*/

/*
static state_machine_result_t faultEntryHandler( state_machine_t* const pState )
{
    Log.notice("Fault Entry Handler" CR);
    return EVENT_HANDLED;
}
*/

static state_machine_result_t faultHandler( state_machine_t* const pState, const uint32_t event )
{
    Log.verbose("%s: Event %s" CR, __func__, eventToString( (door_control_event_t) event ).c_str() );

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

/*
static state_machine_result_t faultExitHandler( state_machine_t* const pState )
{
    Log.notice("Fault Exit Handler" CR);
    return EVENT_HANDLED;
}
*/


/*
static state_machine_result_t door1OpenEntryHandler( state_machine_t* const pState )
{
    Log.notice("Door 1 Open Entry Handler" CR);
    return EVENT_HANDLED;
}
*/

static state_machine_result_t door1OpenHandler( state_machine_t* const pState, const uint32_t event )
{
    Log.verbose("%s: Event %s" CR, __func__, eventToString( (door_control_event_t) event ).c_str() );

    /* Keep the door open */
    setDoorState( DOOR_TYPE_DOOR_1, LOCK_STATE_OPEN );

    switch ( event )
    {
    case DOOR_CONTROL_EVENT_DOOR_1_CLOSE:
        switch_state( pState, &doorControlStates[DOOR_CONTROL_STATE_IDLE] );
        break;
    default:
        break;
    }
    return EVENT_HANDLED;
}

/*
static state_machine_result_t door1OpenExitHandler( state_machine_t* const pState )
{
    Log.notice("Door 1 Open Exit Handler" CR);
    return EVENT_HANDLED;
}
*/



/*
static state_machine_result_t door2OpenEntryHandler( state_machine_t* const pState )
{
    Log.notice("Door 2 Open Entry Handler" CR);
    return EVENT_HANDLED;
}
*/

static state_machine_result_t door2OpenHandler( state_machine_t* const pState, const uint32_t event )
{
    Log.verbose("%s: Event %s" CR, __func__, eventToString( (door_control_event_t) event ).c_str() );

    /* Keep the door open */
    setDoorState( DOOR_TYPE_DOOR_2, LOCK_STATE_OPEN );

    switch ( event )
    {
    case DOOR_CONTROL_EVENT_DOOR_2_CLOSE:
        switch_state( pState, &doorControlStates[DOOR_CONTROL_STATE_IDLE] );
        break;
    default:
        Log.error("%s: EVENT_UN_HANDLED" CR, __func__);
        return EVENT_UN_HANDLED;
    }
    return EVENT_HANDLED;
}

/*
static state_machine_result_t door2OpenExitHandler( state_machine_t* const pState )
{
    Log.notice("Door 2 Open Exit Handler" CR);
    return EVENT_HANDLED;
}
*/



//! Callback function to log the events dispatched by state machine framework.
void eventLogger( uint32_t stateMachine, uint32_t state, uint32_t event )
{
    (void) stateMachine;
    static uint32_t lastEvent = 0;

    /* Only log if the event is changed */
    if ( lastEvent != event )
    {
        Log.notice( "%s: Event: %s, State: %s" CR, __func__,
                                                   eventToString( (door_control_event_t) event ).c_str(),
                                                   stateToString( (door_control_state_t) state ).c_str() );
    }

    lastEvent = event;
}

//! Callback function to log the result of event processed by state machine
void resultLogger(uint32_t state, state_machine_result_t result)
{
    static uint32_t lastState = 0;

    /* Only log if the state is changed */
    if ( lastState != state )
    {
        Log.notice( "%s: Result: %s, Current state: %s" CR, __func__,
                                                            resultToString( result ).c_str(),
                                                            stateToString( (door_control_state_t) state ).c_str() );
    }

    lastState = state;
}


static void getDoorState( const door_type_t door, button_state_t* const doorSwitch, button_state_t* const doorButton )
{
    if (    ( doorSwitch == NULL )
         || ( doorButton == NULL ) )
    {
        Log.error( "doorSwitch or doorButton is NULL" CR );
        return;
    }

    switch ( door )
    {
    case DOOR_TYPE_DOOR_1:
        *doorSwitch = (button_state_t) digitalRead( DOOR_1_SWITCH );
        *doorButton = getDoorButtonState( DOOR_TYPE_DOOR_1 );
        break;

    case DOOR_TYPE_DOOR_2:
        *doorSwitch = (button_state_t) digitalRead( DOOR_2_SWITCH );
        *doorButton = getDoorButtonState( DOOR_TYPE_DOOR_2 );
        break;
    }
}


static void setDoorState( const door_type_t door, const lock_state_t state )
{
    static lock_state_t lastState[DOOR_TYPE_SIZE] = {LOCK_STATE_CLOSED};

    if ( lastState[door] != state )
    {
        Log.notice( "%s: Door %d is %s" CR, __func__, door, ( state == LOCK_STATE_OPEN ) ? "open" : "closed" );
    }

    switch ( door )
    {
    case DOOR_TYPE_DOOR_1:
        digitalWrite( DOOR_1_MAGNET, (uint8_t) state );
        break;
    case DOOR_TYPE_DOOR_2:
        digitalWrite( DOOR_2_MAGNET, (uint8_t) state );
        break;

    default:
        break;
    }

    lastState[door] = state;
}


static button_state_t getDoorButtonState( const door_type_t door )
{
    uint8_t               buttonPin;
    static uint8_t        buttonState[DOOR_TYPE_SIZE]      = {LOW};
    static uint8_t        lastButtonState[DOOR_TYPE_SIZE]  = {LOW};
    static uint32_t       lastDebounceTime[DOOR_TYPE_SIZE] = {0};
    static button_state_t state[DOOR_TYPE_SIZE]            = {BUTTON_STATE_RELEASED};

    switch ( door )
    {
    case DOOR_TYPE_DOOR_1:
        buttonPin = DOOR_1_BUTTON;
        break;
    case DOOR_TYPE_DOOR_2:
        buttonPin = DOOR_2_BUTTON;
        break;
    }

    // read the state of the switch into a local variable:
    uint8_t reading = digitalRead( buttonPin );

    // check to see if you just pressed the button
    // (i.e. the input went from LOW to HIGH), and you've waited long enough
    // since the last press to ignore any noise:

    // If the switch changed, due to noise or pressing:
    if ( reading != lastButtonState[door] )
    {
        // reset the debouncing timer
        lastDebounceTime[door] = millis();
    }

    if ( ( millis() - lastDebounceTime[door] ) > DOOR_BUTTON_DEBOUNCE_DELAY )
    {
        // whatever the reading is at, it's been there for longer than the debounce
        // delay, so take it as the actual current state:

        // if the button state has changed:
        if ( reading != buttonState[door] )
        {
            buttonState[door] = reading;

            if ( buttonState[door] == HIGH )
            {
                state[door] = BUTTON_STATE_PRESSED;
                Log.notice( "%s: Button %d is pressed" CR, __func__, door );
            }
            else
            {
                state[door] = BUTTON_STATE_RELEASED;
                Log.notice( "%s: Button %d is released" CR, __func__, door );
            }
        }
    }

    // save the reading. Next time through the loop, it'll be the lastButtonState:
    lastButtonState[door] = reading;
    return state[door];
}


static void generateEvent( door_control_t* const pDoorControl )
{
    button_state_t door1Switch, door1Button, door2Switch, door2Button;

    /* Get the current state of the door switches and buttons */
    getDoorState( DOOR_TYPE_DOOR_1, &door1Switch, &door1Button );
    getDoorState( DOOR_TYPE_DOOR_2, &door2Switch, &door2Button );

    /* Generate the event */
    if (    ( door1Switch == BUTTON_STATE_RELEASED )
         && ( door2Switch == BUTTON_STATE_RELEASED ) )
    {
        pushEvent( &pDoorControl->machine.event, DOOR_CONTROL_EVENT_DOOR_1_2_OPEN );
    }

    if (    ( door1Switch == BUTTON_STATE_PRESSED )
         && ( door2Switch == BUTTON_STATE_PRESSED ) )
    {
        pushEvent( &pDoorControl->machine.event, DOOR_CONTROL_EVENT_DOOR_1_2_CLOSE );
    }

    if ( door1Switch == BUTTON_STATE_PRESSED )
    {
        pushEvent( &pDoorControl->machine.event, DOOR_CONTROL_EVENT_DOOR_1_CLOSE );
    }

    if ( door2Switch == BUTTON_STATE_PRESSED )
    {
        pushEvent( &pDoorControl->machine.event, DOOR_CONTROL_EVENT_DOOR_2_CLOSE );
    }

    if ( door1Switch == BUTTON_STATE_PRESSED )
    {
        pushEvent( &pDoorControl->machine.event, DOOR_CONTROL_EVENT_DOOR_1_OPEN );
    }

    if ( door2Switch == BUTTON_STATE_PRESSED )
    {
        pushEvent( &pDoorControl->machine.event, DOOR_CONTROL_EVENT_DOOR_2_OPEN );
    }
}


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
    case DOOR_CONTROL_STATE_DOOR_1_OPEN:
        return "DOOR_CONTROL_STATE_DOOR_1_OPEN";
    case DOOR_CONTROL_STATE_DOOR_2_OPEN:
        return "DOOR_CONTROL_STATE_DOOR_2_OPEN";
    default:
        return "UNKNOWN";
    }
}



String eventToString( door_control_event_t event )
{
    switch ( event )
    {
    case DOOR_CONTROL_EVENT_INIT_DONE:
        return "DOOR_CONTROL_EVENT_INIT_DONE";
    case DOOR_CONTROL_EVENT_DOOR_1_OPEN:
        return "DOOR_CONTROL_EVENT_DOOR_1_OPEN";
    case DOOR_CONTROL_EVENT_DOOR_1_CLOSE:
        return "DOOR_CONTROL_EVENT_DOOR_1_CLOSE";
    case DOOR_CONTROL_EVENT_DOOR_1_OPEN_TIMEOUT:
        return "DOOR_CONTROL_EVENT_DOOR_1_OPEN_TIMEOUT";
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


String resultToString( state_machine_result_t result )
{
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




  // Timer1.initialize(1000000); // The led will blink in a half second time
  //                            // interval
  // Timer1.attachInterrupt(blinkLed);


// int ledState = LOW;
// void blinkLed() {
//   ledState = !ledState;
//   digitalWrite(RBG_LED_1_R, ledState);
// }
