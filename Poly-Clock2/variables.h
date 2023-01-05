/* Constants */
const int queueSize = 2;                              //size of the timing queue, how many clock signals do we collect before changing, the longer the queue the slower the reaction, but the stabler the clock
const int clockTrig = 20;                             //default 10 ms clock trigger on startup
const int numOutputs = 4;                             //if these numbers are changed the pin numbers need to be changed as well
const int numOutputPots = 4;

/* Pin numbers */
const int clockInputPin = 2;                          //gate/trigger input for clock
const int resetPin = 3;                               //gate/trigger reset cv
const int baseTimePot = A0;
const int downBeat = 8;                               //output gate for the downbeat of all clocks
const int clockOutput[numOutputs] = {4, 5, 6, 7};     //output digital gates for clock 
const int potPin[numOutputPots] = {A1, A2, A3, A4};   //analog pin numbers for Potentiometer A0 = 14, A1 = 15 etc.
const int cvPinInput = A5;                            //this will be used in the future to create a shuffle effect

/* ISR variable */
volatile int clkState = LOW;                          //handles the interrupt for checking the clock
volatile int resetGate = LOW;

// old clock idea  unsigned long clockPulse;          //stores pulseFull for computing output clock
  
/* Clocking variables */
unsigned long previousClock = 0;                      //this is the clock for the output multiplierslast time millis() was called (micros() resets after 71 minutes, so we have to use millis)
unsigned long previousDownBeatClock = 0;              //this is the clock for the input divider
unsigned long clockQueue[queueSize];                  //store the previous and current clock pulses (based on queueSize)
int currentStep = 0;                                  //what step of the queue we are on
int queueReady = 0;                                   //wait until currentStep fills up the clockQueue before determining multipliers.  We need 2 downbeats before the clock will change
bool waitingForClockInput = true;                     //creates the power on state waiting for the input clock to be attached

/* Potentiometer and CV Variables */
int baseTimeSignature = 1;                            //what time signature is your base clock in, this number decrements according to the baseTimePot, but reading only happens when this number hits 0 so it initializes to 1
int timeSignature[numOutputs];                        //what is the desired clock speed of the output time signatures
int currentTime[numOutputs];                          //record the time each output is triggered
// this will change for shuffle effect float inputTimeSignatureCV;     //CV input for the base time signature

/* Gate variables for output */
int downBeatState = LOW;                              //store the state of the downbeat
int outputState[numOutputs] = {LOW, LOW, LOW, LOW};   //Gate states for the outputs
