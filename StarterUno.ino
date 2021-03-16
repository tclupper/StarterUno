/*
  Name: StarterUno
  Description: A core starting program that includes a simple interactive command set.
      This assumes your project will want to communicate to the Arduino to set parameters and get information.
        (e.g. Get the hardware information like serial number and firmware revision or number of power cycles)
  Author: Tom Clupper
  License: [Attribution-ShareAlike 4.0 International](https://creativecommons.org/licenses/by-sa/4.0)
  Revision: 3/16/2021 (ver 1.3)
    1.0 = Initial version
    1.1 = Modified the core command set (added access to digital IO and other analog channels)
    1.2 = Modified the core command set (added more options to the report output)
        = Renamed variables and functions using "snake case".
    1.3 = Modified report output format

  Key functionality:
    1) Allow user to read analog channels 0 through 5.
    2) Allow user to set or read digital channels 2 through 12.
    3) Allow user to turn on/off on-board LED.
    4) Allow user to turn on/off or "flicker" off-board LED.
    5) Allow user to have a pushbutton input.
    6) Allow user to output a "report" of analog inputs and pushbutton input at regular intervals.
*/

// vvvvvvvvvvvvvvvvvvvvvvvvv  Core variables  vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
#include <EEPROM.h>

/* Number of times Arduino has been booted */
int num_power_cycles = 0;

/* Variable used in serial communication */
char input_buffer[10];
int buffer_index = 0;
char EOL = 13;                    // Make sure the computer is expecting this as the terminating line character
char NL = 10;                     // New line character
bool process_command = false;

/* This variable will be used throughout the program for timing events */
unsigned long current_millis = 0;
// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
/* Define variable for the On-board LED output */
int onboard_led_pin = 13;

/* Define the pushbutton input pin */
int pushbutton_pin = 12;
/* Variables used in the debouncing of the pushbutton input pin */
unsigned long millis_after_change = 0;        // unsigned long goes from 0 to 4,294,967,295
//  pushbutton_state = 0 ; Pushbutton is high
//  pushbutton_state = 1 ; Pushbutton is low
int pushbutton_state = 0;         // Use this variable to determine the "true" state of the pushbutton input pin.
int pushbutton_state_for_report =0;   // Same as pushbutton_state, but only resets when read by user.

/* Define an arbitrary digital pin */
int digital_pin = 10;
/* Define an arbitrary analog pin */
int analog_pin = 0;

/* Define variable for the external LED output */
int flicker_led_pin = 11;
int flicker_led_brightness = 200;                   // A number generally between 10 and 250
unsigned long flicker_led_interval = long(120);     // Number of Milliseconds between changes (generally between 10 and 250)
unsigned long millis_from_last_flicker = 0;         // Number of milliseconds since LED brighness change
bool flicker_led_mode = false;

/* Variables used in outputting report */
int output_interval = 1;                    // Number of seconds between outputs (between 1 to 60 seconds)
unsigned long output_intervalMillis = 0;    // Output Interval in Millis
unsigned long previous_millis = 0;
bool output_data = false;                   // Defines whether or not to output data
int output_format = 0;                      // Report output format

/* ============================================= BEGIN SETUP ================================================== */
void setup() {
/* vvvvvvvvvvvvvvvvvvvvvvvvv  Core setup  vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv */
  // initialize the serial port
  Serial.begin(115200);

  // increment the Number of power-on cycles by one
  EEPROM.get(0, num_power_cycles);
  num_power_cycles = num_power_cycles + 1;
  EEPROM.put(0,num_power_cycles);

  // Flush the serial buffer before we start
  delay(10);
  Serial.flush();
  delay(10);
/* ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */
  // Orange on-board LED.  This can also be used as a debug pin by toggling it when you need to
  pinMode(onboard_led_pin, OUTPUT);
  digitalWrite(onboard_led_pin, LOW);

  // Setup a generic analog input setup
  pinMode(analog_pin,INPUT);  
  
  // Setup a generic digital input/output (I/O) pin
  pinMode(digital_pin, INPUT);

  // LED to turn on/off or flicker
  pinMode(flicker_led_pin, OUTPUT);
  digitalWrite(flicker_led_pin, LOW);
  
  // Pushbutton input setup
  pinMode(pushbutton_pin,INPUT_PULLUP);   // Uses an internal pullup resistor
}
/* ============================================= END SETUP ====================================================== */


/* ============================================= BEGIN LOOP ===================================================== */
void loop() {
  /* vvvvvvvvvvvvvvvvvvvvvvvvv  Core loop code  vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv */
  CheckforSerialData();
  process_commands();
  /* ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */
  current_millis = millis();
  /* NOTE:  millis returns the number of milliseconds passed since the Arduino board began running the current program. 
            This number will overflow (go back to zero), after approximately 50 days.
            A 'long' is 32 bits (4 bytes). Unlike standard longs unsigned longs won’t store negative numbers,
            making their range from 0 to 4,294,967,295 (2^32 - 1). The subtraction will yield the proper result,
            even when current_millis overflows, or wraps around.  */

  /* Randomly change the PWM output value that drives the LED (often enough to make it flicker) */
  if ( ((current_millis - millis_from_last_flicker) >= flicker_led_interval) && flicker_led_mode) {
   millis_from_last_flicker = current_millis;    
   analogWrite(flicker_led_pin, (random(flicker_led_brightness)+255-flicker_led_brightness)); 
  }
  
  /* Output the values of the analog input pin and pushbutton state every output_interval (if enabled) */
  output_intervalMillis = long(output_interval)*long(1000);      // Output interval in milliseconds
  if ( ((current_millis - previous_millis) >= output_intervalMillis) && output_data) {
    previous_millis = current_millis;
    report_data();  
  } 

  /* Check to see if the pushbutton has been pressed for a sufficiently long time to inducte an "on" state */
  if (digitalRead(pushbutton_pin) == 0) {
    if (millis_after_change == 0 && pushbutton_state == 0) {
      millis_after_change = current_millis;          // Start the bounce timer for button press
    }
    if (((millis() - millis_after_change) > 100) && pushbutton_state == 0) {     // wait for 100msec to account for contact "bounce".
      pushbutton_state = 1;
      pushbutton_state_for_report = 1;
      millis_after_change = 0;           
    }
  } else {
    if (millis_after_change == 0 && pushbutton_state == 1) {
      millis_after_change = current_millis;          // Start the bounce timer for button release
    }
    if (((millis() - millis_after_change) > 100) && pushbutton_state == 1) {     // wait for 100msec to account for contact "bounce".
      pushbutton_state = 0;
      millis_after_change = 0;         
    }  
  }
}
/* ============================================= END LOOP ======================================================= */

/* ----------------------------- Check for Serial Data ---------------------------- */
void CheckforSerialData() {
  // See if there is a character ready from the serial port
  if (Serial.available() > 0)  {
    input_buffer[buffer_index] = byte(Serial.read());
    if (input_buffer[buffer_index] == EOL)  {
      buffer_index = 0;
      process_command = true;
      delay(1);
      Serial.flush();
    } else {
      buffer_index = buffer_index + 1;
      if (buffer_index > 9) buffer_index = 0;   // Reset the input buffer (This should practically never happen!!!)
      process_command = false;
    }
  }  
}
/* ----------------------------- Check for Serial Data -------------------------------------- */
  
/* ----------------------------- Start Process Commands ------------------------------------- */
void process_commands() {
  if (process_command)  {
    process_command = false;
    switch (input_buffer[0]) {
      /* vvvvvvvvvvvvvvvvvvvvvvvvv  Core commands  vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv */
      case 'F': case 'f':   // Flush the serial buffer (This is essentially a "do-nothing" command).
        Serial.flush();     // This is sometimes needed before the computer sends a command.
        delay(10);
        break;
        
      case 'I': case 'i':     // Device Information (A throw-back to the IEEE488.2 days of *IDN?)
        // Manufacture,Model,SerialNumber,FirmwareRevision
        Serial.println("Arduino,StarterCode,SC001,1.3");
        break;        
        
      case 'P': case 'p':   // Read the number of Power-on cycles stored in EEprom (Use "P0" to reset)
        if (input_buffer[1] == '0') {
          num_power_cycles = 0;
          EEPROM.put(0,num_power_cycles);
          Serial.println("OK");
        } else {
          Serial.println(num_power_cycles);          
        }
        break;

      /* vvvvvvvvvvvvvvvvvvvvvvvvv  basic I/O commandsvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv */
      case 'A': case 'a':     // usage: Ax  (where x = 0 to 5 sets the analog channel to output and then outputs it)
        analog_pin = (input_buffer[1]-48);
        if (analog_pin < 0 or analog_pin > 5) {
          analog_pin = 0;       // From 0 to 5 only
        }
        /* This returns a number from 0 to 2023 (10 bit A/D converter) that is the voltage on analog_pin*/
        /* Remember to keep the input impedance of the voltage source < 10k Ohms */
        analog_data_out();
        break;  

      case 'D': case 'd':     // usage: Dxxy  (where xx = 02...12 sets pin xx to putput of y (0 or 1), y = nothing sets xx to input and returns state
        digital_pin = (input_buffer[1]-48)*10 + (input_buffer[2]-48);
        if (digital_pin < 2 or digital_pin > 12) {
          digital_pin = 2;      // RX & TX take up pins 0 & 1 and on-board LED takes up 13.
        }      
        if (input_buffer[3] == '1') {
          pinMode(digital_pin,OUTPUT);
          digitalWrite(digital_pin,HIGH);
          Serial.println("OK->High");
        } else if (input_buffer[3] == '0') {
          pinMode(digital_pin,OUTPUT);
          digitalWrite(digital_pin,LOW);
          Serial.println("OK->Low");
        } else {
          pinMode(digital_pin,INPUT_PULLUP);
          digital_data_out();              
        }
        break;

      case 'S': case 's':     // usage: S;  will return a 1 if pushbutton is currently being pressed and 0 for not
        Serial.println(pushbutton_state);
        pushbutton_state_for_report = 0;    // reset this state.
        break;
        
      /* vvvvvvvvvvvvvvvvvvvvvvvvv  Commands for logging data stream vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv */
      case 'R': case 'r':     // usage: Rxy  (Set x = B to begin streaming report, when x = y = nothing, (i.e. just R) then end the stream)
                              //              if x = F, then y specifies the output report format
                              // y = 0 to output: pushbutton_state_for_report, analog0          {default}
                              // y = 1 to output: pushbutton_state_for_report, analog0, analog1
                              // y = 2 to output: pushbutton_state_for_report, analog0, analog1, analog2
                              // y = 3 to output: pushbutton_state_for_report, analog0, analog1, analog2, analog3
                              // y = 4 to output: pushbutton_state_for_report, analog0, analog1, analog2, analog3, analog4
                              // y = 5 to output: pushbutton_state_for_report, analog0, analog1, analog2, analog3, analog4, analog 5
                              // y = ?, then return output format
        if (input_buffer[1] == 'b' or input_buffer[1] == 'B') {
            current_millis = millis();
            previous_millis = current_millis;
            output_data = true;
        } else if (input_buffer[1] == 'f' or input_buffer[1] == 'F') {
          if (input_buffer[2] == '?') {
            Serial.println(output_format);
          } else {
            output_format = (input_buffer[2]-48);
            if (output_format < 0) output_format = 0;     // At least 0
            if (output_format > 5) output_format = 5;     // No more than 5
            Serial.println("OK");            
          }
        } else {
          output_data = false;
          Serial.println("OK-Ended");                 
        }
        break;
           
      case 'O': case 'o':     // usage: Oxx (for an output interval of every xx seconds, O? to inquire)
        if (input_buffer[1] == '?') {
          Serial.println(output_interval);
        } else {
          output_interval = (input_buffer[1]-48)*10+(input_buffer[2]-48);
          if (output_interval < 1) output_interval = 1;       // At least 1 sec.
          if (output_interval > 60) output_interval = 60;     // No more than 60 sec.          
          Serial.println("OK"); 
        }
        break;
        
      /* vvvvvvvvvvvvvvvvvvvvvvvvv  Commands for LED(s) vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv */
      case 'L': case 'l':     // usage: Lx (e.g. L toggles On-board LED on and off, x = O, turns off on-board LED)
        if (input_buffer[1] == 'O' or input_buffer[1] == 'o') {
          digitalWrite(onboard_led_pin,LOW);
          Serial.println("OK");
        } else {
          toggle_pin(onboard_led_pin); 
          Serial.println("OK"); 
        }
        break;
        
      case 'M': case 'm':     // usage: Mx (e.g. M toggles Flicker LED on and off, x = F sets to flicker mode, x = O turns off LED)
        if (input_buffer[1] == 'F' or input_buffer[1] == 'f') {
          flicker_led_mode = true;
          Serial.println("FlickerMode");
        } else if (input_buffer[1] == 'O' or input_buffer[1] == 'o') {
          digitalWrite(flicker_led_pin,LOW);
          flicker_led_mode = false;
          Serial.println("OK");
        } else {
          flicker_led_mode = false;
          toggle_pin(flicker_led_pin); 
          Serial.println("OK"); 
        }
        break; 
        
      case 'B': case 'b':     // usage: Bxxx (e.g. B135 sets max random LED brightness level to 135 (B? to inquire))
        if (input_buffer[1] == '?') {
          Serial.println(flicker_led_brightness);
        } else  {
          flicker_led_brightness = (input_buffer[1]-48)*100+(input_buffer[2]-48)*10+(input_buffer[3]-48);
          if (flicker_led_brightness < 10) flicker_led_brightness = 10;       // At lease 10
          if (flicker_led_brightness > 250) flicker_led_brightness = 250;     // No more than 250         
          Serial.println("OK"); 
        }
        break;  
        
      case 'T': case 't':     // usage: Txxx (e.g. D200 sets LED flicker delay time to 200 msec, T? to inquire))
        if (input_buffer[1] == '?') {
          Serial.println(flicker_led_interval);
        } else {
          flicker_led_interval = long(input_buffer[1]-48)*100+(input_buffer[2]-48)*10+(input_buffer[3]-48);
          if (flicker_led_interval < 10) flicker_led_interval = 10;       // At lease 10
          if (flicker_led_interval > 250) flicker_led_interval = 250;     // No more than 250         
          Serial.println("OK"); 
        }
        break;  
      /* ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */
      
      default:    //  If a non-recognizable command occurs, output the command back to the Serial port
        bool EndofCommand = false;
        Serial.print(":");
        for (int i=0; i <= 9; i++) {
          if (input_buffer[i] == EOL) EndofCommand = true;
          if (!EndofCommand)  {
            Serial.print(input_buffer[i]);
          }
        }
        Serial.println(":");        
        break;
    }
    for (int i=0; i <=9; i++){   // Clears out the input_buffer buffer
      input_buffer[i]=0;  
    }
  }  
}
/* ----------------------------- End Process Commands ---------------------------------- */

/* ----------------------------- Send Output Data -------------------------------------- */
void analog_data_out() {
  Serial.println(analogRead(analog_pin));
}

void digital_data_out() {
  Serial.println(digitalRead(digital_pin));  
}

void report_data() {
  Serial.print( (pushbutton_state | pushbutton_state_for_report) );
  pushbutton_state_for_report = 0;
  Serial.print(", ");
  Serial.print(analogRead(0));
  if (output_format >= 1){
    Serial.print(", ");   
    Serial.print(analogRead(1));
  }
  if (output_format >= 2){
    Serial.print(", ");    
    Serial.print(analogRead(2));
  }
  if (output_format >= 3){
    Serial.print(", ");    
    Serial.print(analogRead(3));
  }
  if (output_format >= 4){
    Serial.print(", ");    
    Serial.print(analogRead(4));
  }
  if (output_format >= 5){
    Serial.print(", ");
    Serial.print(analogRead(5));
  }
  Serial.print('\r');
  Serial.print('\n');
}

void toggle_pin(int toggle_pin) {
  /*  Toggle Digital IO pin and return the value  */
  if (digitalRead(toggle_pin) == 1) {
    digitalWrite(toggle_pin,LOW);
  } else {
    digitalWrite(toggle_pin,HIGH);
  }
}
    
