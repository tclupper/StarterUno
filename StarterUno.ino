/*
  Name: StarterUno
  Description: A core starting program that includes a simple interactive command set.
      This assumes your project will want to communicate to the Arduino to set parameters and get information.
        (e.g. Get the hardware information like serial number and firmware revision or number of power cycles)
  Author: Tom Clupper
  License: [Attribution-ShareAlike 4.0 International](https://creativecommons.org/licenses/by-sa/4.0)
  Revision: 2/24/2021 (ver 1.0)
*/

// vvvvvvvvvvvvvvvvvvvvvvvvv  Core variables  vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
#include <EEPROM.h>
 
/* Number of times Arduino has been booted */
int NumPowerCycles = 0;

/* Variable used in serial communication */
char InBuff[10];
int BuffIndex = 0;
char EOL = 13;
bool ProcessCommand = false;
// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
/* Define variable for the On-board LED output */
int OnBoardLEDpin = 13;

/* Define the pushbutton input pin */
int PushbuttonInputPin = 12;

/* This variable will be used for timing events */
unsigned long currentMillis = 0;

/* Define variable for the Red LED output */
int FlickerLEDpin = 11;
int LEDbrightness = 200;                     // A number between 10 and 250
unsigned long LEDinterval = long(120);       // Number of Milliseconds between changes (between 10 and 250)
unsigned long millisFromLastFlicker = 0;     // Number of milliseconds since LED brighness change
bool FlickerMode = false;

/* Variables used in outputting analog input */
int AnalogInputPin = 0;     // Which analog input pin to monitor
int OutputInterval = 1;     // Number of seconds between outputs (between 1 to 60 seconds)
unsigned long OutputIntervalMillis = 0; // Output Interval in Millis
unsigned long previousMillis = 0;
bool OutputData = false;    // Defines whether or not to output data

/* Variables used in the debouncing of the pushbutton input pin */
unsigned long millisAfterChange = 0;        // unsigned long goes from 0 to 4,294,967,295
//  PushbuttonState = 0 ; Pushbutton is high
//  PushbuttonState = 1 ; Pushbutton is low
int PushbuttonState = 0;        // Use this variable to determine the "true" state of the pushbutton input pin.

/* ============================================= BEGIN SETUP ================================================== */
void setup() {
/* vvvvvvvvvvvvvvvvvvvvvvvvv  Core setup  vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv */
  // initialize the serial port
  Serial.begin(115200);

  // increment the Number of power-on cycles by one
  EEPROM.get(0, NumPowerCycles);
  NumPowerCycles = NumPowerCycles + 1;
  EEPROM.put(0,NumPowerCycles);

  // Flush the serial buffer before we start
  delay(10);
  Serial.flush();
  delay(10);
/* ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */
  // [Orange on-board LED.  This is also used as a debug pin]
  pinMode(OnBoardLEDpin, OUTPUT);
  digitalWrite(OnBoardLEDpin, LOW);
    
  // [LED to flicker]
  pinMode(FlickerLEDpin, OUTPUT);
  digitalWrite(FlickerLEDpin, LOW);
  
  // Analog input setup
  pinMode(AnalogInputPin,INPUT);   

  // Pushbutton setup
  pinMode(PushbuttonInputPin,INPUT_PULLUP);   // Uses an internal pullup resistor
}
/* ============================================= END SETUP ====================================================== */


/* ============================================= BEGIN LOOP ===================================================== */
void loop() {
  /* vvvvvvvvvvvvvvvvvvvvvvvvv  Core loop code  vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv */
  CheckforSerialData();
  ProcessCommands();
  /* ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */
  currentMillis = millis();
  /* NOTE:  millis returns the number of milliseconds passed since the Arduino board began running the current program. 
            This number will overflow (go back to zero), after approximately 50 days.
            A 'long' is 32 bits (4 bytes). Unlike standard longs unsigned longs won’t store negative numbers,
            making their range from 0 to 4,294,967,295 (2^32 - 1). The subtraction will yield the proper result,
            even when currentMillis overflows, or wraps around.  */

  /* Randomly change the PWM output value that drives the LED (often enough to make it flicker) */
  if ( ((currentMillis - millisFromLastFlicker) >= LEDinterval) && FlickerMode) {
   millisFromLastFlicker = currentMillis;    
   analogWrite(FlickerLEDpin, (random(LEDbrightness)+255-LEDbrightness)); 
  }
  
  /* Output the values of the analog input pin and pushbutton state every OutputInterval (if enabled) */
  OutputIntervalMillis = long(OutputInterval)*long(1000);   // Output interval in milliseconds
  if ( ((currentMillis - previousMillis) >= OutputIntervalMillis) && OutputData) {
    previousMillis = currentMillis;
    OutputDataReport(AnalogInputPin,PushbuttonState);  
  } 

  /* Check to see if the pushbutton has been pressed for a sufficiently long time to inducte an "on" state */
  if (digitalRead(PushbuttonInputPin) == 0) {
    if (millisAfterChange == 0 && PushbuttonState == 0) {
      millisAfterChange = currentMillis;          // Start the bounce timer for button press
    }
    if (((millis() - millisAfterChange) > 100) && PushbuttonState == 0) {
      PushbuttonState = 1;
      millisAfterChange = 0;           
    }
  } else {
    if (millisAfterChange == 0 && PushbuttonState == 1) {
      millisAfterChange = currentMillis;          // Start the bounce timer for button release
    }
    if (((millis() - millisAfterChange) > 100) && PushbuttonState == 1) {
      PushbuttonState = 0;
      millisAfterChange = 0;         
    }  
  }
}
/* ============================================= END LOOP ======================================================= */

/* ----------------------------- Check for Serial Data ---------------------------- */
void CheckforSerialData() {
  // See if there is a character ready from the serial port
  if (Serial.available()>0)  {
    InBuff[BuffIndex] = byte(Serial.read());
     if (InBuff[BuffIndex] == EOL)  {
      BuffIndex = 0;
      ProcessCommand = true;
      delay(1);
      Serial.flush();
    } else {
      BuffIndex = BuffIndex + 1;
      if (BuffIndex > 9) BuffIndex = 0;   // Reset the input buffer (This should never happen!!!)
      ProcessCommand = false;
    }
  }  
}
/* ----------------------------- Check for Serial Data -------------------------------------- */
  
/* ----------------------------- Start Process Commands ------------------------------------- */
void ProcessCommands() {
  if (ProcessCommand)  {
    ProcessCommand = false;
    switch (InBuff[0]) {
      /* vvvvvvvvvvvvvvvvvvvvvvvvv  Core commands  vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv */
      case 'F': case 'f':   // Flush the serial buffer (This is essentially a "do-nothing" command).
        Serial.flush();     // This is sometimes needed before the computer sends a command.
        delay(10);
        break;
        
      case 'I': case 'i':     // Device Information (This can be whatever you want)
        // Manufacture,Model,SerialNumber,FirmwareRevision
        Serial.println("Arduino,StarterCode,SC001,1.0");
        break;        
        
      case 'P': case 'p':   // Read the number of Power-on cycles stored in EEprom (Use "P0" to reset)
        if (InBuff[1] == '0') {
          NumPowerCycles = 0;
          EEPROM.put(0,NumPowerCycles);
          Serial.println("OK");
        } else {
          Serial.println(NumPowerCycles);          
        }
        break;       
      /* ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */
      case 'A': case 'a':     // usage: Ax; x=B (Begin outputting data), x=E (End outputting data), else read once
        if (InBuff[1] == 'b' or InBuff[1] == 'B') {
            currentMillis = millis();
            previousMillis = currentMillis;
            OutputData = true;
        } else if (InBuff[1] == 'e' or InBuff[1] == 'E') {
            OutputData = false;
            Serial.println("OK");            
        } else{
          /* This returns a number from 0 to 2023 (10 bit A/D converter) that is the voltage on AnalogInputPin*/
          OutputAnalogData(AnalogInputPin);
        }
        break;  

      case 'B': case 'b':     // usage: Bxxx (e.g. B135 sets max random LED brightness level to 135 (B? to inquire))
        if (InBuff[1] == '?')  {
          Serial.println(LEDbrightness);
        } else  {
          LEDbrightness = (InBuff[1]-48)*100+(InBuff[2]-48)*10+(InBuff[3]-48);
          if (LEDbrightness < 10) LEDbrightness = 10;       // At lease 10
          if (LEDbrightness > 250) LEDbrightness = 250;     // No more than 250         
          Serial.println("OK"); 
        }
        break;  
        
      case 'D': case 'd':     // usage: Dxxx (e.g. D200 sets LED flicker delay at 200 msec (D? to inquire))
        if (InBuff[1] == '?')  {
          Serial.println(LEDinterval);
        } else  {
          LEDinterval = long(InBuff[1]-48)*100+(InBuff[2]-48)*10+(InBuff[3]-48);
          if (LEDinterval < 10) LEDinterval = 10;       // At lease 10
          if (LEDinterval > 250) LEDinterval = 250;     // No more than 250         
          Serial.println("OK"); 
        }
        break;  

      case 'L': case 'l':     // usage: Lx (e.g. L toggles Flicker LED on and off, LF sets to flicker mode, LO turns it off)
        if (InBuff[1] == 'F' or InBuff[1] == 'f')  {
          FlickerMode = true;
          Serial.println("FlickerMode");
        } else if (InBuff[1] == 'O' or InBuff[1] == 'o') {
          digitalWrite(FlickerLEDpin,LOW);
          FlickerMode = false;
          Serial.println("OK");
        } else {
          FlickerMode = false;
          TogglePin(FlickerLEDpin); 
          Serial.println("OK"); 
        }
        break;  

      case 'O': case 'o':     // usage: O01, for an output interval of every 1 second (O? to inquire)
        if (InBuff[1] == '?')  {
          Serial.println(OutputInterval);
        } else  {
          OutputInterval = (InBuff[1]-48)*10+(InBuff[2]-48);
          if (OutputInterval < 1) OutputInterval = 1;       // At lease 1 sec.
          if (OutputInterval > 60) OutputInterval = 60;     // No more than 60 sec.          
          Serial.println("OK"); 
        }
        break;

      case 'S': case 's':     // usage: S, (will return 1 for pressed and 0 for not)
        Serial.println(PushbuttonState); 
        break;
        
      case 'T': case 't':     // usage: Tx (e.g. T toggles On-board LED on and off, TO turns it off)
        if (InBuff[1] == 'O' or InBuff[1] == 'o'){
          digitalWrite(OnBoardLEDpin,LOW);
          Serial.println("OK");
        } else {
          TogglePin(OnBoardLEDpin); 
          Serial.println("OK"); 
        }
        break;
        
      default:    //  If a non-recognizable command occurs, output the command back to the Serial port
        bool EndofCommand = false;
        Serial.print(":");
        for (int i=0; i <= 9; i++) {
          if (InBuff[i] == EOL) EndofCommand = true;
          if (!EndofCommand)  {
            Serial.print(InBuff[i]);
          }
        }
        Serial.println(":");        
        break;
    }
  }  
}
/* ----------------------------- End Process Commands ---------------------------------- */

/* ----------------------------- Send Output Data -------------------------------------- */
void OutputAnalogData(int AnalogPin) {
  Serial.println(analogRead(AnalogPin));
}

void OutputDataReport(int AnalogPin, int Pushbutton) {
  Serial.print(analogRead(AnalogPin));
  Serial.print(", ");
  Serial.println(Pushbutton);
}

void TogglePin(int OutputPin) {
  /*  Toggle Digital IO pin and return the value  */
  if (digitalRead(OutputPin) == 1) {
    digitalWrite(OutputPin,LOW);
  } else {
    digitalWrite(OutputPin,HIGH);
  }
}
    
