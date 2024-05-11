# Busch Microtronic 2095 Tape Emulator for Arduino Uno R3
## An SD Card-Based Emulator of the 2095 Cassette Interface for the Busch 2090 Microtronic Computer System 
#### Author: Michael Wessel
#### License: GPL 3
#### Hompage: [Author's Homepage](https://www.michael-wessel.info/)
#### Version: 1.1 
#### YouTube Video: [https://youtu.be/_KTuKygo-tQ](https://youtu.be/_KTuKygo-tQ)

## Abstract

This is an Arduino Uno-based emulator of the Busch 2095 Cassette
Interface + Tape Recorder for the historical Busch 2090 Microtronic
Computer System. It allows you to save and load Microtronic programs
to and from SD card. 

Version 1.1 (2024) of this project requires an LCD+Buttons Shield, an
SD Data Logger Shield, and an Arduino Uno. The previous Version 1.0
(from 2016, see below for historical reference) used an SDCard +
Ethernet Shield instead of the Data Logger Shield. The current version
is easier to assemble, cheaper, and more elegant. 

The emulator directly connects to the Busch 2090 "expansion port",
using a 14 PIN DIL IC precision socket and DuPont (breadboard) cables.

![Busch Microtronic 2095 Cassette Interface Emulator for Arduino Uno](https://github.com/lambdamikel/microtronic-2095-arduino-emulator/blob/master/images/small/DSC06085.JPG)

## Assembly

Assembly is straight-forward. You will require the following components: 

* [Standard Arduino Uno R3] (or clone) 
* [LCD + Buttons Shield](https://www.amazon.com/HiLetgo-Expansion-Backlight-4-5-5-5V-Duemilanove/dp/B00OGYXN8C)
* [Data Logger Shield](https://www.amazon.com/HiLetgo-Logging-Recorder-Logger-Arduino/dp/B00PI6TQWO)

This stacks together as follows: 

![new-version-parts](new-version/new-version-parts.jpg)

![new-version-1](new-version/new-version-2.jpg)

![new-version-1](new-version/new-version-3.jpg)

We need to connect `A1, A2, A3, A4, A5, PD2, PD3` as well as `GND`,
all available on the top of the LCD shield, to the Microtronic. 

![Microtronic Expansion Header](images/small/pinout.jpg)

Make the following connections: 
 	
~~~~ 	
BUSCH_IN1 <-> PD3
BUSCH_IN2 <-> A5
BUSCH_IN3 <-> A4
BUSCH_IN4 <-> A3

BUSCH_OUT1 <-> PD2
BUSCH_OUT2 <-> A1
BUSCH_OUT3 <-> A2

BUSCH_GND <-> GND 
~~~~

![connector](new-version/connector.jpg)

Now program the Uno with the `busch2095.ino` sketch, insert a FAT16 or
FAT32 formatted SD card, and power it up via USB or the external power
jack (standard 9V - 12 V external Arduino power supply). You should
then get the **"SD Card Ready"** and the main menu:

![new-version-1](new-version/new-version-1.jpg)


### Problems 

#### "SD Card ERROR!" Caused by Incompatible LCD Shield

Please note that the following display shields will NOT work; for
unknown reasons, the SD card library fails to initialize with them.
So, if you are getting an **"SD Card ERROR!"** instead of the **"SD
Card Ready"** during startup, consider changing the display shield.

Display shields that don't work include the following:

![bad-displays](new-version/bad-displays.jpg)

[These are the displays that I am using;](https://www.amazon.com/HiLetgo-Expansion-Backlight-4-5-5-5V-Duemilanove/dp/B00OGYXN8C) they work perfectly!

Of course, it may also be the case that you simply forgot to insert
the SD card, that the SD card is not FAT16 or FAT32 formatted, is too
large (i.e., bigger than 8 GBs), or somehow incompatible. Try another
one then.

#### Buttons Don't Work Correctly 

In case the LCD buttons should not work properly, have a look at the
following function - the threshold values may need to be adjusted:

~~~~
int readLCDButtons() {

  cur_button = analogRead(0);

  // notice: these values may need to be tuned!!
  // and adjusted to your actual LCD Keypad shield
  // the are analog thresholds

  if (cur_button > 850) return NONE;
  if (cur_button < 50)   return RIGHT;
  if (cur_button < 150)  return UP;
  if (cur_button < 350)  return DOWN;
  if (cur_button < 500)  return LEFT;
  if (cur_button < 800)  return SELECT;

  return NONE;

}
~~~~

There is a test mode that allows you to acquire these (analog button)
values - to enter the test mode, hold down any button (other than the
rightmost RESET button) during startup. The program will then enter
this button test mode which allows you to acquire the (minimum)
threshold values for the different buttons:

![new-version-1](new-version/button-test.jpg)

Press the different buttons and take note of displayed values. Then
adjust the code above as needed; i.e., if you are observing an analog
value of 640 for the select button, consider using `< 700`.  Recompile
and reprogramm the Uno. 

## Usage

Connect the cable to the Microtronic. Power up the Uno. Power up the
Microtronic. If the SD card cannot be initialized correctly, the LCD
display should say so.  If the SD card is working correctly, you will
see the main screen, which allows you to select PGM1 via the *Left LCD
button* and *PGM2 with the Right LCD button.* The leftmost button is
the *Select button* which is used for selecting files, and for
confirmation during filename creation. 

The rightmost button is the Arduino hardware reset button.

To save and load a program, use the following main menu functions: 

### PGM2 - Save a Program to SD Card

Have the program in Microtronic memory. Then, enter `PGM 2` on the
Microtronic: `HALT PGM 2`. Next, use the Right LCD button on the
emulator. You will now be asked to create a file name for the program
you are about to save. Filenames are 8 characters long; the `.MIC`
extension is added automatically. Just use the Up and Down keys for
character selection. You can advance to the next character with the
Right key. Notice that the leftmost key (Select) enters the filename
and starts saving, whereas the second leftmost button cancels.

After Select, the emulator retrieves the memory words from the
Microtronic and saves them to SD card under that filename you
determined. The LCD display and Microtronic display will show the
saving progress. Notice that saving ends when the whole memory was
written (at address `&FF`). Otherwise, you will have to hit the Reset
button on the Microtronic, which will also end saving. 

### PGM1 - Load a Program from SD Card

Reset the Arduino first - use the Reset button (rightmost LCD
button). Enter `HALT PGM 5` to erase the Microtronic memory, then
`HALT NEXT 00`, and `HALT PGM 1` to start the Microtronic loading
process. Next, use the Left LCD button to start PGM 1 loading from SD
card.  A directory browser shows up. Use the Up and Down buttons to
select the `.MIC` file you want to load. Use the Select button to
confirm.

After Select, the tape emulator will send the data to the
Microtronic. The LCD display informs about the progress. There is very
little feedback on the Microtronic side - its LED display stays dark
until the program has been loaded. However, you should be able to
observe a slight flickering of the Microtronic output port LEDs as
data arrives. Use the Microtronic Reset button at any time to stop the
loading process. 

## Microtronic Software

A couple of programs are in the [software
subdirectory](https://github.com/lambdamikel/microtronic-2095-arduino-emulator/tree/master/software)
of this repository. See the `README.txt` in there.

To create programs with your laptop or desktop computer, I recommend
[Martin Sauter's Microtronic
Assembler](https://github.com/martinsauter/Busch-2090-Projects/tree/master/05%20-%20Busch%202090%20Assembler);
cool stuff!

## Acknowledgments 

This project is based on [Martin
Sauter's](https://github.com/martinsauter) [Raspberry Pi-based tape
emulator for the Busch 2090 historical
microcomputer](https://github.com/martinsauter/Busch-2090-Projects),
who did the [reverse engineering of the Busch Microtronic 2095
Cassette
Interface](https://blog.wirelessmoves.com/2017/06/emulating-a-busch-2090-tape-interface-part-1.html)
protocol.

I reimplemented his findings in C for the Arduino, because I prefer a
microcontroller-based "all in one" solution over the Raspberry Pi.  In
addition to Martin's emulator, this emulator also supports saving of
programs from the Microtronic to SD card.


## Disclaimer 

Use at your own risk. I am not responsible for any potential damage
that you might do to your Microtronic, other machinery, or yourself,
in the process of assembling this piece of hardware.

Enjoy! 


## OLD VERSION (2016 - OBSOLETE - FOR REFERENCE ONLY!)

### Assembly

The emulator is easy to assemble. I have used the following components: 

* [Standard Arduino Uno R3](https://www.amazon.com/Arduino-Uno-R3-Microcontroller-A000066/dp/B008GRTSV6/ref=sr_1_3?ie=UTF8&qid=1499053393&sr=8-3&keywords=arduino+uno+r3) 
* [LCD + Buttons shield - I used the one from Kuman, but I guess any other brand will do as well](https://www.amazon.com/Kuman-Shield-Display-Arduino-MEGA2560/dp/B01C466H1S/ref=sr_1_3?ie=UTF8&qid=1499052519&sr=8-3&keywords=keypad%2Blcd+shield+arduino)
* [SD Card + Ethernet Shield - I used the one from SainSmart, but I guess any other brand will do as well](https://www.amazon.com/SainSmart-Ethernet-Shield-Arduino-Duemilanove/dp/B006J4FZTW/ref=sr_1_fkmr2_1?ie=UTF8&qid=1499078651&sr=8-1-fkmr2&keywords=sainsmart+sdcard+and+ethernet+shield)

Just stick them together. You might want to use [Arduino pin
headers](https://www.amazon.com/Hilitchi-110pcs-Arduino-Stackable-Assortment/dp/B01IP60YQA/ref=sr_1_1?ie=UTF8&qid=1499053615&sr=8-1&keywords=arduino+headers),
because pin 10 of the LCD shield needs to be disconnected from the SD
Card shield for proper functioning (see next picture!), and also
because the SD Card shield is very tall, so it is difficult to put the 
LCD shield on top without headers:

![Connections Right Side - PIN 10 Disconnected](https://github.com/lambdamikel/microtronic-2095-arduino-emulator/blob/master/images/small/DSC06096.JPG) 

![Shields SD Card Shield + LCD Buttons](https://github.com/lambdamikel/microtronic-2095-arduino-emulator/blob/master/images/small/DSC06092.JPG)

Next, solder the wires (using pin headers or directly) to the LCD
shield, as shown in this picture:

![LCD Cables](https://github.com/lambdamikel/microtronic-2095-arduino-emulator/blob/master/images/small/DSC06089.JPG)

We will need `A1, A2, A3, A4, A5, GND`, as well as `PD2` and
`PD3`. Next, connect these to the Microtronic expansion IC socket for
the 2095. I have simply used a 14 PIN IC socket; I didn't even have to
solder the breadboard wires, the connection is tight enough:

![Microtronic Expansion Header](https://github.com/lambdamikel/microtronic-2095-arduino-emulator/blob/master/images/small/pinout.jpg)

That means, make the following connections, as shown in the
`busch2095.ino` sketch (source code):
 	
~~~~ 	
#define BUSCH_IN1 PD3
#define BUSCH_IN2 A5
#define BUSCH_IN3 A4
#define BUSCH_IN4 A3

#define BUSCH_OUT1 PD2
#define BUSCH_OUT2 A1
#define BUSCH_OUT3 A2
~~~~

![IC Socket](https://github.com/lambdamikel/microtronic-2095-arduino-emulator/blob/master/images/small/DSC06087.JPG) 

![Connections Left Side](https://github.com/lambdamikel/microtronic-2095-arduino-emulator/blob/master/images/small/DSC06093.JPG) 

That's it! Just flash your Uno with the `busch2095.ino` sketch, put in
a Micro SD card (I have used an 8 GB card). The card must be FAT32
formated. 

In case the LCD buttons should not work properly, have a look at the following 
function - the threshold values might need to be adjusted to match you equipment.
Notice that these are analog values: 

~~~~
int readLCDButtons() {

  cur_button = analogRead(0);

  // notice: these values may need to be tuned!!
  // and adjusted to your actual LCD Keypad shield
  // the are analog thresholds

  if (cur_button > 850) return NONE;
  if (cur_button < 50)   return RIGHT;
  if (cur_button < 150)  return UP;
  if (cur_button < 350)  return DOWN;
  if (cur_button < 500)  return LEFT;
  if (cur_button < 800)  return SELECT;

  return NONE;

}
~~~~

### Usage 

Connect the cable. Power up the Uno. Power up the Microtronic. If the
SD Card cannot be initialized correctly, the LCD display should say
so.  If the SD Card is working correctly, you will see the main
screen, which allows you to select PGM1 via the Left LCD button
(buttons are capitalized in the following) and PGM2 with the Right LCD
button.  Notice that the rightmost button is the hardware reset button
of the Uno. The leftmost button is the Select button, and I am using
the second button from the left (right of Select) for Cancel. Pressing
any of these from the main screen will just bring an info message.

To save and load a program:

#### PGM2 - Saving a Program to SD Card

Have the program in Microtronic memory. Then, enter `PGM 2` on the
Microtronic. Next, use the Right LCD button on the emulator. You will now
be asked to create a file name for the program you are about to
save. Filenames are 8 characters long; the `.MIC` extension is added
automatically. Just use the Up and Down keys for character
selection. You can advance to the next character with the Right
key. Notice that the leftmost key (Select) enters the filename and
starts saving, whereas the second leftmost button cancels.

After Select, the emulator retrieves the memory words from the
Microtronic and saves them to SD Card under that filename you
determined. The LCD display and Microtronic display will show the
saving progress. Notice that saving ends when the whole memory was
written (at address `&FF`). Otherwise, you will have to hit the Reset
button on the Microtronic, which will also end saving. The tape
emulator will then go to the main menu.

#### PGM1 - Loading a Program from SD Card

Reset the Arduino first - use the Reset button (rightmost LCD
button). Enter `HALT PGM 5` to erase the Microtronic memory, then
`HALT NEXT 00`, and `HALT PGM 1` to start the Microtronic loading
process. Next, use the Left LCD button to start PGM 1 loading from SD
Card.  A directory browser shows up. Use the Up and Down LCD buttons
to select the `.MIC` file you want to load. Use the Select LCD button
to confirm, and the LCD button next to the Select button to Cancel the
loading process.

After Select, the tape emulator will send the data to the
Microtronic. The LCD display informs about the progress. There is no
feedback on the Microtronic side; its display just stays dark until
the program has been loaded. Use the Microtronic Reset button when the
program has been fully transmitted; the tape emulator will then stop
and return to the main menu.

#### Microtronic Software

A couple of programs are in the [software
subdirectory](https://github.com/lambdamikel/microtronic-2095-arduino-emulator/tree/master/software)
of this repository. See the `README.txt` in there.

To create programs on the desktop or laptop computer, I recommend
[Martin Sauter's Microtronic
Assembler](https://github.com/martinsauter/Busch-2090-Projects/tree/master/05%20-%20Busch%202090%20Assembler);
cool stuff!

#### Disclaimer 

Use at your own risk. I am not responsible for any potential damage
that you might do to your Microtronic, other machinery, or yourself,
in the process of assembling this piece of hardware.

Enjoy! 

