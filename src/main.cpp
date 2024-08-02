#include <Arduino.h>
#include <ArduinoLog.h>
#include <TimerOne.h>

#include "hsm.h"

/****************************************************************************************/
/*                                    PIN CONFIGURATION                                 */
/****************************************************************************************/
/* DOOR 1 */
/****************************************************************************************/
#define RBG_LED_1_R       5   /*!< Pin for the red LED of the RGB-LED */
#define RBG_LED_1_G       6   /*!< Pin for the green LED of the RGB-LED */
#define RBG_LED_1_B       7   /*!< Pin for the blue LED of the RGB-LED */

#define TUER_1_TASTER     2   /*!< Pin for the button of the door */
#define TUER_1_SCHALTER   3   /*!< Pin for the switch of the door */
#define TUER_1_MAGNET     4   /*!< Pin for the magnet of the door */

/****************************************************************************************/
/* DOOR 2 */
/****************************************************************************************/
#define RBG_LED_2_R       8   /*!< Pin for the red LED of the RGB-LED */
#define RBG_LED_2_G       9   /*!< Pin for the green LED of the RGB-LED */
#define RBG_LED_2_B       10  /*!< Pin for the blue LED of the RGB-LED */

#define TUER_2_TASTER     11  /*!< Pin for the button of the door */
#define TUER_2_SCHALTER   12  /*!< Pin for the switch of the door */
#define TUER_2_MAGNET     13  /*!< Pin for the magnet of the door */

/****************************************************************************************/


/************************************* ENUMERATION **************************************/

/**
 * @brief
 */
typedef enum
{
    PROCESS_STATE_INIT,        /*!< Initializing the state machine */
    PROCESS_STATE_IDLE,        /*!< The state machine is in idle state */
    PROCESS_STATE_FAULT,       /*!< The state machine is in fault state */
    PROCESS_STATE_DOOR_1_OPEN, /*!< The door 1 is open */
    PROCESS_STATE_DOOR_2_OPEN  /*!< The door 2 is open */
} process_state_t;

/**
 * @brief
 */
typedef enum
{
    PROCESS_EVENT_INIT_DONE = 1,       /*!< The initialization is done */
    PROCESS_EVENT_DOOR_1_OPEN,         /*!< The door 1 is open */
    PROCESS_EVENT_DOOR_1_CLOSE,        /*!< The door 1 is closed */
    PROCESS_EVENT_DOOR_1_OPEN_TIMEOUT, /*!< The door 1 is open timeout */
    PROCESS_EVENT_DOOR_2_OPEN,         /*!< The door 2 is open */
    PROCESS_EVENT_DOOR_2_CLOSE,        /*!< The door 2 is closed */
    PROCESS_EVENT_DOOR_2_OPEN_TIMEOUT, /*!< The door 2 is open timeout */
    PROCESS_EVENT_DOOR_1_2_OPEN,       /*!< The door 1 and 2 are open */
} process_event_t;

/************************************* STRUCTURE **************************************/

//! process state machine
typedef struct
{
  state_machine_t machine;      //!< Abstract state machine
  // uint32_t Set_Time;    //! Set time of a process
  // uint32_t Resume_Time; //!< Remaining time when the process is paused
  // uint32_t Timer;       //!< Process timer
}process_t;




/******************************** Function prototype ************************************/

static state_machine_result_t initHandler( state_machine_t* const pState );
static state_machine_result_t initEntryHandler( state_machine_t* const pState );
static state_machine_result_t initExitHandler( state_machine_t* const pState );

static state_machine_result_t idleHandler( state_machine_t* const pState );
static state_machine_result_t idleEntryHandler( state_machine_t* const pState );
static state_machine_result_t idleExitHandler( state_machine_t* const pState );

static state_machine_result_t faultHandler( state_machine_t* const pState );
static state_machine_result_t faultEntryHandler( state_machine_t* const pState );
static state_machine_result_t faultExitHandler( state_machine_t* const pState );

static state_machine_result_t door1OpenHandler( state_machine_t* const pState );
static state_machine_result_t door1OpenEntryHandler( state_machine_t* const pState );
static state_machine_result_t door1OpenExitHandler( state_machine_t* const pState );

static state_machine_result_t door2OpenHandler( state_machine_t* const pState );
static state_machine_result_t door2OpenEntryHandler( state_machine_t* const pState );
static state_machine_result_t door2OpenExitHandler( state_machine_t* const pState );

static void                   initProcess( process_t* const pProcess, uint32_t processTime );
void                          eventLogger( uint32_t stateMachine, uint32_t state, uint32_t event );
void                          resultLogger( uint32_t state, state_machine_result_t result );

/******************************** Global variables ************************************/

static const state_t processStates[] = {

    [PROCESS_STATE_INIT] = {
        .Handler = initHandler,
        .Entry   = initEntryHandler,
        .Exit    = initExitHandler,
        .Id      = PROCESS_STATE_INIT
    },

    [PROCESS_STATE_IDLE] = {
        .Handler = idleHandler,
        .Entry   = idleEntryHandler,
        .Exit    = idleExitHandler,
        .Id      = PROCESS_STATE_IDLE
    },

    [PROCESS_STATE_FAULT] = {
        .Handler = faultHandler,
        .Entry   = faultEntryHandler,
        .Exit    = faultExitHandler,
        .Id      = PROCESS_STATE_FAULT
    },

    [PROCESS_STATE_DOOR_1_OPEN] = {
        .Handler = door1OpenHandler,
        .Entry   = door1OpenEntryHandler,
        .Exit    = door1OpenExitHandler,
        .Id      = PROCESS_STATE_DOOR_1_OPEN
    },

    [PROCESS_STATE_DOOR_2_OPEN] = {
        .Handler = door2OpenHandler,
        .Entry   = door2OpenEntryHandler,
        .Exit    = door2OpenExitHandler,
        .Id      = PROCESS_STATE_DOOR_2_OPEN
    }
};

process_t              process;                                          /*!< Instance of process_t */
state_machine_t* const stateMachines[] = {(state_machine_t*) &process}; /*!< Create and initialize the array of state machines. */

/**
 * @brief Setup Funktion
 * @details Wird einmalig beim Start des Programms ausgeführt und initialisiert
 * die Steuerung
 */
void setup()
{

    // Initialize with log level and log output.
    Serial.begin( 115200 );
    Log.begin( LOG_LEVEL_INFO, &Serial );

    // Log.notice("*** Logging example " CR); 
    
    /* Initialize the process */
    initProcess( &process, 0 );

  // digitalWrite(RBG_LED_1_R, HIGH);
  // digitalWrite(RBG_LED_1_G, HIGH);
  // digitalWrite(RBG_LED_1_R, HIGH);
}

/* ***************************************************************************************
 *                                     HAUPTPROGRAMM
 * **************************************************************************************/
void loop()
{

    /* Dispatch the event to the state machine */
    if ( dispatch_event( stateMachines, 1, eventLogger, resultLogger ) == EVENT_UN_HANDLED )
    {
        Log.error( "Event is not handled" CR );
    }
}


static void initProcess( process_t* const pProcess, uint32_t processTime )
{
    pProcess->machine.State = &processStates[PROCESS_STATE_INIT];
    pProcess->machine.Event = 0;
    // pProcess->Set_Time      = processTime;
    // pProcess->Resume_Time   = 0;

    initEntryHandler( (state_machine_t*) pProcess );
}


static state_machine_result_t initHandler( state_machine_t* const pState )
{
    Log.notice("Init Handler" CR);
    return EVENT_HANDLED;
}


static state_machine_result_t initEntryHandler( state_machine_t* const pState )
{
    Log.notice("Init Entry Handler" CR);
    return EVENT_HANDLED;
}


static state_machine_result_t initExitHandler( state_machine_t* const pState )
{
    Log.notice("Init Exit Handler" CR);
    return EVENT_HANDLED;
}


static state_machine_result_t idleHandler( state_machine_t* const pState )
{
    Log.notice("Idle Handler" CR);
    return EVENT_HANDLED;
}


static state_machine_result_t idleEntryHandler( state_machine_t* const pState )
{
    Log.notice("Idle Entry Handler" CR);
    return EVENT_HANDLED;
}


static state_machine_result_t idleExitHandler( state_machine_t* const pState )
{
    Log.notice("Idle Exit Handler" CR);
    return EVENT_HANDLED;
}


static state_machine_result_t faultHandler( state_machine_t* const pState )
{
    Log.notice("Fault Handler" CR);
    return EVENT_HANDLED;
}


static state_machine_result_t faultEntryHandler( state_machine_t* const pState )
{
    Log.notice("Fault Entry Handler" CR);
    return EVENT_HANDLED;
}

static state_machine_result_t faultExitHandler( state_machine_t* const pState )
{
    Log.notice("Fault Exit Handler" CR);
    return EVENT_HANDLED;
}


static state_machine_result_t door1OpenHandler( state_machine_t* const pState )
{
    Log.notice("Door 1 Open Handler" CR);
    return EVENT_HANDLED;
}


static state_machine_result_t door1OpenEntryHandler( state_machine_t* const pState )
{
    Log.notice("Door 1 Open Entry Handler" CR);
    return EVENT_HANDLED;
}


static state_machine_result_t door1OpenExitHandler( state_machine_t* const pState )
{
    Log.notice("Door 1 Open Exit Handler" CR);
    return EVENT_HANDLED;
}

static state_machine_result_t door2OpenHandler( state_machine_t* const pState )
{
    Log.notice("Door 2 Open Handler" CR);
    return EVENT_HANDLED;
}


static state_machine_result_t door2OpenEntryHandler( state_machine_t* const pState )
{
    Log.notice("Door 2 Open Entry Handler" CR);
    return EVENT_HANDLED;
}


static state_machine_result_t door2OpenExitHandler( state_machine_t* const pState )
{
    Log.notice("Door 2 Open Exit Handler" CR);
    return EVENT_HANDLED;
}




//! Callback function to log the events dispatched by state machine framework.
void eventLogger(uint32_t stateMachine, uint32_t state, uint32_t event)
{
    Log.notice( "Event: %d, State: %d, State Machine: %d\n", event, state, stateMachine );
}

//! Callback function to log the result of event processed by state machine
void resultLogger(uint32_t state, state_machine_result_t result)
{
    Log.notice( "Result: %d, State: %d\n", result, state );
}




















  // Timer1.initialize(1000000); // The led will blink in a half second time
  //                            // interval
  // Timer1.attachInterrupt(blinkLed);


// int ledState = LOW;
// void blinkLed() {
//   ledState = !ledState;
//   digitalWrite(RBG_LED_1_R, ledState);
// }
