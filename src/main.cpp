#include <Arduino.h>
#include <TimerOne.h>

#define RBG_1_R         5
#define RBG_1_G         6
#define RBG_1_B         7

#define TASTER_1        2
#define TUERSCHALTER_1  3
#define TUERMAGNET_1    4

int ledState = LOW;

void blinkLed(){
  ledState = !ledState;
  digitalWrite(RBG_1_R, ledState);
}

void setup() {
  digitalWrite(RBG_1_R, HIGH);
  digitalWrite(RBG_1_G, HIGH);
  digitalWrite(RBG_1_B, HIGH);

  Timer1.initialize(500000); //The led will blink in a half second time interval
  Timer1.attachInterrupt(blinkLed);
}




void loop() {
  
  if( digitalRead(TUERSCHALTER_1) == HIGH )
  {
    digitalWrite(TUERMAGNET_1, HIGH);
  }
  else
  {
    digitalWrite(TUERMAGNET_1, LOW);
  }

  if(digitalRead(TASTER_1) == HIGH)
  {
    digitalWrite(RBG_1_G, HIGH);
    digitalWrite(RBG_1_B, LOW);
  }
  else
  {
    digitalWrite(RBG_1_G, LOW);
    digitalWrite(RBG_1_B, HIGH);
  }
}