//Poly Rhytmic Clock Divider and Multiplier
//By Apoc Pro
/*This code is designed to run on an Arduino and generate multiple clock outputs based on a single clock input, 
allowing for polyrhythmic patterns. It reads the timing of incoming clock signals, calculates a stable BPM, and 
generates output clocks according to user-defined time signatures set by potentiometers. The code also includes a 
reset function and a debugging mode for monitoring system performance.*/

/* Debugging Parameter */

const bool DEBUG = true;   //Set to true and reflash arduino to check from problems in the Serial window

#include "variables.h"

void setup() {
  if (DEBUG) {
    Serial.begin(9600);
    delay(1000);  // Give serial time to initialize
    Serial.println("\n\n========================================");
    Serial.println("    POLY-CLOCK DEBUG MODE ENABLED");
    Serial.println("========================================");
    Serial.print("Queue size: ");
    Serial.println(queueSize);
    Serial.print("BPM change threshold: ");
    Serial.println(bpmChangeThreshold);
    Serial.println("Waiting for clock input...");
    Serial.println("========================================\n");
  }
  
  pinMode(clockInputPin, INPUT);                         //input clocks
  pinMode(resetPin, INPUT);                              //reset cv signal
  
  for (int i=0; i<4; i++){
    pinMode(clockOutput[i], OUTPUT);                     //output clocks
    digitalWrite(clockOutput[i], LOW);                   //initialise clock state
  }
  
  for (int i=0; i<4; i++){
    pinMode(potPin[i], INPUT);                         //Set Potentiometer Pins 
  }
  
  pinMode(cvPinInput, INPUT);                          //cv inputs for clock shift input and output
  pinMode(cvPinOutput, INPUT);

  // Interrupts for clock and reset
  attachInterrupt(digitalPinToInterrupt(clockInputPin), isrClock, RISING);
  attachInterrupt(digitalPinToInterrupt(resetPin), isrReset, RISING);
}

void loop() {
  // Read potentiometer settings
  if (currentMode == EXTERNAL) {
    // External mode: A0 controls input time signature
    for (int i=0; i<4; i++){
      timeSignature[i] = readTimeSignature(potPin[i]);
    }
  } else {
    // Internal mode: A0 controls BPM, A1-A3 control polyrhythm ratios
    // Read BPM from A0 every loop to keep responsive
    int bpmPot = analogRead(potPin[0]);
    internalBPM = map(bpmPot, 0, 1023, 60, 240);  // Map to 60-240 BPM range
    internalClockPeriod = 60000000.0 / internalBPM;  // Convert BPM to microseconds per beat
    
    // Set input time signature to 4 (standard 4/4) for internal mode
    timeSignature[0] = 4;
    
    // Read output polyrhythm ratios from A1-A3 (less frequently to reduce load)
    static unsigned long lastPotRead = 0;
    if (millis() - lastPotRead >= 100) {  // Read every 100ms
      lastPotRead = millis();
      for (int i=1; i<4; i++){
        timeSignature[i] = readTimeSignature(potPin[i]);
      }
    }
    
    // Generate internal clock pulses
    unsigned long now = micros();
    if (now - lastInternalClock >= internalClockPeriod) {
      lastInternalClock = now;
      
      // Trigger clock processing
      clkState = HIGH;
      isrClockCount++;
      waitingForClockInput = false;  // Mark as active
      
      if (DEBUG && (isrClockCount % 10 == 0)) {  // Print every 10th clock to reduce spam
        Serial.print("Internal clock pulse #");
        Serial.print(isrClockCount);
        Serial.print(" - BPM: ");
        Serial.print(internalBPM, 1);
        Serial.print(", Period: ");
        Serial.print(internalClockPeriod);
        Serial.println(" us");
      }
    }
    
    // Set stable clock period for internal mode (used by clockOut)
    stableClockPeriod = internalClockPeriod;
  }

  // Handle reset button - detect short press (reset) vs long press (mode toggle)
  if (resetGate == HIGH && !resetProcessed){
    // Check if reset pin is still high (button still pressed)
    if (digitalRead(resetPin) == HIGH) {
      unsigned long pressDuration = millis() - resetPressTime;
      
      // Long press detected - toggle mode
      if (pressDuration >= LONG_PRESS_TIME) {
        resetProcessed = true;
        resetGate = LOW;
        
        // Toggle clock mode
        if (currentMode == EXTERNAL) {
          currentMode = INTERNAL;
          if (DEBUG) {
            Serial.println("\n****************************************");
            Serial.println("   SWITCHED TO INTERNAL CLOCK MODE");
            Serial.println("   A0 pot now controls BPM (60-240)");
            Serial.println("****************************************\n");
          }
        } else {
          currentMode = EXTERNAL;
          if (DEBUG) {
            Serial.println("\n****************************************");
            Serial.println("   SWITCHED TO EXTERNAL CLOCK MODE");
            Serial.println("   A0 pot controls input time signature");
            Serial.println("   Waiting for external clock...");
            Serial.println("****************************************\n");
          }
        }
        
        // Flash output 4 LED to indicate mode change
        for (int i=0; i<3; i++) {
          digitalWrite(clockOutput[3], HIGH);
          delay(100);
          digitalWrite(clockOutput[3], LOW);
          delay(100);
        }
      }
    } else {
      // Button released - check if it was a short press (normal reset)
      unsigned long pressDuration = millis() - resetPressTime;
      
      if (pressDuration < LONG_PRESS_TIME && !resetProcessed) {
        resetProcessed = true;
        resetGate = LOW;
        
        if (DEBUG) {
          Serial.println("\n========== RESET EVENT ==========");
          Serial.print("Reset triggered at: ");
          Serial.print(millis());
          Serial.println(" ms");
          Serial.print("Clock interrupts received: ");
          Serial.println(isrClockCount);
          Serial.print("Previous stable period: ");
          Serial.print(stableClockPeriod);
          Serial.println(" us");
          Serial.println("================================\n");
        }
        
        // Perform reset
        for (int i=0; i<queueSize; i++){
          clockQueue[i] = 0;
        }
        currentStep = 0;
        queueReady = 0;
        stableClockPeriod = 0;
        lastBPM = 0.0;
        previousClock = 0;
        lastInternalClock = 0;
        waitingForClockInput = true;
        
        // Reset all output states
        for (int i=0; i<4; i++){
          digitalWrite(clockOutput[i], LOW);
        }
      }
    }
  }
  
  // Reset the processed flag when button is released
  if (resetGate == LOW && digitalRead(resetPin) == LOW) {
    resetProcessed = false;
  }
  
  // Process clock signal (external or internally generated)
  if (clkState == HIGH){
    clkState = LOW;
    
    if (currentMode == EXTERNAL) {
      unsigned long now = micros();
      
      // Log state transition
      if (waitingForClockInput && DEBUG) {
        Serial.println("\n>>> STATE CHANGE: First external clock received, system active");
      }
      waitingForClockInput = false;
      
      if (previousClock != 0){
        // store measured period
        unsigned long measuredPeriod = now - previousClock;
        clockQueue[currentStep] = measuredPeriod;
        
        if (DEBUG) {
          Serial.print("Clock pulse #");
          Serial.print(isrClockCount);
          Serial.print(" - Period: ");
          Serial.print(measuredPeriod);
          Serial.print(" us (");
          Serial.print(60000000.0 / measuredPeriod);
          Serial.println(" BPM)");
        }
        
        currentStep++;
        if (currentStep >= queueSize) {
          currentStep = 0;
          queueReady = 1;
        }
      }
      // update last timestamp for next measurement
      previousClock = now;
      
      // Calculate stable clock timing after collecting enough samples
      if (queueReady == 1) {
      // Calculate average period from queue
      unsigned long totalPeriod = 0;
      for (int i = 0; i < queueSize; i++) {
        totalPeriod += clockQueue[i];
      }
      unsigned long avgPeriod = totalPeriod / queueSize;
      
      // Convert to BPM (60,000,000 microseconds per minute / period in microseconds)
      float currentBPM = 60000000.0 / (float)avgPeriod;
      
      if (DEBUG) {
        Serial.println("\n--- Timing Analysis ---");
        Serial.print("Queue contents: [");
        for (int i = 0; i < queueSize; i++) {
          Serial.print(clockQueue[i]);
          if (i < queueSize - 1) Serial.print(", ");
        }
        Serial.println("] us");
        Serial.print("Average period: ");
        Serial.print(avgPeriod);
        Serial.print(" us, Current BPM: ");
        Serial.println(currentBPM, 2);
      }
      
      // Only update stable timing if change exceeds threshold or first calculation
      if (stableClockPeriod == 0 || abs(currentBPM - lastBPM) >= bpmChangeThreshold) {
        stableClockPeriod = avgPeriod;
        lastBPM = currentBPM;
        
        if (DEBUG) {
          Serial.println("*** TIMING LOCK UPDATED ***");
          Serial.print("Stable Period: ");
          Serial.print(stableClockPeriod);
          Serial.print(" us, Stable BPM: ");
          Serial.print(currentBPM, 2);
          Serial.print(" (Change: ");
          Serial.print(abs(currentBPM - lastBPM), 2);
          Serial.println(" BPM)");
        }
      } else if (DEBUG) {
        Serial.print("Timing stable (change: ");
        Serial.print(abs(currentBPM - lastBPM), 2);
        Serial.print(" BPM < threshold: ");
        Serial.print(bpmChangeThreshold, 2);
        Serial.println(" BPM)");
      }
      
      if (DEBUG) {
        Serial.println("----------------------\n");
      }
    }
    // End of external mode clock processing
    }
  }
  
  // Read CV input for time signature modulation (external mode only)
  if (currentMode == EXTERNAL) {
    inputTimeSignatureCV = readTimeSignature(cvPinInput);
  }
  
  // Generate output clocks based on polyrhythm ratios
  for (int i = 0; i < 3; i++) {
    clockOut(i);
  }
  
  // Periodic debug summary every 5 seconds
  loopCount++;
  if (DEBUG && (millis() - lastDebugReport >= 5000)) {
    lastDebugReport = millis();
    Serial.println("\n====== DEBUG SUMMARY ======");
    Serial.print("Clock mode: ");
    Serial.println(currentMode == EXTERNAL ? "EXTERNAL" : "INTERNAL");
    if (currentMode == INTERNAL) {
      Serial.print("Internal BPM: ");
      Serial.println(internalBPM, 1);
    }
    Serial.print("System uptime: ");
    Serial.print(millis() / 1000);
    Serial.println(" seconds");
    Serial.print("Clock ISR count: ");
    Serial.println(isrClockCount);
    Serial.print("Reset ISR count: ");
    Serial.println(isrResetCount);
    Serial.print("Loop iterations: ");
    Serial.println(loopCount);
    Serial.print("System state: ");
    Serial.println(waitingForClockInput ? "WAITING" : "ACTIVE");
    if (currentMode == EXTERNAL) {
      Serial.print("Queue ready: ");
      Serial.println(queueReady ? "YES" : "NO");
    }
    Serial.print("Stable period: ");
    Serial.print(stableClockPeriod);
    Serial.println(" us");
    Serial.print("Time signatures: [");
    for (int i = 0; i < 4; i++) {
      Serial.print(timeSignature[i]);
      if (i < 3) Serial.print(", ");
    }
    Serial.println("]);
    Serial.println("==========================\n");
  }
}

/* interrupt handlers */
void isrClock()
{
  clkState = HIGH;
  isrClockCount++;  // Track interrupt calls
}

void isrReset()
{
  resetGate = HIGH;
  resetPressTime = millis();  // Capture when button was pressed
  isrResetCount++;  // Track reset calls
}

void clockOut(int outputIndex)
{
  if (outputIndex < 0 || outputIndex >= 3) return;  // Only 3 outputs (indices 0-2)
  
  // Only output clocks if we have a stable clock period
  if (stableClockPeriod == 0) return;
  
  // Prevent division by zero
  if (timeSignature[0] == 0 || timeSignature[outputIndex + 1] == 0) return;
  
  unsigned long now = micros();
  static unsigned long lastOutputTime[3] = {0, 0, 0};
  static bool outputState[3] = {false, false, false};
  const unsigned long gatePulseWidth = 10000;  // 10ms gate pulse width
  
  // Calculate output period based on polyrhythm ratio
  // Example: if input is 4/4 and output is 3/4, output will trigger every (stableClockPeriod * 4) / 3
  unsigned long outputPeriod = (stableClockPeriod * timeSignature[0]) / timeSignature[outputIndex + 1];
  
  // Turn off gate after pulse width expires
  if (outputState[outputIndex] && (now - lastOutputTime[outputIndex] >= gatePulseWidth)) {
    outputState[outputIndex] = false;
    digitalWrite(clockOutput[outputIndex], LOW);
  }
  
  // Check if it's time to trigger a new pulse
  if (!outputState[outputIndex] && (now - lastOutputTime[outputIndex] >= outputPeriod)) {
    outputState[outputIndex] = true;
    digitalWrite(clockOutput[outputIndex], HIGH);
    lastOutputTime[outputIndex] = now;
    
    if (DEBUG) {
      Serial.print("Output ");
      Serial.print(outputIndex + 1);
      Serial.print(" pulse - Period: ");
      Serial.print(outputPeriod);
      Serial.print(" us, Ratio: ");
      Serial.print(timeSignature[0]);
      Serial.print("/");
      Serial.println(timeSignature[outputIndex + 1]);
    }
  }
}

// Function to read potentiometer and return time signature value
// Reads potentiometer value and returns corresponding time signature
// A0 (potPin[0]): Input clock time signature division
// A1-A3 (potPin[1-3]): Output clock polyrhythm ratios relative to input
int readTimeSignature(int pin)
{
  int pot;                     //holds value from whichever potentiometer is being read
  pot = analogRead(pin);
  
  if (DEBUG) {                   //troubleshooting - identify which pot
    if (pin == A0)
      Serial.print("A0 Input Time Sig");
    else if (pin == A1)
      Serial.print("A1 Output 1");
    else if (pin == A2)
      Serial.print("A2 Output 2");
    else if (pin == A3)
      Serial.print("A3 Output 3");
    else if (pin == cvPinInput)
      Serial.print("CV Input");
    else if (pin == cvPinOutput)
      Serial.print("CV Output");
    Serial.print(" = ");
    Serial.print(pot);
  }
  
  // Map analog reading (0-1023) to time signature values
  if (pot < 93)
  {
    if (DEBUG) Serial.println(" -> 1/4");
    return 1;    //1/4 time signature
  }
  else if (pot >= 93 and pot < 186)
  {
    if (DEBUG) Serial.println(" -> 2/4");
    return 2;    //2/4 time signature
  }
  else if (pot >= 186 and pot < 279)
  {
    if (DEBUG) Serial.println(" -> 3/4");
    return 3;    //3/4 time signature
  }
  else if (pot >= 279 and pot < 372)
  {
    if (DEBUG) Serial.println(" -> 4/4");
    return 4;    //4/4 time signature
  }
  else if (pot >= 372 and pot < 465)
  {
    if (DEBUG) Serial.println(" -> 5/4");
    return 5;    //5/4 time signature
  }
  else if (pot >= 465 and pot < 558)
  {
    if (DEBUG) Serial.println(" -> 6/4");
    return 6;    //6/4 time signature
  }
  else if (pot >= 558 and pot < 651)
  {
    if (DEBUG) Serial.println(" -> 7/4");
    return 7;    //7/4 time signature
  }
  else if (pot >= 651 and pot < 744)
  {
    if (DEBUG) Serial.println(" -> 9/4");
    return 9;    //9/4 time signature
  }
  else if (pot >= 744 and pot < 837)
  {
    if (DEBUG) Serial.println(" -> 11/4");
    return 11;    //11/4 time signature
  }
  else if (pot >= 837 and pot < 930)
  {
    if (DEBUG) Serial.println(" -> 12/4");
    return 12;    //12/4 time signature
  }
  else
  {
    if (DEBUG) Serial.println(" -> 13/4");
    return 13;    //13/4 time signature
  }
  
}
