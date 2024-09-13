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

void ioMan_Setup( void )
{
    Log.noticeln( "%s: Setting up the input/output management", __func__ );

    for ( uint8_t i = 0; i < sizeof( buttonSwitchIoConfig ) / sizeof( buttonSwitchIoConfig[0] ); i++ )
    {
        pinMode( buttonSwitchIoConfig[i].pinNumber, buttonSwitchIoConfig[i].direction );
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
 * @brief Get the state of the door
 *
 * @param door - The door type
 * @return lock_state_t - The state of the door
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
 * @brief Set the LED state and color
 *
 * @param enable - Whether to enable or disable the LED.
 * @param door - The type of door.
 * @param color - The color of the LED.
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

void ioMan_setDebounceDelay( const io_t io, const uint16_t delay )
{
    if ( io >= IO_INPUT_SIZE )
    {
        Log.errorln( "%s: Invalid input: %d", __func__, io );
        return;
    }
    buttonSwitchIoConfig[io].debounceDelay = delay;
}