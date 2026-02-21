#include <Arduino.h>

/* Pin numbers */
const int clockInputPin = 2;   //gate/trigger input for clock
const int resetPin = 3;       //gate/trigger reset cv
const int clockOutput[4] = {4, 5, 6, 7};    //output digital gates for clock 
const int potPin[4] = {A0, A1, A2, A3};     //analog pin numbers for Potentiometer
const int cvPinInput = A4;      //analog control voltage for Input Time Signature
const int cvPinOutput = A5;     //analog cv for output Time Signature

/* Constants */
const int queueSize = 2;        //size of the timing queue, how many clock signals do we collect before changing
const float bpmChangeThreshold = 1.0;  //minimum BPM change required to update timing (stability threshold)

/* ISR variable */
volatile int clkState = LOW;    //handles the interrupt for checking the clock
volatile int resetGate = LOW;

/* Clocking variables */
unsigned long previousClock = 0;   //last time micros() was called
unsigned long clockQueue[queueSize];  //store the previous and current clock pulses (based on queueSize)
int currentStep = 0;
int queueReady = 0;
bool waitingForClockInput = true;
unsigned long stableClockPeriod = 0;  //stable clock period in microseconds (only updates if BPM change > threshold)
float lastBPM = 0.0;                  //last calculated BPM for stability checking

/* Potentiometer and CV Variables */
int timeSignature[4];           //user input for time signatures
float inputTimeSignatureCV;     //CV input for the base time signature

/* Debug counters */
volatile unsigned long isrClockCount = 0;    //count clock interrupts
volatile unsigned long isrResetCount = 0;    //count reset interrupts
unsigned long loopCount = 0;                 //count main loop iterations
unsigned long lastDebugReport = 0;           //timestamp for periodic debug reports

/* Clock Mode System */
enum ClockMode { EXTERNAL, INTERNAL };
ClockMode currentMode = EXTERNAL;           //default to external clock mode

/* Internal Clock Variables */
float internalBPM = 120.0;                   //internal clock BPM (60-240 range)
unsigned long internalClockPeriod = 500000;  //120 BPM = 500000 microseconds
unsigned long lastInternalClock = 0;         //last time we generated internal clock

/* Reset Button Long Press Detection */
volatile unsigned long resetPressTime = 0;   //when reset button was pressed
const unsigned long LONG_PRESS_TIME = 1000;  //1 second for mode toggle
bool resetProcessed = false;                 //prevent multiple triggers