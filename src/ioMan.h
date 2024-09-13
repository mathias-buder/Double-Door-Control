/**
 * \file    ioManagement.h
 * \brief   The header file for the input/output management

 * \author  Mathias Buder
 * \date    2024-09-13

 *  Copyright (c) 2024 Mathias Buder
 */

#ifndef IO_MANAGEMENT_H
#define IO_MANAGEMENT_H

#include <Arduino.h>
#include "hsm.h"

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
    INPUT_STATE_INACTIVE, /*!< The input is not active */
    INPUT_STATE_ACTIVE      /*!< The input is active */
} input_state_t;

/**
 * @brief Enumeration of the sensor debounce state
 * @details The sensor debounce state defines whether the sensor is debounced or not
 */
typedef enum
{
    INPUT_DEBOUNCE_UNSTABLE, /*!< The input is under debouncing */
    INPUT_DEBOUNCE_STABLE    /*!< The input is debounced */
} input_debounce_t;

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
 * @brief The sensor status structure
 * @details The sensor status structure is used to hold the state and the debounce state of the sensor
 */
typedef struct {
    input_state_t    state;      //!< The state of the input
    input_debounce_t debounce;   //!< The debounce state of the input
} input_status_t;

/**
 * @brief The pin configuration structure
 * @details The pin configuration structure is used to hold the pin ID, number, and direction
 */
typedef struct
{
    const io_t    io;            /*!< The input/output */
    const uint8_t pinNumber;     /*!< The pin number */
    const uint8_t direction;     /*!< The direction of the pin */
    const uint8_t activeState;   /*!< The active state of the pin */
    uint16_t      debounceDelay; /*!< The debounce delay of the pin */
} io_config_t;

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


/******************************** Function prototype ************************************/

void           ioMan_Setup( void );
void           ioMan_setDoorState( const door_type_t door, const lock_state_t state );
input_status_t ioMan_getDoorIoState( const io_t sensor );
void           ioMan_setLed( bool enable, door_type_t door, led_color_t color );
void processTimers( door_control_t* const pDoorControl );


#endif  // IO_MANAGEMENT_H