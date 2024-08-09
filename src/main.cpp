#include <Arduino.h>
#include <ArduinoLog.h>
#include <TimerOne.h>

#include "hsm.h"

/****************************************************************************************/
/*                                    PIN CONFIGURATION                                 */
/****************************************************************************************/
/* DOOR 1 */
/****************************************************************************************/
#define RBG_LED_1_R                7    /*!< Pin for the red LED of the RGB-LED */
#define RBG_LED_1_G                6    /*!< Pin for the green LED of the RGB-LED */
#define RBG_LED_1_B                5    /*!< Pin for the blue LED of the RGB-LED */
#define DOOR_1_BUTTON              2    /*!< Pin for the button of the door */
#define DOOR_1_SWITCH              3    /*!< Pin for the switch of the door */
#define DOOR_1_MAGNET              4    /*!< Pin for the magnet of the door */

/****************************************************************************************/
/* DOOR 2 */
/****************************************************************************************/
#define RBG_LED_2_R                13   /*!< Pin for the red LED of the RGB-LED */
#define RBG_LED_2_G                12   /*!< Pin for the green LED of the RGB-LED */
#define RBG_LED_2_B                11   /*!< Pin for the blue LED of the RGB-LED */
#define DOOR_2_BUTTON              10   /*!< Pin for the button of the door */
#define DOOR_2_SWITCH              9    /*!< Pin for the switch of the door */
#define DOOR_2_MAGNET              8    /*!< Pin for the magnet of the door */
/****************************************************************************************/
#define DOOR_BUTTON_DEBOUNCE_DELAY 100  /*!< Debounce delay for the door button @unit ms*/


/************************************* ENUMERATION **************************************/

/**
 * @brief Enumeration of the door control state
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
 * @brief Enumeration of the door control event
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

/**
 * @brief Enumeration of the door type
 */
typedef enum
{
    DOOR_TYPE_DOOR_1, /*!< The door 1 */
    DOOR_TYPE_DOOR_2, /*!< The door 2 */
    DOOR_TYPE_SIZE    /*!< Number of doors */
} door_type_t;

/**
 * @brief Enumeration of the lock state
 */
typedef enum
{
    LOCK_STATE_LOCKED   = LOW, /*!< The door is closed */
    LOCK_STATE_UNLOCKED = HIGH /*!< The door is open */
} lock_state_t;

/**
 * @brief Enumeration of the button state
 */
typedef enum
{
    BUTTON_STATE_RELEASED = 0, /*!< The button is released */
    BUTTON_STATE_PRESSED       /*!< The button is pressed */
} button_state_t;

/**
 * @brief Enumeration of the RGB LED pin
 */
typedef enum
{
    RGB_LED_PIN_R,   /*!< Red color */
    RGB_LED_PIN_G,   /*!< Green color */
    RGB_LED_PIN_B,   /*!< Blue color */
    RGB_LED_PIN_SIZE /*!< Number of colors */
} rgb_led_pin_t;

/**
 * @brief Enumeration of the led color
 */
typedef enum
{
    LED_COLOR_RED,     /*!< Red color */
    LED_COLOR_GREEN,   /*!< Green color */
    LED_COLOR_BLUE,    /*!< Blue color */
    LED_COLOR_YELLOW,  /*!< Yellow color */
    LED_COLOR_MAGENTA, /*!< Magenta color */
    LED_COLOR_CYAN,    /*!< Cyan color */
    LED_COLOR_WHITE,   /*!< White color */
    LED_COLOR_SIZE     /*!< Number of colors */
} led_color_t;

/************************************* STRUCTURE **************************************/

/**
 * @brief The door control state machine
 */
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
static state_machine_result_t idleEntryHandler( state_machine_t* const pState, const uint32_t event );
static state_machine_result_t idleExitHandler( state_machine_t* const pState, const uint32_t event );

static state_machine_result_t faultHandler( state_machine_t* const pState, const uint32_t event );
static state_machine_result_t faultEntryHandler( state_machine_t* const pState, const uint32_t event );
static state_machine_result_t faultExitHandler( state_machine_t* const pState, const uint32_t event );
static void                   faultBlinkLedIsrHandler( void );

static state_machine_result_t door1OpenHandler( state_machine_t* const pState, const uint32_t event );
static state_machine_result_t door1OpenEntryHandler( state_machine_t* const pState, const uint32_t event );
static state_machine_result_t door1OpenExitHandler( state_machine_t* const pState, const uint32_t event );
static void                   door1BlinkLedIsrHandler( void );

static state_machine_result_t door2OpenHandler( state_machine_t* const pState, const uint32_t event );
static state_machine_result_t door2OpenEntryHandler( state_machine_t* const pState, const uint32_t event );
static state_machine_result_t door2OpenExitHandler( state_machine_t* const pState, const uint32_t event );
static void                   door2BlinkLedIsrHandler( void );

static void           init( door_control_t* const pDoorControl, uint32_t processTime );
void                  eventLogger( uint32_t stateMachine, uint32_t state, uint32_t event );
void                  resultLogger( uint32_t state, state_machine_result_t result );
static void           setDoorState( const door_type_t door, const lock_state_t state );
static button_state_t getDoorButtonState( const door_type_t door );
static void           generateEvent( door_control_t* const pDoorControl );
String                stateToString( door_control_state_t state );
String                eventToString( door_control_event_t event );
String                resultToString( state_machine_result_t result );
void                  setLed( bool enable, door_type_t door, led_color_t color );

/******************************** Global variables ************************************/

/**
 * @brief The state machine for the door control
 */
static const state_t doorControlStates[] = {

    [DOOR_CONTROL_STATE_INIT] = {
        .Handler = initHandler,
        .Entry   = NULL,
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

    [DOOR_CONTROL_STATE_DOOR_1_OPEN] = {
        .Handler = door1OpenHandler,
        .Entry   = door1OpenEntryHandler,
        .Exit    = door1OpenExitHandler,
        .Id      = DOOR_CONTROL_STATE_DOOR_1_OPEN
    },

    [DOOR_CONTROL_STATE_DOOR_2_OPEN] = {
        .Handler = door2OpenHandler,
        .Entry   = door2OpenEntryHandler,
        .Exit    = door2OpenExitHandler,
        .Id      = DOOR_CONTROL_STATE_DOOR_2_OPEN
    }
};

door_control_t         doorControl;                                         /*!< Instance of door_control_t */
state_machine_t* const stateMachines[] = {(state_machine_t*) &doorControl}; /*!< Create and initialize the array of state machines. */

uint8_t ledPins[DOOR_TYPE_SIZE][RGB_LED_PIN_SIZE] = {                       /*!< RGB LED pins */
    {RBG_LED_1_R, RBG_LED_1_G, RBG_LED_1_B},
    {RBG_LED_2_R, RBG_LED_2_G, RBG_LED_2_B}
};


/**
 * @brief Setup the application
 */
void setup()
{
    /* Initialize with log level and log output */
    Serial.begin( 115200 );
    Log.begin( LOG_LEVEL_INFO, &Serial );

    /* Initialize the doorControl */
    init( &doorControl, 0 );

    /* Initialize the led blink timer with the interval of 500ms */
    Timer1.initialize( 1000000 );
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

/**
 * @brief Convert the state to string
 * 
 * @param state - The state to convert
 * @return String - The string representation of the state
 */
static void init( door_control_t* const pDoorControl, uint32_t processTime )
{
    pDoorControl->machine.State = &doorControlStates[DOOR_CONTROL_STATE_INIT];
    pDoorControl->machine.event = NULL;

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

/**
 * @brief Handler for the init state
 * 
 * @param pState - The state machine
 * @param event - The event
 * @return state_machine_result_t - The result of the handler
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


/**
 * @brief Handler for the idle state entry
 * 
 * @param pState - The state machine
 * @param event - The event
 * @return state_machine_result_t - The result of the handler
 */
static state_machine_result_t idleEntryHandler( state_machine_t* const pState, const uint32_t event )
{
    Log.verbose( "%s: Event %s" CR, __func__, eventToString( (door_control_event_t) event ).c_str() );

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
    Log.verbose( "%s: Event %s" CR, __func__, eventToString( (door_control_event_t) event ).c_str() );

    /* Process door buttons */
    button_state_t buttonState[DOOR_TYPE_SIZE] = {BUTTON_STATE_RELEASED};

    for ( uint8_t i = 0; i < DOOR_TYPE_SIZE; i++ )
    {
        buttonState[i] = getDoorButtonState( (door_type_t) i );
    }

    /* XOR-logic to allow only one door to be open */
    if (    ( buttonState[DOOR_TYPE_DOOR_1] == BUTTON_STATE_PRESSED )
         && ( buttonState[DOOR_TYPE_DOOR_2] == BUTTON_STATE_RELEASED ) )
    {
        setDoorState( DOOR_TYPE_DOOR_1, LOCK_STATE_UNLOCKED );
    }
    else if (    ( buttonState[DOOR_TYPE_DOOR_1] == BUTTON_STATE_RELEASED )
              && ( buttonState[DOOR_TYPE_DOOR_2] == BUTTON_STATE_PRESSED ) )
    {
        setDoorState( DOOR_TYPE_DOOR_2, LOCK_STATE_UNLOCKED );
    }
    else
    {
        setDoorState( DOOR_TYPE_DOOR_1, LOCK_STATE_LOCKED );
        setDoorState( DOOR_TYPE_DOOR_2, LOCK_STATE_LOCKED );
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


/**
 * @brief Handler for the idle state exit
 * 
 * @param pState - The state machine
 * @param event - The event
 * @return state_machine_result_t - The result of the handler
 */
static state_machine_result_t idleExitHandler( state_machine_t* const pState, const uint32_t event )
{
    Log.verbose("%s: Event %s" CR, __func__, eventToString( (door_control_event_t) event ).c_str() );

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
    Log.verbose("%s: Event %s" CR, __func__, eventToString( (door_control_event_t) event ).c_str() );

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


/**
 * @brief Handler for the fault state exit
 * 
 * @param pState - The state machine
 * @param event - The event
 * @return state_machine_result_t - The result of the handler
 */
static state_machine_result_t faultExitHandler( state_machine_t* const pState, const uint32_t event )
{
    Log.verbose("%s: Event %s" CR, __func__, eventToString( (door_control_event_t) event ).c_str() );

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
 * @brief Handler for the door 1 open entry
 * 
 * @param pState - The state machine
 * @param event - The event
 * @return state_machine_result_t - The result of the handler
 */
static state_machine_result_t door1OpenEntryHandler( state_machine_t* const pState, const uint32_t event )
{
    Log.verbose("%s: Event %s" CR, __func__, eventToString( (door_control_event_t) event ).c_str() );

    /* Unlock the door */
    setDoorState( DOOR_TYPE_DOOR_1, LOCK_STATE_UNLOCKED );
    Timer1.attachInterrupt( door1BlinkLedIsrHandler );
    Timer1.start();

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
    Log.verbose("%s: Event %s" CR, __func__, eventToString( (door_control_event_t) event ).c_str() );

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


/**
 * @brief Handler for the door 1 open exit
 * 
 * @param pState - The state machine
 * @param event - The event
 * @return state_machine_result_t - The result of the handler
 */
static state_machine_result_t door1OpenExitHandler( state_machine_t* const pState, const uint32_t event )
{
    Log.verbose("%s: Event %s" CR, __func__, eventToString( (door_control_event_t) event ).c_str() );

    /* Lock the door */
    setDoorState( DOOR_TYPE_DOOR_1, LOCK_STATE_LOCKED );

    Timer1.stop();
    Timer1.detachInterrupt();
    setLed( false, DOOR_TYPE_DOOR_1, LED_COLOR_SIZE );
    setLed( false, DOOR_TYPE_DOOR_2, LED_COLOR_SIZE );

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

static state_machine_result_t door2OpenEntryHandler( state_machine_t* const pState, const uint32_t event )
{
    Log.verbose("%s: Event %s" CR, __func__, eventToString( (door_control_event_t) event ).c_str() );

    /* Unlock the door */
    setDoorState( DOOR_TYPE_DOOR_2, LOCK_STATE_UNLOCKED );

    Timer1.attachInterrupt( door2BlinkLedIsrHandler );
    Timer1.start();

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
    Log.verbose("%s: Event %s" CR, __func__, eventToString( (door_control_event_t) event ).c_str() );

    switch ( event )
    {
    case DOOR_CONTROL_EVENT_DOOR_2_CLOSE:
        switch_state( pState, &doorControlStates[DOOR_CONTROL_STATE_IDLE] );
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
    Log.verbose("%s: Event %s" CR, __func__, eventToString( (door_control_event_t) event ).c_str() );

    /* Lock the door */
    setDoorState( DOOR_TYPE_DOOR_2, LOCK_STATE_LOCKED );

    Timer1.stop();
    Timer1.detachInterrupt();
    setLed( false, DOOR_TYPE_DOOR_1, LED_COLOR_SIZE );
    setLed( false, DOOR_TYPE_DOOR_2, LED_COLOR_SIZE );

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

    /* Only log if the event is changed */
    if (    ( lastEvent != event )
         && ( lastState != state ) )
    {
        Log.notice( "%s: Event: %s, State: %s" CR, __func__,
                    eventToString( (door_control_event_t) event ).c_str(),
                    stateToString( (door_control_state_t) state ).c_str() );
    }

    lastEvent = event;
    lastState = state;
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


/**
 * @brief Set the state of the door
 *
 * @param door - The door type
 * @param state - The state of the door
 */
static void setDoorState( const door_type_t door, const lock_state_t state )
{
    static lock_state_t lastState[DOOR_TYPE_SIZE] = {LOCK_STATE_LOCKED};

    if ( lastState[door] != state )
    {
        Log.notice( "%s: Door %d is %s" CR, __func__, door, ( state == LOCK_STATE_UNLOCKED ) ? "open" : "closed" );
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


/**
 * @brief Get the state of the door button
 *
 * @param door - The door type
 * @return button_state_t - The state of the button
 */
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


/**
 * @brief Generate the event based on the door switches and buttons
 *
 * @param pDoorControl - Pointer to the door control
 */
static void generateEvent( door_control_t* const pDoorControl )
{
    /* Read the state of both door switches */
    button_state_t door1Switch = (button_state_t) digitalRead( DOOR_1_SWITCH );
    button_state_t door2Switch = (button_state_t) digitalRead( DOOR_2_SWITCH );

    /* Get pointer to the current event */
    event_t** pCurrentEvent = &pDoorControl->machine.event;

    /* Generate the event */
    if (    ( door1Switch == BUTTON_STATE_RELEASED )
         && ( door2Switch == BUTTON_STATE_RELEASED ) )
    {
        pushEvent( pCurrentEvent, DOOR_CONTROL_EVENT_DOOR_1_2_OPEN );
    }

    if (    ( door1Switch == BUTTON_STATE_PRESSED )
         && ( door2Switch == BUTTON_STATE_PRESSED ) )
    {
        pushEvent( pCurrentEvent, DOOR_CONTROL_EVENT_DOOR_1_2_CLOSE );
    }

    if ( door1Switch == BUTTON_STATE_PRESSED )
    {
        pushEvent( pCurrentEvent, DOOR_CONTROL_EVENT_DOOR_1_CLOSE );
    }

    if ( door1Switch == BUTTON_STATE_RELEASED )
    {
        pushEvent( pCurrentEvent, DOOR_CONTROL_EVENT_DOOR_1_OPEN );
    }

    if ( door2Switch == BUTTON_STATE_PRESSED )
    {
        pushEvent( pCurrentEvent, DOOR_CONTROL_EVENT_DOOR_2_CLOSE );
    }

    if ( door2Switch == BUTTON_STATE_RELEASED )
    {
        pushEvent( &pDoorControl->machine.event, DOOR_CONTROL_EVENT_DOOR_2_OPEN );
    }
}


/**
 * Sets the LED color for a specific door.
 *
 * @param enable - Whether to enable or disable the LED.
 * @param door - The type of door.
 * @param color - The color of the LED.
 */
void setLed( bool enable, door_type_t door, led_color_t color )
{

    if ( enable )
    {
        switch ( color )
        {
            case LED_COLOR_RED:
                digitalWrite( ledPins[door][RGB_LED_PIN_R], HIGH );
                digitalWrite( ledPins[door][RGB_LED_PIN_G], LOW );
                digitalWrite( ledPins[door][RGB_LED_PIN_B], LOW );
                break;
            case LED_COLOR_GREEN:
                digitalWrite( ledPins[door][RGB_LED_PIN_R], LOW );
                digitalWrite( ledPins[door][RGB_LED_PIN_G], HIGH );
                digitalWrite( ledPins[door][RGB_LED_PIN_B], LOW );
                break;
            case LED_COLOR_BLUE:
                digitalWrite( ledPins[door][RGB_LED_PIN_R], LOW );
                digitalWrite( ledPins[door][RGB_LED_PIN_G], LOW );
                digitalWrite( ledPins[door][RGB_LED_PIN_B], HIGH );
                break;
            case LED_COLOR_YELLOW:
                digitalWrite( ledPins[door][RGB_LED_PIN_R], HIGH );
                digitalWrite( ledPins[door][RGB_LED_PIN_G], HIGH );
                digitalWrite( ledPins[door][RGB_LED_PIN_B], LOW );
                break;
            case LED_COLOR_MAGENTA:
                digitalWrite( ledPins[door][RGB_LED_PIN_R], HIGH );
                digitalWrite( ledPins[door][RGB_LED_PIN_G], LOW );
                digitalWrite( ledPins[door][RGB_LED_PIN_B], HIGH );
                break;
            case LED_COLOR_CYAN:
                digitalWrite( ledPins[door][RGB_LED_PIN_R], LOW );
                digitalWrite( ledPins[door][RGB_LED_PIN_G], HIGH );
                digitalWrite( ledPins[door][RGB_LED_PIN_B], HIGH );
                break;
            case LED_COLOR_WHITE:
                digitalWrite( ledPins[door][RGB_LED_PIN_R], HIGH );
                digitalWrite( ledPins[door][RGB_LED_PIN_G], HIGH );
                digitalWrite( ledPins[door][RGB_LED_PIN_B], HIGH );
                break;
            default:
                break;
        }
    }
    else
    {
        digitalWrite( ledPins[door][RGB_LED_PIN_R], LOW );
        digitalWrite( ledPins[door][RGB_LED_PIN_G], LOW );
        digitalWrite( ledPins[door][RGB_LED_PIN_B], LOW );
    }
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
    case DOOR_CONTROL_STATE_DOOR_1_OPEN:
        return "DOOR_CONTROL_STATE_DOOR_1_OPEN";
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


/**
 * @brief Convert the result to string
 * 
 * @param result - The result to convert
 * @return String - The string representation of the result
 */
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
