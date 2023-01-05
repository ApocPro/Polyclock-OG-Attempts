/* 
Attempt 3 for creating the poly clock.  I have already split the clock with a 4017
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#define DEBUG true
#define DEBUG_TRIGGERS true
#define TRIGGER_LENGTH 10
#define MULT_POT       A1
#define DIV_POT        A2
#define MODE_SELECT_POT A0
#define CLOCK_IN       2
#define MULT_POT_MAX   500
#define DIV_POT_MAX    500
#define MODE_SELECT_POT_MAX  500
#define NUM_POT_DIVISIONS  4
#define MODE_SIMPLE    0
#define MODE_COMPLEX   1
#define PWM_PIN        4

long variable_trigger_length;
long last_trigger_in = 0;
long last_trigger_out = 0;
long now = 0;
long time_between_ins = 0;
long time_between_outs = 0;
int num_triggs = 0;
int edge_skipper = 1;

boolean in_clock_high = false;
boolean stopped = false;
boolean out_clock_high = false;
boolean edge = false;
boolean getting_triggers = true;

int mode = -1;
float factor = 0;

int simple_factors[16] = {1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768};
int complex_factors[16] = {1,3,5,7,11,13,17,19,23,29,31,37,41,43,51,53};

void setup() {
  //clock_high = false;
  if(DEBUG || DEBUG_TRIGGERS){
    Serial.begin(9600);
  }
}

int slice2factor(int slice, int mode){
  if(mode==MODE_SIMPLE){
    if(DEBUG){
      Serial.print("Simple Factor is: ");
      Serial.println(simple_factors[slice]);
    }
    return simple_factors[slice];
  }
  else{
    if(DEBUG){
      Serial.print("Complex Factor is: ");
      Serial.println(complex_factors[slice]);
    }
    return complex_factors[slice];
  }
}

int get_mult(int mode){
  //int slice = analogRead(MULT_POT)*(NUM_POT_DIVISIONS-1)/MULT_POT_MAX;
  int slice = (analogRead(MULT_POT) >> 6);
  if(DEBUG){
    int Pot1 = (analogRead(MULT_POT) >> 6);
    Serial.print("MULT_POT = ");
    Serial.println(Pot1);
    Serial.print("Mult number is: ");
    Serial.println(slice);
  }
  return slice2factor(slice, mode);
}

int get_div(int mode){
  //int slice = analogRead(DIV_POT)*(NUM_POT_DIVISIONS-1)/DIV_POT_MAX;
  int slice = (analogRead(DIV_POT) >> 6);
  if(DEBUG){
    int Pot2 = (analogRead(DIV_POT) >> 6);
    Serial.print("DIV_POT = ");
    Serial.println(Pot2);
    Serial.print("Div number is: ");
    Serial.println(slice);
  }
  return slice2factor(slice, mode);
}

long get_time(){
  return millis();
}

void trigger_out(){
  analogWrite(PWM_PIN, 255);
  out_clock_high = true;
  last_trigger_out = now;
  if(DEBUG_TRIGGERS){
    Serial.print(1);
    Serial.print(" ");
  }
  //num_triggs--;
}

void loop() {
 int gate = analogRead(CLOCK_IN);
 int reset = analogRead(MODE_SELECT_POT);

 if(DEBUG){
   Serial.print("Mode selector position: ");
   Serial.println(reset);
 }

 now = get_time();
 edge = false;

 //detecting gates
 if(gate > 32){                 //check if analog input of 0-255 is greater than 32
  if(!in_clock_high){
    if(last_trigger_in != 0){
      time_between_ins = now - last_trigger_in;
    }
    getting_triggers = time_between_ins < 1000 ? true:false;
    last_trigger_in = now;

    if(!getting_triggers){
      edge_skipper = 0;
    }
    else{
      edge_skipper = (edge_skipper + 1) % get_div(mode);
    }
    if(edge_skipper == 0){
      edge = true;
    }
  }
  in_clock_high = true;
 }
 else{
  in_clock_high = false;
 }

 //detect reset and mode
 if(reset < MODE_SELECT_POT_MAX/3){  
   //Counter Clockwise on the lower pot sets the mode to simple
   if(edge){
     if(DEBUG){
       Serial.print("Mode is Simple");
     }
     mode = MODE_SIMPLE;
     stopped = false;
  }
 }
  else if(reset > MODE_SELECT_POT_MAX/3*2){
    //Clockwise sets the mode to complex
    if(edge){
      if(DEBUG){
        Serial.print("Mode is Simple");
      }
      mode = MODE_COMPLEX;
      stopped = false;
    }
  }
  else{
    //stopped
    stopped = true;
    last_trigger_out = 0;
  }

  //setting up the multiplication and division
  if(edge && getting_triggers){
    // only update if triggers faster than 1 second
    time_between_outs = time_between_ins / get_mult(mode);
  }

  if(time_between_ins > 0 && !stopped){
    if(num_triggs <= 1 && edge){
      trigger_out();
      num_triggs = get_mult(mode);
    }
    else if(now - last_trigger_out >= time_between_outs * get_div(mode)){
      if(num_triggs > 1){
        trigger_out();
        num_triggs--;
      }
    }
  }

  // reduce trigger length if the input clock is exceedingly fast
  if(time_between_outs <= TRIGGER_LENGTH*2){
    variable_trigger_length = time_between_outs/2;
  }
  else{
    variable_trigger_length = TRIGGER_LENGTH;
  }

  // turn trigger output low
  if(now - last_trigger_out > variable_trigger_length && out_clock_high){
    analogWrite(PWM_PIN, 0);
    out_clock_high = false;
  }
}
