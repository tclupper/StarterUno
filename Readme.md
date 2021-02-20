# Starter project: The Arduino Uno board source code (that can interface to a computer via a virtual serial port)
### Rev 2/20/2021
### License: [Attribution-ShareAlike 4.0 International](https://creativecommons.org/licenses/by-sa/4.0)

---
## Background
The _Starter Project_ the final part of a Basic Electronics course that was taught at the [Route 9 Library and Innovation center](https://nccde.org/1389/Route-9-Library-Innovation-Center) back in October of 2019.  The videos were recorded as a follow-up for the students taking the class (*not meant to be a substitute for the in-person class*).  They can be viewed here:

### Course title: _Basic Electrical Circuits_

[Introduction video](https://youtu.be/7xKFPJ8yrWM)

[Part1: Current, Resistance and Voltage](https://youtu.be/wcw07wuuB8o)

[Part2: Ohm's Law and Power](https://youtu.be/5naIT84_2M0)

[Part3: Sensors and the Arduino](https://youtu.be/qC13UVfvqh0)

[Part4: Programming the Arduino](https://youtu.be/MEm4goe0QIw)

---
## Arduino code
This repository contains the C source code to be programmed into an Arduino Uno board.  It is part of the _Starter project_ that is intended to interface an Arduino Uno microcontroller to a computer running a custom application. To use the code in this project, you will need to get a hold of an Arduino Uno microcontroller (or compatible board) and install the [Arduino IDE.](https://www.arduino.cc/en/software).

The basic Arduino Uno _Starter Project_ setup consists of a 180 Ohm resistor from digital output pin 11 to the anode of an LED.  Then, the cathode of the LED (flat edge side of the LED base) goes to ground. See Figure #1.  Also, a 5k Ohm potentiometer is attached to the Analog input 0.  The top part of the potentiometer goes to +5V and the opposite side goes to ground.  The middle (wiper) contact goes directly to Analog pin 0.  You will also need a pushbutton switch connected from ground to digital input 12. See Figure #1. When you plug the Arduino into the USB port, the operating system should automatically install the drivers.

![](images\ArduinoUnoTestSetup.jpg)
### Figure#1: Crude test setup schematic

---
## Details about the code
The main purpose is to provide a "language" that you could use serial commands to interact with the Arduino.  You can turn on LEDS, get pushbutton status, have the analog input automatically outputted every second, etc.  These are just a few of things one might use an Arduino to do. Once you have programmed the UNO, you can use the built-in Serial Monitor in the IDE to send the commands by hand to make sure they work.  The default baud rate is 115200.

![](images\IDEserialMonitor.png)
### Figure#2: Serial Monitor window

Below is a summary of the commands (you can add or delete as needed)
* __`I`__    (returns: Manufacture,Model,SerialNumber,FirmwareRevision)
    * A throw-back to the *IDN? of GBIB488.2 days.
* __`P`__ (returns: Number of power-on cycles)
    - __`P0`__ (resets the number of power-on cycles to 0)
* __`A`__ (returns: value of analog input pin 0 (0 to 1023))
    * __`AB`__ (Begin sending analog value every "xx" seconds)
    * __`AE`__ (End sending data)
    * __`Oxx`__  (Sets the number of xx seconds between analog value outputs)
* __`L`__ (Toggles LED on and OFF)
    * __`LF`__ (sets the LED to flicker like a light bulb mode)
    * __`LO`__ (turns off the LED)
    * __`Bxxx`__ (sets the max random LED brightness during flicker mode)
        * __`B?`__ (returns the current max brightness level)
    * __`Dxxx`__ (sets the flicker delay between changes.)
        * __`D?`__ (returns the current flicker delay value)
* __`S`__ (returns: debounced value fo the pushbutton. 0 for not pushed, 1 for pushed)
* __`T`__ (Toggles the on-board LED)
    * __`TO`__ (turns off the on-board LED)

This project will be continuously evolving and improving.  However, I will try and prevent unnecessary "feature creep".

Enjoy!!


