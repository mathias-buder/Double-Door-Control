#include <Arduino.h>
#include <TimerOne.h>

/****************************************************************************************/
/*                                    PIN KONFIGURATION                                 */
/****************************************************************************************/
/* TÜR 1 */
/****************************************************************************************/
#define RBG_LED_1_R       5   /*!< Pin für die rote LED der RGB-LED */
#define RBG_LED_1_G       6   /*!< Pin für die grüne LED der RGB-LED */
#define RBG_LED_1_B       7   /*!< Pin für die blaue LED der RGB-LED */

#define TUER_1_TASTER     2   /*!< Pin für den Taster an Tür 1 */
#define TUER_1_SCHALTER   3   /*!< Pin für den Schalter an Tür 1 */
#define TUER_1_MAGNET     4   /*!< Pin für den Magnetschalter an Tür 1 */

/****************************************************************************************/
/* TÜR 2 */
/****************************************************************************************/
#define RBG_LED_2_R       8   /*!< Pin für die rote LED der RGB-LED */
#define RBG_LED_2_G       9   /*!< Pin für die grüne LED der RGB-LED */
#define RBG_LED_2_B       10  /*!< Pin für die blaue LED der RGB-LED */

#define TUER_2_TASTER     11  /*!< Pin für den Taster an Tür 2 */
#define TUER_2_SCHALTER   12  /*!< Pin für den Schalter an Tür 2 */
#define TUER_2_MAGNET     13  /*!< Pin für den Magnetschalter an Tür 2 */

/****************************************************************************************/


/**
 * @brief Zustände der Steuerung
 */
typedef enum {
  ZUSTAND_TYP_INIT,       /*!< Initialisierung */
  ZUSTAND_TYP_FEHLER,     /*!< Fehlerzustand */
  ZUSTAND_TYP_LEERLAUF,   /*!< Leerlauf */
  ZUSTAND_TYP_TUER_1_AUF, /*!< Tür 1 ist auf */
  ZUSTAND_TYP_TUER_2_AUF, /*!< Tür 2 ist auf */
  ZUSTAND_TYP_ANZAHL      /*!< Anzahl der Zustände */
} Zustand_typ_t;

/**
 * @brief Events der Steuerung
 */
typedef enum {
  EVENT_LEERLAUF,     /*!< Leerlauf, im aktuellen Zustand verbleiben */
  EVENT_TUER_1_AUF,   /*!< Tür 1 ist auf */
  EVENT_TUER_2_AUF,   /*!< Tür 2 ist auf */
  EVENT_TUER_1_2_ZU,  /*!< Tür 1 und Tür 2 sind zu */
  EVENT_TUER_1_2_AUF, /*!< Tür 1 und Tür 2 sind auf */
  EVENT_ANZAHL        /*!< Anzahl der Events */
} Event_t;

typedef void (*ZustandsRoutine)();

/**
 * @brief Struktur für die Zustandsmaschine
 */
typedef struct {
  Zustand_typ_t   zustand;  /*!< Enthält den aktuellen Zustand */
  ZustandsRoutine routine;  /*!< Enthält die Routine des Zustands */
} Zustand_t;

// General functions
void evaluate_state(Event e);

// State routines to be executed at each state
void state_routine_a(void);
void state_routine_b(void);
void state_routine_c(void);

// Defining each state with associated state routine
const State state_a = {STATE_A, state_routine_a};
const State state_b = {STATE_B, state_routine_b};
const State state_c = {STATE_C, state_routine_c};

// Defning state transition matrix as visualized in the header (events not
// defined, result in mainting the same state)
State state_transition_mat[STATE_SIZE][EVENT_SIZE] = {
    {state_a, state_b, state_a, state_a},
    {state_b, state_b, state_c, state_b},
    {state_c, state_c, state_c, state_a}};

// Define current state and initialize
State current_state = state_a;


int ledState = LOW;
void blinkLed() {
  ledState = !ledState;
  digitalWrite(RBG_LED_1_R, ledState);
}



/**
 * @brief Setup Funktion
 * @details Wird einmalig beim Start des Programms ausgeführt und initialisiert
 * die Steuerung
 */
void setup() {

  Serial.begin(115200);
  Serial.println("Initialisiere Steuerung");

  digitalWrite(RBG_LED_1_R, HIGH);
  digitalWrite(RBG_1_G, HIGH);
  digitalWrite(RBG_1_B, HIGH);

  Timer1.initialize(500000); // The led will blink in a half second time
                             // interval
  Timer1.attachInterrupt(blinkLed);
}

/* ***************************************************************************************
 *                                     HAUPTPROGRAMM
 * **************************************************************************************/
void loop() {}

/*
 * Determine state based on event and perform state routine
 */
void evaluate_state(Event ev) {
  // Determine state based on event
  current_state = state_transition_mat[current_state.id][ev];
  // Run state routine
  (*current_state.routine)();
}

/*
 * State routines
 */
void state_routine_a() { Serial.println("State A routine ran."); }
void state_routine_b() { Serial.println("State B routine ran."); }
void state_routine_c() { Serial.println("State C routine ran."); }