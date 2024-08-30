#include <Arduino.h>
#include <ArduinoLog.h>
#include <TimerOne.h>

#include "hsm.h"

/****************************************************************************************/
/*                                    PIN CONFIGURATION                                 */
/****************************************************************************************/
/* DOOR 1 */
/****************************************************************************************/
#define RBG_LED_1_R                     7       /*!< Pin for the red LED of the RGB-LED */
#define RBG_LED_1_G                     6       /*!< Pin for the green LED of the RGB-LED */
#define RBG_LED_1_B                     5       /*!< Pin for the blue LED of the RGB-LED */
#define DOOR_1_BUTTON                   2       /*!< Pin for the button of the door */
#define DOOR_1_SWITCH                   3       /*!< Pin for the switch of the door */
#define DOOR_1_MAGNET                   4       /*!< Pin for the magnet of the door */

/****************************************************************************************/
/* DOOR 2 */
/****************************************************************************************/
#define RBG_LED_2_R                     13      /*!< Pin for the red LED of the RGB-LED */
#define RBG_LED_2_G                     12      /*!< Pin for the green LED of the RGB-LED */
#define RBG_LED_2_B                     11      /*!< Pin for the blue LED of the RGB-LED */
#define DOOR_2_BUTTON                   10      /*!< Pin for the button of the door */
#define DOOR_2_SWITCH                   9       /*!< Pin for the switch of the door */
#define DOOR_2_MAGNET                   8       /*!< Pin for the magnet of the door */
/****************************************************************************************/
/*                                GENERAL CONFIGURATION                                 */
/****************************************************************************************/
#define DEBOUNCE_DELAY_DOOR_BUTTON_1    100     /*!< Debounce delay for the door button @unit ms*/
#define DEBOUNCE_DELAY_DOOR_BUTTON_2    100     /*!< Debounce delay for the door button @unit ms*/
#define DEBOUNCE_DELAY_DOOR_SWITCH_1    100     /*!< Debounce delay for the door switch @unit ms*/
#define DEBOUNCE_DELAY_DOOR_SWITCH_2    100     /*!< Debounce delay for the door switch @unit ms*/

#define SERIAL_BAUD_RATE                115200  /*!< Baud rate of the serial communication @unit bps */
#define LED_BLINK_INTERVAL              500     /*!< Interval of the led blink @unit ms */
#define DOOR_OPEN_TIMEOUT               600      /*!< Timeout for the door open ( 0 = disabled ) @unit s */

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
 * @details The lock state defines whether the door is magnetically locked or not
 */
typedef enum
{
    LOCK_STATE_UNLOCKED = LOW,  /*!< The door is open */
    LOCK_STATE_LOCKED   = HIGH  /*!< The door is closed */
} lock_state_t;

/**
 * @brief Enumeration of the door sensor
 * @details The door sensor is a combination of a button and a switch
 */
typedef enum
{
    SENSOR_BUTTON_1 = 0, /*!< The button of the door 1 */
    SENSOR_BUTTON_2,     /*!< The button of the door 2 */
    SENSOR_SWITCH_1,     /*!< The switch of the door 1 */
    SENSOR_SWITCH_2,     /*!< The switch of the door 2 */
    SENSOR_SIZE          /*!< Number of door sensors */
} sensor_t;

/**
 * @brief Enumeration of the button state
 * @details The button state defines whether the button is pressed or released
 */
typedef enum
{
    SENSOR_STATE_RELEASED = 0, /*!< The button is released */
    SENSOR_STATE_PRESSED       /*!< The button is pressed */
} sensor_state_t;

/**
 * @brief Enumeration of the sensor debounce state
 * @details The sensor debounce state defines whether the sensor is debounced or not
 */
typedef enum
{
    SENSOR_DEBOUNCE_UNSTABLE = 0, /*!< The sensor is under debouncing */
    SENSOR_DEBOUNCE_STABLE        /*!< The sensor is debounced */
} sensor_debounce_t;

/**
 * @brief Enumeration of the RGB LED pin
 */
typedef enum
{
    RGB_LED_PIN_R,   /*!< Red color pin */
    RGB_LED_PIN_G,   /*!< Green color pin */
    RGB_LED_PIN_B,   /*!< Blue color pin */
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
    state_machine_t machine;      //!< Abstract state machine
    door_timer_t    door1Timer;   //!< Timer for the door 1
    door_timer_t    door2Timer;   //!< Timer for the door 2
} door_control_t;

/******************************** Function prototype ************************************/

static state_machine_result_t initHandler( state_machine_t* const pState, const uint32_t event );
static state_machine_result_t initEntryHandler( state_machine_t* const pState, const uint32_t event );
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
static void                   door1OpenTimeoutHandler( uint32_t time );

static state_machine_result_t door2OpenHandler( state_machine_t* const pState, const uint32_t event );
static state_machine_result_t door2OpenEntryHandler( state_machine_t* const pState, const uint32_t event );
static state_machine_result_t door2OpenExitHandler( state_machine_t* const pState, const uint32_t event );
static void                   door2BlinkLedIsrHandler( void );
static void                   door2OpenTimeoutHandler( uint32_t time );

static void           init( door_control_t* const pDoorControl, uint32_t processTime );
void                  eventLogger( uint32_t stateMachine, uint32_t state, uint32_t event );
void                  resultLogger( uint32_t state, state_machine_result_t result );
static void           setDoorState( const door_type_t door, const lock_state_t state );
static sensor_state_t getDoorSensorState( const sensor_t sensor, sensor_debounce_t* const debounceState );
static void           generateEvent( door_control_t* const pDoorControl );
static String         stateToString( door_control_state_t state );
static String         eventToString( door_control_event_t event );
static String         resultToString( state_machine_result_t result );
static String         sensorToString( sensor_t sensor );
static void           setLed( bool enable, door_type_t door, led_color_t color );
static void           processTimers( door_control_t* const pDoorControl );

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

/**
 * @brief The RGB LED pins
 * @details The RGB LED pins are defined as an array of pins for each door
 */
uint8_t ledPins[DOOR_TYPE_SIZE][RGB_LED_PIN_SIZE] = {                       /*!< RGB LED pins */
    {RBG_LED_1_R, RBG_LED_1_G, RBG_LED_1_B},
    {RBG_LED_2_R, RBG_LED_2_G, RBG_LED_2_B}
};

/**
 * @brief The debounce time for the door sensor
 * @details The debounce time is used to filter out the noise of each sensor
 */
uint16_t sensorDebounceTime[SENSOR_SIZE] = {
    DEBOUNCE_DELAY_DOOR_BUTTON_1,     /*!< Button 1 debounce time @umit ms */
    DEBOUNCE_DELAY_DOOR_BUTTON_2,     /*!< Button 2 debounce time @umit ms */
    DEBOUNCE_DELAY_DOOR_SWITCH_1,     /*!< Switch 1 debounce time @umit ms */
    DEBOUNCE_DELAY_DOOR_SWITCH_2      /*!< Switch 2 debounce time @umit ms */
};


/******************************** Function definition ************************************/

/**
 * @brief Setup the application
 */
void setup()
{
    /* Initialize with log level and log output */
    Serial.begin( SERIAL_BAUD_RATE );
    Log.begin( LOG_LEVEL_INFO, &Serial );

    /* Initialize the magnetic lock pins */
    pinMode( DOOR_1_MAGNET, OUTPUT );
    pinMode( DOOR_2_MAGNET, OUTPUT );

    /* Initialize the doorControl */
    init( &doorControl, 0 );

    /* Initialize the led blink timer */
    Timer1.initialize( ( (uint32_t) 2000 ) * ( (uint32_t) LED_BLINK_INTERVAL ) );

    Log.notice( "Application started" CR );
}


/**
 * @brief The main loop of the application
 */
void loop()
{
    /* Generate/Process events */
    generateEvent( &doorControl );

    /* Process door open timers */
    processTimers( &doorControl );

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
    /* Initialize the door open timers */
    pDoorControl->door1Timer.handler       = door1OpenTimeoutHandler;
    pDoorControl->door1Timer.timeout       = ( (uint32_t) DOOR_OPEN_TIMEOUT ) * ( (uint32_t) 1000 );
    pDoorControl->door1Timer.timeReference = 0;
    pDoorControl->door2Timer.handler       = door2OpenTimeoutHandler;
    pDoorControl->door2Timer.timeout       = ( (uint32_t) DOOR_OPEN_TIMEOUT ) * ( (uint32_t) 1000 );
    pDoorControl->door2Timer.timeReference = 0;

    /* Initialize the state machine */
    pDoorControl->machine.event = NULL;
    switch_state( &pDoorControl->machine, &doorControlStates[DOOR_CONTROL_STATE_INIT] );
}


static state_machine_result_t initEntryHandler( state_machine_t* const pState, const uint32_t event )
{
    Log.verbose("%s: Event %s" CR, __func__, eventToString( (door_control_event_t) event ).c_str() );

    /* Check whether door 1 and 2 are open */
    sensor_debounce_t door1SwitchDebounceState, door2SwitchDebounceState;
    sensor_state_t door1Switch = getDoorSensorState( SENSOR_SWITCH_1, &door1SwitchDebounceState );
    sensor_state_t door2Switch = getDoorSensorState( SENSOR_SWITCH_2, &door1SwitchDebounceState );

    /* Check whether the door switches are debouncing. This "waiting" state is only used
     * for the initialization as the door switches are checked here in an one-shot manner.
     */
    if (    ( door1SwitchDebounceState == SENSOR_DEBOUNCE_UNSTABLE )
         || ( door2SwitchDebounceState == SENSOR_DEBOUNCE_UNSTABLE ) )
    {
        return TRIGGERED_TO_SELF;
    }

     /* Get pointer to the current event */
    event_t** pCurrentEvent = &pState->event;

    if (    ( door1Switch == SENSOR_STATE_PRESSED )
         && ( door2Switch == SENSOR_STATE_PRESSED) )
    {
        pushEvent( pCurrentEvent, DOOR_CONTROL_EVENT_DOOR_1_2_CLOSE );
    }

    if ( door1Switch == SENSOR_STATE_RELEASED )
    {
        pushEvent( pCurrentEvent, DOOR_CONTROL_EVENT_DOOR_1_OPEN );
    }

    if ( door2Switch == SENSOR_STATE_RELEASED )
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
    Log.verbose("%s: Event %s" CR, __func__, eventToString( (door_control_event_t) event ).c_str() );

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
    Log.verbose("%s: Event %s" CR, __func__, eventToString( (door_control_event_t) event ).c_str() );
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

    /* Get the state of the door buttons. The debounce state isn't used here,
     * but it is necessary to call the function.
     */
    sensor_debounce_t door1ButtonDebounceState, door2ButtonDebounceState;
    sensor_state_t door1Button = getDoorSensorState( SENSOR_BUTTON_1, &door1ButtonDebounceState );
    sensor_state_t door2Button = getDoorSensorState( SENSOR_BUTTON_2, &door2ButtonDebounceState );

    /* XOR-logic to allow only one door to be open */
    if (    ( door1Button == SENSOR_STATE_PRESSED )
         && ( door2Button == SENSOR_STATE_RELEASED ) )
    {
        setDoorState( DOOR_TYPE_DOOR_1, LOCK_STATE_UNLOCKED );
    }
    else if (    ( door1Button == SENSOR_STATE_RELEASED )
              && ( door2Button == SENSOR_STATE_PRESSED ) )
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

    /* Start the door open timer */
    doorControl.door1Timer.timeReference = millis();

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
    Log.verbose("%s: Event %s" CR, __func__, eventToString( (door_control_event_t) event ).c_str() );

    /* Lock the door */
    setDoorState( DOOR_TYPE_DOOR_1, LOCK_STATE_LOCKED );

    /* Stop the led blink */
    Timer1.stop();
    Timer1.detachInterrupt();
    setLed( false, DOOR_TYPE_DOOR_1, LED_COLOR_SIZE );
    setLed( false, DOOR_TYPE_DOOR_2, LED_COLOR_SIZE );

    /* Reset the door open timer */
    doorControl.door1Timer.timeReference = 0;

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
 * @brief Handler for the door 1 open timeout
 * 
 * @param time - The time
 */
static void door1OpenTimeoutHandler( uint32_t time )
{
    Log.verbose("%s: Time: %d" CR, __func__, time );

    /* Switch to the fault state if the door is not closed in time */
    switch_state( &doorControl.machine, &doorControlStates[DOOR_CONTROL_STATE_FAULT] );
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
    Log.verbose("%s: Event %s" CR, __func__, eventToString( (door_control_event_t) event ).c_str() );

    /* Unlock the door */
    setDoorState( DOOR_TYPE_DOOR_2, LOCK_STATE_UNLOCKED );

    Timer1.attachInterrupt( door2BlinkLedIsrHandler );
    Timer1.start();

    /* Start the door open timer */
    doorControl.door2Timer.timeReference = millis();

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
    Log.verbose("%s: Event %s" CR, __func__, eventToString( (door_control_event_t) event ).c_str() );

    /* Lock the door */
    setDoorState( DOOR_TYPE_DOOR_2, LOCK_STATE_LOCKED );

    /* Stop the led blink */
    Timer1.stop();
    Timer1.detachInterrupt();
    setLed( false, DOOR_TYPE_DOOR_1, LED_COLOR_SIZE );
    setLed( false, DOOR_TYPE_DOOR_2, LED_COLOR_SIZE );

    /* Reset the door open timer */
    doorControl.door2Timer.timeReference = 0;

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
 * @brief Handler for the door 2 open timeout
 * 
 * @param time - The time
 */
static void door2OpenTimeoutHandler( uint32_t time )
{
    Log.verbose("%s: Time: %d" CR, __func__, time );

    /* Switch to the fault state if the door is not closed in time */
    switch_state( &doorControl.machine, &doorControlStates[DOOR_CONTROL_STATE_FAULT] );
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

    /* Only log if the event and state are changed */
    if (    ( lastEvent != event )
         && ( lastState != state ) )
    {
        Log.notice( "%s: Event: %s, State: %s" CR, __func__,
                    eventToString( (door_control_event_t) event ).c_str(),
                    stateToString( (door_control_state_t) state ).c_str() );
    }

    /* Save the last event and state */
    lastEvent = event;
    lastState = state;
}


/**
 * @brief Convert the state to string
 * 
 * @param state - The state to convert
 * @return String - The string representation of the state
 */
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

    /* Save the last state */
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
 * @brief Get the Door Sensor State object
 * 
 * @param sensor 
 * @param debounceState 
 * @return sensor_state_t 
 */
static sensor_state_t getDoorSensorState( const sensor_t sensor, sensor_debounce_t* const debounceState )
{

    /* Check if the sensor is valid */
    if ( sensor >= SENSOR_SIZE )
    {
        Log.error( "%s: Invalid sensor: %d" CR, __func__, sensor );
        return SENSOR_STATE_RELEASED;
    }

    uint8_t                  sensorPinMap[SENSOR_SIZE]     = {DOOR_1_BUTTON, DOOR_2_BUTTON, DOOR_1_SWITCH, DOOR_2_SWITCH};
    static uint8_t           sensorState[SENSOR_SIZE]      = {0};
    static uint8_t           lastSensorState[SENSOR_SIZE]  = {0};
    static uint32_t          lastDebounceTime[SENSOR_SIZE] = {0};
    static sensor_state_t    state[SENSOR_SIZE]            = {SENSOR_STATE_RELEASED};
    static sensor_debounce_t debounce[SENSOR_SIZE]         = {SENSOR_DEBOUNCE_UNSTABLE};

    /* Read the state of the switch into a local variable */
    uint8_t reading = digitalRead( sensorPinMap[sensor] );

    /* check to see if you just pressed the sensor
     * (i.e. the input went from LOW to HIGH), and you've waited long enough
     * since the last press to ignore any noise: */

    /* If the switch changed, due to noise or pressing: */
    if ( reading != lastSensorState[sensor] )
    {
        /* reset the debouncing timer */
        lastDebounceTime[sensor] = millis();
        debounce[sensor]    = SENSOR_DEBOUNCE_UNSTABLE;
    }
    else
    {
        debounce[sensor] = SENSOR_DEBOUNCE_STABLE;
    }

    if ( ( millis() - lastDebounceTime[sensor] ) > sensorDebounceTime[sensor] )
    {
        /* whatever the reading is at, it's been there for longer than the debounce
         * delay, so take it as the actual current state: */

        debounce[sensor] = SENSOR_DEBOUNCE_STABLE;

        /* if the sensor state has changed: */
        if ( reading != sensorState[sensor] )
        {
            sensorState[sensor] = reading;

            if ( sensorState[sensor] == HIGH )
            {
                state[sensor] = SENSOR_STATE_PRESSED;
                Log.notice( "%s: %s is pressed" CR, __func__, sensorToString( sensor ).c_str() );
            }
            else
            {
                state[sensor] = SENSOR_STATE_RELEASED;
                Log.notice( "%s: %s is released" CR, __func__, sensorToString( sensor ).c_str() );
            }
        }
    }

    /* save the reading. Next time through the loop, it'll be the lastSensorState: */
    lastSensorState[sensor] = reading;

    /* Setup result values */
    *debounceState = debounce[sensor];
    return state[sensor];
}

/**
 * @brief Generate the event based on the door switches and buttons
 *
 * @param pDoorControl - Pointer to the door control
 */
static void generateEvent( door_control_t* const pDoorControl )
{
    /* Read the state of both door switches */
    sensor_debounce_t door1SwitchDebounceState, door2SwitchDebounceState;


    sensor_state_t door1Switch = getDoorSensorState( SENSOR_SWITCH_1, &door1SwitchDebounceState );
    sensor_state_t door2Switch = getDoorSensorState( SENSOR_SWITCH_2, &door2SwitchDebounceState );

    /* Get pointer to the current event */
    event_t** pCurrentEvent = &pDoorControl->machine.event;

    /* Generate the event */
    if (    ( door1Switch == SENSOR_STATE_RELEASED )
         && ( door2Switch == SENSOR_STATE_RELEASED ) )
    {
        pushEvent( pCurrentEvent, DOOR_CONTROL_EVENT_DOOR_1_2_OPEN );
    }

    if (    ( door1Switch == SENSOR_STATE_PRESSED )
         && ( door2Switch == SENSOR_STATE_PRESSED ) )
    {
        pushEvent( pCurrentEvent, DOOR_CONTROL_EVENT_DOOR_1_2_CLOSE );
    }

    if ( door1Switch == SENSOR_STATE_PRESSED )
    {
        pushEvent( pCurrentEvent, DOOR_CONTROL_EVENT_DOOR_1_CLOSE );
    }

    if ( door1Switch == SENSOR_STATE_RELEASED )
    {
        pushEvent( pCurrentEvent, DOOR_CONTROL_EVENT_DOOR_1_OPEN );
    }

    if ( door2Switch == SENSOR_STATE_PRESSED )
    {
        pushEvent( pCurrentEvent, DOOR_CONTROL_EVENT_DOOR_2_CLOSE );
    }

    if ( door2Switch == SENSOR_STATE_RELEASED )
    {
        pushEvent( &pDoorControl->machine.event, DOOR_CONTROL_EVENT_DOOR_2_OPEN );
    }
}


/**
 * @brief Set the LED state and color
 *
 * @param enable - Whether to enable or disable the LED.
 * @param door - The type of door.
 * @param color - The color of the LED.
 */
void setLed( bool enable, door_type_t door, led_color_t color )
{
    /* Check if the door is valid */
    if ( door >= DOOR_TYPE_SIZE )
    {
        Log.error( "%s: Invalid door type: %d" CR, __func__, door );
        return;
    }

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
 * @brief Process the door open timers
 * 
 * @param pDoorControl - Pointer to the door control
 */
static void processTimers( door_control_t* const pDoorControl )
{
    Log.verbose( "%s" CR, __func__ );

    /* Get the current time */
    uint64_t currentTime = millis();

    /* Process the door 1 timer ( if timeout is set ) */
    if (    ( pDoorControl->door1Timer.timeout       != 0 )
         && ( pDoorControl->door1Timer.timeReference != 0 ) )
    {
        if ( ( currentTime - pDoorControl->door1Timer.timeReference ) >= pDoorControl->door1Timer.timeout )
        {
            pDoorControl->door1Timer.handler( currentTime );
        }

        /* Calculate remaining time */
        String remainingTime = String( ( pDoorControl->door1Timer.timeout - ( currentTime - pDoorControl->door1Timer.timeReference ) ) / 1000.0F ).c_str();
        Log.notice( "Door 1 open timer: %s s" CR, remainingTime.c_str() );
    }

    /* Process the door 2 timer ( if timeout is set ) */
    if (    ( pDoorControl->door2Timer.timeout       != 0 )
         && ( pDoorControl->door2Timer.timeReference != 0 ) )
    {
        if ( ( currentTime - pDoorControl->door2Timer.timeReference ) >= pDoorControl->door2Timer.timeout )
        {
            pDoorControl->door2Timer.handler( currentTime );
        }

        /* Calculate remaining time */
        String remainingTime = String( ( pDoorControl->door2Timer.timeout - ( currentTime - pDoorControl->door2Timer.timeReference ) ) / 1000.0F ).c_str();
        Log.notice( "Door 2 open timer: %s s" CR, remainingTime.c_str() );
    }
}


/**
 * @brief Convert the state to string
 * 
 * @param state - The state to convert
 * @return String - The string representation of the state
 */
static String stateToString( door_control_state_t state )
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
static String eventToString( door_control_event_t event )
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
static String resultToString( state_machine_result_t result )
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


/**
 * @brief Convert the sensor to string
 * 
 * @param sensor - The sensor to convert
 * @return String - The string representation of the sensor
 */
static String sensorToString( sensor_t sensor )
{
    switch ( sensor )
    {
    case SENSOR_BUTTON_1:
        return "SENSOR_BUTTON_1";
    case SENSOR_BUTTON_2:
        return "SENSOR_BUTTON_2";
    case SENSOR_SWITCH_1:
        return "SENSOR_SWITCH_1";
    case SENSOR_SWITCH_2:
        return "SENSOR_SWITCH_2";
    case SENSOR_SIZE:
        return "SENSOR_SIZE";
    default:
        return "UNKNOWN";
    }
}
