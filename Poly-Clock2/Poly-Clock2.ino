               /* Debugging Parameter */

const bool DEBUG = true;        //Set to true and reflash arduino to check from problems in the Serial window

#include "variables.h"

void setup() {
  if (DEBUG) Serial.begin(9600);                //used for troubleshooting 
  
  pinMode(clockInputPin, INPUT);                //input clocks
  pinMode(resetPin, INPUT);                     //reset cv signal
  
  pinMode(downBeat, OUTPUT);
  digitalWrite(downBeat, LOW);
  
  for (int i=0; i<4; i++){
    pinMode(clockOutput[i], OUTPUT);            //output clocks
    digitalWrite(clockOutput[i], LOW);          //initialise clock state
  }
  
  for (int i=0; i<4; i++){
    pinMode(potPin[i], INPUT);                  //Set Potentiometer Pins 
  }
  
  pinMode(baseTimePot, INPUT);
  pinMode(cvPinInput, INPUT);                   //cv inputs for clock shift input

  // Interrupts for clock and reset
  attachInterrupt(digitalPinToInterrupt(clockInputPin), isrClock, RISING);
  attachInterrupt(digitalPinToInterrupt(resetPin), isrReset, RISING);
}

void loop() {

/* We can initialise the clock to function without a clock input.  
 * This way when turned on we can see that it functions and that 
 * there is still function without external clock input */

  while (waitingForClockInput){
    //create a loop of for internal clock based on BPM of the input Pot1

    if(!waitingForClockInput){
      break;
    }
  }
  
  if (resetGate == HIGH){                                        //reset the clock
    resetGate == LOW;
    baseTimeSignature = ((analogRead(baseTimePot) >> 6) + 1);    //bitshifting the base time signature pot changes the range from 0 - 1024, to 0 - 15, + 1 = 1 - 16
  }

/* Each time the input clock state goes high the baseTimeSignature is decremented.  
 * This works as a clock divider based on the baseTimeSignature pot.
 * When baseTimeSignature reaches 0 a clock pulse is sent to every clock and the 
 * clock queue is incremented with updated values.  This means that changes
 * will only occur on a downbeat.
 */
 
  if (clkState == HIGH){
    clkState = LOW;                                              //reset for the next clock
    baseTimeSignature --;                                        //decrement count for input clock divider
    if (DEBUG) Serial.print(baseTimeSignature);
    if (baseTimeSignature < 1){                                  //when count reaches 0 perform downbeat gates and calculations
      downBeatState = HIGH;
      previousDownBeatClock = millis();
      digitalWrite(downBeat, HIGH);
      baseTimeSignature = ((analogRead(baseTimePot) >> 6) + 1);
      if (previousClock != 0){                                   //make sure this is not the first ever downbeat, if it is skip to just record millis and then loop
        clockQueue[currentStep] = millis() - previousClock;
        currentStep++;

        if (currentStep >= queueSize) {                          //build up the queue so there is enough pulses to compare
          currentStep = 0;
          queueReady = 1;
        }

        if (queueReady == 1){                                    //average clock pulses in the queue to prevent jitter
          long avgTime = 0;
          for (int i=0; i<queueSize; i++){
            avgTime += clockQueue[i];
          }
          avgTime = avgTime / queueSize;                         //this idea may cause issues when the baseTimeSignature is changed as it would average a 4/4 with 3/4 for one measure

         for (int i=0; i<numOutputs; i++){
            timeSignature[i] = (avgTime /(analogRead(potPin[i]) >> 6) +1);   //this creates the timing for all the output clock pulses by dividing the downbeat by the chosen ratio selected on each pot
            if (DEBUG) Serial.println(timeSignature[i]);          //used to troubleshoot if there are issues
            outputState[i] = HIGH;                                //every clock goes high on the downbeat
            currentTime[i] = millis();
            digitalWrite(clockOutput[i], outputState[i]);
        }
      }
    }
    previousClock = millis();
    }
    
/* turn off the downbeat triggers */
  if ((downBeatState == HIGH) && ((millis() - previousDownBeatClock) > clockTrig)){
    downBeatState = LOW;
    digitalWrite(downBeat, LOW);
    for (int i=0; i<numOutputs; i++){
      outputState[i] = LOW;                                     //every clock goes high on the downbeat
      currentTime[i] = millis();
      digitalWrite(clockOutput[i], outputState[i]);
    }
   }
  }
  if (queueReady){                                              //checking the elapsed time for clock multipliers
    for (int i=0; i<numOutputs; i++){
      if (millis() - currentTime[i] > timeSignature[i]){
        currentTime[i] = millis();
        if (outputState[i] != HIGH){
          outputState[i] = HIGH;
          digitalWrite(clockOutput[i], outputState[i]);
        }
      }
    }
    for (int i=0; i<numOutputs; i++){
      if ((outputState[i] == HIGH) && (millis() - currentTime[i] > clockTrig)){
        outputState[i] = LOW;
        digitalWrite(clockOutput[i], outputState[i]);
      }
    }
  }
}

/* interrupt handlers */
void isrClock()
{
  if(waitingForClockInput) {
    waitingForClockInput = 0;
  }
  clkState = HIGH;
}

void isrReset()
{
  resetGate = HIGH;
}
