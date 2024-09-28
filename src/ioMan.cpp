/**
 * \file    ioManagement.c
 * \brief   The source file for the input/output management

 * \author  Mathias Buder
 * \date    2024-09-13

 *  Copyright (c) 2024 Mathias Buder
 */

#include <TimerOne.h>
#include "logging.h"

#include "ioMan.h"
#include "appSettings.h"


/**************************** Static Function prototype *********************************/


/******************************** Global variables ************************************/

static io_config_t buttonSwitchIoConfig[] = {
    { IO_BUTTON_1, DOOR_1_BUTTON, INPUT,  HIGH, DEBOUNCE_DELAY_DOOR_BUTTON_1 }, /*!< Button 1 */
    { IO_BUTTON_2, DOOR_2_BUTTON, INPUT,  HIGH, DEBOUNCE_DELAY_DOOR_BUTTON_2 }, /*!< Button 2 */
    { IO_SWITCH_1, DOOR_1_SWITCH, INPUT,  LOW,  DEBOUNCE_DELAY_DOOR_SWITCH_1 }, /*!< Switch 1 */
    { IO_SWITCH_2, DOOR_2_SWITCH, INPUT,  LOW,  DEBOUNCE_DELAY_DOOR_SWITCH_2 }, /*!< Switch 2 */
};


static const io_config_t magnetIoConfig[DOOR_TYPE_SIZE] = {
    { IO_MAGNET_1, DOOR_1_MAGNET, OUTPUT, LOW,  0 }, /*!< Magnet 1 */
    { IO_MAGNET_2, DOOR_2_MAGNET, OUTPUT, LOW,  0 }, /*!< Magnet 2 */
};


static const io_config_t ledIoConfig[DOOR_TYPE_SIZE][RGB_LED_PIN_SIZE] = {
    {
        { IO_LED_1_R, RBG_LED_1_R, OUTPUT, HIGH,  0 }, /*!< Red LED 1 */
        { IO_LED_1_G, RBG_LED_1_G, OUTPUT, HIGH,  0 }, /*!< Green LED 1 */
        { IO_LED_1_B, RBG_LED_1_B, OUTPUT, HIGH,  0 }  /*!< Blue LED 1 */
    },
    {
        { IO_LED_2_R, RBG_LED_2_R, OUTPUT, HIGH,  0 }, /*!< Red LED 2 */
        { IO_LED_2_G, RBG_LED_2_G, OUTPUT, HIGH,  0 }, /*!< Green LED 2 */
        { IO_LED_2_B, RBG_LED_2_B, OUTPUT, HIGH,  0 }  /*!< Blue LED 2 */
    }
};


/******************************** Function definition ************************************/



/**
 * @brief Sets up the input/output management for the system.
 *
 * This function initializes the pin modes for various input/output configurations
 * including button switches, magnets, and RGB LEDs. It iterates through the 
 * configuration arrays and sets the pin modes accordingly.
 *
 * The function performs the following steps:
 * 1. Logs the start of the setup process.
 * 2. Configures the pin modes for button switches.
 * 3. Configures the pin modes for magnets.
 * 4. Configures the pin modes for RGB LEDs for each door type.
 */
void ioMan_Setup( void )
{
    Log.noticeln( "%s: Setting up the input/output management", __func__ );

    for ( uint8_t i = 0; i < sizeof( buttonSwitchIoConfig ) / sizeof( buttonSwitchIoConfig[0] ); i++ )
    {
        pinMode( buttonSwitchIoConfig[i].pinNumber, buttonSwitchIoConfig[i].direction );
        ioMan_setDebounceDelay( (io_t) i, appSettings_getSettings()->debounceDelay[i] );
    }

    for ( uint8_t i = 0; i < sizeof( magnetIoConfig ) / sizeof( magnetIoConfig[0] ); i++ )
    {
        pinMode( magnetIoConfig[i].pinNumber, magnetIoConfig[i].direction );
    }

    for ( uint8_t i = 0; i < DOOR_TYPE_SIZE; i++ )
    {
        for ( uint8_t j = 0; j < RGB_LED_PIN_SIZE; j++ )
        {
            pinMode( ledIoConfig[i][j].pinNumber, ledIoConfig[i][j].direction );
        }
    }
}


/**
 * @brief Set the state of the door
 *
 * @param door - The door type
 * @param state - The state of the door
 */
void ioMan_setDoorState( const door_type_t door, const lock_state_t state )
{
    static lock_state_t lastState[DOOR_TYPE_SIZE] = {LOCK_STATE_LOCKED, LOCK_STATE_LOCKED};

    /* Check if the door is valid */
    if ( door >= DOOR_TYPE_SIZE )
    {
        Log.errorln( "%s: Invalid door type: %d", __func__, door );
        return;
    }

    /* Set the lock state ( The magnet is active low ) */
    digitalWrite(  magnetIoConfig[door].pinNumber, 
                 ( state == LOCK_STATE_LOCKED ) ? !magnetIoConfig[door].activeState : magnetIoConfig[door].activeState );

    if ( lastState[door] != state )
    {
        Log.noticeln( "%s: Door %d is %s", __func__, door, ( state == LOCK_STATE_UNLOCKED ) ? "unlocked" : "locked" );
    }

    lastState[door] = state;
}



/**
 * @brief Get the current state of the door input.
 *
 * This function reads the state of a specified input and determines if it is active or inactive,
 * taking into account debounce logic to filter out noise. It logs the state changes and returns
 * the current state of the input.
 *
 * @param input The input to check the state of.
 * @return input_status_t The current state of the input, including its activity state and debounce stability.
 *
 * The function performs the following steps:
 * 1. Logs the input being checked.
 * 2. Checks if the input index is valid.
 * 3. Reads the current state of the input pin.
 * 4. If the state has changed, resets the debounce timer and marks the state as unstable.
 * 5. If the debounce delay has passed, updates the state to stable and checks if the input state has changed or if it's the first reading.
 * 6. Logs the new state if it has changed.
 * 7. Saves the current reading for future comparisons.
 */
input_status_t ioMan_getDoorState( const io_t input )
{
    Log.verboseln( "%s: input: %s", __func__, logging_ioToString( input ).c_str() );

    static bool           initialReadingDone[IO_INPUT_SIZE] = {0};
    static uint8_t        ioState[IO_INPUT_SIZE]            = {0};
    static uint8_t        lastIoState[IO_INPUT_SIZE]        = {0};
    static uint32_t       lastDebounceTime[IO_INPUT_SIZE]   = {0};
    static input_status_t state[IO_INPUT_SIZE]              = {INPUT_STATE_INACTIVE, INPUT_DEBOUNCE_UNSTABLE};

    if ( input >= IO_INPUT_SIZE )
    {
        Log.errorln( "%s: Invalid input: %d", __func__, input );
        return ( ( input_status_t ){INPUT_STATE_INACTIVE, INPUT_DEBOUNCE_UNSTABLE} );
    }

    /* Read the state of the switch into a local variable */
    uint8_t reading = digitalRead( buttonSwitchIoConfig[input].pinNumber );

    /* check to see if you just pressed the input
     * (i.e. the input went from LOW to HIGH), and you've waited long enough
     * since the last press to ignore any noise: */

    /* If the switch changed, due to noise or pressing: */
    if ( reading != lastIoState[input] )
    {
        /* reset the debouncing timer */
        lastDebounceTime[input] = millis();
        state[input].state      = INPUT_STATE_INACTIVE;
        state[input].debounce   = INPUT_DEBOUNCE_UNSTABLE;
    }

    if ( ( millis() - lastDebounceTime[input] ) > buttonSwitchIoConfig[input].debounceDelay )
    {
        /* whatever the reading is at, it's been there for longer than the debounce
         * delay, so take it as the actual current state: */
        state[input].debounce = INPUT_DEBOUNCE_STABLE;

        /* if the input state has changed or it's the first reading */
        if ( ( reading != ioState[input] ) || !initialReadingDone[input] )
        {
            ioState[input] = reading;

            if ( ioState[input] == buttonSwitchIoConfig[input].activeState )
            {
                state[input].state = INPUT_STATE_ACTIVE;
                Log.noticeln( "%s: %s is active", __func__, logging_ioToString( input ).c_str() );
            }
            else
            {
                state[input].state = INPUT_STATE_INACTIVE;
                Log.noticeln( "%s: %s is inactive", __func__, logging_ioToString( input ).c_str() );
            }

            /* Set the first reading done flag */
            if ( !initialReadingDone[input] )
            {
                initialReadingDone[input] = true;
            }
        }
    }

    /* save the reading. Next time through the loop, it'll be the lastIoState: */
    lastIoState[input] = reading;

    return state[input];
}


/**
 * @brief Sets the LED state for a specified door.
 *
 * This function controls the LED color and state (on/off) for a given door.
 * It logs the operation and checks for valid door types before proceeding.
 *
 * @param enable Boolean value to enable (true) or disable (false) the LED.
 * @param door The door type for which the LED state is being set.
 * @param color The color to set the LED to, if enabling.
 *
 * The function supports the following LED colors:
 * - LED_COLOR_RED
 * - LED_COLOR_GREEN
 * - LED_COLOR_BLUE
 * - LED_COLOR_YELLOW
 * - LED_COLOR_MAGENTA
 * - LED_COLOR_CYAN
 * - LED_COLOR_WHITE
 *
 * If the door type is invalid, an error is logged and the function returns without making changes.
 * When disabling the LED, all color pins are set to their inactive state.
 */
void ioMan_setLed( bool enable, door_type_t door, led_color_t color )
{
    Log.verboseln( "%s: Enable: %d, Door: %d, Color: %d", __func__, enable, door, color );

    /* Check if the door is valid */
    if ( door >= DOOR_TYPE_SIZE )
    {
        Log.errorln( "%s: Invalid door type: %d", __func__, door );
        return;
    }

    if ( enable )
    {
        switch ( color )
        {
            case LED_COLOR_RED:
                digitalWrite( ledIoConfig[door][RGB_LED_PIN_R].pinNumber,  ledIoConfig[door][RGB_LED_PIN_R].activeState );
                digitalWrite( ledIoConfig[door][RGB_LED_PIN_G].pinNumber, !ledIoConfig[door][RGB_LED_PIN_G].activeState );
                digitalWrite( ledIoConfig[door][RGB_LED_PIN_B].pinNumber, !ledIoConfig[door][RGB_LED_PIN_B].activeState );
                break;
            case LED_COLOR_GREEN:
                digitalWrite( ledIoConfig[door][RGB_LED_PIN_R].pinNumber, !ledIoConfig[door][RGB_LED_PIN_R].activeState );
                digitalWrite( ledIoConfig[door][RGB_LED_PIN_G].pinNumber,  ledIoConfig[door][RGB_LED_PIN_G].activeState );
                digitalWrite( ledIoConfig[door][RGB_LED_PIN_B].pinNumber, !ledIoConfig[door][RGB_LED_PIN_B].activeState );
                break;
            case LED_COLOR_BLUE:
                digitalWrite( ledIoConfig[door][RGB_LED_PIN_R].pinNumber, !ledIoConfig[door][RGB_LED_PIN_R].activeState );
                digitalWrite( ledIoConfig[door][RGB_LED_PIN_G].pinNumber, !ledIoConfig[door][RGB_LED_PIN_G].activeState );
                digitalWrite( ledIoConfig[door][RGB_LED_PIN_B].pinNumber,  ledIoConfig[door][RGB_LED_PIN_B].activeState );
                break;
            case LED_COLOR_YELLOW:
                digitalWrite( ledIoConfig[door][RGB_LED_PIN_R].pinNumber,  ledIoConfig[door][RGB_LED_PIN_R].activeState );
                digitalWrite( ledIoConfig[door][RGB_LED_PIN_G].pinNumber,  ledIoConfig[door][RGB_LED_PIN_G].activeState );
                digitalWrite( ledIoConfig[door][RGB_LED_PIN_B].pinNumber, !ledIoConfig[door][RGB_LED_PIN_B].activeState );
                break;
            case LED_COLOR_MAGENTA:
                digitalWrite( ledIoConfig[door][RGB_LED_PIN_R].pinNumber,  ledIoConfig[door][RGB_LED_PIN_R].activeState );
                digitalWrite( ledIoConfig[door][RGB_LED_PIN_G].pinNumber, !ledIoConfig[door][RGB_LED_PIN_G].activeState );
                digitalWrite( ledIoConfig[door][RGB_LED_PIN_B].pinNumber,  ledIoConfig[door][RGB_LED_PIN_B].activeState );
                break;
            case LED_COLOR_CYAN:
                digitalWrite( ledIoConfig[door][RGB_LED_PIN_R].pinNumber, !ledIoConfig[door][RGB_LED_PIN_R].activeState );
                digitalWrite( ledIoConfig[door][RGB_LED_PIN_G].pinNumber,  ledIoConfig[door][RGB_LED_PIN_G].activeState );
                digitalWrite( ledIoConfig[door][RGB_LED_PIN_B].pinNumber,  ledIoConfig[door][RGB_LED_PIN_B].activeState );
                break;
            case LED_COLOR_WHITE:
                digitalWrite( ledIoConfig[door][RGB_LED_PIN_R].pinNumber,  ledIoConfig[door][RGB_LED_PIN_R].activeState );
                digitalWrite( ledIoConfig[door][RGB_LED_PIN_G].pinNumber,  ledIoConfig[door][RGB_LED_PIN_G].activeState );
                digitalWrite( ledIoConfig[door][RGB_LED_PIN_B].pinNumber,  ledIoConfig[door][RGB_LED_PIN_B].activeState );
                break;
            default:
                break;
        }
    }
    else
    {
        digitalWrite( ledIoConfig[door][RGB_LED_PIN_R].pinNumber, !ledIoConfig[door][RGB_LED_PIN_R].activeState );
        digitalWrite( ledIoConfig[door][RGB_LED_PIN_G].pinNumber, !ledIoConfig[door][RGB_LED_PIN_G].activeState );
        digitalWrite( ledIoConfig[door][RGB_LED_PIN_B].pinNumber, !ledIoConfig[door][RGB_LED_PIN_B].activeState );
    }
}


/**
 * @brief Sets the debounce delay for a specified input.
 *
 * This function configures the debounce delay for a given input pin.
 * Debouncing is used to ensure that only a single signal is registered
 * when a button is pressed or released, preventing multiple signals
 * caused by mechanical noise.
 *
 * @param io The input pin identifier. Must be less than IO_INPUT_SIZE.
 * @param delay The debounce delay in milliseconds.
 *
 * @note If the input pin identifier is invalid (greater than or equal to IO_INPUT_SIZE),
 *       an error message is logged and the function returns without making any changes.
 */
void ioMan_setDebounceDelay( const io_t io, const uint16_t delay )
{
    if ( io >= IO_INPUT_SIZE )
    {
        Log.errorln( "%s: Invalid input: %d", __func__, io );
        return;
    }
    buttonSwitchIoConfig[io].debounceDelay = delay;
}
