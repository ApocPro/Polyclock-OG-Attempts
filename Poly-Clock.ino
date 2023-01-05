/* Debugging Parameter */

const bool DEBUG = true;   //Set to true and reflash arduino to check from problems in the Serial window

/* Pin numbers */
const int clockInputPin = 2;   //gate/trigger input for clock
const int resetPin = 3;       //gate/trigger reset cv
const int clockOutput1 = 4;    //output digital gates for clock Input divided
const int clockOutput2 = 5;    //output digital gates for all 3 clocks
const int clockOutput3 = 6;
const int clockOutput4 = 7;
const int potPinInput = A0;     //analog pin number for Input Potentiometer
const int potPin1 = A1;         //analog pin numbers for Output Potentiometers
const int potPin2 = A2;
const int potPin3 = A3;
const int cvPinInput = A4;      //analog control voltage for Input Time Signature
//const int cvPinOutput = A5;     //analog cv for output Time Signature

/* Variables */
unsigned long clockPulse;       //stores pulseFull for computing output clock
int inputTimeSignature;         //determining what the base time signture should be.
int outputTimeSignature1;       //Time Signatures for each output
int outputTimeSignature2;
int outputTimeSignature3;
int outputTimeSignature4;
float inputTimeSignatureCV;
float outputTimeSignatureCV;
bool resetGate;
unsigned long outputClock1;     //Time in microseconds for each clock
unsigned long outputClock2;
unsigned long outputClock3;
unsigned long outputClock4;

void setup() {
  if (DEBUG) Serial.begin(9600);            //used for troubleshooting 
  
  pinMode(clockInputPin, INPUT);            //input clocks
  pinMode(resetPin, INPUT);                 //reset cv signal
  
  pinMode(clockOutput1, OUTPUT);            //output clocks
  pinMode(clockOutput2, OUTPUT);
  pinMode(clockOutput3, OUTPUT);
  pinMode(clockOutput4, OUTPUT);
  
  pinMode(potPinInput, INPUT);        //Potentiometer Pin for base time signature
  pinMode(potPin1, INPUT);            //Potentiometer for Output Clocks
  pinMode(potPin2, INPUT);
  pinMode(potPin3, INPUT);
  
  pinMode(cvPinInput, INPUT);         //cv inputs for clock shift input and output
  //pinMode(cvPinOutput, INPUT);
}

void loop() {
  inputTimeSignature = readTimeSignature(potPinInput);          //reading the settings of the potentiometers
  outputTimeSignature1 = readTimeSignature(potPin1);
  outputTimeSignature2 = readTimeSignature(potPin2);
  outputTimeSignature3 = readTimeSignature(potPin3);

  inputTimeSignatureCV = readTimeSignature(cvPinInput);           //check for CV signal and adjust
  //outputTimeSignatureCV = readTimeSignature(cvPinOutput); 
  
  clockPulse = readClock(clockInputPin);                        //reading the clock signal
  
  outputClock1 = actualTime(clockPulse, inputTimeSignature, outputTimeSignature1, inputTimeSignatureCV, outputTimeSignatureCV);   //timing for each clock
  outputClock2 = actualTime(clockPulse, inputTimeSignature, outputTimeSignature2, inputTimeSignatureCV, outputTimeSignatureCV);
  outputClock3 = actualTime(clockPulse, inputTimeSignature, outputTimeSignature3, inputTimeSignatureCV, outputTimeSignatureCV);
}

void clockOut(int Time)
{
  
}
unsigned long actualTime(unsigned long clockIn, int inputSignature, int outputSignature, int cvIn, int cvOut)
{
  long clockOut;
  if (cvIn == 1)
  {
    if (cvOut == 1)
    {
      clockOut = ((clockIn * inputSignature) /outputSignature);
      return clockOut;
    }
    else
    {
       clockOut = ((clockIn * inputSignature) /((outputSignature + cvOut)/2));
      return clockOut;
    }
  }
  else
  {
    if (cvOut == 1)
    {
      clockOut = ((clockIn * (inputSignature + cvIn/2)) /outputSignature);
      return clockOut;
    }
    else
    {
       clockOut = ((clockIn * (inputSignature + cvIn/2)) /((outputSignature + cvOut)/2));
      return clockOut;
    }
  }
}

long readClock(int pin)
{
  unsigned long pulseHigh;       //holds value for how long the pin is held high
  unsigned long pulseLow;        //same for pin held low
  unsigned long pulseFull;       //Full length of the clock signal
  pulseHigh = pulseIn(pin, HIGH);          //reads in µs duration of High pulse
  if (DEBUG) {                             //used for troubleshooting
    Serial.print("High Pulse width = ");    
    Serial.print(pulseHigh);
  }
  pulseLow = pulseIn(pin, LOW);            //read in µs duration of Low pulse
  if (DEBUG){
    Serial.print("Low Pulse width = ");
    Serial.print(pulseLow);
  }
  pulseFull = pulseHigh + pulseLow;
  if (DEBUG){
    Serial.print("Full Pulse width = ");
    Serial.println(pulseFull);
  }
  return pulseFull;
}

int readTimeSignature(int pin)
{
  float pot;                     //holds value from whichever potentiometer is being read
  pot = analogRead(pin);
  if (DEBUG) {                   //troubleshooting
    if (pin == 14)
      Serial.print("Input");
    else if (pin == 15)
      Serial.print("Output 1");
    else if (pin == 16)
      Serial.print("Output 2");
    else if (pin == 17)
      Serial.print("Output 3");
    else if (pin == 18)
      Serial.print("CV input");
    Serial.print(" pot is ");
    Serial.print(pot);
  }
  if (pot < 100)
  {
    if (DEBUG) Serial.println(" Time Signature is matching");        //trouble shooting        
    return 1;    //clock will match input signal
  }
  else if (pot >= 100 and pot < 200)
  {
    if (DEBUG) Serial.println(" Time Signature is 2/4");
    return 2;    //this is the representation for 2/4 or 2/8
  }
  else if (pot >= 200 and pot < 300)
  {
    if (DEBUG) Serial.println(" Time Signature is 3/4");
    return 3;    //this represents 3/4 or 3/8
  }
  else if (pot >= 300 and pot < 400)
  {
    if (DEBUG) Serial.println(" Time Signature is 4/4");
    return 4;    //this represents 4/4 or 4/8
  }
  else if (pot >= 400 and pot < 500)
  {
    if (DEBUG) Serial.println(" Time Signature is 5/4");
    return 5;    //this represents 5/4 or 5/8
  }
  else if (pot >= 500 and pot < 600)
  {
    if (DEBUG) Serial.println(" Time Signature is 6/4");
    return 6;    //this represents 6/4 or 6/8
  }
  else if (pot >= 600 and pot < 700)
  {
    if (DEBUG) Serial.println(" Time Signature is 7/4");
    return 7;    //this represents 7/4 or 7/8
  }
  else if (pot >= 700 and pot < 800)
  {
    if (DEBUG) Serial.println(" Time Signature is 9/4");
    return 9;    //this represents 9/4 or 9/8
  }
  else if (pot >= 800 and pot < 900)
  {
    if (DEBUG) Serial.println(" Time Signature is 11/4");
    return 11;    //this represents 11/4 or 11/8
  }
  else if (pot >= 900 and pot < 990)
  {
    if (DEBUG) Serial.println(" Time Signature is 12/4");
    return 12;    //this represents 12/4 or 12/8
  }
  else
  {
    if (DEBUG) Serial.println(" Time Signature is 13/4");
    return 13;    //this represents 13/4 or 13/8
  }
  
}
