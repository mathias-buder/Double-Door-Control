#include <Arduino.h>
#include <ArduinoLog.h>
#include <TimerOne.h>

#include "hsm.h"

/***************************************************************************************************/
/*                                           PIN CONFIGURATION                                     */
/***************************************************************************************************/
/* DOOR 1 */
/***************************************************************************************************/
#define RBG_LED_1_R                     7              /*!< Pin for the red LED of the RGB-LED */
#define RBG_LED_1_G                     6              /*!< Pin for the green LED of the RGB-LED */
#define RBG_LED_1_B                     5              /*!< Pin for the blue LED of the RGB-LED */
#define DOOR_1_BUTTON                   2              /*!< Pin for the button of the door */
#define DOOR_1_SWITCH                   3              /*!< Pin for the switch of the door */
#define DOOR_1_MAGNET                   4              /*!< Pin for the magnet of the door */

/***************************************************************************************************/
/* DOOR 2 */
/***************************************************************************************************/
#define RBG_LED_2_R                     13             /*!< Pin for the red LED of the RGB-LED */
#define RBG_LED_2_G                     12             /*!< Pin for the green LED of the RGB-LED */
#define RBG_LED_2_B                     11             /*!< Pin for the blue LED of the RGB-LED */
#define DOOR_2_BUTTON                   10             /*!< Pin for the button of the door */
#define DOOR_2_SWITCH                   9              /*!< Pin for the switch of the door */
#define DOOR_2_MAGNET                   8              /*!< Pin for the magnet of the door */
/***************************************************************************************************/
/*                                        GENERAL CONFIGURATION                                    */
/***************************************************************************************************/
#define DEBOUNCE_DELAY_DOOR_BUTTON_1    100            /*!< Debounce delay for the door button @unit ms*/
#define DEBOUNCE_DELAY_DOOR_BUTTON_2    100            /*!< Debounce delay for the door button @unit ms*/
#define DEBOUNCE_DELAY_DOOR_SWITCH_1    100            /*!< Debounce delay for the door switch @unit ms*/
#define DEBOUNCE_DELAY_DOOR_SWITCH_2    100            /*!< Debounce delay for the door switch @unit ms*/
#define DEBOUNCE_STABLE_TIMEOUT         300            /*!< Timeout for the debounce stable state @unit ms */

#define SERIAL_BAUD_RATE                115200         /*!< Baud rate of the serial communication @unit bps */
#define DEFAULT_LOG_LEVEL               LOG_LEVEL_INFO /*!< Default log level */

#define LED_BLINK_INTERVAL              500            /*!< Interval of the led blink @unit ms */
#define DOOR_UNLOCK_TIMEOUT             5              /*!< Timeout for the door unlock ( 0 = disabled ) @unit s */
#define DOOR_OPEN_TIMEOUT               600            /*!< Timeout for the door open ( 0 = disabled ) @unit s */

/********************************************** ENUMERATION ****************************************/

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
    LOCK_STATE_UNLOCKED, /*!< The door is open */
    LOCK_STATE_LOCKED    /*!< The door is closed */
} lock_state_t;

/**
 * @brief Enumeration of the door sensor
 * @details The door sensor is a combination of a button and a switch
 */
typedef enum
{
    /* Inputs */
    IO_BUTTON_1,   /*!< The button of the door 1 */
    IO_BUTTON_2,   /*!< The button of the door 2 */
    IO_SWITCH_1,   /*!< The switch of the door 1 */
    IO_SWITCH_2,   /*!< The switch of the door 2 */
    IO_INPUT_SIZE, /*!< Number of inputs */

    /* Outputs */
    IO_MAGNET_1,   /*!< The magnet of the door 1 */
    IO_MAGNET_2,   /*!< The magnet of the door 2 */
    IO_LED_1_R,    /*!< The red LED of the door 1 */
    IO_LED_1_G,    /*!< The green LED of the door 1 */
    IO_LED_1_B,    /*!< The blue LED of the door 1 */
    IO_LED_2_R,    /*!< The red LED of the door 2 */
    IO_LED_2_G,    /*!< The green LED of the door 2 */
    IO_LED_2_B     /*!< The blue LED of the door 2 */
} io_t;

/**
 * @brief Enumeration of the button state
 * @details The button state defines whether the button is pressed or released
 */
typedef enum
{
    SENSOR_STATE_RELEASED, /*!< The button is released */
    SENSOR_STATE_PRESSED   /*!< The button is pressed */
} sensor_state_t;

/**
 * @brief Enumeration of the sensor debounce state
 * @details The sensor debounce state defines whether the sensor is debounced or not
 */
typedef enum
{
    SENSOR_DEBOUNCE_UNSTABLE, /*!< The sensor is under debouncing */
    SENSOR_DEBOUNCE_STABLE    /*!< The sensor is debounced */
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

/**
 * @brief Enumeration of the door timer
 */
typedef enum
{
    DOOR_TIMER_TYPE_UNLOCK, /*!< The unlock timer */
    DOOR_TIMER_TYPE_OPEN,   /*!< The open timer */
    DOOR_TIMER_TYPE_SIZE    /*!< Number of timers */
} door_timer_type_t;

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
 * @brief The sensor status structure
 * @details The sensor status structure is used to hold the state and the debounce state of the sensor
 */
typedef struct {
    sensor_state_t    state;      //!< The state of the sensor
    sensor_debounce_t debounce;   //!< The debounce state of the sensor
} sensor_status_t;

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
 * @brief The pin configuration structure
 * @details The pin configuration structure is used to hold the pin ID, number, and direction
 */
typedef struct
{
    io_t     io;            /*!< The input/output */
    uint8_t  pinNumber;     /*!< The pin number */
    uint8_t  direction;     /*!< The direction of the pin */
    uint8_t  activeState;   /*!< The active state of the pin */
    uint16_t debounceDelay; /*!< The debounce delay of the pin */
} io_config_t;

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

static state_machine_result_t door1UnlockHandler( state_machine_t* const pState, const uint32_t event );
static state_machine_result_t door1UnlockEntryHandler( state_machine_t* const pState, const uint32_t event );
static state_machine_result_t door1UnlockExitHandler( state_machine_t* const pState, const uint32_t event );
static state_machine_result_t door1OpenHandler( state_machine_t* const pState, const uint32_t event );
static state_machine_result_t door1OpenEntryHandler( state_machine_t* const pState, const uint32_t event );
static state_machine_result_t door1OpenExitHandler( state_machine_t* const pState, const uint32_t event );
static void                   door1BlinkLedIsrHandler( void );

static state_machine_result_t door2UnlockHandler( state_machine_t* const pState, const uint32_t event );
static state_machine_result_t door2UnlockEntryHandler( state_machine_t* const pState, const uint32_t event );
static state_machine_result_t door2UnlockExitHandler( state_machine_t* const pState, const uint32_t event );
static state_machine_result_t door2OpenHandler( state_machine_t* const pState, const uint32_t event );
static state_machine_result_t door2OpenEntryHandler( state_machine_t* const pState, const uint32_t event );
static state_machine_result_t door2OpenExitHandler( state_machine_t* const pState, const uint32_t event );
static void                   door2BlinkLedIsrHandler( void );

static void                   doorOpenTimeoutHandler( uint32_t time );
static void                   doorUnlockTimeoutHandler( uint32_t time );

void                          eventLogger( uint32_t stateMachine, uint32_t state, uint32_t event );
void                          resultLogger( uint32_t state, state_machine_result_t result );
static void                   setDoorState( const door_type_t door, const lock_state_t state );
static sensor_status_t        getDoorIoState( const io_t sensor );
static void                   generateEvent( door_control_t* const pDoorControl );
static String                 stateToString( door_control_state_t state );
static String                 eventToString( door_control_event_t event );
static String                 resultToString( state_machine_result_t result );
static String                 ioToString( io_t io );
static String                 timerTypeToString( door_timer_type_t timerType );
static void                   setLed( bool enable, door_type_t door, led_color_t color );
static void                   processTimers( door_control_t* const pDoorControl );

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
                    { doorUnlockTimeoutHandler, (uint32_t) DOOR_UNLOCK_TIMEOUT * (uint32_t) 1000, 0 },
                    { doorOpenTimeoutHandler,   (uint32_t) DOOR_OPEN_TIMEOUT   * (uint32_t) 1000, 0 }
                }
};

/**
 * @brief The array of state machines
 * @details The array of state machines is used to dispatch the event to the state machines.
 */
state_machine_t* const stateMachines[] = {(state_machine_t*) &doorControl};

/**
 * @brief The RGB LED pins
1 * @details The RGB LED pins are defined as an array of pins for each door
 */
uint8_t ledPins[DOOR_TYPE_SIZE][RGB_LED_PIN_SIZE] = {
    {RBG_LED_1_R, RBG_LED_1_G, RBG_LED_1_B},    /*!< RGB LED pins for door 1 */
    {RBG_LED_2_R, RBG_LED_2_G, RBG_LED_2_B}     /*!< RGB LED pins for door 2 */
};

/**
 * @brief The pin configuration
 * @details The pin configuration is used to define the pin number and direction
 */
io_config_t ioConfig[] = {
    /* Inputs */
    { IO_BUTTON_1, DOOR_1_BUTTON, INPUT,  HIGH, DEBOUNCE_DELAY_DOOR_BUTTON_1 }, /*!< Button 1 */
    { IO_BUTTON_2, DOOR_2_BUTTON, INPUT,  HIGH, DEBOUNCE_DELAY_DOOR_BUTTON_2 }, /*!< Button 2 */
    { IO_SWITCH_1, DOOR_1_SWITCH, INPUT,  HIGH, DEBOUNCE_DELAY_DOOR_SWITCH_1 }, /*!< Switch 1 */
    { IO_SWITCH_2, DOOR_2_SWITCH, INPUT,  HIGH, DEBOUNCE_DELAY_DOOR_SWITCH_2 }, /*!< Switch 2 */

    /* Outputs */
    { IO_MAGNET_1, DOOR_1_MAGNET, OUTPUT, LOW,  0                            }, /*!< Magnet 1 */
    { IO_MAGNET_2, DOOR_2_MAGNET, OUTPUT, LOW,  0                            }, /*!< Magnet 2 */
    { IO_LED_1_R,  RBG_LED_1_R,   OUTPUT, LOW,  0                            }, /*!< Red LED 1 */
    { IO_LED_1_G,  RBG_LED_1_G,   OUTPUT, LOW,  0                            }, /*!< Green LED 1 */
    { IO_LED_1_B,  RBG_LED_1_B,   OUTPUT, LOW,  0                            }, /*!< Blue LED 1 */
    { IO_LED_2_R,  RBG_LED_2_R,   OUTPUT, LOW,  0                            }, /*!< Red LED 2 */
    { IO_LED_2_G,  RBG_LED_2_G,   OUTPUT, LOW,  0                            }, /*!< Green LED 2 */
    { IO_LED_2_B,  RBG_LED_2_B,   OUTPUT, LOW,  0                            }  /*!< Blue LED 2 */
};

/******************************** Function definition ************************************/

/**
 * @brief Setup the application
 */
void setup()
{
    /* Initialize with log level and log output */
    Serial.begin( SERIAL_BAUD_RATE );
    Log.begin( DEFAULT_LOG_LEVEL, &Serial );

    Log.notice( "Starting ... " CR );

    /* Initialize the pins */
    for ( uint8_t i = 0; i < sizeof( ioConfig ) / sizeof( ioConfig[0] ); i++ )
    {
        pinMode( ioConfig[i].pinNumber, ioConfig[i].direction );
    }

    /* Initialize the led blink timer */
    Timer1.initialize( ( (uint32_t) 2000 ) * ( (uint32_t) LED_BLINK_INTERVAL ) );

    /* Switch to the init state */
    switch_state( &doorControl.machine, &doorControlStates[DOOR_CONTROL_STATE_INIT] );

    Log.notice( "... Done" CR );
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
 * @brief Handler for the init state entry
 * 
 * @param pState - The state machine
 * @param event - The event
 * @return state_machine_result_t - The result of the handler
 */
static state_machine_result_t initEntryHandler( state_machine_t* const pState, const uint32_t event )
{
    Log.verbose("%s: Event %s" CR, __func__, eventToString( (door_control_event_t) event ).c_str() );

    /* Initialize both doors to locked */
    setDoorState( DOOR_TYPE_DOOR_1, LOCK_STATE_LOCKED );
    setDoorState( DOOR_TYPE_DOOR_2, LOCK_STATE_LOCKED );

    /* Check whether the door switches are debouncing. This "waiting" mechanism is only used
     * for the initialization as the door switches are checked here in an one-shot manner. If
     * the switches are not stable within the timeout, the state machine switches to the fault state.
     */
    sensor_status_t door1SwitchStatus, door2SwitchStatus;
    uint64_t        currentTime = millis();

    do {
        door1SwitchStatus = getDoorIoState( IO_SWITCH_1 );
        door2SwitchStatus = getDoorIoState( IO_SWITCH_2 );

        if ( ( millis() - currentTime ) >= DEBOUNCE_STABLE_TIMEOUT )
        {
            Log.error( "Door switches wheren't stable within %d ms" CR, DEBOUNCE_STABLE_TIMEOUT );

            /* Switch to fault state */
            return switch_state( pState, &doorControlStates[DOOR_CONTROL_STATE_FAULT] );
        }
    }
    while (    ( door1SwitchStatus.debounce == SENSOR_DEBOUNCE_UNSTABLE )
            || ( door2SwitchStatus.debounce == SENSOR_DEBOUNCE_UNSTABLE ) );
    

     /* Get pointer to the current event */
    event_t** pCurrentEvent = &pState->event;

    if (    ( door1SwitchStatus.state == SENSOR_STATE_PRESSED )
         && ( door2SwitchStatus.state == SENSOR_STATE_PRESSED) )
    {
        pushEvent( pCurrentEvent, DOOR_CONTROL_EVENT_DOOR_1_2_CLOSE );
    }

    if ( door1SwitchStatus.state == SENSOR_STATE_RELEASED )
    {
        pushEvent( pCurrentEvent, DOOR_CONTROL_EVENT_DOOR_1_OPEN );
    }

    if ( door2SwitchStatus.state == SENSOR_STATE_RELEASED )
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
    sensor_state_t door1Button = getDoorIoState( IO_BUTTON_1 ).state;
    sensor_state_t door2Button = getDoorIoState( IO_BUTTON_2 ).state;

    /* XOR-logic to allow only one door to be open */
    if (    ( door1Button == SENSOR_STATE_PRESSED )
         && ( door2Button == SENSOR_STATE_RELEASED ) )
    {
        pushEvent( &pState->event, DOOR_CONTROL_EVENT_DOOR_1_UNLOCK );
    }
    else if (    ( door1Button == SENSOR_STATE_RELEASED )
              && ( door2Button == SENSOR_STATE_PRESSED ) )
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
 * @brief Handler for the door 1 unlock entry
 * 
 * @param pState - The state machine
 * @param event - The event
 * @return state_machine_result_t - The result of the handler
 */
static state_machine_result_t door1UnlockEntryHandler( state_machine_t* const pState, const uint32_t event )
{
    Log.verbose("%s: Event %s" CR, __func__, eventToString( (door_control_event_t) event ).c_str() );

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
    Log.verbose("%s: Event %s" CR, __func__, eventToString( (door_control_event_t) event ).c_str() );

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
    Log.verbose("%s: Event %s" CR, __func__, eventToString( (door_control_event_t) event ).c_str() );

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
    Log.verbose("%s: Event %s" CR, __func__, eventToString( (door_control_event_t) event ).c_str() );

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
    Log.verbose("%s: Event %s" CR, __func__, eventToString( (door_control_event_t) event ).c_str() );

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
    Log.verbose("%s: Event %s" CR, __func__, eventToString( (door_control_event_t) event ).c_str() );

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
    Log.verbose("%s: Event %s" CR, __func__, eventToString( (door_control_event_t) event ).c_str() );

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
    Log.verbose("%s: Event %s" CR, __func__, eventToString( (door_control_event_t) event ).c_str() );

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
    Log.verbose("%s: Time: %d" CR, __func__, time );

    /* Switch to the fault state if the door is not closed in time */
    switch_state( &doorControl.machine, &doorControlStates[DOOR_CONTROL_STATE_FAULT] );
}


static void doorUnlockTimeoutHandler( uint32_t time )
{
    Log.verbose("%s: Time: %d" CR, __func__, time );

    /* Switch back to the idle state if the door is not opened in time */
    pushEvent( &doorControl.machine.event, DOOR_CONTROL_EVENT_DOOR_1_UNLOCK_TIMEOUT );
    pushEvent( &doorControl.machine.event, DOOR_CONTROL_EVENT_DOOR_2_UNLOCK_TIMEOUT );
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
    static lock_state_t lastState[DOOR_TYPE_SIZE] = {LOCK_STATE_LOCKED, LOCK_STATE_LOCKED};
    static uint8_t      pinMap[DOOR_TYPE_SIZE]    = {DOOR_1_MAGNET, DOOR_2_MAGNET};

    /* Check if the door is valid */
    if ( door >= DOOR_TYPE_SIZE )
    {
        Log.error( "%s: Invalid door type: %d" CR, __func__, door );
        return;
    }

    if ( lastState[door] != state )
    {
        Log.notice( "%s: Door %d is %s" CR, __func__, door, ( state == LOCK_STATE_UNLOCKED ) ? "unlocked" : "locked" );
    }

    /* Set the state of the door */
    digitalWrite( pinMap[door], state );

    lastState[door] = state;
}


/**
 * @brief Get the Door Sensor State object
 * 
 * @param sensor 
 * @param debounceState 
 * @return sensor_state_t 
 */
static sensor_status_t getDoorIoState( const io_t input )
{
    static uint8_t           sensorState[IO_INPUT_SIZE]      = {0};
    static uint8_t           lastSensorState[IO_INPUT_SIZE]  = {0};
    static uint32_t          lastDebounceTime[IO_INPUT_SIZE] = {0};
    static sensor_status_t   state[IO_INPUT_SIZE]            = {SENSOR_STATE_RELEASED, SENSOR_DEBOUNCE_UNSTABLE};

    /* Read the state of the switch into a local variable */
    uint8_t reading = digitalRead( ioConfig[input].pinNumber );

    /* check to see if you just pressed the input
     * (i.e. the input went from LOW to HIGH), and you've waited long enough
     * since the last press to ignore any noise: */

    /* If the switch changed, due to noise or pressing: */
    if ( reading != lastSensorState[input] )
    {
        /* reset the debouncing timer */
        lastDebounceTime[input] = millis();
        state[input].state      = SENSOR_STATE_RELEASED;
        state[input].debounce   = SENSOR_DEBOUNCE_UNSTABLE;
    }

    if ( ( millis() - lastDebounceTime[input] ) > ioConfig[input].debounceDelay )
    {
        /* whatever the reading is at, it's been there for longer than the debounce
         * delay, so take it as the actual current state: */
        state[input].debounce = SENSOR_DEBOUNCE_STABLE;

        /* if the input state has changed: */
        if ( reading != sensorState[input] )
        {
            sensorState[input] = reading;

            if ( sensorState[input] == HIGH )
            {
                state[input].state = SENSOR_STATE_PRESSED;
                Log.notice( "%s: %s is pressed" CR, __func__, ioToString( input ).c_str() );
            }
            else
            {
                state[input].state = SENSOR_STATE_RELEASED;
                Log.notice( "%s: %s is released" CR, __func__, ioToString( input ).c_str() );
            }
        }
    }

    /* save the reading. Next time through the loop, it'll be the lastSensorState: */
    lastSensorState[input] = reading;

    return state[input];
}

/**
 * @brief Generate the event based on the door switches and buttons
 *
 * @param pDoorControl - Pointer to the door control
 */
static void generateEvent( door_control_t* const pDoorControl )
{
    /* Read the state of both door switches */
    sensor_state_t door1Switch = getDoorIoState( IO_SWITCH_1 ).state;
    sensor_state_t door2Switch = getDoorIoState( IO_SWITCH_2 ).state;

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
            Log.notice( "%s: %s" CR, timerTypeToString( (door_timer_type_t) i ).c_str(), remainingTime.c_str() );
        }
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
    case DOOR_CONTROL_STATE_DOOR_1_UNLOCKED:
        return "DOOR_CONTROL_STATE_DOOR_1_UNLOCKED";
    case DOOR_CONTROL_STATE_DOOR_1_OPEN:
        return "DOOR_CONTROL_STATE_DOOR_1_OPEN";
    case DOOR_CONTROL_STATE_DOOR_2_UNLOCKED:
        return "DOOR_CONTROL_STATE_DOOR_2_UNLOCKED";
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
    case DOOR_CONTROL_EVENT_DOOR_1_UNLOCK:
        return "DOOR_CONTROL_EVENT_DOOR_1_UNLOCK";
    case DOOR_CONTROL_EVENT_DOOR_1_UNLOCK_TIMEOUT:
        return "DOOR_CONTROL_EVENT_DOOR_1_UNLOCK_TIMEOUT";
    case DOOR_CONTROL_EVENT_DOOR_1_OPEN:
        return "DOOR_CONTROL_EVENT_DOOR_1_OPEN";
    case DOOR_CONTROL_EVENT_DOOR_1_CLOSE:
        return "DOOR_CONTROL_EVENT_DOOR_1_CLOSE";
    case DOOR_CONTROL_EVENT_DOOR_1_OPEN_TIMEOUT:
        return "DOOR_CONTROL_EVENT_DOOR_1_OPEN_TIMEOUT";
    case DOOR_CONTROL_EVENT_DOOR_2_UNLOCK:
        return "DOOR_CONTROL_EVENT_DOOR_2_UNLOCK";
    case DOOR_CONTROL_EVENT_DOOR_2_UNLOCK_TIMEOUT:
        return "DOOR_CONTROL_EVENT_DOOR_2_UNLOCK_TIMEOUT";
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
 * @brief Convert the input/output to string
 * 
 * @param sensor - The input/output to convert
 * @return String - The string representation of the input/output
 */
static String ioToString( io_t io )
{
    switch ( io )
    {
    case IO_BUTTON_1:
        return "IO_BUTTON_1";
    case IO_BUTTON_2:
        return "IO_BUTTON_2";
    case IO_SWITCH_1:
        return "IO_SWITCH_1";
    case IO_SWITCH_2:
        return "IO_SWITCH_2";
    case IO_MAGNET_1:
        return "IO_MAGNET_1";
    case IO_MAGNET_2:
        return "IO_MAGNET_2";
    case IO_LED_1_R:
        return "IO_LED_1_R";
    case IO_LED_1_G:
        return "IO_LED_1_G";
    case IO_LED_1_B:
        return "IO_LED_1_B";
    case IO_LED_2_R:
        return "IO_LED_2_R";
    case IO_LED_2_G:
        return "IO_LED_2_G";
    case IO_LED_2_B:
        return "IO_LED_2_B";
    default:
        return "UNKNOWN";
    }
}


/**
 * @brief Convert the timer type to string
 * 
 * @param timerType - The timer type to convert
 * @return String - The string representation of the timer type
 */
static String timerTypeToString( door_timer_type_t timerType )
{
    switch ( timerType )
    {
    case DOOR_TIMER_TYPE_OPEN:
        return "DOOR_TIMER_TYPE_OPEN";
    case DOOR_TIMER_TYPE_UNLOCK:
        return "DOOR_TIMER_TYPE_UNLOCK";
    default:
        return "UNKNOWN";
    }
}
