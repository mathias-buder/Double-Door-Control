/**
 * \file
 * \brief hierarchical state machine

 * \author  Nandkishor Biradar
 * \date    01 December 2018

 *  Copyright (c) 2018-2019 Nandkishor Biradar
 *  https://github.com/kiishor

 *  Distributed under the MIT License, (See accompanying
 *  file LICENSE or copy at https://mit-license.org/)
 */

/*
 *  --------------------- INCLUDE FILES ---------------------
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "hsm.h"

/*
 *  --------------------- DEFINITION ---------------------
 */

#define EXECUTE_HANDLER( handler, triggerd, state_machine )                                \
do                                                                                         \
{                                                                                          \
    if ( handler != NULL )                                                                 \
    {                                                                                      \
        state_machine_result_t result = handler( state_machine, state_machine->event->id ); \
        switch ( result )                                                                  \
        {                                                                                  \
        case TRIGGERED_TO_SELF:                                                            \
            triggerd = true;                                                               \
        case EVENT_HANDLED:                                                                \
            break;                                                                         \
                                                                                           \
        default:                                                                           \
            return result;                                                                 \
        }                                                                                  \
    }                                                                                      \
} while ( 0 )

/*
 *  --------------------- FUNCTION BODY ---------------------
 */

/** \brief dispatch events to state machine
 *
 * \param pState_Machine[] state_machine_t* const  array of state machines
 * \param quantity uint32_t number of state machines
 * \return state_machine_result_t result of state machine
 *
 */
state_machine_result_t dispatch_event(state_machine_t* const pState_Machine[]
                                      ,uint32_t quantity
#if STATE_MACHINE_LOGGER
                                      ,state_machine_event_logger event_logger
                                      ,state_machine_result_logger result_logger
#endif // STATE_MACHINE_LOGGER
                                      )
{
    state_machine_result_t result;

  // Iterate through all state machines in the array to check if event is pending to dispatch.
    for(uint32_t index = 0; index < quantity;)
    {
        event_t* currentEvent  = pState_Machine[index]->event;
        event_t* previousEvent = NULL;

        while( currentEvent != NULL )
        {
            const state_t* pState = pState_Machine[index]->State;

            do
            {
#if STATE_MACHINE_LOGGER
                event_logger(index, pState->Id, pState_Machine[index]->event->id);
#endif // STATE_MACHINE_LOGGER
        // Call the state handler.
                result = pState->Handler(pState_Machine[index], pState_Machine[index]->event->id);
#if STATE_MACHINE_LOGGER
                result_logger(pState_Machine[index]->State->Id, result);
#endif // STATE_MACHINE_LOGGER

                switch(result)
                {
                case EVENT_HANDLED:
                    if (previousEvent == NULL)
                    {
                        /* Remove the event from the queue */
                        pState_Machine[index]->event = currentEvent->next;
                    }
                    else
                    {
                        /* Bypass the current event in the queue */
                        previousEvent->next = currentEvent->next;
                    }

                    /* Free the memory allocated for the event */
                    event_t* temp = currentEvent;
                    currentEvent  = currentEvent->next;
                    free( temp );

                    // intentional fall through

                    // State handler handled the previous event successfully,
                    // and posted a new event to itself.
                case TRIGGERED_TO_SELF:

                index = 0;  // Restart the event dispatcher from the first state machine.
                break;

                #if HIERARCHICAL_STATES
                // State handler could not handled the event.
                // Traverse to its parent state and dispatch event to parent state handler.
                case EVENT_UN_HANDLED:

                do
                {
                // check if state has parent state.
                if(pState->Parent == NULL)   // Is Node reached top
                {
                    // This is a fatal error. terminate state machine.
                    return EVENT_UN_HANDLED;
                }

                pState = pState->Parent;        // traverse to parent state
                } while(pState->Handler == NULL);   // repeat again if parent state doesn't have handler
                continue;
#endif // HIERARCHICAL_STATES

                // Either state handler could not handle the event or it has returned
                // the unknown return code. Terminate the state machine.
                default:
                    previousEvent = currentEvent;
                    currentEvent  = currentEvent->next;
                }
                break;

            } while(1);
        }

        index++;
        continue;
    }
    return result;
}

/** \brief Switch to target states without traversing to hierarchical levels.
 *
 * \param pState_Machine state_machine_t* const   pointer to state machine
 * \param pTarget_State const state_t* const      Target state to traverse
 * \return extern state_machine_result_t          Result of state traversal
 *
 */
extern state_machine_result_t switch_state(state_machine_t* const pState_Machine,
                                           const state_t* const pTarget_State)
{
  const state_t* const pSource_State = pState_Machine->State;
  bool triggered_to_self = false;
  pState_Machine->State = pTarget_State;    // Save the target node

  // Call Exit function before leaving the Source state.
    EXECUTE_HANDLER(pSource_State->Exit, triggered_to_self, pState_Machine);
  // Call entry function before entering the target state.
    EXECUTE_HANDLER(pTarget_State->Entry, triggered_to_self, pState_Machine);

  if(triggered_to_self == true)
  {
    return TRIGGERED_TO_SELF;
  }

  return EVENT_HANDLED;
}

#if HIERARCHICAL_STATES
/** \brief Traverse to target state. It calls exit functions before leaving
      the source state & calls entry function before entering the target state.
 *
 * \param pState_Machine state_machine_t* const   pointer to state machine
 * \param pTarget_State const state_t*            Target state to traverse
 * \return state_machine_result_t                 Result of state traversal
 *
 */
state_machine_result_t traverse_state(state_machine_t* const pState_Machine,
                                              const state_t* pTarget_State)
{
  const state_t *pSource_State = pState_Machine->State;
  bool triggered_to_self = false;
  pState_Machine->State = pTarget_State;    // Save the target node

#if (HSM_USE_VARIABLE_LENGTH_ARRAY == 1)
  const state_t *pTarget_Path[pTarget_State->Level];  // Array to store the target node path
#else
  #if  (!defined(MAX_HIERARCHICAL_LEVEL) || (MAX_HIERARCHICAL_LEVEL == 0))
  #error "MAX_HIERARCHICAL_LEVEL is undefined."\
         "Define the maximum hierarchical level of the state machine or \
          use variable length array by setting HSM_USE_VARIABLE_LENGTH_ARRAY to 1"
  #endif

  const state_t* pTarget_Path[MAX_HIERARCHICAL_LEVEL];     // Array to store the target node path
#endif

  uint32_t index = 0;

  // make the source state & target state at the same hierarchy level.

  // Is source hierarchy level is less than target hierarchy level?
  if(pSource_State->Level > pTarget_State->Level)
  {
    // Traverse the source state to upward,
    // till it matches with target state hierarchy level.
    while(pSource_State->Level > pTarget_State->Level)
    {
      EXECUTE_HANDLER(pSource_State->Exit, triggered_to_self, pState_Machine);
      pSource_State = pSource_State->Parent;
    }
  }
  // Is Source hierarchy level greater than target level?
  else if(pSource_State->Level < pTarget_State->Level)
  {
    // Traverse the target state to upward,
    // Till it matches with source state hierarchy level.
    while(pSource_State->Level < pTarget_State->Level)
    {
      pTarget_Path[index++] = pTarget_State;  // Store the target node path.
      pTarget_State = pTarget_State->Parent;
    }
  }

  // Now Source & Target are at same hierarchy level.
  // Traverse the source & target state to upward, till we find their common parent.
  while(pSource_State->Parent != pTarget_State->Parent)
  {
    EXECUTE_HANDLER(pSource_State->Exit, triggered_to_self, pState_Machine);
    pSource_State = pSource_State->Parent;  // Move source state to upward state.

    pTarget_Path[index++] = pTarget_State;  // Store the target node path.
    pTarget_State = pTarget_State->Parent;    // Move the target state to upward state.
  }

  // Call Exit function before leaving the Source state.
    EXECUTE_HANDLER(pSource_State->Exit, triggered_to_self, pState_Machine);
  // Call entry function before entering the target state.
    EXECUTE_HANDLER(pTarget_State->Entry, triggered_to_self, pState_Machine);

    // Now traverse down to the target node & call their entry functions.
    while(index)
    {
      index--;
      EXECUTE_HANDLER(pTarget_Path[index]->Entry, triggered_to_self, pState_Machine);
    }

  if(triggered_to_self == true)
  {
    return TRIGGERED_TO_SELF;
  }
  return EVENT_HANDLED;
}
#endif // HIERARCHICAL_STATES


/** \brief Push event to the event queue
 *
 * \param head event_t* head of the event queue
 * \param event uint32_t event to be pushed
 */
void pushEvent( event_t** head, uint32_t event )
{
    /* Check if head is empty */
    if ( *head == NULL )
    {
        *head           = (event_t*) malloc( sizeof( event_t ) );
        ( *head )->id   = event;
        ( *head )->next = NULL;
        return;
    }

    event_t* current = *head;

    while ( current->next != NULL )
    {
        current = current->next;
    }

    /* Now we can add a new variable */
    current->next = (event_t*) malloc( sizeof( event_t ) );

    /* Check if memory allocation was successful */
    if ( current->next == NULL )
    {
        return;
    }

    current->next->id   = event;
    current->next->next = NULL;
}
